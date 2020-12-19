#include "stdafx.h"
#include "StringView.h"

StringView::StringView():m_begin(nullptr), m_end(nullptr) {}
StringView::StringView(std::nullptr_t) : StringView() {}
StringView::StringView(const char* begin, const char* end): m_begin(begin), m_end(end) {}
StringView::StringView(const char* s, std::size_t size): m_begin(s), m_end(s + (size == std::string::npos ? strlen(s) : size) ){ }
StringView::StringView(const std::string& s, std::size_t size):m_begin(s.c_str()), m_end(m_begin + (size == std::string::npos ? s.size() : size)) { }
StringView::~StringView() { }
bool StringView::operator==(const StringView& v) const { const char* c1 = m_begin, * c2 = v.m_begin; while (c1 != m_end && c2 != v.m_end) { if (*c1++ != *c2++) return false; } return true; }
bool StringView::operator==(const char* c) const { int i = 0; const char* current = m_begin; while (current != m_end && c[i] != '\0') { if (*current != c[i]) return false; current++; i++; } return true; }
bool StringView::operator==(const std::string& s) const { return s.compare(0, m_end - m_begin, m_begin) == 0; }
std::string StringView::str() const { return m_begin ? std::string(m_begin, m_end) : std::string(); }
const char* StringView::c_str() const { return m_begin; }
std::size_t StringView::size() const { return m_begin ? m_end - m_begin : 0; }
StringView::operator const char* () const { return m_begin; }
StringView::operator std::string() const { return m_begin ? std::string(m_begin, m_end) : std::string(); }