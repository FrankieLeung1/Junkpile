#pragma once

#include <string>
#include <vector>
#include "Meta.h"

struct lua_State;
namespace Meta
{
	class LuaRegisterer : public Visitor
	{
	public:
		LuaRegisterer();
		~LuaRegisterer();

		int visit(const char* name, bool&);
		int visit(const char* name, int&);
		int visit(const char* name, float&);
		int visit(const char* name, std::string&);
		int visit(const char* name, void* object, const Object&);

		int visit(const char* name, bool*);
		int visit(const char* name, int*);
		int visit(const char* name, float*);
		int visit(const char* name, std::string*);
		int visit(const char* name, void** object, const Object&);

		int startObject(const char* name, void* v, const Meta::Object& objectInfo);
		int endObject();

		int startArray(const char* name);
		int endArray(std::size_t);

		int startFunction(const char* name, bool hasReturn, bool isConstructor);
		int endFunction();

		int startFunctionObject(const char* name, bool hasReturn);
		int endFunctionObject();

		template<typename T>
		void open(lua_State*);
		void open(const Object&);

		static void test();

	protected:
		static int call(lua_State*);

	protected:
		struct Function
		{
			std::string m_name;
			std::size_t m_argSize;
			enum class Type {None, Int, Float, String, Object};
			std::vector<Type> m_argTypes;
			Type m_returnType;
		};
		std::vector<Function> m_functions;
	};

	// ----------------------- IMPLEMENTATION ----------------------- 
	template<typename T>
	void LuaRegisterer::open(lua_State* s)
	{
		open(getMeta<T>());
	}
}