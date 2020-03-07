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
std::string stringf(char* fmt, ...)
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
	template<typename R, typename F, typename Tuple, int ...S> void callFunc(F f, Tuple& args, seq<S...>, std::true_type hasReturn) { f(std::get<S>(args) ...); }
	template<typename R, typename F, typename Tuple, int ...S> R callFunc(F f, Tuple& args, seq<S...>, std::false_type hasReturn) { return f(std::get<S>(args) ...); }
}

template<typename R, typename... Args>
R callWithTuple(R(*f)(Args...), std::tuple<Args...>& args)
{
	using namespace MiscInternal;
	return callFunc<R, R(*)(Args...)>(f, args, typename gens<sizeof...(Args)>::type(), std::is_void<R>::type{});
}

template<typename... Args>
void callWithTupleNoReturn(void(*f)(Args...), std::tuple<Args...>& args)
{
	using namespace MiscInternal;
	callFunc<void, void(*)(Args...)>(f, args, typename gens<sizeof...(Args)>::type(), false);
}

template<typename R, typename T, typename... Args>
R callWithTuple(T* instance, R(T::*f)(Args...), std::tuple<Args...>& args)
{
	using namespace MiscInternal;
	return callFunc<R>([&] (Args... args) { return (instance->*f)(std::forward<Args>(args)...); }, args, typename gens<sizeof...(Args)>::type(), std::is_void<R>::type{});
}

template<typename T, typename... Args>
void callWithTupleNoReturn(T* instance, void(T::*f)(Args...), std::tuple<Args...>& args)
{
	using namespace MiscInternal;
	callFunc<void>([&](Args... args) { (instance->*f)(std::forward<Args>(args)...); } , args, typename gens<sizeof...(Args)>::type(), std::false_type{});
}

std::string escape(std::string&);
std::size_t generateHash(const void*, std::size_t);
std::string prettySize(std::size_t);

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