#include "stdafx.h"
#include "LuaHelpers.h"
#include "Files/FileManager.h"
#include "Misc/Misc.h"

LuaTable::LuaTable(lua_State* s, int stack) :
	m_lua(s),
	m_ref(LUA_NOREF),
	m_writeOnNil(true)
{
	if (stack != 0)
	{
		initRef(stack);
	}
}

LuaTable::LuaTable(const LuaTable& t) :
	m_lua(t.m_lua),
	m_ref(LUA_NOREF),
	m_writeOnNil(t.m_writeOnNil)
{
	LuaStackCheck sc(m_lua);
	const_cast<LuaTable&>(t).initRef();

	lua_rawgeti(m_lua, LUA_REGISTRYINDEX, t.m_ref);
	initRef(-1);
}

LuaTable::LuaTable(LuaTable&& t) :
	m_lua(t.m_lua),
	m_ref(t.m_ref),
	m_writeOnNil(t.m_writeOnNil)
{
	t.m_ref = LUA_NOREF;
}

LuaTable LuaTable::fromStack(lua_State* s, int stack)
{
	LuaStackCheck sc(s);
	sc.m_stack--;

	LuaTable result(s);
	result.initRef(stack);
	return result;
}

LuaTable::~LuaTable()
{
	reset();
}

LuaTable& LuaTable::operator=(const LuaTable& t)
{
	copyFrom(t);
	return *this;
}

void LuaTable::setWriteDefaultValueIfNil(bool b)
{
	m_writeOnNil = b;
}

bool LuaTable::isNil(Index index) const
{
	LuaStackCheck sc(m_lua);
	pushTableAndValue(index);

	bool isNil = lua_isnil(m_lua, -1);
	lua_pop(m_lua, 2);
	return isNil;
}

std::size_t LuaTable::getLength() const
{
	LuaStackCheck sc(m_lua);

	const_cast<LuaTable*>(this)->initRef();
	pushTable();

	std::size_t s = lua_rawlen(m_lua, -1);
	lua_pop(m_lua, 1);
	return s;
}

std::string LuaTable::getIndexString(Index i) const
{
	return i.m_type == Index::Type::INT ? std::to_string(i.m_int) : i.m_string;
}

lua_State* LuaTable::getLuaState() const
{
	return m_lua;
}

void LuaTable::pushIndex(Index index) const
{
	if (index.m_type == Index::Type::INT) lua_pushnumber(m_lua, index.m_int);
	else if (index.m_type == Index::Type::STRING) lua_pushstring(m_lua, index.m_string);
}

void LuaTable::pushTable() const
{
	lua_rawgeti(m_lua, LUA_REGISTRYINDEX, m_ref);
}

void LuaTable::pushTableAndValue(Index index) const
{
	LuaStackCheck sc(m_lua);

	const_cast<LuaTable*>(this)->initRef();
	lua_rawgeti(m_lua, LUA_REGISTRYINDEX, m_ref);
	pushIndex(index);
	lua_gettable(m_lua, -2);

	sc.m_stack += 2;
}

void LuaTable::clear()
{
	reset();
}

bool LuaTable::valid() const
{
	return m_ref != LUA_NOREF;
}

void LuaTable::reset()
{
	if (m_ref != LUA_NOREF)
	{
		luaL_unref(m_lua, LUA_REGISTRYINDEX, m_ref);
		m_ref = LUA_NOREF;
	}
}

void LuaTable::dump()
{
	if (!valid())
	{
		LOG_F(ERROR, "Tried to dump an invalid LuaTable");
		return;
	}

	LuaStackCheck sc(m_lua);
	std::stringstream ss;
	serialize(ss);

	LOG_F(INFO, "TABLE DUMP:\n%s\n", ss.str().c_str());
}

void LuaTable::serialize(std::ostream& out, SerializeMode mode) const
{
	if (!valid())
		return;

	lua_rawgeti(m_lua, LUA_REGISTRYINDEX, m_ref);
	serialize(out, -1, 0, mode);
}

void LuaTable::serialize(std::ostream& out, int stack, int indent, SerializeMode mode) const
{
	auto addIndent = [&]() { for (int i = 0; i < indent; ++i) out << '\t';  };

	DCHECK_F(lua_istable(m_lua, -1));
	LuaStackCheck lsc(m_lua);

	out << "{\n";
	indent++;

	int implicitKeyIndex = 1;
	lua_pushnil(m_lua);
	while (lua_next(m_lua, -2))
	{
		addIndent();

		bool implicitKey = false;
		int keyType = lua_type(m_lua, -2);
		switch (keyType)
		{
		case LUA_TSTRING:
		{
			implicitKeyIndex = -1;
			const char* key = lua_tostring(m_lua, -2);
			if (strchr(key, ' ') != nullptr)
			{
				out << "[\"" << escapeLuaString(key) << "\"]";
			}
			else
			{
				out << escapeLuaString(key);
			}
		}
		break;

		case LUA_TBOOLEAN:
		case LUA_TNUMBER:
		{
			if (implicitKeyIndex > 0 && implicitKeyIndex == lua_tonumber(m_lua, -2))
			{
				implicitKeyIndex++;
				implicitKey = true;
			}
			else
			{
				switch (keyType)
				{
				case LUA_TBOOLEAN: out << (lua_toboolean(m_lua, -2) ? "true" : "false"); break;
				case LUA_TNUMBER: out << lua_tonumber(m_lua, -2); break;
				}
			}
		}
		break;

		default: LOG_F(WARNING, "Unsupported type as key (%s)\n", lua_typename(m_lua, keyType)); continue;
		}

		if(!implicitKey)
			out << " = ";

		int valueType = lua_type(m_lua, -1);
		switch (valueType)
		{
		case LUA_TSTRING: out << '"' << escapeLuaString(lua_tostring(m_lua, -1)) << '"'; break;
		case LUA_TBOOLEAN: out << (lua_toboolean(m_lua, -1) ? "true" : "false"); break;
		case LUA_TNUMBER: out << lua_tonumber(m_lua, -1); break;
		case LUA_TTABLE: lua_pushvalue(m_lua, -1);  serialize(out, -1, indent, mode); break;
		default: LOG_F(WARNING, "Unsupported type as value (%s)\n", lua_typename(m_lua, valueType)); continue;
		}

		out << ',';
		out << '\n';

		lua_pop(m_lua, 1);
	}

	lua_pop(m_lua, 1);

	indent--;
	addIndent();

	out << "}";
	lsc.m_stack--;
}

void LuaTable::initRef(int stack)
{
	if (m_ref == LUA_NOREF)
	{
		if (stack == 0)
			lua_createtable(m_lua, 0, 0);
		
		LOG_IF_F(ERROR, stack != 0 && !lua_istable(m_lua, stack), "Tried to assign a non-table to LuaTable\n");

		m_ref = luaL_ref(m_lua, LUA_REGISTRYINDEX);
	}
}

void LuaTable::copyFrom(const LuaTable& t)
{
	LuaStackCheck sc(m_lua);
	reset();
	m_lua = t.m_lua;
	lua_rawgeti(m_lua, LUA_REGISTRYINDEX, t.m_ref);
	initRef(-1);
}

bool LuaTable::getValue(bool* b, bool log) const
{
	LuaStackCheck sc(m_lua);
	if (!lua_isboolean(m_lua, -1))
	{
		LOG_IF_F(ERROR, log, "Wrong type (got %s, expected bool)\n", lua_typename(m_lua, lua_type(m_lua, -1)));
		return false;
	}

	if (b)
		*b = lua_toboolean(m_lua, -1) != 0;

	return true;
}

bool LuaTable::getValue(int* i, bool log) const
{
	LuaStackCheck sc(m_lua);
	if (!lua_isinteger(m_lua, -1))
	{
		LOG_IF_F(ERROR, log, "Wrong type (got %s, expected int)\n", lua_typename(m_lua, lua_type(m_lua, -1)));
		return false;
	}

	if (i)
		*i = (int)lua_tonumber(m_lua, -1);

	return true;
}

bool LuaTable::getValue(float* f, bool log) const
{
	LuaStackCheck sc(m_lua);
	if (!lua_isnumber(m_lua, -1))
	{
		LOG_IF_F(ERROR, log, "Wrong type (got %s, expected float)\n", lua_typename(m_lua, lua_type(m_lua, -1)));
		return false;
	}

	if (f)
		*f = lua_tonumber(m_lua, -1) != 0;

	return true;
}

bool LuaTable::getValue(std::string* s, bool log) const
{
	LuaStackCheck sc(m_lua);
	if(!lua_isstring(m_lua, -1))
	{
		LOG_IF_F(ERROR, log, "Wrong type (got %s, expected string)", lua_typename(m_lua, lua_type(m_lua, -1)));
		return false;
	}

	if (s)
		*s = lua_tostring(m_lua, -1);

	return true;
}

bool LuaTable::getValue(LuaTable* t, bool log) const
{
	LuaStackCheck sc(m_lua);
	if(!lua_istable(m_lua, -1))
	{
		LOG_IF_F(ERROR, log, "Wrong type (got %s, expected table)", lua_typename(m_lua, lua_type(m_lua, -1)));
		return false;
	}

	if (t)
	{
		lua_pushvalue(m_lua, -1);
		*t = LuaTable::fromStack(m_lua, -1);
	}

	return true;
}

void LuaTable::pushValue(bool b)
{
	lua_pushboolean(m_lua, b);
}

void LuaTable::pushValue(int i)
{
	lua_pushnumber(m_lua, i);
}

void LuaTable::pushValue(float f)
{
	lua_pushnumber(m_lua, f);
}

void LuaTable::pushValue(const std::string& s)
{
	lua_pushstring(m_lua, s.c_str());
}

void LuaTable::pushValue(const LuaTable& t)
{
	const_cast<LuaTable&>(t).initRef();
	lua_rawgeti(m_lua, LUA_REGISTRYINDEX, t.m_ref);
}

LuaTable::Index::Index(int i): m_int(i), m_type(Type::INT) {}
LuaTable::Index::Index(const char* c) : m_string(c), m_type(Type::STRING) {}

LuaCall::LuaCall(lua_State* s, const char* globalFunction):
m_lua(s),
m_results(),
m_resultCount(0),
m_argCount(0),
m_errorMessage(nullptr)
{
	lua_getglobal(m_lua, globalFunction);
	CHECK_F(lua_isfunction(m_lua, -1));
}

LuaCall::LuaCall(lua_State* s, const LuaTable& t, const char* functionName):
m_lua(s),
m_results(),
m_resultCount(0),
m_argCount(0),
m_errorMessage(nullptr)
{
	t.pushTableAndValue(functionName);
	CHECK_F(lua_isfunction(m_lua, -1));
}

LuaCall::~LuaCall()
{
	if (m_errorMessage)
		lua_pcall(m_lua, m_argCount, m_resultCount, 0);
	else
		lua_call(m_lua, m_argCount, m_resultCount);
}

LuaCall& LuaCall::arg(bool b)
{
	lua_pushboolean(m_lua, b);
	return *this;
}

LuaCall& LuaCall::arg(int i)
{
	lua_pushnumber(m_lua, i);
	return *this;
}

LuaCall& LuaCall::arg(float f)
{
	lua_pushnumber(m_lua, f);
	return *this;
}

LuaCall& LuaCall::arg(const LuaTable& t)
{
	t.pushTable();
	return *this;
}

LuaCall& LuaCall::result(bool* b)
{
	CHECK_F(m_resultCount < countof(m_results));
	m_results[m_resultCount++] = { TYPE_BOOLEAN, b };
	return *this;
}

LuaCall& LuaCall::result(int* i)
{
	CHECK_F(m_resultCount < countof(m_results));
	m_results[m_resultCount++] = { TYPE_INTEGER, i };
	return *this;
}

LuaCall& LuaCall::result(float* f)
{
	CHECK_F(m_resultCount < countof(m_results));
	m_results[m_resultCount++] = { TYPE_NUMBER, f };
	return *this;
}

LuaCall& LuaCall::result(LuaTable* t)
{
	CHECK_F(m_resultCount < countof(m_results));
	m_results[m_resultCount++] = { TYPE_TABLE, t };
	return *this;
}

LuaCall& LuaCall::error(std::string* message)
{
	m_errorMessage = message;
	return *this;
}

lua_State* LuaStackCheck::s_defaultState = nullptr;

LuaStackCheck::LuaStackCheck(lua_State* s)
{
	if (!s) s = s_defaultState;
	m_lua = s;
	m_stack = lua_gettop(s);
}

LuaStackCheck::LuaStackCheck(LuaTable& t):
LuaStackCheck(t.m_lua)
{

}

LuaStackCheck::~LuaStackCheck()
{
	CHECK_F(m_stack == lua_gettop(m_lua), "Lua stack check failed (expected %d, got %d)\n", m_stack, lua_gettop(m_lua));
}

LuaTableResource::~LuaTableResource()
{
	reset();
	lua_close(m_lua);
}

LuaTableResource::LuaTableLoader::LuaTableLoader(const std::string& path, int flags) :m_path(path), m_file(NewPtr, path.c_str(), flags)
{

}

LuaTableResource::LuaTableLoader::~LuaTableLoader()
{

}

LuaTableResource* LuaTableResource::LuaTableLoader::load(std::tuple<int, std::string>* error)
{
	if (!ready(error, m_file))
		return nullptr;

	lua_State* state = luaL_newstate();
	LuaTable t = readLuaDataFile(state, *m_file);
	return new LuaTableResource(std::move(t));
}

std::string LuaTableResource::LuaTableLoader::getDebugName() const
{
	return m_path;
}

const char* LuaTableResource::LuaTableLoader::getTypeName() const
{
	return "LuaTable";
}

LuaTableResource::LuaTableLoader* LuaTableResource::createLoader(const char* path, int flags)
{
	return new LuaTableLoader(path, flags);
}

LuaIterator::LuaIterator(LuaTable& table) :
m_state(table.m_lua)
{
	table.pushTable();
	lua_pushnil(m_state);
	lua_pushnil(m_state); // push a second nil because next() will pop off the value which we don't have on first run

	int top = lua_gettop(m_state);
	m_keyIndex = top - 1;
	m_valueIndex = top;
}

LuaIterator::~LuaIterator()
{
	lua_pop(m_state, 1); // pop key
}

bool LuaIterator::next()
{
	lua_pop(m_state, 1); // pop value
	return lua_next(m_state, -2) != 0;
}

bool LuaIterator::_get(bool* b, int stack) const
{
	if (!lua_isboolean(m_state, stack))
		return false;

	if (b) *b = lua_toboolean(m_state, stack) != 0;
	return true;
}

bool LuaIterator::_get(int* i, int stack) const
{
	if (!lua_isinteger(m_state, stack))
		return false;

	if (i) *i = (int)lua_tointeger(m_state, stack);
	return true;
}

bool LuaIterator::_get(float* f, int stack) const
{
	if (!lua_isnumber(m_state, stack))
		return false;

	if (f) *f = (float)lua_tonumber(m_state, stack);
	return true;
}

bool LuaIterator::_get(std::string* s, int stack) const
{
	if (!lua_isstring(m_state, stack))
		return false;

	if (s) *s = lua_tostring(m_state, stack);
	return true;
}

bool LuaIterator::_get(LuaTable* t, int stack) const
{
	if (!lua_istable(m_state, stack))
		return false;

	if (t)
	{
		lua_pushvalue(m_state, stack);
		*t = LuaTable::fromStack(m_state, -1);
	}
	return true;
}

template<> void push<bool>(lua_State* s, const bool& b)
{
	lua_pushboolean(s, b);
}

template<> void push<int>(lua_State* s, const int& i)
{
	lua_pushnumber(s, (lua_Number)i);
}

template<> void push<float>(lua_State* s, const float& f)
{
	lua_pushnumber(s, f);
}

template<> void push<std::string>(lua_State* s, const std::string& string)
{
	lua_pushstring(s, string.c_str());
}

LuaTable readLuaDataFile(lua_State* s, const File& f)
{
	const std::string& path = f.getPath();
	ERROR_CONTEXT("readLuaDataFile", path.c_str());
	LuaStackCheck lsc(s);

	std::string contents(f.getContents(), f.getSize());
	int error = luaL_dostring(s, contents.c_str());
	if (error)
	{
		lua_pop(s, 1); // pop the error message

		LOG_F(1, "Failed to read lua data file (%s), trying to add \"return\"\n", path.c_str());
		size_t firstBracket = contents.find('{');
		if (firstBracket == std::string::npos)
		{
			LOG_F(ERROR, "Invalid lua data file (%s)\n", path.c_str());
			return LuaTable(s);
		}

		contents.insert(firstBracket, "return ");
		error = luaL_dostring(s, contents.c_str());
		if (error)
		{
			LOG_F(ERROR, "Invalid lua data file (%s): %s\n", path.c_str(), lua_tostring(s, -1));
			lua_pop(s, 1);
			return LuaTable(s);
		}
	}

	return LuaTable(s, -1);
}

void writeLuaDataFile(const LuaTable& t, const char* path)
{
	std::ofstream f(path, std::ios_base::trunc);
	LOG_IF_F(FATAL, !f.good(), "Failed to open file %s\n", path);
	LOG_IF_F(FATAL, !t.valid(), "Invalid table given to writeLuaDataFile\n");

	t.serialize(f);
}

std::string escapeLuaString(const char* c)
{
	std::string output;
	output.reserve(strlen(c));
	while(*c != '\0')
	{
		switch (*c)
		{
		case '\a': output += "\\a"; break;
		case '\b': output += "\\b"; break;
		case '\f': output += "\\f"; break;
		case '\n': output += "\\n"; break;
		case '\r': output += "\\r"; break;
		case '\t': output += "\\t"; break;
		case '\v': output += "\\v"; break;
		case '"': output += "\\\""; break;
		case '\'': output += "\\\'"; break;
		default: output += *c; break;
		}
		c++;
	}

	return output;
}

void printStack(lua_State* s)
{
	int size = lua_gettop(s);
	LOG_F(INFO, "{Stack size: %d\n", size);
	for (int i = 1; i <= size; i++)
	{
		int type = lua_type(s, i);
		switch (type)
		{
		case LUA_TNONE: LOG_F(INFO, ". %d: %s\n", i, lua_typename(s, type)); break;
		case LUA_TNIL: LOG_F(INFO, ". %d: %s\n", i, lua_typename(s, type)); break;
		case LUA_TBOOLEAN: LOG_F(INFO, ". %d: %s = %s\n", i, lua_typename(s, type), lua_toboolean(s, i) ? "true" : "false"); break;
		case LUA_TLIGHTUSERDATA: LOG_F(INFO, ". %d: %s\n", i, lua_typename(s, type)); break;
		case LUA_TNUMBER: LOG_F(INFO, ". %d: %s = %f\n", i, lua_typename(s, type), lua_tonumber(s, i)); break;
		case LUA_TSTRING: LOG_F(INFO, ". %d: %s = \"%s\"\n", i, lua_typename(s, type), lua_tostring(s, i)); break;
		case LUA_TTABLE: LOG_F(INFO, ". %d: %s\n", i, lua_typename(s, type)); break;
		case LUA_TFUNCTION: LOG_F(INFO, ". %d: %s\n", i, lua_typename(s, type)); break;
		case LUA_TUSERDATA: LOG_F(INFO, ". %d: %s = 0x%X\n", i, lua_typename(s, type), lua_touserdata(s, i)); break;
		case LUA_TTHREAD: LOG_F(INFO, ". %d: %s\n", i, lua_typename(s, type)); break;
		default: LOG_F(INFO, ". %d: %s\n", i, lua_typename(s, type)); break;
		}
	}
	LOG_F(INFO, "}\n");
}