#pragma once

#include <tuple>
namespace Framework
{
	struct ProcessHandle;
	ProcessHandle* popen(const char* cmd);
	bool pclose(ProcessHandle*);

	void closeWrite(ProcessHandle*);
	bool getExitCode(ProcessHandle*, int*);

	std::size_t read(ProcessHandle*, char* buffer, std::size_t bufferSize);
	std::size_t write(ProcessHandle*, const char* buffer, std::size_t bufferSize);
	std::size_t readError(ProcessHandle*, char* buffer, std::size_t bufferSize);

	struct GUID
	{
		uint32_t Data1;
		uint16_t Data2;
		uint16_t Data3;
		uint8_t  Data4[8];
	};
	GUID createGUID();

	const char* getResPath();
	const char* getExternalPath();
}