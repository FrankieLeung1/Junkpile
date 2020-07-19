#include "stdafx.h"
#include "Serializer.h"
#include "../Misc/Misc.h"

namespace Meta {
Serializer::Serializer():
m_stream(),
m_indentStack()
{
	m_indentStack.push(0);
}

Serializer::~Serializer()
{

}

int Serializer::visit(const char* name, bool& b)
{
	return writeValue(name, b);
}

int Serializer::visit(const char* name, int& i)
{
	return writeValue(name, i);
}

int Serializer::visit(const char* name, float& f)
{
	return writeValue(name, f);
}

int Serializer::visit(const char* name, std::string& s)
{
	return writeValue(name, '"' + escape(s) + '"');
}

int Serializer::visit(const char* name, bool* b)
{
	return writeValue(name, '"' + stringf("0x%p", b) + '"');
}

int Serializer::visit(const char* name, int* i)
{
	return writeValue(name, '"' + stringf("0x%p", i) + '"');
}

int Serializer::visit(const char* name, float* f)
{
	return writeValue(name, '"' + stringf("0x%p", f) + '"');
}

int Serializer::visit(const char* name, std::string* s)
{
	return writeValue(name, '"' + stringf("0x%p", s) + '"');
}

int Serializer::visit(const char* name, void** object, const Object&)
{
	return writeValue(name, '"' + stringf("0x%p", object) + '"');
}

int Serializer::visit(const char* name, void* object, const Object&)
{
	return writeValue(name, '"' + stringf("0x%p", object) + '"');
}

int Serializer::startObject(const char* name, const Meta::Object& objectInfo)
{
	prepareForValue();
	modifyIndent(1);

	m_stream << sanitizeForKey(std::string(name)) << " = {";

	return 0;
}

int Serializer::endObject()
{
	m_stream << "\n";
	modifyIndent(-1);
	addIndents();
	m_stream << '}';

	return 0;
}

int Serializer::startArray(const char* name)
{
	prepareForValue();
	m_stream << sanitizeForKey(std::string(name)) << " = {";
	modifyIndent(1);

	return 0;
}

int Serializer::endArray(std::size_t)
{
	m_stream << "\n";
	modifyIndent(-1);
	addIndents();
	m_stream << "}";
	return 0;
}

int Serializer::startFunction(const char* name, bool hasReturn, bool isConstructor)
{
	return 0;
}

int Serializer::endFunction()
{
	return 0;
}

int Serializer::startFunctionObject(const char* name, bool hasReturn)
{
	return 0;
}

int Serializer::endFunctionObject()
{
	return 0;
}

std::string Serializer::getString() const
{
	return m_stream.str();
}

void Serializer::prepareForValue()
{
	m_stream << (m_indentStack.top() == 0 ? "\n" : ",\n");
	addIndents();

	m_indentStack.top()++;
}

void Serializer::addIndents()
{
	for (int i = 0; i < m_indentStack.size() - 1; ++i)
		m_stream.put('\t');
}

std::string Serializer::sanitizeForKey(std::string& s) const
{
	auto valid = [](int ch) { return ch == '_' || isalnum(ch) != 0; };

	if (std::find_if_not(s.begin(), s.end(), valid) == s.end())
		return std::move(s);

	return "[\"" + escape(s) + "\"]";
}

void Serializer::modifyIndent(int offset)
{
	CHECK_F(offset == 1 || offset == -1);
	if (offset == -1)
	{
		m_indentStack.pop();
	}
	else
	{
		m_indentStack.push(0);
	}
	
}

}