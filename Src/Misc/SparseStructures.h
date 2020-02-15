#pragma once

template<typename T>
class SparseArray
{
public:
	SparseArray();
	SparseArray(const SparseArray<T>&) = default;
	SparseArray(SparseArray<T>&&) = default;
	SparseArray<T>& operator=(SparseArray<T>&&) = default;
	~SparseArray();

	T* get(std::size_t);
	const T* get(std::size_t) const;

	template<typename U> T* set(std::size_t index, U);
	void erase(std::size_t);

	void reserve(std::size_t);
	bool empty() const;

	struct Iterator
	{
		Iterator operator++(int);
		void operator++();
		T& operator*();
		T* operator->();

		bool operator!=(const Iterator&) const;
		bool operator==(const Iterator&) const;

		std::size_t m_index;
		T* m_value;

		Iterator(std::size_t, T*,
			typename std::vector< std::tuple<T, std::size_t> >::iterator,
			typename std::vector< std::tuple<T, std::size_t> >::iterator);

	private:
		typename std::vector< std::tuple<T, std::size_t> >::iterator m_it, m_end;
	};
	Iterator begin();
	Iterator end();

	static void test();

protected:
	std::vector< std::tuple<T, std::size_t> > m_data;
};

// ----------------------- IMPLEMENTATION -----------------------
template<typename T> SparseArray<T>::SparseArray() {}
template<typename T> SparseArray<T>::~SparseArray() {}

template<typename T>
template<typename U>
T* SparseArray<T>::set(std::size_t index, U value)
{
	std::size_t currentIndex = 0;
	auto it = m_data.begin();
	while (it != m_data.end())
	{
		std::size_t distance = std::get<std::size_t>(*it);
		if (currentIndex == index)
		{
			std::get<T>(*it) = std::forward<U>(value);
			return &std::get<T>(*it);
		}
		else if(currentIndex < index && index < currentIndex + distance)
		{
			std::size_t newDistance = index - currentIndex;
			std::get<std::size_t>(*it) -= newDistance;
			it = m_data.insert(it, std::make_tuple(value, newDistance));
			return &std::get<T>(*it);
		}
		else
		{
			currentIndex += distance;
			++it;
		}
	}
	CHECK_F(it == m_data.end());

	m_data.push_back(std::make_tuple(value, index - currentIndex));
	return &std::get<T>(m_data.back());
}

template<typename T>
T* SparseArray<T>::get(std::size_t index)
{
	auto it = m_data.begin();
	while (it != m_data.end())
	{
		index -= std::get<std::size_t>(*it);
		if (index == 0)
			break;

		++it;
	}

	return it == m_data.end() ? nullptr : &std::get<T>(*it);
}

template<typename T> const T* SparseArray<T>::get(std::size_t i) const { return const_cast<SparseArray<T>*>(this)->get(i); }
template<typename T> void SparseArray<T>::erase(std::size_t index)
{
	for(auto it = m_data.begin(); it < m_data.end(); ++it)
	{
		index -= std::get<std::size_t>(*it);
		if (index == 0)
		{
			auto nextIt = it + 1;
			if (nextIt != m_data.end())
				std::get<std::size_t>(*nextIt) += std::get<std::size_t>(*it);

			m_data.erase(it);
			return;
		}
	}
}

template<typename T> void SparseArray<T>::reserve(std::size_t s) { m_data.reserve(s); }
template<typename T> bool SparseArray<T>::empty() const { return m_data.empty(); }

template<typename T>
typename SparseArray<T>::Iterator SparseArray<T>::Iterator::operator++(int)
{
	Iterator copy = *this;
	++(*this);
	return copy;
}

template<typename T>
void SparseArray<T>::Iterator::operator++()
{
	++m_it;
	if (m_it != m_end)
	{
		m_index += std::get<std::size_t>(*m_it);
		m_value = &std::get<T>(*m_it);
	}
	else
	{
		m_index = 0;
		m_value = nullptr;
	}
}

template<typename T>
T& SparseArray<T>::Iterator::operator*()
{
	return *m_value;
}

template<typename T>
T* SparseArray<T>::Iterator::operator->()
{
	return m_value;
}

template<typename T>
bool SparseArray<T>::Iterator::operator!=(const Iterator& it) const
{
	return m_it != it.m_it;
}

template<typename T>
bool SparseArray<T>::Iterator::operator==(const Iterator& it) const
{
	return m_it == it.m_it;
}

template<typename T>
SparseArray<T>::Iterator::Iterator(std::size_t i, T* v, typename std::vector< std::tuple<T, std::size_t> >::iterator b, typename std::vector< std::tuple<T, std::size_t> >::iterator e):
m_index(i), m_value(v), m_it(b), m_end(e){ }

template<typename T>
typename SparseArray<T>::Iterator SparseArray<T>::begin()
{
	if (m_data.empty())
		return end();

	auto it = m_data.begin();
	return Iterator(std::get<std::size_t>(*it), &std::get<T>(*it), it, m_data.end());
}

template<typename T>
typename SparseArray<T>::Iterator SparseArray<T>::end()
{
	return Iterator(0, nullptr, m_data.end(), m_data.end());
}

template<typename T>
void SparseArray<T>::test()
{
	SparseArray<int> t;
	for(int i = 0; i < 10; ++i)
		t.set(i, i);

	auto checkValues = [&]() {
		auto it = t.begin();
		while (it != t.end())
		{
			CHECK_F(it.m_index == *it);
			++it;
		}
	};
	checkValues();

	t.erase(2);
	t.erase(10);
	checkValues();

	t.erase(9);
	checkValues();
}