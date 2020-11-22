#pragma once

#include "../Misc/Callbacks.h"
#include "../Misc/StringView.h"
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
	File(StringView path, HANDLE hFile, HANDLE hMapping, void* content);
	~File();

	std::int64_t getModificationTime() const;
	std::size_t getSize() const;
	StringView getContents() const;

	const std::string& getPath() const;

	class FileLoader : public Loader
	{
	public:
		FileLoader(StringView path, int flags = 0);
		Resource* load(std::tuple<int, std::string>* error);
		std::string getDebugName() const;
		StringView getTypeName() const;

	protected:
		int m_flags;
		std::string m_path;
		ResourcePtr<FileManager> m_fileManager;
	};
	static FileLoader* createLoader(StringView path, int flags = 0);
	static std::tuple<bool, std::size_t> getSharedHash(StringView path, int flags = 0);

protected:
	std::string m_path;
	std::int64_t m_modificationTime;
	std::size_t m_size;
	StringView m_content;
	
	HANDLE m_hFile, m_hMapping;
};