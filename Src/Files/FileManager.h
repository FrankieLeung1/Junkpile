#pragma once

#include <fstream>
#include "File.h"
#include "../Resources/ResourceManager.h"
#include "../Managers/EventManager.h"

struct FileImplWin32 : public FileImpl
{
	~FileImplWin32() {};
	std::vector<char> m_contents;
};

struct FileChangeEvent : public Event<FileChangeEvent>
{
	std::vector<std::string> m_files;
};

class FileManager : public SingletonResource<FileManager>, FW::FileWatchListener
{
public:
	FileManager();
	~FileManager();

	void addPath(const char*);
	void removePath(const char*);

	void update();

	bool exists(const char*) const;
	
	enum Type {NotFound, File, Directory, Other};
	Type type(const char*) const;

	struct FileInfo
	{
		std::string m_path;
		Type m_type;
	};
	std::vector<FileInfo> files(const char* dir) const;
	void reducePath(std::string&) const;
	
	void save(const char* path, std::vector<char>&&);

protected:
	std::string resolvePath(const char*) const;
	Type type(DWORD) const;
	void handleFileAction(FW::WatchID watchid, const FW::String& dir, const FW::String& filename, FW::Action action);

protected:
	std::vector<std::string> m_paths;
	ResourcePtr<ThreadPool> m_threadPool;
	FW::FileWatcher m_fileWatcher;
	
	struct FileChange
	{
		float m_time;
		std::wstring m_file;
	};
	std::vector<FileChange> m_bufferedFileChanges;
	float m_lastFileChange;

	friend class File;
};