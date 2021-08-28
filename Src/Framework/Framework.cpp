#include "stdafx.h"
#include "Framework.h"
#include <Windows.h>
#include "dbghelp.h"
#include "Shlwapi.h"
#include "../Misc/Misc.h"
#include "../Misc/CallStack.h"

using namespace Framework;
struct Framework::ProcessHandle {
	PROCESS_INFORMATION m_information;
	HANDLE m_readPipe, m_writePipe, m_errorPipe;
};

ProcessHandle* Framework::popen(const char* cmd)
{
	ProcessHandle* handle = new ProcessHandle;
	HANDLE hChildStd_IN_Rd = NULL;
	HANDLE hChildStd_IN_Wr = NULL;
	HANDLE hChildStd_OUT_Rd = NULL;
	HANDLE hChildStd_OUT_Wr = NULL;
	HANDLE hChildStd_ERR_Rd = NULL;
	HANDLE hChildStd_ERR_Wr = NULL;

	auto ErrorExit = [&](const char* error) { 
		if(hChildStd_IN_Rd) CloseHandle(hChildStd_IN_Rd);
		if (hChildStd_IN_Wr) CloseHandle(hChildStd_IN_Wr);
		if (hChildStd_OUT_Rd) CloseHandle(hChildStd_OUT_Rd);
		if (hChildStd_OUT_Wr) CloseHandle(hChildStd_OUT_Wr);
		if (hChildStd_ERR_Rd) CloseHandle(hChildStd_ERR_Rd);
		if (hChildStd_ERR_Wr) CloseHandle(hChildStd_ERR_Wr);
		delete handle;
		LOG_F(ERROR, error);
		return nullptr; 
	};

	SECURITY_ATTRIBUTES saAttr;

	// Set the bInheritHandle flag so pipe handles are inherited. 

	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	// Create a pipe for the child process's STDERR. 

	if (!CreatePipe(&hChildStd_ERR_Rd, &hChildStd_ERR_Wr, &saAttr, 0))
		return ErrorExit("Stderr CreatePipe");

	// Ensure the read handle to the pipe for STDERR is not inherited. 

	if (!SetHandleInformation(hChildStd_ERR_Rd, HANDLE_FLAG_INHERIT, 0))
		return ErrorExit("Stdin SetHandleInformation");

	// Create a pipe for the child process's STDOUT. 

	if (!CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0))
		return ErrorExit("StdoutRd CreatePipe");

	// Ensure the read handle to the pipe for STDOUT is not inherited.

	if (!SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
		return ErrorExit("Stdout SetHandleInformation");

	// Create a pipe for the child process's STDIN. 

	if (!CreatePipe(&hChildStd_IN_Rd, &hChildStd_IN_Wr, &saAttr, 0))
		return ErrorExit("Stdin CreatePipe");

	// Ensure the write handle to the pipe for STDIN is not inherited. 

	if (!SetHandleInformation(hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0))
		return ErrorExit("Stdin SetHandleInformation");

	// Create the child process. 

	STARTUPINFOA siStartInfo;
	BOOL bSuccess = FALSE;

	// Set up members of the PROCESS_INFORMATION structure. 

	ZeroMemory(&handle->m_information, sizeof(PROCESS_INFORMATION));

	// Set up members of the STARTUPINFO structure. 
	// This structure specifies the STDIN and STDOUT handles for redirection.

	ZeroMemory(&siStartInfo, sizeof(STARTUPINFOA));
	siStartInfo.cb = sizeof(STARTUPINFOA);
	siStartInfo.hStdError = hChildStd_ERR_Wr;
	siStartInfo.hStdOutput = hChildStd_OUT_Wr;
	siStartInfo.hStdInput = hChildStd_IN_Rd;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

	// Create the child process. 

	bSuccess = CreateProcessA(NULL,
		const_cast<char*>(cmd),			// command line 
		NULL,							// process security attributes 
		NULL,						    // primary thread security attributes 
		TRUE,							// handles are inherited 
		CREATE_NO_WINDOW,			    // creation flags 
		NULL,						    // use parent's environment 
		NULL,							// use parent's current directory 
		&siStartInfo,					// STARTUPINFO pointer 
		&handle->m_information);		// receives PROCESS_INFORMATION 

					   // If an error occurs, exit the application. 
	if (!bSuccess)
		ErrorExit("CreateProcess");

	handle->m_readPipe = hChildStd_OUT_Rd;
	handle->m_writePipe = hChildStd_IN_Wr;
	handle->m_errorPipe = hChildStd_ERR_Rd;

	return handle;
}

std::size_t Framework::read(ProcessHandle* process, char* buffer, std::size_t bufferSize)
{
	CHECK_F(process != nullptr && buffer != nullptr && bufferSize > 0);
	DWORD sizeRead = 0;
	LOG_IF_F(ERROR, PeekNamedPipe(process->m_readPipe, NULL, 0, NULL, &sizeRead, NULL) == FALSE, "PeekNamedPipe failed\n");
	if (sizeRead <= 0)
		return 0;

	LOG_IF_F(ERROR, ReadFile(process->m_readPipe, buffer, std::min((DWORD)bufferSize, sizeRead), &sizeRead, NULL) == FALSE, "ReadFile failed\n");
	return sizeRead;
}

std::size_t Framework::write(ProcessHandle* process, const char* buffer, std::size_t bufferSize)
{
	CHECK_F(process != nullptr && buffer != nullptr && bufferSize > 0);
	DWORD sizeWritten = 0;
	LOG_IF_F(ERROR, WriteFile(process->m_writePipe, buffer, (DWORD)bufferSize, &sizeWritten, NULL) == FALSE, "WriteFile failed\n");
	return sizeWritten;
}

std::size_t Framework::readError(ProcessHandle* process, char* buffer, std::size_t bufferSize)
{
	CHECK_F(process != nullptr && buffer != nullptr && bufferSize > 0);
	DWORD sizeRead = 0;
	LOG_IF_F(ERROR, PeekNamedPipe(process->m_errorPipe, NULL, 0, NULL, &sizeRead, NULL) == FALSE, "PeekNamedPipe failed\n");
	if (sizeRead <= 0)
		return 0;

	LOG_IF_F(ERROR, ReadFile(process->m_errorPipe, buffer, std::min((DWORD)bufferSize, sizeRead), &sizeRead, NULL) == FALSE, "ReadFile failed\n");
	return sizeRead;
}

bool Framework::pclose(ProcessHandle* process)
{
	// Close handles to the child process and its primary thread.
	// Some applications might keep these handles to monitor the status
	// of the child process, for example. 
	CloseHandle(process->m_information.hProcess);
	CloseHandle(process->m_information.hThread);
	delete process;
	return true;
}

void Framework::closeWrite(ProcessHandle* process)
{
	CHECK_F(process != nullptr);
	CloseHandle(process->m_writePipe);
}

bool Framework::getExitCode(ProcessHandle* process, int* exitCode)
{
	CHECK_F(process != nullptr);

	DWORD dwordExitCode;
	LOG_IF_F(ERROR, GetExitCodeThread(process->m_information.hThread, &dwordExitCode) == 0, "getExitCodeThread failed: %X\n", GetLastError());
	if(exitCode) *exitCode = (int)dwordExitCode;
	return dwordExitCode != STILL_ACTIVE;
}

Framework::GUID Framework::createGUID()
{
	::GUID guid;
	HRESULT r = CoCreateGuid(&guid);
	CHECK_F(SUCCEEDED(r));
	CHECK_F(sizeof(Framework::GUID) == sizeof(::GUID));

	Framework::GUID result;
	memcpy(&result, &guid, sizeof(result));
	return result;
}

const char* Framework::getResPath()
{
	return "../Res/";
}

const char* Framework::getExternalPath()
{
	return "../External/";
}