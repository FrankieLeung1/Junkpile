#pragma once

class StringView
{
public:
	StringView();
	StringView(const char*);
	StringView(const std::string&);
	~StringView();

	std::string str() const;
	const char* c_str() const;
	std::size_t size() const;

	operator const char*() const;
	operator std::string() const;

protected:
	const char *m_begin, *m_end;
};