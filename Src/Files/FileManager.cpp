#include "stdafx.h"
#include "FileManager.h"
#include "../Misc/Misc.h"
#include "../Managers/TimeManager.h"
#include "../Threading/ThreadPool.h"

FileManager::FileManager()
{
	addPath("");
	addPath("../../Res/");

	m_fileWatcher.addWatch(L"../../Res/", this, true);
}

FileManager::~FileManager()
{
	
}

void FileManager::addPath(const char* path)
{
	CHECK_F(std::find(m_paths.begin(), m_paths.end(), path) == m_paths.end());
	m_paths.emplace_back(path);
}

void FileManager::removePath(const char* path)
{
	m_paths.erase(std::remove(m_paths.begin(), m_paths.end(), path), m_paths.end());
}

void FileManager::update()
{
	m_fileWatcher.update();
}

bool FileManager::exists(const char* path) const
{
	return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
}

void FileManager::save(const char* path, std::vector<char>&& buffer)
{
	std::vector<char>* contents = new std::vector<char>(std::move(buffer));
	auto save = [contents, path](){
		std::fstream f(path, std::fstream::binary | std::fstream::out | std::fstream::trunc);
		f.write(&contents->front(), contents->size());
		LOG_IF_F(WARNING, !f.good(), "Failed to write to \"%s\"", path);
		delete contents;
	};
	m_threadPool->enqueue(save);
	CHECK_F(buffer.empty());
}

void FileManager::handleFileAction(FW::WatchID watchid, const FW::String& dir, const FW::String& filename, FW::Action action)
{
	char* actionStr;
	switch (action)
	{
	case FW::Action::Add: actionStr = "added"; break;
	case FW::Action::Delete: actionStr = "deleted"; break;
	case FW::Action::Modified:
	default: actionStr = "modified"; break;
	}

	LOG_F(INFO, "file %s: %S %S\n", actionStr, dir.c_str(), filename.c_str());

	FileChange* fileChange = nullptr;
	auto changeIt = std::find_if(m_fileChanges.begin(), m_fileChanges.end(), [&](const FileChange& f) { return f.m_file == filename; });
	if (changeIt != m_fileChanges.end())
	{
		fileChange = &(*changeIt);
	}
	else
	{
		m_fileChanges.emplace_back();
		fileChange = &m_fileChanges.back();
	}

	ResourcePtr<TimeManager> time;
	fileChange->m_file = filename;
	fileChange->m_time = time->getTime();
}

std::string FileManager::resolvePath(const char* path) const
{
	char buffer[256];
	std::vector<std::string>::const_reverse_iterator it = m_paths.rbegin();
	while (it != m_paths.rend())
	{
		std::size_t pathSize = strlen(path);

		memcpy(buffer, it->c_str(), it->size());
		memcpy(buffer + it->size(), path, strlen(path) + 1);
		LOG_IF_F(FATAL, (countof(buffer) < it->size() + pathSize + 1), "Increase buffer size\n");
		if (std::experimental::filesystem::exists(buffer))
			return buffer;

		++it;
	}

	LOG_F(WARNING, "File not found: %s\n", path);
	return std::string();
}