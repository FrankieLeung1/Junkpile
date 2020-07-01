#include "stdafx.h"
#include "StringView.h"

StringView::StringView():m_begin(nullptr), m_end(nullptr) {}
StringView::StringView(const char* s): m_begin(s), m_end(s + strlen(s) + 1){ }
StringView::StringView(const std::string& s):m_begin(s.c_str()), m_end(m_begin + s.size() + 1) { }
StringView::~StringView() { }
std::string StringView::str() const { return m_begin ? std::string(m_begin, m_end) : std::string(); }
const char* StringView::c_str() const { return m_begin; }
std::size_t StringView::size() const { return m_begin ? m_end - m_begin : 0; }
StringView::operator const char* () const { return m_begin; }
StringView::operator std::string() const { return m_begin ? std::string(m_begin, m_end) : std::string(); }