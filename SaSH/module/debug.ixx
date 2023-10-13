module;

#include <Windows.h>
#include <DbgHelp.h>
#include <QException>
#include <QMessageBox>
#include <QTextStream>
#include <string>
#include "injector.h"

#pragma comment(lib, "dbghelp.lib")

export module debug;
import <iostream>;
import string;
import global;

namespace sash
{
	void CreateConsole()
	{
		if (!AllocConsole())
		{
			return;
		}

		FILE* fDummy;
		::freopen_s(&fDummy, "CONOUT$", "w", stdout);
		::freopen_s(&fDummy, "CONOUT$", "w", stderr);
		::freopen_s(&fDummy, "CONIN$", "r", stdin);
		std::cout.clear();
		std::clog.clear();
		std::cerr.clear();
		std::cin.clear();

		// std::wcout, std::wclog, std::wcerr, std::wcin
		HANDLE hConOut = CreateFile(TEXT("CONOUT$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		HANDLE hConIn = CreateFile(TEXT("CONIN$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		SetStdHandle(STD_OUTPUT_HANDLE, hConOut);
		SetStdHandle(STD_ERROR_HANDLE, hConOut);
		SetStdHandle(STD_INPUT_HANDLE, hConIn);
		std::wcout.clear();
		std::wclog.clear();
		std::wcerr.clear();
		std::wcin.clear();

		SetConsoleCP(CP_UTF8);
		SetConsoleOutputCP(CP_UTF8);
		setlocale(LC_ALL, "en_US.UTF-8");
	}

	void printStackTrace()
	{
		sash::TextStream out(stderr);
		void* stack[100];
		unsigned short frames;
		SYMBOL_INFO* symbol;
		HANDLE process;
		process = GetCurrentProcess();
		SymInitialize(process, NULL, TRUE);
		frames = CaptureStackBackTrace(0, 100, stack, NULL);
		symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
		if (symbol)
		{
			symbol->MaxNameLen = 255;
			symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
			for (__int64 i = 0; i < frames; ++i)
			{
				SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol);
				out << i << ": " << QString(symbol->Name) << " - " << symbol->Address << Qt::endl;
			}

			free(symbol);
		}
	}

	void qtMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
	{
		if (type != QtCriticalMsg && type != QtFatalMsg)
		{
			return;
		}

		try
		{
			throw QException();
		}
		catch (const QException& e)
		{
			if (QString(e.what()).contains("Unknown exception"))
			{
				return;
			}

			for (__int64 i = 0; i < SASH_MAX_THREAD; ++i)
			{
				Injector* pinstance = nullptr;
				if (Injector::get(i, &pinstance) && pinstance != nullptr)
					pinstance->log->close();
			}

			CreateConsole();
			sash::TextStream out(stderr);
			out << QString("Qt exception caught: ") << QString(e.what()) << Qt::endl;
			out << QString("Context: ") << context.file << ":" << context.line << " - " << context.function << Qt::endl;
			out << QString("Message: ") << msg << QString(e.what()) << Qt::endl;
			printStackTrace();
			system("pause");

		}

	}

#if defined _M_X64 || defined _M_IX86
	LPTOP_LEVEL_EXCEPTION_FILTER WINAPI
		dummySetUnhandledExceptionFilter(
			LPTOP_LEVEL_EXCEPTION_FILTER)
	{
		return NULL;
	}
#else
#error "This code works only for x86 and x64!"
#endif

	BOOL preventSetUnhandledExceptionFilter()
	{
		HMODULE hKernel32 = LoadLibraryW(L"kernel32.dll");
		if (hKernel32 == nullptr)
			return FALSE;

		void* pOrgEntry = GetProcAddress(hKernel32, "SetUnhandledExceptionFilter");
		if (pOrgEntry == nullptr)
			return FALSE;

		DWORD dwOldProtect = 0;
		SIZE_T jmpSize = 5;
#ifdef _M_X64
		jmpSize = 13;
#endif
		BOOL bProt = VirtualProtect(pOrgEntry, jmpSize, PAGE_EXECUTE_READWRITE, &dwOldProtect);

		BYTE newJump[20];
		memset(newJump, 0, sizeof(newJump));
		void* pNewFunc = &dummySetUnhandledExceptionFilter;
#ifdef _M_IX86
		DWORD dwOrgEntryAddr = (DWORD)pOrgEntry;
		dwOrgEntryAddr += jmpSize; // add 5 for 5 op-codes for jmp rel32
		DWORD dwNewEntryAddr = (DWORD)pNewFunc;
		DWORD dwRelativeAddr = dwNewEntryAddr - dwOrgEntryAddr;
		// JMP rel32: Jump near, relative, displacement relative to next instruction.
		newJump[0] = 0xE9;  // JMP rel32
		memcpy(&newJump[1], &dwRelativeAddr, sizeof(pNewFunc));
#elif _M_X64
		// We must use R10 or R11, because these are "scratch" registers 
		// which need not to be preserved accross function calls
		// For more info see: Register Usage for x64 64-Bit
		// http://msdn.microsoft.com/en-us/library/ms794547.aspx
		// Thanks to Matthew Smith!!!
		newJump[0] = 0x49;  // MOV R11, ...
		newJump[1] = 0xBB;  // ...
		memcpy(&newJump[2], &pNewFunc, sizeof(pNewFunc));
		//pCur += sizeof (ULONG_PTR);
		newJump[10] = 0x41;  // JMP R11, ...
		newJump[11] = 0xFF;  // ...
		newJump[12] = 0xE3;  // ...
#endif
		SIZE_T bytesWritten;
		BOOL bRet = WriteProcessMemory(GetCurrentProcess(),
			pOrgEntry, newJump, jmpSize, &bytesWritten);

		if (bProt != FALSE)
		{
			DWORD dwBuf;
			VirtualProtect(pOrgEntry, jmpSize, dwOldProtect, &dwBuf);
		}
		return bRet;
	}

	LONG CALLBACK MinidumpCallback(PEXCEPTION_POINTERS pException)
	{
		do
		{
			if (!pException)
				break;

			//忽略可繼續執行的
			if (pException->ExceptionRecord->ExceptionFlags != EXCEPTION_NONCONTINUABLE)
			{
				return EXCEPTION_CONTINUE_EXECUTION;
			}

			auto PathFileExists = [](const wchar_t* name)->BOOL
				{
					DWORD dwAttrib = GetFileAttributes(name);
					return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
				};

			// Check if dump directory exists
			if (!PathFileExists(L".\\lib\\dump"))
			{
				CreateDirectory(L".\\lib\\dump", NULL);
			}

			wchar_t pszFileName[MAX_PATH] = {};
			SYSTEMTIME stLocalTime = {};
			GetLocalTime(&stLocalTime);
			swprintf_s(pszFileName, L"lib\\dump\\%04d%02d%02d_%02d%02d%02d.dmp",
				stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay,
				stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond);

			HANDLE hDumpFile = CreateFile(pszFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hDumpFile == INVALID_HANDLE_VALUE)
				break;

			MINIDUMP_EXCEPTION_INFORMATION dumpInfo = {};
			dumpInfo.ExceptionPointers = pException;
			dumpInfo.ThreadId = GetCurrentThreadId();
			dumpInfo.ClientPointers = TRUE;

			MiniDumpWriteDump(
				GetCurrentProcess(),
				GetCurrentProcessId(),
				hDumpFile,
				(MINIDUMP_TYPE)(
					MiniDumpNormal
					| MiniDumpWithFullMemory
					| MiniDumpWithHandleData
					| MiniDumpWithThreadInfo
					| MiniDumpWithUnloadedModules
					| MiniDumpWithProcessThreadData
					),
				&dumpInfo,
				NULL,
				NULL
			);

			if (pException->ExceptionRecord->ExceptionFlags == EXCEPTION_NONCONTINUABLE)
			{

				QString msg = QString(
					"A fatal error occured, \r\n"
					"it's a noncontinuable exception, \r\n"
					"sorry but we have to terminate this program.\r\n"
					"please send minidump to developer.\r\n"
					"Basic Infomations:\r\n\r\n"
					"ExceptionAddress:0x%1\r\n"
					"ExceptionFlags:%2\r\n"
					"ExceptionCode:0x%3\r\n"
					"NumberParameters:%4")
					.arg(QString::number(reinterpret_cast<unsigned __int64>(pException->ExceptionRecord->ExceptionAddress), 16))
					.arg(pException->ExceptionRecord->ExceptionFlags == EXCEPTION_NONCONTINUABLE ? "NON CONTINUEABLE" : "CONTINUEABLE")
					.arg(QString::number(static_cast<unsigned __int64>(pException->ExceptionRecord->ExceptionCode), 16))
					.arg(pException->ExceptionRecord->NumberParameters);

				//Open dump folder
				//QMessageBox::critical(nullptr, "Fatal Error", msg);
				//ShellExecute(NULL, L"open", L"dump", NULL, NULL, SW_SHOWNORMAL);

				for (__int64 i = 0; i < SASH_MAX_THREAD; ++i)
				{
					Injector* pinstance = nullptr;
					if (Injector::get(i, &pinstance) && pinstance != nullptr)
						pinstance->log->close();
				}

				throw EXCEPTION_EXECUTE_HANDLER;

				return EXCEPTION_CONTINUE_SEARCH;
			}
			else
			{
				QString msg = QString(
					"A warning error occured, it's a continuable exception \r\npress \'continue\' to skip this exception\r\n\r\n"
					"Basic Infomations:\r\n\r\n"
					"ExceptionAddress:0x%1\r\n"
					"ExceptionFlags:%2\r\n"
					"ExceptionCode:0x%3\r\n"
					"NumberParameters:%4")
					.arg(QString::number(reinterpret_cast<unsigned __int64>(pException->ExceptionRecord->ExceptionAddress), 16))
					.arg(pException->ExceptionRecord->ExceptionFlags == EXCEPTION_NONCONTINUABLE ? "NON CONTINUEABLE" : "CONTINUEABLE")
					.arg(QString::number(static_cast<unsigned __int64>(pException->ExceptionRecord->ExceptionCode), 16))
					.arg(pException->ExceptionRecord->NumberParameters);
				//QMessageBox::warning(nullptr, "Warning", msg);
				//ShellExecute(NULL, L"open", L"dump", NULL, NULL, SW_SHOWNORMAL);
			}
		} while (false);

		return EXCEPTION_CONTINUE_SEARCH;
	}

	}
