#pragma once

#include <fstream>
#include "File.h"
#include "../Resources/ResourceManager.h"

struct FileImplWin32 : public FileImpl
{
	~FileImplWin32() {};
	std::vector<char> m_contents;
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

	std::vector<std::string> files(const char* dir) const;
	
	void save(const char* path, std::vector<char>&&);

protected:
	std::string resolvePath(const char*) const;
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
	std::vector<FileChange> m_fileChanges;

	friend class File;
};