#pragma once
template<typename T, size_t N>
constexpr size_t countof(T(&)[N])
{
	return N;
}

void initLoggingForVisualStudio(const char* logname);

int nextPowerOf2(int);

struct BitBltBuffer
{
	char* m_buffer;
	std::size_t m_pixelSize; // size of a single pixel
	int m_width, m_height; // width and height in pixels
};

void bitblt(const BitBltBuffer& dest, int x, int y, int width, int height, const BitBltBuffer& src, int sx, int sy);

template<int bufferSize = 512>
std::string stringf(const char* fmt, ...)
{
	char buffer[bufferSize];
	va_list args;

	va_start(args, fmt);
	vsnprintf_s(buffer, bufferSize, fmt, args);
	va_end(args);

	return std::string(buffer);
}

namespace MiscInternal {
	template<int ...> struct seq { };
	template<int n, int ...s> struct gens : gens<n - 1, n - 1, s...> { };
	template<int ...s> struct gens<0, s...> { typedef seq<s...> type; };
	template<typename R, typename F, typename Tuple, int ...S> void callFunc(F f, Tuple& args, seq<S...>, std::true_type noReturn) { f(std::get<S>(args) ...); }
	template<typename R, typename F, typename Tuple, int ...S> R callFunc(F f, Tuple& args, seq<S...>, std::false_type noReturn) { return f(std::get<S>(args) ...); }
}

template<typename R, typename Tuple, typename... Args>
R callWithTuple(R(*f)(Args...), Tuple& args)
{
	using namespace MiscInternal;
	return callFunc<R, R(*)(Args...)>(f, args, typename gens<sizeof...(Args)>::type(), std::is_void<R>::type{});
}

template<typename Tuple, typename... Args>
void callWithTupleNoReturn(void(*f)(Args...), Tuple& args)
{
	using namespace MiscInternal;
	callFunc<void, void(*)(Args...)>(f, args, typename gens<sizeof...(Args)>::type(), false);
}

template<typename R, typename T, typename Tuple, typename... Args>
R callWithTuple(void* instance, R(T::*f)(Args...), Tuple& args)
{
	using namespace MiscInternal;
	return callFunc<R>([&] (Args... args) { return ((T*)instance->*f)(std::forward<Args>(args)...); }, args, typename gens<sizeof...(Args)>::type(), std::is_void<R>::type{});
}

template<typename T, typename Tuple, typename... Args>
void callWithTupleNoReturn(void* instance, void(T::*f)(Args...), Tuple& args)
{
	using namespace MiscInternal;
	callFunc<void>([&](Args... args) { ((T*)instance->*f)(std::forward<Args>(args)...); } , args, typename gens<sizeof...(Args)>::type(), std::false_type{});
}

bool endsWith(const std::string& s, const char* ending, std::size_t endingSize = 0);
std::string toUtf8(const std::wstring&);
std::wstring toWideString(const std::string&);
std::string escape(std::string&);
std::size_t generateHash(const void*, std::size_t);
std::string prettySize(std::size_t);
std::string normalizePath(const char* path);
std::string& normalizePath(std::string& path);

template<typename Container, typename T>
bool contains(const Container& container, const T& t)
{
	return std::find(container.begin(), container.end(), t) != container.end();
}

namespace Misc {
	struct TestResourceBase {
		virtual ~TestResourceBase()=0 {};
	};

	template<typename T>
	struct TestResource : public TestResourceBase {
		TestResource(T* res) :m_res(res) {}
		~TestResource() { delete m_res; }
		T* m_res;
	};

	extern std::vector<Misc::TestResourceBase*> _testResources;
}

template<typename T>
void addTestResource(T r) { Misc::_testResources.push_back(new TestResource(r)); }
template<typename T, typename... Args> T* createTestResource(Args&&... args) { T* r = new T(std::forward<Args>(args)...);  Misc::_testResources.push_back(new Misc::TestResource<T>(r)); return r; }
void deleteTestResources();

// I think I just rewrote the TypeHelper in VariableSizedMemoryPool?
class TypeHelper
{
public:
	virtual void destruct(void*) =0;
	virtual void moveConstruct(void* dest, void* src) = 0;
	virtual std::size_t getSize() const =0;
};

template<typename T>
class TypeHelperInstance : public TypeHelper
{
public:
	static TypeHelperInstance<T> s_instance;
	void destruct(void* v) { static_cast<T*>(v)->~T(); }
	void moveConstruct(void* dest, void* src) { new(dest)T(std::move(*static_cast<T*>(src))); }
	std::size_t getSize() const { return sizeof(T); }
};

template<typename T>
TypeHelperInstance<T> TypeHelperInstance<T>::s_instance;

class Scope
{
public:
	Scope() {}
	template<typename T> Scope(const T& onExit) { exit(onExit); }
	~Scope() { if (m_exit) m_exit(); }
	Scope(const Scope&) = delete;

	template<typename T> Scope& exit(const T& onExit)
	{
		m_exit = onExit;
		return *this;
	}

protected:
	std::function<void()> m_exit;
};

#define HERE { LOG_F(INFO, "HERE\n"); }; (void)0
#define HERE_SCOPE(varName) { LOG_F(INFO, "HERE START (varName)\n"); } Scope varName([](){LOG_F(INFO, "HERE END (varName)\n");)}); (void)0