#include "stdafx.h"
#include <time.h>
#include "File.h"
#include "FileManager.h"
#include "../Misc/Misc.h"

File::File(StringView path, HANDLE hFile, HANDLE hMapping, void* content):
m_path(path.str())
{
	CHECK_F(path.c_str() != nullptr && hFile != INVALID_HANDLE_VALUE && hMapping != INVALID_HANDLE_VALUE && content);

	LARGE_INTEGER size;
	if (!GetFileSizeEx(hFile, &size))
		LOG_F(ERROR, "GetFileSizeEx failed \"%s\" (%X)\n", path.c_str(), GetLastError());

	static_assert(sizeof(m_size) == sizeof(size), "");
	m_size = size.QuadPart;

	FILETIME fileTime;
	if (!GetFileTime(hFile, nullptr, nullptr, &fileTime))
		LOG_F(ERROR, "GetFileTime failed \"%s\" (%X)\n", path.c_str(), GetLastError());

	static_assert(sizeof(fileTime) == sizeof(m_modificationTime), "");
	memcpy_s(&m_modificationTime, sizeof(m_modificationTime), &fileTime, sizeof(fileTime));

	m_hFile = hFile;
	m_hMapping = hMapping;
	m_content = { static_cast<char*>(content), static_cast<char*>(content) + m_size };
}

File::~File()
{
	if(!UnmapViewOfFile(m_content))
		LOG_F(ERROR, "UnmapViewOfFile failed \"%s\" %X", m_path.c_str(), GetLastError());

	if (!CloseHandle(m_hMapping))
		LOG_F(ERROR, "CloseHandle failed \"%s\" %X", m_path.c_str(), GetLastError());

	if (!CloseHandle(m_hFile))
		LOG_F(ERROR, "CloseHandle failed \"%s\" %X", m_path.c_str(), GetLastError());
}

std::int64_t File::getModificationTime() const
{
	return m_modificationTime;
}

std::size_t File::getSize() const
{
	return m_size;
}

StringView File::getContents() const
{
	return m_content;
}

const std::string& File::getPath() const
{
	return m_path;
}

File::FileLoader::FileLoader(StringView path, int flags) : m_path(path.str()), m_flags(flags) {}
Resource* File::FileLoader::load(std::tuple<int, std::string>* error)
{
	std::string path = m_fileManager->resolvePath(m_path.c_str());
	if (path.empty() && (m_flags & File::CreateIfDoesNotExist) == 0)
	{
		*error = { FileNotFound, stringf("\"%s\" not found", m_path.c_str()) };
		return nullptr;
	}

	DWORD flags = (m_flags & File::CreateIfDoesNotExist) ? CREATE_NEW : OPEN_EXISTING;
	HANDLE hFile = CreateFileA(path.c_str(), GENERIC_READ, 0, nullptr, flags, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		if (GetLastError() == ERROR_FILE_EXISTS)
			hFile = CreateFileA(path.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

		if (hFile == INVALID_HANDLE_VALUE)
		{
			auto error2 = GetLastError();

			LOG_F(ERROR, "%s %X\n", m_path.c_str(), error2);
			*error = { SystemError, stringf("CreateFileA failed \"%s\" (%X)", m_path.c_str(), GetLastError()) };
			return nullptr;
		}
	}

	HANDLE hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (hMapping == INVALID_HANDLE_VALUE)
	{
		CloseHandle(hFile);
		*error = { SystemError, stringf("CreateFileMapping failed \"%s\" (%X)", m_path.c_str(), GetLastError()) };
		return nullptr;
	}

	void* m = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
	if (!m)
	{
		CloseHandle(hMapping);
		CloseHandle(hFile);
		*error = { SystemError, stringf("MapViewOfFile failed \"%s\" (%X)", m_path.c_str(), GetLastError()) };
		return nullptr;
	}
	return new File(m_path.c_str(), hFile, hMapping, m);
}

// Probably don't need this because files are locked
/*File::Reloader* File::FileLoader::createReloader()
{
	std::string& path = m_path;
	int flags = m_flags;
	auto create = [path, flags]() { return new FileLoader(path, flags); };
	return new ReloaderOnFileChange(path, create);
}*/

std::string File::FileLoader::getDebugName() const
{
	return m_path;
}

StringView File::FileLoader::getTypeName() const { return "File"; }
File::FileLoader* File::createLoader(StringView path, int flags) { return new FileLoader(path, flags); }

std::tuple<bool, std::size_t> File::getSharedHash(StringView path, int flags)
{
	return{ true, std::hash<std::string>{}(path) };
}