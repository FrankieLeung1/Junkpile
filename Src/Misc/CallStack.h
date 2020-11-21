#pragma once

class StringView;
class CallStack
{
public:
	CallStack();
	~CallStack();

	void trace();
	std::size_t size();
	std::tuple<std::string, int> getLineNumber(std::size_t) const;
	std::string getFunctionName(std::size_t) const;

	std::string str() const;

protected:
	std::vector<void*> m_stack;
};