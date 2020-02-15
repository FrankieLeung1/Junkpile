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

	const char* getResPath();
	const char* getExternalPath();
}