#include "stdafx.h"
#include "StringView.h"

StringView::StringView():m_begin(nullptr), m_end(nullptr) {}
StringView::StringView(const char* begin, const char* end): m_begin(begin), m_end(end) {}
StringView::StringView(const char* s, std::size_t size): m_begin(s), m_end(s + (size == std::string::npos ? strlen(s) : size) + 1){ }
StringView::StringView(const std::string& s, std::size_t size):m_begin(s.c_str()), m_end(m_begin + (size == std::string::npos ? s.size() + 1 : size)) { }
StringView::~StringView() { }
std::string StringView::str() const { return m_begin ? std::string(m_begin, m_end) : std::string(); }
const char* StringView::c_str() const { return m_begin; }
std::size_t StringView::size() const { return m_begin ? m_end - m_begin : 0; }
StringView::operator const char* () const { return m_begin; }
StringView::operator std::string() const { return m_begin ? std::string(m_begin, m_end) : std::string(); }