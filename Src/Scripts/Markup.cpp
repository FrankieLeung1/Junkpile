#include "stdafx.h"
#include "Markup.h"
#include "../Misc/Misc.h"

// `float:Speed:8.7f`

Markup::Markup()
{

}

Markup::~Markup()
{

}

void Markup::parseScript(StringView script)
{
	m_marks.clear();

	const char* current = script.c_str();
	const char* end = (current + script.size());
	std::size_t lineNum = 1;
	const char* lineStart = current;
	while (current != end)
	{
		if (*current == '`')
		{
			std::size_t startIndex = current - script.c_str();
			current++;

			const char* varEnd = std::find_if(current, end, [](const char c) { return c == '`' || c == '\n' || c == '\0'; });
			if (*varEnd != '`')
				throw ParseError{ "Cannot find closing `", lineNum, (std::size_t)(current - lineStart), (std::size_t)(current - script.c_str()) };

			std::size_t endIndex = varEnd - script.c_str() + 1;
			const char* typeEnd = std::find_if(current, varEnd, [](const char c) { return c == ':'; });
			if (*typeEnd != ':')
				typeEnd++;

			std::string typeStr(current, typeEnd);
			VariableType type;
			if (typeStr == "float") type = VariableType::Float;
			else if (typeStr == "string") type = VariableType::String;
			else if (typeStr == "RGB") type = VariableType::RGB;
			else if (typeStr == "RGBA") type = VariableType::RGBA;
			else throw ParseError{ stringf("Unknown Type:%s", typeStr.c_str()), lineNum, (std::size_t)(current - lineStart), (std::size_t)(current - script.c_str()) };

			current = typeEnd;
			std::string secondThing;
			if (*current == ':')
			{
				current++;
				const char* secondEnd = std::find_if(current, varEnd, [](const char c) { return c == ':'; });
				secondThing.assign(current, secondEnd - current);
				current = secondEnd;
			}

			std::string thirdThing;
			if (*current == ':')
			{
				current++;
				thirdThing.assign(current, varEnd - current);
			}

			if(!thirdThing.empty())
				m_marks.push_back({ type, std::move(secondThing), std::move(thirdThing), startIndex, endIndex });
			else if(!secondThing.empty())
				m_marks.push_back({ type, std::string(), std::move(secondThing), startIndex, endIndex });
			else
				m_marks.push_back({ type, std::string(), std::string(), startIndex, endIndex });

			current = varEnd;
		}
		else if (*current == '\n')
		{
			lineNum++;
			lineStart = current;
		}

		current++;
	}

	m_values.assign(m_marks.size(), decltype(m_values)::value_type());
}

std::string Markup::markUp(StringView scriptView) const
{
	if (m_marks.empty())
		return scriptView;

	const char* script = scriptView.c_str();
	const char* current = script;
	//const char* end = current + scriptView.size();
	std::stringstream ss;
	std::size_t markIndex = 0;
	std::size_t lastMarksEnd = 0;
	//for(auto it = m_marks.begin(); it != m_marks.end(); ++it)
	for(std::size_t i = 0; i < m_marks.size(); i++)
	{
		const Mark& mark = m_marks[i];
		const auto& value = m_values[i];

		ss.write(script + lastMarksEnd, mark.m_start - lastMarksEnd);

		if (value)
		{
			switch (mark.m_type)
			{
			case VariableType::Float: ss << value.get<float>(); break;
			case VariableType::String: ss << '"' << value.get<std::string>().c_str() << '"';  break;
			case VariableType::RGB:
				{
					glm::vec3 colour = value.get<glm::vec3>();
					ss << colour.x << ',' << colour.y << ',' << colour.z;
				}
				break;

			case VariableType::RGBA:
				{
					glm::vec4 colour = value.get<glm::vec4>();
					ss << colour.x << ',' << colour.y << ',' << colour.z << ',' << colour.w;
				}
				break;
			}
		}
		else
		{
			ss << mark.m_default;
		}

		lastMarksEnd = mark.m_end;
	}

	std::size_t s = scriptView.size() - lastMarksEnd;
	ss.write(script + lastMarksEnd, s);
	return std::move(ss.str());
}

const std::vector<Markup::Mark>& Markup::getMarks() const
{
	return m_marks;
}

StringView Markup::getName(std::size_t index) const
{
	return m_marks[index].m_name;
}

void Markup::setValue(std::size_t index, float f)
{
	CHECK_F(m_marks[index].m_type == VariableType::Float);
	m_values[index] = f;
}

void Markup::setValue(std::size_t index, StringView s)
{
	CHECK_F(m_marks[index].m_type == VariableType::String);
	m_values[index] = std::string(s.c_str(), s.size());
}

void Markup::setValue(std::size_t index, glm::vec3 v)
{
	CHECK_F(m_marks[index].m_type == VariableType::RGB);
	m_values[index] = v;
}

void Markup::setValue(std::size_t index, glm::vec4 v)
{
	CHECK_F(m_marks[index].m_type == VariableType::RGBA);
	m_values[index] = v;
}

bool Markup::getValue(std::size_t index, float* f) const
{
	CHECK_F(m_marks[index].m_type == VariableType::Float);
	auto& any = m_values[index];
	if (any) *f = any.get<float>();
	return any;
}

bool Markup::getValue(std::size_t index, std::string* s) const
{
	CHECK_F(m_marks[index].m_type == VariableType::String);
	auto& any = m_values[index];
	if (any) *s = any.get<std::string>();
	return any;
}

bool Markup::getValue(std::size_t index, glm::vec3* v) const
{
	CHECK_F(m_marks[index].m_type == VariableType::RGB);
	auto& any = m_values[index];
	if (any) *v = any.get<glm::vec3>();
	return any;
}

bool Markup::getValue(std::size_t index, glm::vec4* v) const
{
	CHECK_F(m_marks[index].m_type == VariableType::RGBA);
	auto& any = m_values[index];
	if (any) *v = any.get<glm::vec4>();
	return any;
}

bool Markup::getDefault(std::size_t index, std::string* s) const
{
	*s = m_marks[index].m_default;
	return true;
}