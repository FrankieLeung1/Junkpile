#pragma once

template<typename T>
class ClassMask
{
public:
	ClassMask(T* instance = nullptr) :m_instance(instance) {}
	template<typename Variable> void set(T& instance, Variable(T::*), const Variable&);
	template<typename Variable> void set(T* instance, Variable(T::*), const Variable&);
	template<typename Variable> void set(Variable(T::*), Variable&&);
	template<typename Variable> void set(Variable (T::*));
	template<typename Variable> void set(Variable&, const Variable& value);
	template<typename Variable> void set(Variable*, const Variable& value);
	template<typename Variable> void unset(Variable(T::*));
	template<typename Variable> bool isSet(Variable(T::*)) const;
	void setInstance(T* instance);
	void writeTo(ClassMask<T>*) const;
	void print() const;

	static void test();

protected:
	unsigned char m_mask[sizeof(T)];
	T* m_instance;
};

template<typename T>
class Masked : public ClassMask<T>, public T
{
public:
	Masked();
	virtual ~Masked();
};

// ----------------------- IMPLEMENTATION ----------------------- 

template<typename T>
template<typename Variable>
void ClassMask<T>::set(T& instance, Variable(T::*ptr), const Variable& value)
{
	instance.*ptr = value;
	set(ptr);
}

template<typename T>
template<typename Variable> 
void ClassMask<T>::set(T* instance, Variable(T::*ptr), const Variable& value)
{
	(instance->*ptr) = value;
	set(ptr);
}

template<typename T>
template<typename Variable>
void ClassMask<T>::set(Variable(T::*ptr), Variable&& value)
{
	CHECK_F(m_instance != nullptr);
	(m_instance->*ptr) = value;
	set(ptr);
}

template<typename T>
template<typename Variable>
void ClassMask<T>::set(Variable(T::*ptr))
{
	Variable* v = &(reinterpret_cast<T*>(m_mask)->*ptr);
	memset(v, 0xFF, sizeof(Variable));
}

template<typename T>
template<typename Variable> 
void ClassMask<T>::set(Variable& var, const Variable& value)
{
	CHECK_F(m_instance != nullptr);
	CHECK_F(static_cast<void*>(m_instance) <= &var && static_cast<void*>(m_instance + 1) > &var);
	var = value;
	std::size_t offset = (reinterpret_cast<char*>(&var) - reinterpret_cast<char*>(m_instance));
	memset(m_mask + offset, 0xFF, sizeof(Variable));
}

template<typename T>
template<typename Variable>
void ClassMask<T>::set(Variable* var, const Variable& value)
{
	set(std::remove_pointer<T>(var), value);
}

template<typename T>
template<typename Variable>
void ClassMask<T>::unset(Variable(T::*ptr))
{
	Variable* v = &(reinterpret_cast<T*>(m_mask)->*ptr);
	memset(v, 0x00, sizeof(Variable));
}

template<typename T>
template<typename Variable>
bool ClassMask<T>::isSet(Variable(T::*ptr)) const
{
	bool allZeroes = true;
	Variable* v = &(reinterpret_cast<T*>(m_mask)->*ptr);
	for (char* bytes = reinterpret_cast<char*>(v); bytes != v + 1; bytes++)
	{
		if (!allZeroes)
			CHECK_F(*bytes == 0xFF);

		allZeroes = allZeroes && (*bytes == 0);
	}

	return allZeroes;
}

template<typename T>
void ClassMask<T>::setInstance(T* instance)
{
	m_instance = instance; 
}

template<typename T>
void ClassMask<T>::writeTo(ClassMask<T>* t) const
{
	CHECK_F(m_instance != nullptr);
	for (std::size_t i = 0; i < sizeof(T); ++i)
	{
		if (t->m_mask[i] == 0x00 && m_mask[i] == 0xFF)
		{
			t->m_mask[i] = 0xFF;
			(reinterpret_cast<char*>(t->m_instance)[i]) = (reinterpret_cast<char*>(m_instance)[i]);
		}
	}
}

template<typename T>
void ClassMask<T>::print() const
{
	std::stringstream ss;
	for (std::size_t i = 0; i < sizeof(T); ++i)
	{
		std::string s = stringf("%2hhx", m_mask[i]);
		std::replace(s.begin(), s.end(), ' ', '0');
		ss << s;
	}
	LOG_F(INFO, "size %d:%s", sizeof(T), ss.str().c_str());
}

template<typename T>
void ClassMask<T>::test()
{
	struct Test {
		char m_c1{ 'D' };
		char m_c2{ 'D' };
		char m_c3{ 'D' };
	};
	Test t1, t2, t3, t4;
	ClassMask<Test> m1(&t1), m2(&t2), m3(&t3), m4(&t4);
	m1.set(&Test::m_c1, '~');
	m2.set(&Test::m_c1, 'A');
	m3.set(&Test::m_c2, 'B');
	m4.set(&Test::m_c3, 'C');

	m2.writeTo(&m1);
	m3.writeTo(&m1);
	m4.writeTo(&m1);
}

template<typename T>
Masked<T>::Masked():
ClassMask(this)
{
}

template<typename T>
Masked<T>::~Masked()
{
}