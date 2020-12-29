#pragma once
#include "../Misc/StringView.h"

class Markup
{
public:
	struct ParseError
	{
		std::string m_error;
		std::size_t m_lineNumber, m_lineCharNumber, m_overallCharNumber;
	};
	
public:
	Markup();
	~Markup();

	void parseScript(StringView script);
	std::string markUp(StringView script) const;

	enum class VariableType { Float, String, RGB, RGBA };
	struct Mark
	{
		VariableType m_type;
		std::string m_name;
		std::string m_default;
		std::size_t m_start, m_end;
	};
	const std::vector<Mark>& getMarks() const;

	StringView getName(std::size_t index) const;
	void setValue(std::size_t index, float);
	void setValue(std::size_t index, StringView);
	void setValue(std::size_t index, glm::vec3);
	void setValue(std::size_t index, glm::vec4);

	bool getValue(std::size_t index, float*) const;
	bool getValue(std::size_t index, std::string*) const;
	bool getValue(std::size_t index, glm::vec3*) const;
	bool getValue(std::size_t index, glm::vec4*) const;

	bool getDefault(std::size_t index, std::string*) const;

	bool empty() const;

protected:
	std::vector<Mark> m_marks;
	std::vector< AnyWithSize<sizeof(std::string)> > m_values;
};