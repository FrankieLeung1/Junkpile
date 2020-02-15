#pragma once

template<typename T>
struct DefaultPoolType
{
public:
	static void move(void* dest, void* src, std::size_t);
	static std::size_t getSize(void*);
	static void destruct(void*);
};

template<typename T>
struct PODPoolType : public DefaultPoolType<T>
{
public:
	static void move(void* dest, void* src, std::size_t);
};

template<typename T, typename TypeHelper = DefaultPoolType<T>>
class VariableSizedMemoryPool
{
public:
	class Iterator
	{
	public:
		T& operator*();
		T* operator->();
		Iterator operator+(std::size_t i);
		Iterator& operator++();
		Iterator operator++(int);
		bool operator!=(const Iterator&) const;
		bool operator==(const Iterator&) const;

		void advance(std::size_t);

	protected:
		std::vector<char>::iterator m_it;
		Iterator(std::vector<char>::iterator it):m_it(it) {}

		friend class VariableSizedMemoryPool<T, TypeHelper>;
	};
	struct Element { public: std::size_t m_elementSize; protected: Element(std::size_t size) : m_elementSize(size) { CHECK_F(size >= sizeof(Element)); }; friend class Iterator; };

public:
	VariableSizedMemoryPool();
	~VariableSizedMemoryPool();

	void reserve(std::size_t size);
	template<typename NewElement> NewElement* push_back(const NewElement&);
	template<typename NewElement, typename... Ts> NewElement* emplace_back(Ts&&...);
	template<typename NewElement, typename... Ts> NewElement* emplace_back_with_size(std::size_t size, Ts&&...);
	Iterator erase(T* begin, T* end = nullptr);
	Iterator erase(Iterator begin);
	Iterator erase(Iterator begin, Iterator end);

	Iterator begin();
	Iterator end();

	static void test();

protected:
	char* allocateBack(std::size_t);

	std::vector<char> m_buffer;
	std::size_t m_size; // actual used size; m_buffer.size() is our capacity
};

// ----------------------- IMPLEMENTATION ----------------------- 
template<typename T, typename TypeHelper>
VariableSizedMemoryPool<T, TypeHelper>::VariableSizedMemoryPool():
m_size(0)
{
	
}

template<typename T, typename TypeHelper>
VariableSizedMemoryPool<T, TypeHelper>::~VariableSizedMemoryPool()
{
}

template<typename T, typename TypeHelper>
void VariableSizedMemoryPool<T, TypeHelper>::reserve(std::size_t size)
{
	if (m_size >= size)
		return;

	std::vector<char> newBuffer(size);
	TypeHelper::copy(newBuffer.data(), m_buffer.data(), m_size);
	m_buffer = std::move(newBuffer);
}

template<typename T, typename TypeHelper>
template<typename NewElement>
NewElement* VariableSizedMemoryPool<T, TypeHelper>::push_back(const NewElement& element)
{
	return new(allocateBack(sizeof(NewElement))) NewElement(element);
}

template<typename T, typename TypeHelper>
template<typename NewElement, typename... Ts> 
NewElement* VariableSizedMemoryPool<T, TypeHelper>::emplace_back(Ts&&... args)
{
	return new(allocateBack(sizeof(NewElement))) NewElement(std::forward<Ts>(args)...);
}

template<typename T, typename TypeHelper>
template<typename NewElement, typename... Ts>
NewElement* VariableSizedMemoryPool<T, TypeHelper>::emplace_back_with_size(std::size_t size, Ts&&... args)
{
	return new(allocateBack(size)) NewElement(std::forward<Ts>(args)...);
}

template<typename T, typename TypeHelper>
typename VariableSizedMemoryPool<T, TypeHelper>::Iterator VariableSizedMemoryPool<T, TypeHelper>::erase(T* begin, T* end)
{
	if (end == nullptr)
		end = (begin + TypeHelper::getSize(begin)); // end = pointer to the next element

	T* current = begin;
	while (current < end)
	{
		std::size_t elementSize = TypeHelper::getSize(current);
		TypeHelper::destruct(*current);
		static_cast<char*>(current) += elementSize;
		m_size -= elementSize;
	}

	return Iterator(m_buffer.erase(begin, end));
}

template<typename T, typename TypeHelper>
typename VariableSizedMemoryPool<T, TypeHelper>::Iterator VariableSizedMemoryPool<T, TypeHelper>::erase(Iterator begin)
{
	return erase(begin, begin + 1);
}

template<typename T, typename TypeHelper>
typename VariableSizedMemoryPool<T, TypeHelper>::Iterator VariableSizedMemoryPool<T, TypeHelper>::erase(Iterator begin, Iterator end)
{
	Iterator current = begin;
	while (current != end)
	{
		std::size_t elementSize = TypeHelper::getSize(&(*current));
		TypeHelper::destruct(&(*current++));
		m_size -= elementSize;
	}
	CHECK_F(m_size >= 0);

	bool empty = (m_size <= 0);
	if (empty)
	{
		m_buffer.clear();
		return Iterator(m_buffer.end());
	}
	else
	{
		return Iterator(m_buffer.erase(begin.m_it, end.m_it));
	}
}

template<typename T, typename TypeHelper>
typename VariableSizedMemoryPool<T, TypeHelper>::Iterator VariableSizedMemoryPool<T, TypeHelper>::begin()
{
	return Iterator(m_buffer.begin());
}

template<typename T, typename TypeHelper>
typename VariableSizedMemoryPool<T, TypeHelper>::Iterator VariableSizedMemoryPool<T, TypeHelper>::end()
{
	return Iterator(m_buffer.begin() + m_size);
}

template<typename T, typename TypeHelper>
typename T& VariableSizedMemoryPool<T, TypeHelper>::Iterator::operator*()
{
	return *(T*)(&(*m_it));
}

template<typename T, typename TypeHelper>
typename T* VariableSizedMemoryPool<T, TypeHelper>::Iterator::operator->()
{
	return (T*)&(*m_it);
}

template<typename T, typename TypeHelper>
typename VariableSizedMemoryPool<T, TypeHelper>::Iterator VariableSizedMemoryPool<T, TypeHelper>::Iterator::operator+(std::size_t i)
{
	Iterator it = *this;
	it.advance(i);
	return it;
}

template<typename T, typename TypeHelper>
typename VariableSizedMemoryPool<T, TypeHelper>::Iterator& VariableSizedMemoryPool<T, TypeHelper>::Iterator::operator++()
{
	advance(1);
	return *this;
}

template<typename T, typename TypeHelper>
typename VariableSizedMemoryPool<T, TypeHelper>::Iterator VariableSizedMemoryPool<T, TypeHelper>::Iterator::operator++(int)
{
	auto r = *this;
	advance(1);
	return r;
}

template<typename T, typename TypeHelper>
bool VariableSizedMemoryPool<T, TypeHelper>::Iterator::operator!=(const Iterator& it) const
{
	return m_it != it.m_it;
}

template<typename T, typename TypeHelper>
bool VariableSizedMemoryPool<T, TypeHelper>::Iterator::operator==(const Iterator& it) const
{
	return m_it == it.m_it;
}

template<typename T, typename TypeHelper>
void VariableSizedMemoryPool<T, TypeHelper>::Iterator::advance(std::size_t count)
{
	while (count-- > 0)
	{
		void* element = &(*m_it);
		std::advance(m_it, TypeHelper::getSize(element));
	}
}

template<typename T, typename TypeHelper>
char* VariableSizedMemoryPool<T, TypeHelper>::allocateBack(std::size_t size)
{
	if (m_size + size > m_buffer.size())
	{
		const std::size_t newSize = m_size + (std::size_t)std::max((float)size, std::floor((m_size + size) * 0.5f));
		std::vector<char> newBuffer(newSize);
		if(!m_buffer.empty() && !newBuffer.empty())
			TypeHelper::move(newBuffer.data(), m_buffer.data(), m_size);

		m_buffer = std::move(newBuffer);
	}

	std::size_t oldSize = m_size;
	m_size += size;
	return &m_buffer[oldSize];
}

template<typename T, typename TypeHelper>
void VariableSizedMemoryPool<T, TypeHelper>::test()
{
	struct TestElement : VariableSizedMemoryPool<TestElement>::Element {
		TestElement(std::size_t size, int type) : m_type(type), Element(size) { };
		int m_type;
	};
	struct SmallElement : TestElement { SmallElement() :TestElement(sizeof(SmallElement), 1) { };  char m_smallBuffer[12]; };
	struct BigElement : TestElement { BigElement() :TestElement(sizeof(BigElement), 2) { }; char m_bigBuffer[32]; };
	VariableSizedMemoryPool<TestElement> pool;

	LOG_F(INFO, "BigElement: %d SmallElement: %d\n", sizeof(BigElement), sizeof(SmallElement));
	for (int i = 0; i < 200; ++i)
	{
		if (std::rand() < RAND_MAX / 2)
		{
			auto it = pool.emplace_back<BigElement>();
			sprintf(it->m_bigBuffer, "This is a big buffer! %d", i);
		}
		else
		{
			auto it = pool.emplace_back<SmallElement>();
			sprintf(it->m_smallBuffer, "smbuf %d", i);
		}
	}

	for (int i = 0; i < 10; ++i)
	{
		float rand = (float)std::rand() / RAND_MAX * 200.0f;
		auto it = pool.begin();
		it.advance((std::size_t)std::floor(rand));
		pool.erase(it);

		LOG_F(INFO, "Removed %f\n", std::floor(rand));
	}

	auto it = pool.begin();
	it.advance(189);
	pool.erase(it);

	int index = 0;
	for (auto it = pool.begin(); it != pool.end(); ++it, index++)
	{
		switch (it->m_type)
		{
		case 1:
			LOG_F(INFO, "%d %s\n", index, ((SmallElement*)&(*it))->m_smallBuffer);
			break;
		case 2:
			LOG_F(INFO, "%d %s\n", index, ((BigElement*)&(*it))->m_bigBuffer);
			break;
		}
	}
}

template<typename T>
void DefaultPoolType<T>::move(void* _dest, void* _src, std::size_t size)
{
	static_assert(std::is_convertible<T, VariableSizedMemoryPool<T, DefaultPoolType<T>>::Element>::value == true, "T must be convertable to VariableSizedMemoryPool::Element");

	char *dest = (char*)_dest, *src = (char*)_src;
	char* end = dest + size;
	while(dest < end)
	{
		std::size_t elementSize = reinterpret_cast<VariableSizedMemoryPool<T, DefaultPoolType<T>>::Element*>(src)->m_elementSize;
		*reinterpret_cast<T*>(dest) = std::move(*reinterpret_cast<T*>(src));

		dest += elementSize;
		src += elementSize;
	}
}

template<typename T>
std::size_t DefaultPoolType<T>::getSize(void* element)
{
	static_assert(std::is_convertible<T, VariableSizedMemoryPool<T, DefaultPoolType<T>>::Element>::value == true, "T must be convertable to VariableSizedMemoryPool::Element");
	return static_cast<VariableSizedMemoryPool<T, DefaultPoolType<T>>::Element*>(element)->m_elementSize;
}

template<typename T>
void DefaultPoolType<T>::destruct(void* element)
{
	static_assert(std::is_convertible<T, VariableSizedMemoryPool<T, DefaultPoolType<T>>::Element>::value == true, "T must be convertable to VariableSizedMemoryPool::Element");
	static_cast<T*>(element)->~T();
}

template<typename T>
void PODPoolType<T>::move(void* dest, void* src, std::size_t s)
{
	memcpy(dest, src, s);
}