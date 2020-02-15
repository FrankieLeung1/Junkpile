#include "stdafx.h"
#include "Misc.h"
#include "xxhash/xxhash.h"
#include <shellapi.h>

void logToOutput(void*, const loguru::Message& message)
{
	char buffer[2048] = { 0 };
	sprintf_s(buffer, "%s(%d) : %s", message.filename, message.line, message.message);
	OutputDebugStringA(buffer);
}

LONG CALLBACK unhandledExceptionFilter(EXCEPTION_POINTERS* pointers)
{
	LOG_F(0, loguru::get_error_context().c_str());
	return EXCEPTION_EXECUTE_HANDLER;
}

void initLoggingForVisualStudio(const char* logname)
{
	loguru::g_internal_verbosity = loguru::Verbosity_9;
	SetUnhandledExceptionFilter(unhandledExceptionFilter);
	loguru::add_callback("VisualStudio", &logToOutput, nullptr, loguru::Verbosity_INFO);
	loguru::add_file(logname, loguru::Append, loguru::Verbosity_MAX);
}

int nextPowerOf2(int size)
{
	int n = -1;
	while (size >> ++n > 0);
	return 1 << n;
}

void bitblt(const BitBltBuffer& dest, int x, int y, int width, int height, const BitBltBuffer& src, int sx, int sy)
{
	LOG_IF_F(FATAL, dest.m_pixelSize != src.m_pixelSize, "Differing pixel sizes not supported yet");
	for (int row = 0; row < height; row++)
	{
		std::size_t destLineStride = dest.m_width * dest.m_pixelSize;
		std::size_t srcLineStride = src.m_width * src.m_pixelSize;
		char* d = &dest.m_buffer[(y + row) * destLineStride + (x * dest.m_pixelSize)];
		char* s = &src.m_buffer[(sy + row) * srcLineStride + (sx * src.m_pixelSize)];
		memcpy(d, s, width * dest.m_pixelSize);
	}
}

std::string escape(std::string& s)
{
	const char* escapedChars = "\"";
	if (s.find(escapedChars, 0) == std::string::npos)
		return std::move(s);

	std::stringstream ss;
	for (auto ch = s.begin(); ch != s.end(); ++ch)
	{
		const char* escapedChar = escapedChars;
		while (*escapedChar != '\0')
		{
			if (*escapedChar == *ch)
			{
				ss << '\\';
				break;
			}
			escapedChar++;
		}

		ss << *ch;
	}

	return ss.str();
}

std::string prettySize(std::size_t s)
{
	if (s < 1024) return stringf("%db", s);
	else if (s < 1024 * 1024) return stringf("%.2fkb", s / (1024.0));
	else if (s < 1024 * 1024 * 1024) return stringf("%.2fMB", s / (1024.0 * 1024.0));
	else return stringf("%.2fGB", s / (1024.0 * 1024.0 * 1024.0));
}

std::size_t generateHash(const void* buffer, std::size_t size)
{
	const XXH64_hash_t seed = 0x23f73f8daea1084c;
	static_assert(sizeof(std::size_t) >= sizeof(XXH64_hash_t), "");
	return XXH64(buffer, size, seed);
}

std::vector<Misc::TestResourceBase*> Misc::_testResources;
void deleteTestResources()
{
	for (auto res : Misc::_testResources)
	{
		delete res;
	}
	Misc::_testResources.clear();
}