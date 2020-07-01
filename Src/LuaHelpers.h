#pragma once

#include "Resources/ResourceManager.h"
#include "Misc/Any.h"

class File;
class LuaTable
{
public:
	struct Index
	{
		Index(int);
		Index(const char*);

	private:
		enum class Type { INT, STRING } m_type;
		union {
			int m_int;
			const char* m_string;
		};

		friend class LuaTable;
	};

public:
	LuaTable(lua_State* = nullptr, int stack = 0);
	LuaTable(const LuaTable&);
	LuaTable(LuaTable&&);
	static LuaTable fromStack(lua_State*, int);
	~LuaTable();

	LuaTable& operator=(const LuaTable&);

	void setWriteDefaultValueIfNil(bool);

	bool isNil(Index index) const;

	std::size_t getLength() const;

	std::string getIndexString(Index) const;

	lua_State* getLuaState() const;

	template<typename T> T get(Index) const;
	template<typename T> T get(Index, T defaultValue);
	template<typename T> void set(Index, const T&);

	void clear();
	bool valid() const;
	void reset();
	void dump();

	enum class SerializeMode { Readable, Compact, Compressed };
	void serialize(std::ostream& out, SerializeMode = SerializeMode::Readable) const;

protected:
	void initRef(int stack = 0);
	void copyFrom(const LuaTable&);

	void pushIndex(Index) const;
	void pushTable() const;
	void pushTableAndValue(Index) const;

	bool getValue(bool*, bool log) const;
	bool getValue(int*, bool log) const;
	bool getValue(float*, bool log) const;
	bool getValue(std::string*, bool log) const;
	bool getValue(LuaTable*, bool log) const;

	void pushValue(bool);
	void pushValue(int);
	void pushValue(float);
	void pushValue(const std::string&);
	void pushValue(const LuaTable&);

	void serialize(std::ostream& out, int stack, int indent, SerializeMode) const;

protected:
	lua_State* m_lua;
	int m_ref;
	bool m_writeOnNil;

	friend class LuaCall;
	friend class LuaIterator;
	friend class LuaStackCheck;
};

class LuaIterator
{
public:
	LuaIterator(LuaTable&);
	~LuaIterator();
	
	bool next();

	template<typename T> T getKey() const;
	template<typename T> T getValue() const;
	template<typename T> bool isKeyType() const;
	template<typename T> bool isValueType() const;

protected:
	bool _get(bool*, int stack) const;
	bool _get(int*, int stack) const;
	bool _get(float*, int stack) const;
	bool _get(std::string*, int stack) const;
	bool _get(LuaTable*, int stack) const;

protected:
	lua_State* m_state;
	int m_keyIndex, m_valueIndex;
};

class LuaCall
{
public:
	LuaCall(lua_State*, const char* globalFunction);
	LuaCall(lua_State*, const LuaTable&, const char* functionName);
	~LuaCall();

	LuaCall& arg(bool);
	LuaCall& arg(int);
	LuaCall& arg(float);
	LuaCall& arg(const LuaTable&);
	LuaCall& arg(void* ptr, const char* type);

	LuaCall& result(bool*);
	LuaCall& result(int*);
	LuaCall& result(float*);
	LuaCall& result(LuaTable*);

	LuaCall& error(std::string*);

protected:
	lua_State* m_lua;
	enum Type { TYPE_NIL, TYPE_BOOLEAN, TYPE_NUMBER, TYPE_INTEGER, TYPE_STRING, TYPE_TABLE, TYPE_TOLUA };
	struct ResultData
	{
		Type m_type;
		void* m_address;
	};
	ResultData m_results[6];
	int m_resultCount;

	int m_argCount;

	std::string* m_errorMessage;
};

class LuaStackCheck
{
public:
	LuaStackCheck(lua_State* s = nullptr);
	LuaStackCheck(LuaTable& t);
	~LuaStackCheck();

	lua_State* m_lua;
	int m_stack;

	static lua_State* s_defaultState;
};

class LuaTableResource : public LuaTable, public Resource
{
public:
	template<typename... Ts> LuaTableResource(Ts... args);
	~LuaTableResource();

public:
	class LuaTableLoader : public Loader
	{
	public:
		LuaTableLoader(const std::string& path, int flags);
		~LuaTableLoader();
		LuaTableResource* load(std::tuple<int, std::string>* error);
		std::string getDebugName() const;
		StringView getTypeName() const;

	protected:
		std::string m_path;
		ResourcePtr<File> m_file;
	};
	static LuaTableLoader* createLoader(const char* path, int flags = 0);
};

template<typename Tuple, typename T = void, std::size_t = 0> struct LuaArgs {};
template<typename Tuple, std::size_t n> struct LuaArgs<Tuple, void, n> {
	static bool get(lua_State* s, Tuple& tuple); // entry
	static bool get(lua_State* s, Tuple& tuple, std::true_type end);
	static bool get(lua_State* s, Tuple& tuple, std::false_type end);
};
template<typename Tuple, std::size_t n> struct LuaArgs<Tuple, bool, n> { static bool get(lua_State* s, Tuple& tuple); };
template<typename Tuple, std::size_t n> struct LuaArgs<Tuple, int, n> { static bool get(lua_State* s, Tuple& tuple); };
template<typename Tuple, std::size_t n> struct LuaArgs<Tuple, float, n> { static bool get(lua_State* s, Tuple& tuple); };
template<typename Tuple, std::size_t n> struct LuaArgs<Tuple, std::string, n> { static bool get(lua_State* s, Tuple& tuple); };

template<typename Tuple, typename R, typename... Args> struct LuaCFunctionCaller { static int call(lua_State*, R(*)(Args...), Tuple&); };
template<typename Tuple, typename... Args> struct LuaCFunctionCaller<Tuple, void, Args...> { static int call(lua_State*, void(*)(Args...), Tuple&); };
template<typename R, typename... Args> void pushCFunction(lua_State* s, R(*f)(Args...), std::array<const char*, sizeof...(Args)>& names, std::array<Any, sizeof...(Args)>& defaults);
template<typename R, typename... Args> void pushCFunction(lua_State* s, R(*f)(Args...), std::array<const char*, sizeof...(Args)>& names);
template<typename R, typename... Args> void pushCFunction(lua_State* s, R(*f)(Args...));

template<typename T> void push(lua_State*, const T&);
template<> void push<bool>(lua_State*, const bool&);
template<> void push<int>(lua_State*, const int&);
template<> void push<float>(lua_State*, const float&);
template<> void push<std::string>(lua_State*, const std::string&);

LuaTable readLuaDataFile(lua_State*, const File& f);
void writeLuaDataFile(const LuaTable&, const char*);

std::string escapeLuaString(const char*);

void printStack(lua_State*);

// ---------------------------------- IMPLEMENTATION ----------------------------------
template<typename T> T LuaTable::get(Index index) const
{
	ERROR_CONTEXT("LuaTable::get", getIndexString(index).c_str());
	LuaStackCheck sc(m_lua);

	pushTableAndValue(index);
	
	T result;
	getValue(&result, true);
	lua_pop(m_lua, 2);
	return result;
}

template<typename T> T LuaTable::get(Index index, T defaultValue)
{
	ERROR_CONTEXT("LuaTable::get", getIndexString(index).c_str());
	LuaStackCheck sc(m_lua);

	pushTableAndValue(index);

	T result = defaultValue;
	bool success = getValue(&result, false);
	lua_pop(m_lua, 1); // pops value
	if (!success && m_writeOnNil)
	{
		pushIndex(index);
		pushValue(defaultValue);
		lua_settable(m_lua, -3);
		result = defaultValue;
	}

	lua_pop(m_lua, 1); // pops table

	return result;
}

template<typename T> void LuaTable::set(Index index, const T& value)
{
	ERROR_CONTEXT("LuaTable::set", getIndexString(index).c_str());

	LuaStackCheck sc(m_lua);
	initRef();
	pushTable();
	pushIndex(index);
	pushValue(value);
	lua_settable(m_lua, -3);
	lua_pop(m_lua, 1);
}

template<typename... Ts>
LuaTableResource::LuaTableResource(Ts... args):
LuaTable(std::forward<Ts...>(args...))
{

}

template<typename T>
T LuaIterator::getKey() const
{
	T result;
	bool success = _get(&result, m_keyIndex);
	CHECK_F(success);
	return result;
}

template<typename T>
T LuaIterator::getValue() const
{
	T result;
	bool success = _get(&result, m_valueIndex);
	CHECK_F(success);
	return result;
}

template<typename T>
bool LuaIterator::isKeyType() const
{
	bool success = _get((T*)nullptr, m_keyIndex);
	return success;
}

template<typename T> 
bool LuaIterator::isValueType() const
{
	bool success = _get((T*)nullptr, m_valueIndex);
	return success;
}

template<typename Tuple, std::size_t n> bool LuaArgs<Tuple, void, n>::get(lua_State* s, Tuple& tuple)
{
	return get(s, tuple, std::integral_constant<bool, n >= std::tuple_size<Tuple>::value>{});
}

template<typename Tuple, std::size_t n> bool LuaArgs<Tuple, void, n>::get(lua_State* s, Tuple& tuple, std::true_type)
{
	return true;
}

template<typename Tuple, std::size_t n> bool LuaArgs<Tuple, void, n>::get(lua_State* s, Tuple& tuple, std::false_type)
{
	return LuaArgs<Tuple, std::tuple_element<n, Tuple>::type, n>::get(s, tuple) ? LuaArgs<Tuple, void, n + 1>::get(s, tuple) : false;
}

template<typename Tuple, std::size_t n> bool LuaArgs<Tuple, bool, n>::get(lua_State* s, Tuple& tuple)
{
	int stack = (int)(n - std::tuple_size<Tuple>::value);
	if (!lua_isboolean(s, stack))
	{
		LOG_F(ERROR, "Lua argument (%d) is \"%s\"; expected boolean\n", n, lua_typename(s, lua_type(s, n)));
		return false;
	}

	std::get<n>(tuple) = (lua_toboolean(s, stack) != 0);
	return true;
}

template<typename Tuple, std::size_t n> bool LuaArgs<Tuple, int, n>::get(lua_State* s, Tuple& tuple)
{
	int stack = (int)(n - std::tuple_size<Tuple>::value);
	if (!lua_isnumber(s, stack) || lua_tonumber(s, stack) != (int)lua_tonumber(s, stack))
	{
		LOG_F(ERROR, "Lua argument (%d) is \"%s\"; expected integer\n", n, lua_typename(s, lua_type(s, n)));
		return false;
	}

	std::get<n>(tuple) = (int)lua_tonumber(s, stack);
	return true;
}

template<typename Tuple, std::size_t n> bool LuaArgs<Tuple, float, n>::get(lua_State* s, Tuple& tuple)
{
	int stack = (int)(n - std::tuple_size<Tuple>::value);
	if (!lua_isnumber(s, stack))
	{
		LOG_F(ERROR, "Lua argument (%d) is \"%s\"; expected number\n", n, lua_typename(s, lua_type(s, n)));
		return false;
	}

	std::get<n>(tuple) = lua_tonumber(s, stack);
	return true;
}

template<typename Tuple, std::size_t n> bool LuaArgs<Tuple, std::string, n>::get(lua_State* s, Tuple& tuple)
{
	int stack = (int)(n - std::tuple_size<Tuple>::value);
	if (!lua_isstring(s, stack))
	{
		LOG_F(ERROR, "Lua argument (%d) is \"%s\"; expected string\n", n, lua_typename(s, lua_type(s, n)));
		return false;
	}

	std::get<n>(tuple) = lua_tostring(s, stack);
	return true;
}

template<typename Tuple, typename R, typename... Args>
int LuaCFunctionCaller<Tuple, R, Args...>::call(lua_State* s, R(*f)(Args...), Tuple& args)
{
	push(s, callWithTuple(f, args));
	return 1;
}

template<typename Tuple, typename... Args>
int LuaCFunctionCaller<Tuple, void, Args...>::call(lua_State*, void(*f)(Args...), Tuple& args)
{
	callWithTuple(f, args);
	return 0;
}

template<typename R, typename... Args>
void pushCFunction(lua_State* s, R(*f)(Args...), std::array<const char*, sizeof...(Args)>& names, std::array<Any, sizeof...(Args)>& defaults)
{
	// TODO: names and defaults
	auto impl = [](lua_State* s)
	{
		int t = lua_gettop(s);
		LOG_F(INFO, "top %d\n", t);

		std::tuple<Args...> args;
		if (!LuaArgs<decltype(args)>::get(s, args))
			return 0;
		
		typedef R(*F)(Args...);
		F f = (F)lua_touserdata(s, lua_upvalueindex(1));
		return LuaCFunctionCaller<decltype(args), R, Args...>::call(s, f, args);
	};

	lua_pushlightuserdata(s, f);
	lua_pushcclosure(s, impl, 1);
}

template<typename R, typename... Args> void pushCFunction(lua_State* s, R(*f)(Args...), std::array<const char*, sizeof...(Args)>& names)
{
	pushCFunction(s, f, names, std::array < Any, sizeof...(Args)> { });
}

template<typename R, typename... Args> void pushCFunction(lua_State* s, R(*f)(Args...))
{
	pushCFunction(s, f, std::array<const char*, sizeof...(Args)>{ "" }, std::array < Any, sizeof...(Args)> { Any{} });
}