#include "stdafx.h"
#include "PythonRegisterer.h"

Meta::PythonRegisterer::PythonRegisterer()
{

}

Meta::PythonRegisterer::~PythonRegisterer()
{

}

int Meta::PythonRegisterer::visit(const char* name, bool&)
{
	return 0;
}

int Meta::PythonRegisterer::visit(const char* name, int&)
{
	return 0;
}

int Meta::PythonRegisterer::visit(const char* name, float&)
{
	return 0;
}

int Meta::PythonRegisterer::visit(const char* name, std::string&)
{
	return 0;
}

int Meta::PythonRegisterer::visit(const char* name, bool*)
{
	return 0;
}

int Meta::PythonRegisterer::visit(const char* name, int*)
{
	return 0;
}

int Meta::PythonRegisterer::visit(const char* name, float*)
{
	return 0;
}

int Meta::PythonRegisterer::visit(const char* name, std::string*)
{
	return 0;
}

int Meta::PythonRegisterer::visit(const char* name, void* object, const Object&)
{
	return 0;
}

int Meta::PythonRegisterer::startObject(const char* name, const Meta::Object& objectInfo)
{
	return 0;
}

int Meta::PythonRegisterer::endObject()
{
	return 0;
}

int Meta::PythonRegisterer::startArray(const char* name)
{
	return 0;
}

int Meta::PythonRegisterer::endArray(std::size_t)
{
	return 0;
}

int Meta::PythonRegisterer::startFunction(const char* name, bool hasReturn)
{
	return 0;
}

int Meta::PythonRegisterer::endFunction()
{
	return 0;
}