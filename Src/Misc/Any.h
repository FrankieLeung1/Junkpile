#pragma once
#pragma warning( push )
#pragma warning( disable : 4521) // multiple copy constructors specified
#pragma warning( disable : 4522) // multiple assignment operators specified

template<std::size_t BufferSize>
class AnyWithSize
{
public:
	AnyWithSize();
	template<typename T> AnyWithSize(T&&);
	AnyWithSize(AnyWithSize<BufferSize>&&);
	AnyWithSize(const AnyWithSize<BufferSize>&);
	AnyWithSize(AnyWithSize<BufferSize>&);
	~AnyWithSize();

	template<typename T> bool isType() const;
	template<typename T> T& get();
	template<typename T> T* getPtr();

	operator bool() const;
	bool operator!() const;
	AnyWithSize<BufferSize>& operator=(const AnyWithSize<BufferSize>&);
	AnyWithSize& operator=(AnyWithSize<BufferSize>&);
	template<typename T> AnyWithSize& operator=(T&&);

	template<typename T> AnyWithSize& operator!=(T);
	template<typename T> bool operator==(T) const;

	void copyValueFrom(const AnyWithSize&);
	void copyValueFrom(AnyWithSize&&);

	static void test();

protected:
	template<typename T> AnyWithSize<BufferSize>& assignT(T&&, std::true_type lvalue);
	template<typename T> AnyWithSize<BufferSize>& assignT(T&&, std::false_type lvalue);
	template<typename T> AnyWithSize<BufferSize>& assignT(T);
	bool usingInternalBuffer() const;
	void destruct();

protected:
	struct ImplBase
	{
		virtual void destruct(void*) =0;
		virtual void copy(void* dest, void* copy) =0;
		virtual std::size_t getSize() const =0;
	};

	template<typename T>
	struct Impl : public ImplBase
	{
		static Impl<T> Instance;
		void destruct(void*);
		void copy(void* dest, void* copy);
		std::size_t getSize() const;
	};
protected:
	ImplBase* m_impl;
	union {
		char m_buffer[BufferSize];
		void* m_ptr;
	};
};

typedef AnyWithSize<8> Any;

// ----------------------- IMPLEMENTATION ----------------------- 
template<std::size_t BufferSize>
AnyWithSize<BufferSize>::AnyWithSize():
m_impl(nullptr)
{
}

template<std::size_t BufferSize>
template<typename T>
AnyWithSize<BufferSize>::AnyWithSize(T&& t):
m_impl(nullptr)
{
	this->operator=(std::forward<T&&>(t));
}

template<std::size_t BufferSize>
AnyWithSize<BufferSize>::AnyWithSize(AnyWithSize<BufferSize>&& m):
m_impl(m.m_impl)
{
	memcpy_s(m_buffer, sizeof(m_buffer), m.m_buffer, sizeof(m.m_buffer));
	m.m_impl = nullptr;
	memset(m.m_buffer, 0x00, sizeof(m.m_buffer));
}

template<std::size_t BufferSize>
AnyWithSize<BufferSize>::AnyWithSize(const AnyWithSize<BufferSize>& m):
m_impl(nullptr)
{
	copyValueFrom(m);
}

template<std::size_t BufferSize>
AnyWithSize<BufferSize>::AnyWithSize(AnyWithSize<BufferSize>& m) :
m_impl(nullptr)
{
	copyValueFrom(m);
}

template<std::size_t BufferSize>
AnyWithSize<BufferSize>::~AnyWithSize()
{
	destruct();
}

template<std::size_t BufferSize>
template<typename T> bool AnyWithSize<BufferSize>::isType() const
{
	return (static_cast<void*>(m_impl) == static_cast<void*>(&Impl<T>::Instance));
}

template<std::size_t BufferSize>
template<typename T>
AnyWithSize<BufferSize>& AnyWithSize<BufferSize>::assignT(T&& t, std::true_type)
{
	return assignT<T&>(t);
}

template<std::size_t BufferSize>
template<typename T>
AnyWithSize<BufferSize>& AnyWithSize<BufferSize>::assignT(T&& t, std::false_type)
{
	return assignT<T&&>(std::forward<T&&>(t));
}

template<std::size_t BufferSize>
template<typename T> 
AnyWithSize<BufferSize>& AnyWithSize<BufferSize>::assignT(T t)
{
	static_assert(BufferSize >= sizeof(char*), "BufferSize must fit a pointer");
	using plainT = std::remove_reference<T>::type;

	destruct();
	m_impl = &Impl<plainT>::Instance;
	if (BufferSize >= sizeof(plainT))
	{
		new(m_buffer) plainT(std::forward<T&&>(t));
	}
	else
	{
		m_ptr = new char[sizeof(plainT)];
		new (m_ptr) plainT(std::forward<T&&>(t));
	}
	return *this;
}

template<std::size_t BufferSize>
template<typename T> T& AnyWithSize<BufferSize>::get()
{
	CHECK_F(isType<T>());
	if (usingInternalBuffer()) return *reinterpret_cast<T*>(m_buffer);
	else return *static_cast<T*>(m_ptr);
}

template<std::size_t BufferSize>
template<typename T> T* AnyWithSize<BufferSize>::getPtr()
{
	return isType<T>() ? &get<T>() : nullptr;
}

template<std::size_t BufferSize>
AnyWithSize<BufferSize>::operator bool() const
{
	return m_impl != nullptr;
}

template<std::size_t BufferSize>
bool AnyWithSize<BufferSize>::operator!() const
{
	return m_impl == nullptr;
}

template<std::size_t BufferSize>
AnyWithSize<BufferSize>& AnyWithSize<BufferSize>::operator=(const AnyWithSize<BufferSize>& c)
{
	copyValueFrom(c);
	return *this;
}

template<std::size_t BufferSize>
AnyWithSize<BufferSize>& AnyWithSize<BufferSize>::operator=(AnyWithSize<BufferSize>& c)
{
	copyValueFrom(c);
	return *this;
}

template<std::size_t BufferSize>
template<typename T> AnyWithSize<BufferSize>& AnyWithSize<BufferSize>::operator=(T&& t)
{
	return assignT(std::forward<T&&>(t), std::is_lvalue_reference<T>());
}

template<std::size_t BufferSize>
void AnyWithSize<BufferSize>::copyValueFrom(const AnyWithSize<BufferSize>& other)
{
	union {
		char newBuffer[BufferSize] = { 0 };
		void* newPtr;
	};

	if (other.m_impl)
	{
		std::size_t tSize = other.m_impl->getSize();
		if (BufferSize >= tSize)
		{
			other.m_impl->copy((void*)newBuffer, (void*)other.m_buffer);
		}
		else
		{
			newPtr = new char[tSize];
			other.m_impl->copy(newPtr, other.m_ptr);
		}
	}

	destruct();
	m_impl = other.m_impl;
	memcpy_s(m_buffer, sizeof(m_buffer), newBuffer, sizeof(newBuffer));
}

template<std::size_t BufferSize>
void AnyWithSize<BufferSize>::copyValueFrom(AnyWithSize<BufferSize>&& other)
{
	destruct();

	m_impl = other.m_impl;
	memcpy(m_buffer, other.m_buffer, sizeof(buffer));

	other.m_impl = nullptr;
	memset(other.m_buffer, 0x00, sizeof(other.m_buffer));
}

template<std::size_t BufferSize>
template<typename T> AnyWithSize<BufferSize>& AnyWithSize<BufferSize>::operator!=(T t)
{
	if (!isType<T>()) return true;
	if (usingInternalBuffer())	return t != reinterpret_cast<T>(*m_buffer);
	else						return t != *static_cast<T*>(m_ptr);
}

template<std::size_t BufferSize>
template<typename T> bool AnyWithSize<BufferSize>::operator==(T) const
{
	if (!isType<T>()) return false;
	if (usingInternalBuffer())	return t == reinterpret_cast<T>(*m_buffer);
	else						return t == *static_cast<T*>(m_ptr);
}

template<std::size_t BufferSize>
bool AnyWithSize<BufferSize>::usingInternalBuffer() const
{
	return m_impl == nullptr || (BufferSize >= m_impl->getSize());
}

template<std::size_t BufferSize>
void AnyWithSize<BufferSize>::destruct()
{
	if (!m_impl) return;
	if (usingInternalBuffer())
	{
		m_impl->destruct(m_buffer);
	}
	else
	{
		m_impl->destruct(m_ptr);
		delete[] (char*)m_ptr;
	}

	memset(m_buffer, 0x00, sizeof(m_buffer));
	m_impl = nullptr;
}

template<std::size_t BufferSize>
template<typename T>
void AnyWithSize<BufferSize>::Impl<T>::destruct(void* m)
{
	static_cast<T*>(m)->~T();
}

template<std::size_t BufferSize>
template<typename T>
void AnyWithSize<BufferSize>::Impl<T>::copy(void* dest, void* copy)
{
	new(dest)T(*static_cast<T*>(copy));
}

template<std::size_t BufferSize>
template<typename T>
std::size_t AnyWithSize<BufferSize>::Impl<T>::getSize() const
{
	return sizeof(T);
}

template<std::size_t BufferSize>
template<typename T>
typename AnyWithSize<BufferSize>::Impl<T> AnyWithSize<BufferSize>::Impl<T>::Instance;

template<std::size_t BufferSize>
void AnyWithSize<BufferSize>::test()
{
	Any t;
	LOG_F(INFO, "sizeof %d\n", sizeof(t));

	t = 4.0f;
	LOG_F(INFO, "isBool %s\n", t.isType<bool>() ? "true" : "false");
	LOG_F(INFO, "isFloat %s %f\n", t.isType<float>() ? "true" : "false", t.get<float>());

	struct Struct
	{
		Struct() { LOG_F(INFO, "construct\n"); }
		Struct(const Struct&) { LOG_F(INFO, "copy\n"); }
		Struct(Struct&&) { LOG_F(INFO, "move\n"); }
		~Struct() { LOG_F(INFO, "destruct\n"); }

		std::string m_text{ "test" };
	};

	{
		Any t3 = Struct();
	}

	Struct s;
	t = s;

	{
		Any t2;
		t2 = t;
	}

	LOG_F(INFO, "%s\n", t.get<Struct>().m_text.c_str());

	t = Struct();
	t = nullptr;
	t = (char*)"string";
}

#pragma warning( pop )