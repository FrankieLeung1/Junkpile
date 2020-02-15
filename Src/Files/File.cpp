#include "stdafx.h"
#include "File.h"
#include "FileManager.h"
#include "../Misc/Misc.h"

File::File(const char* path):
m_path(path)
{
	
}

File::~File()
{

}

std::size_t File::getSize() const
{
	return m_contents.size();
}

const char* File::getContents() const
{
	return &m_contents[0];
}

const std::string& File::getPath() const
{
	return m_path;
}

File::FileLoader::FileLoader(const char* path) : m_path(path) {}
Resource* File::FileLoader::load(std::tuple<int, std::string>* error)
{
	std::string path = m_fileManager->resolvePath(m_path.c_str());
	if (path.empty())
	{
		*error = { FileNotFound, stringf("\"%s\" not found", m_path.c_str()) };
		return nullptr;
	}

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

const char* File::FileLoader::getTypeName() const { return "File"; }
File::FileLoader* File::createLoader(const char* path) { return new FileLoader(path); }

std::tuple<bool, std::size_t> File::getSharedHash(const char* path)
{
	return{ true, std::hash<std::string>{}(path) };
}

std::string normalizePath(const char* path)
{
	std::string s(path);
	normalizePath(s);
	return std::move(s);
}

void normalizePath(std::string& path)
{
	std::replace(path.begin(), path.end(), '/', '\\');
}