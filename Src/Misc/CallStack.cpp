#include "stdafx.h"
#include "Callstack.h"
#include "dbghelp.h"
#include "Shlwapi.h"
#include "Misc.h"

//#define USE_STACKWALK // slower but more accurate
//#define LOG_ERROR(...) LOG_F(ERROR, ...)
#define LOG_ERROR(...) 

CallStack::CallStack()
{
	static bool initedSym = false;
	if (!initedSym)
	{
		initedSym = true;
		SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
		SymInitialize(GetCurrentProcess(), NULL, TRUE);
	}

	trace();
}

CallStack::~CallStack()
{
}

#ifdef USE_STACKWALK
void CallStack::trace()
{
	CONTEXT currentContext;
	RtlCaptureContext(&currentContext);

	STACKFRAME64 frame;
	memset(&frame, 0x0, sizeof(frame));
	frame.AddrPC.Offset = currentContext.Rip;
	frame.AddrPC.Mode = AddrModeFlat;
	frame.AddrStack.Offset = currentContext.Rsp;
	frame.AddrStack.Mode = AddrModeFlat;
	frame.AddrFrame.Offset = currentContext.Rbp;
	frame.AddrFrame.Mode = AddrModeFlat;
	frame.Virtual = TRUE;

	m_stack.clear();
	for (int i = 0; i < 100; i++)
	{
		if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, GetCurrentProcess(), GetCurrentThread(), &frame, &currentContext, NULL,
			SymFunctionTableAccess64, SymGetModuleBase64, NULL))
			break; // Couldn't trace back through any more frames.

		if (frame.AddrFrame.Offset == 0)
			break; // End of stack.

		m_stack.push_back((void*)frame.AddrPC.Offset);
	}
}

#else
void CallStack::trace()
{
	void* stack[100];
	unsigned short stackCount = CaptureStackBackTrace(0, 100, stack, NULL);
	m_stack.resize(stackCount);
	for (int i = 0; i < stackCount; i++)
		m_stack[i] = stack[i];
}

#endif

std::size_t CallStack::size()
{
	return m_stack.size();
}

std::tuple<std::string, int> CallStack::getLineNumber(std::size_t index) const
{
	if (index > m_stack.size())
	{
		LOG_ERROR("Index out of range %d\n", index);
		return { "", -1 };
	}

	IMAGEHLP_LINE64 info;
	info.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
	DWORD displacement = 0;
	if (!SymGetLineFromAddr64(GetCurrentProcess(), (DWORD64)(m_stack[index]), &displacement, &info))
	{
		LOG_ERROR("SymGetLineFromAddr64 failed %X\n", GetLastError());
		return { "", -1 };
	}

	return { info.FileName, info.LineNumber };
}

std::string CallStack::getFunctionName(std::size_t index) const
{
	SYMBOL_INFO* symbol = (SYMBOL_INFO*)alloca(sizeof(SYMBOL_INFO) + 256 * sizeof(char));
	symbol->MaxNameLen = 255;
	symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
	DWORD64 dwDisplacement = 0;
	if (!SymFromAddr(GetCurrentProcess(), (DWORD64)(m_stack[index]), &dwDisplacement, symbol))
	{
		LOG_ERROR("SymGetSymFromAddr64 failed %X\n", GetLastError());
		return std::string();
	}
	return symbol->Name;
}

std::string CallStack::str() const
{
	std::stringstream ss;
	for (int i = 0; i < m_stack.size(); i++)
	{
		std::string functionName = getFunctionName(i);
		std::tuple<std::string, int> line = getLineNumber(i);
		ss << std::get<0>(line) << '(' << std::get<1>(line) << ") : " << functionName << std::endl;

		if (functionName == "WinMain" || functionName == "main")
			break;
	}
	return ss.str();
}