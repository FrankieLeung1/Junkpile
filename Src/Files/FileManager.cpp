#include "stdafx.h"
#include "FileManager.h"
#include "../Misc/Misc.h"
#include "../Managers/TimeManager.h"
#include "../Threading/ThreadPool.h"

FileManager::FileManager():
m_lastFileChange(0)
{
	addPath("");
	addPath("../Res/");

	m_fileWatcher.addWatch(L"../Res/", this, true);

	ResourcePtr<EventManager> events;
	events->addListener<UpdateEvent>([this](const UpdateEvent*) { this->update(); return EventManager::ListenerResult::Persist; });
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
	if (!m_bufferedFileChanges.empty())
	{
		ResourcePtr<TimeManager> t;
		if (t->getTime() - m_lastFileChange > 0.5f)
		{
			ResourcePtr<EventManager> events;
			FileChangeEvent* e = events->addOneFrameEvent<FileChangeEvent>();
			for(auto& change : m_bufferedFileChanges)
				e->m_files.push_back(normalizePath(toUtf8(change.m_file)));

			e->m_files.erase(std::unique(e->m_files.begin(), e->m_files.end()), e->m_files.end());

			m_bufferedFileChanges.clear();
		}
	}
}

bool FileManager::exists(const char* path) const
{
	return type(path) != Type::NotFound;
}

FileManager::Type FileManager::type(const char* path) const
{
	DWORD t = GetFileAttributesA(path);
	if (t == INVALID_FILE_ATTRIBUTES)
		return Type::NotFound;

	if (t & FILE_ATTRIBUTE_DIRECTORY)
		return Type::Directory;

	if (t & FILE_ATTRIBUTE_SYSTEM)
		return Type::Other;

	return Type::File;
}

std::vector<std::string> FileManager::files(const char* dir) const
{
	std::string path = resolvePath(dir);
	path.append(1, '/');

	WIN32_FIND_DATAA data;
	HANDLE hFind = FindFirstFileA((path + "*").c_str(), &data);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		LOG_F(ERROR, "FindFirstFile failed (%d)\n", GetLastError());
		return {};
	}
	else
	{
		std::vector<std::string> r;
		do {
			if (data.cFileName[0] != '.')
			{
				r.push_back(normalizePath(path + data.cFileName));
			}
		}
		while (FindNextFileA(hFind, &data));
		FindClose(hFind);

		return std::move(r);
	}
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

	LOG_F(1, "file %s: %S %S\n", actionStr, dir.c_str(), filename.c_str());

	FileChange* fileChange = nullptr;
	auto changeIt = std::find_if(m_bufferedFileChanges.begin(), m_bufferedFileChanges.end(), [&](const FileChange& f) { return f.m_file == filename; });
	if (changeIt != m_bufferedFileChanges.end())
	{
		fileChange = &(*changeIt);
	}
	else
	{
		m_bufferedFileChanges.emplace_back();
		fileChange = &m_bufferedFileChanges.back();
	}

	ResourcePtr<TimeManager> time;
	fileChange->m_file = filename;
	fileChange->m_time = time->getTime();

	m_lastFileChange = fileChange->m_time;
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
		if (exists(buffer))
			return buffer;

		++it;
	}

	LOG_F(WARNING, "File not found: %s\n", path);
	return std::string();
}