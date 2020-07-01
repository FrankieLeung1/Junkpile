#include "stdafx.h"
#include <time.h>
#include "File.h"
#include "FileManager.h"
#include "../Misc/Misc.h"

File::File(StringView path):
m_path(path.str())
{
	struct stat s;
	stat(path, &s);
	m_modification = s.st_mtime;
}

File::~File()
{

}

std::size_t File::getSize() const
{
	return m_contents.size();
}

StringView File::getContents() const
{
	return &m_contents[0];
}

const std::string& File::getPath() const
{
	return m_path;
}

bool File::checkChanged()
{
	struct stat s;
	stat(m_path.c_str(), &s);
	if (s.st_mtime > m_modification)
	{
		std::fstream fstream(m_path, std::ios::in | std::fstream::binary);
		fstream.seekg(0, fstream.end);
		std::size_t size = fstream.tellg();
		fstream.seekg(0, fstream.beg);

		m_contents.resize(size);
		fstream.read(&m_contents[0], size);

		m_modification = s.st_mtime;

		return true;
	}

	return false;
}

File::FileLoader::FileLoader(StringView path, int flags) : m_path(path.str()), m_flags(flags) {}
Resource* File::FileLoader::load(std::tuple<int, std::string>* error)
{
	std::string path = m_fileManager->resolvePath(m_path.c_str());
	if (path.empty())
	{
		*error = { FileNotFound, stringf("\"%s\" not found", m_path.c_str()) };
		return nullptr;
	}

	// TODO: m_flags & File::CreateIfDoesNotExist
	std::fstream fstream(path, std::ios::in | std::fstream::binary);
	if (!fstream.good())
	{
		*error = { FileNotFound, stringf("\"%s\" unaccessable", m_path.c_str()) };
		return nullptr;
	}

	fstream.seekg(0, fstream.end);
	std::size_t size = fstream.tellg();
	fstream.seekg(0, fstream.beg);

	File* f = new File(path.c_str());
	f->m_contents.resize(size);
	fstream.read(&f->m_contents[0], size);
	return f;
}

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