#pragma once

class StringView
{
public:
	StringView();
	StringView(const char*, const char*);
	StringView(const char*, std::size_t size = std::string::npos);
	StringView(const std::string&, std::size_t size = std::string::npos);
	~StringView();

	std::string str() const;
	const char* c_str() const;
	std::size_t size() const;

	operator const char*() const;
	operator std::string() const;

protected:
	const char *m_begin, *m_end;
};