#pragma once

#include "../Meta/Meta.h"
namespace Meta
{
	class PythonRegisterer : public Visitor
	{
	public:
		PythonRegisterer();
		~PythonRegisterer();

		int visit(const char* name, bool&);
		int visit(const char* name, int&);
		int visit(const char* name, float&);
		int visit(const char* name, std::string&);

		int visit(const char* name, bool*);
		int visit(const char* name, int*);
		int visit(const char* name, float*);
		int visit(const char* name, std::string*);
		int visit(const char* name, void* object, const Object&);

		int startObject(const char* name, const Meta::Object& objectInfo);
		int endObject();

		int startArray(const char* name);
		int endArray(std::size_t);

		int startFunction(const char* name, bool hasReturn);
		int endFunction();
	};
}