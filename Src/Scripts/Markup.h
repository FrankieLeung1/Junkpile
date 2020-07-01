#pragma once
#include "../Misc/StringView.h"

class Markup
{
public:
	Markup();
	~Markup();

	void parseScript(StringView script);
	std::string markUp(StringView script) const;

	enum class VariableType { Float, String };
	struct Mark
	{
		VariableType m_type;
		std::string m_name;
	};
	const std::vector<Mark>& getMarks() const;

	void setValue(StringView name, float);
	void setValue(StringView name, StringView);

protected:
	std::vector<Mark> m_marks;
};