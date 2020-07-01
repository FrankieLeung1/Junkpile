#include "stdafx.h"
#include "Markup.h"

Markup::Markup()
{

}

Markup::~Markup()
{

}

void Markup::parseScript(StringView script)
{

}

std::string Markup::markUp(StringView script) const
{
	return script;
}

const std::vector<Markup::Mark>& Markup::getMarks() const
{
	return m_marks;
}

void Markup::setValue(StringView name, float)
{

}

void Markup::setValue(StringView name, StringView)
{

}