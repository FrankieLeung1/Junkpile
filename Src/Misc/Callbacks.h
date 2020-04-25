#pragma once

template<typename R, typename... Args>
class BasicFunction
{
public:
	typedef R(*fnType)(void*, Args...);

public:
	BasicFunction();
	BasicFunction(const BasicFunction&);
	template<typename FunctionPtr, typename UserData = nullptr_t> BasicFunction(FunctionPtr, UserData* ud = nullptr);
	~BasicFunction();

	R call(Args...);

	operator bool() const;
	R operator()(Args...);

protected:
	struct Caller { char m_d[5]; virtual R call(void*, Args...) = 0; };
	template<typename FunctionPtr, typename UserData>
	struct CallerImpl : public Caller
	{
		CallerImpl(FunctionPtr);
		R call(void*, Args...) override;
		FunctionPtr m_fn;
	};

protected:
	char m_caller[sizeof(fnType)];
	void* m_ud;
};

template<typename R, typename... Args>
class FunctionBase
{
public:
	typedef R(*InvolvePtr)(FunctionBase<R, Args...>*, Args...);

public:
	FunctionBase(InvolvePtr);
	virtual ~FunctionBase() = 0;
	virtual std::size_t getSizeNeeded() const = 0;

	virtual void placementMove(void* dest) =0;

	R operator()(Args...);

	class PoolHelper
	{
	public:
		static void move(void* dest, void* src, std::size_t);
		static std::size_t getSize(void*);
		static void destruct(void*);
	};

protected:
	template<typename Signature, typename Functor> static R involve(FunctionBase<R, Args...>* f, Args...);
	InvolvePtr m_involve;
};

template<class T> struct GetFunctionBase;
template<typename R, typename... Args>
struct GetFunctionBase<R (Args...)>
{
	typedef FunctionBase<R, Args...> Value;
};

template<typename Signature, typename Functor>
class Function : public GetFunctionBase<Signature>::Value
{
	typedef typename GetFunctionBase<Signature>::Value Base;

public:
	Function();
	Function(Functor);
	~Function() override;

	void placementMove(void* dest) override;
	std::size_t getSizeNeeded() const override;

protected:
	Functor m_fn;
	friend typename Base;
};

void functionTest();

// For generic types that are functors, delegate to its 'operator()'
template <typename FunctionType, typename T>
struct function_traits : public function_traits<FunctionType, decltype(&T::operator())> {};
// for pointers to member function
template <typename FunctionType, typename ClassType, typename ReturnType, typename... Args>
struct function_traits<FunctionType, ReturnType(ClassType::*)(Args...) const> { typedef Function<ReturnType(Args...), FunctionType> f_type; };
// for pointers to member function
template <typename FunctionType, typename ClassType, typename ReturnType, typename... Args>
struct function_traits<FunctionType, ReturnType(ClassType::*)(Args...) > { typedef Function<ReturnType(Args...), FunctionType> f_type; };
// for function pointers
template <typename FunctionType, typename ReturnType, typename... Args>
struct function_traits<FunctionType, ReturnType(*)(Args...)> { typedef Function<ReturnType(Args...), FunctionType> f_type; };

template <typename L>
static typename function_traits<L, L>::f_type makeFunction(L l) { return (typename function_traits<L, L>::f_type)(l); }
//handles bind & multiple function call operator()'s
template<typename ReturnType, typename... Args, class T>
auto makeFunction(T&& t) -> Function<decltype(ReturnType(t(std::declval<Args>()...)))(Args...), T> { return{ std::forward<T>(t) }; }
//handles explicit overloads
template<typename ReturnType, typename... Args>
auto makeFunction(ReturnType(*p)(Args...)) -> Function<ReturnType(Args...), decltype(*p)> { return{ p }; }
//handles explicit overloads
template<typename ReturnType, typename... Args, typename ClassType>
auto makeFunction(ReturnType(ClassType::*p)(Args...)) -> Function<ReturnType(Args...), decltype(*p)> { return{ p }; }

// ----------------------- IMPLEMENTATION ----------------------- 
template<typename R, typename... Args>
BasicFunction<R, Args...>::BasicFunction():
m_caller(),
m_ud(nullptr)
{
	memset(m_caller, 0x00, sizeof(m_caller));
}

template<typename R, typename... Args>
template<typename FunctionPtr, typename UserData>
BasicFunction<R, Args...>::BasicFunction(FunctionPtr f, UserData* ud)
{
	static_assert(sizeof(f) <= sizeof(fnType), "captured lambdas not supported");
	static_assert(sizeof(m_caller) <= sizeof(CallerImpl<FunctionPtr, UserData>), "captured lambdas not supported");
	new(&m_caller) CallerImpl<FunctionPtr, UserData>(f);
	m_ud = ud;
}

template<typename R, typename... Args>
BasicFunction<R, Args...>::BasicFunction(const BasicFunction& copy)
{
	memcpy(&m_caller, &copy.m_caller, sizeof(sizeof(fnType)));
	m_ud = copy.m_ud;
}

template<typename R, typename... Args>
BasicFunction<R, Args...>::~BasicFunction()
{
	reinterpret_cast<Caller*>(&m_caller)->~Caller();
}

template<typename R, typename... Args>
R BasicFunction<R, Args...>::call(Args... args)
{
	Caller* c = reinterpret_cast<Caller*>(&m_caller);
	return c->call(m_ud, std::forward<Args>(args)...);
}

template<typename R, typename... Args>
BasicFunction<R, Args...>::operator bool() const
{
	char zeroes[sizeof(fnType)] = { 0 };
	return memcmp(m_caller, zeroes, sizeof(fnType)) != 0;
}

template<typename R, typename... Args>
R BasicFunction<R, Args...>::operator()(Args... args)
{
	return call(args...);
}

template<typename R, typename... Args>
template<typename FunctionPtr, typename UserData>
BasicFunction<R, Args...>::CallerImpl<FunctionPtr, UserData>::CallerImpl(FunctionPtr Functor): m_fn(Functor)
{
	strcpy_s(m_d, "test");
}

template<typename R, typename... Args>
template<typename FunctionPtr, typename UserData>
R BasicFunction<R, Args...>::CallerImpl<FunctionPtr, UserData>::call(void* ud, Args... args)
{
	return m_fn(reinterpret_cast<UserData*>(ud), std::forward<Args>(args)...);
}

template<typename R, typename... Args>
FunctionBase<R, Args...>::FunctionBase(InvolvePtr involve):
m_involve(involve)
{

}

template<typename R, typename... Args>
FunctionBase<R, Args...>::~FunctionBase()
{
}

template<typename R, typename... Args>
R FunctionBase<R, Args...>::operator()(Args... args)
{
	return (*m_involve)(this, std::forward<Args>(args)...);
}

template<typename R, typename... Args>
template<typename Signature, typename Functor>
R FunctionBase<R, Args...>::involve(FunctionBase<R, Args...>* f, Args... args)
{
	return ((Function<Signature, Functor>*)f)->m_fn(std::forward<Args>(args)...);
}

template<typename R, typename... Args>
void FunctionBase<R, Args...>::PoolHelper::move(void* dest, void* src, std::size_t size)
{
	auto advanceInBytes = [](void* p, std::size_t size) { return static_cast<char*>(p) + size; };
	void* startDest = dest;
	void* end = advanceInBytes(src, size);
	while (src != end)
	{
		auto destf = static_cast<FunctionBase<R, Args...>*>(dest);
		auto srcf = static_cast<FunctionBase<R, Args...>*>(src);

		srcf->placementMove(destf);

		std::size_t functionSize = destf->getSizeNeeded();
		dest = advanceInBytes(dest, functionSize);
		src = advanceInBytes(src, functionSize);

		CHECK_F(src <= end);
	}

	CHECK_F(dest == advanceInBytes(startDest, size));
}

template<typename R, typename... Args>
std::size_t FunctionBase<R, Args...>::PoolHelper::getSize(void* f)
{
	return static_cast<FunctionBase<R, Args...>*>(f)->getSizeNeeded();
}

template<typename R, typename... Args>
void FunctionBase<R, Args...>::PoolHelper::destruct(void* f)
{
	static_cast<FunctionBase<R, Args...>*>(f)->~FunctionBase();
}

template<typename Signature, typename Functor>
Function<Signature, Functor>::Function():
Base(&Base::involve<Signature, Functor>)
{

}


template<typename Signature, typename Functor>
Function<Signature, Functor>::Function(Functor f):
Base(&Base::involve<Signature, Functor>),
m_fn(f)
{

}

template<typename Signature, typename Functor>
Function<Signature, Functor>::~Function()
{

}

template<typename Signature, typename Functor>
void Function<Signature, Functor>::placementMove(void* dest)
{
	auto f = new(dest) Function<Signature, Functor>(std::move(m_fn));
}

template<typename Signature, typename Functor>
std::size_t Function<Signature, Functor>::getSizeNeeded() const
{
	return sizeof(Function<Signature, Functor>);
}

static void functionTest()
{
	auto f = makeFunction([](void*, int) { LOG_F(INFO, "success\n"); });
	FunctionBase<void, void*, int>* base = &f;
	base->operator()(nullptr, 2);
}