#pragma once

#include <string>
#include <stack>
#include "Meta.h"

namespace Meta
{
	class Serializer : public Visitor
	{
	public:
		Serializer();
		~Serializer();

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

		std::string getString() const;

	protected:
		void prepareForValue();
		void addIndents();
		std::string sanitizeForKey(std::string&) const;
		void modifyIndent(int offset);

	protected:
		template<typename T>
		int writeValue(const char* name, const T& v);

	protected:
		std::stringstream m_stream;
		std::stack<int> m_indentStack;
	};

	// ----------------------- IMPLEMENTATION -----------------------
	template<typename T>
	int Serializer::writeValue(const char* name, const T& v)
	{
		prepareForValue();
		if (name)
			m_stream << sanitizeForKey(std::string(name)) << " = " << v;
		else
			m_stream << v;

		return 0;
	}
}