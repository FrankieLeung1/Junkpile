#pragma once

#include "../Misc/Callbacks.h"
#include "../Resources/ResourceManager.h"

struct FileImpl
{
	virtual ~FileImpl(){};
};

class FileManager;
class File : public Resource
{
public:
	static const int CreateIfDoesNotExist = 1;

public:
	File(const char* path);
	~File();

	std::size_t getSize() const;
	const char* getContents() const;

	const std::string& getPath() const;

	class FileLoader : public Loader
	{
	public:
		FileLoader(const char* path, int flags = 0);
		Resource* load(std::tuple<int, std::string>* error);
		std::string getDebugName() const;
		const char* getTypeName() const;

	protected:
		int m_flags;
		std::string m_path;
		ResourcePtr<FileManager> m_fileManager;
	};
	static FileLoader* createLoader(const char* path, int flags = 0);
	static std::tuple<bool, std::size_t> getSharedHash(const char* path, int flags = 0);

protected:
	std::string m_path;
	std::vector<char> m_contents;
};

extern std::string normalizePath(const char* path);
extern void normalizePath(std::string& path);