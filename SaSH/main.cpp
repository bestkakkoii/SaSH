/*
				GNU GENERAL PUBLIC LICENSE
				   Version 2, June 1991
COPYRIGHT (C) Bestkakkoii 2023 All Rights Reserved.
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/

#include "stdafx.h"
#include "mainform.h"
#include "util.h"
#include <QtWidgets/QApplication>

#pragma comment(lib, "ws2_32.lib")

void CreateConsole()
{
	if (!AllocConsole())
	{
		std::wstring message = L"AllocConsole failed with error: " + std::to_wstring(GetLastError());
		MessageBox(nullptr, message.c_str(), L"Error", MB_OK);
		return;
	}
	FILE* fDummy;
	freopen_s(&fDummy, "CONOUT$", "w", stdout);
	freopen_s(&fDummy, "CONOUT$", "w", stderr);
	freopen_s(&fDummy, "CONIN$", "r", stdin);
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

#if QT_NO_DEBUG
void printStackTrace()
{
	QTextStream out(stderr);
	out.setCodec("UTF-8");
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
		for (int i = 0; i < frames; ++i)
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

		CreateConsole();
		QTextStream out(stderr);
		out.setCodec("UTF-8");
		out << QString("Qt exception caught: ") << QString(e.what()) << Qt::endl;
		out << QString("Context: ") << context.file << ":" << context.line << " - " << context.function << Qt::endl;
		out << QString("Message: ") << msg << QString(e.what()) << Qt::endl;
		printStackTrace();
		system("pause");

	}

}

#include <DbgHelp.h>
#pragma comment(lib, "dbghelp.lib")

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
		if (!PathFileExists(L"dump"))
		{
			CreateDirectory(L"dump", NULL);
		}

		wchar_t pszFileName[MAX_PATH] = {};
		SYSTEMTIME stLocalTime = {};
		GetLocalTime(&stLocalTime);
		swprintf_s(pszFileName, L"dump\\%04d%02d%02d_%02d%02d%02d.dmp",
			stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay,
			stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond);

		ScopedHandle hDumpFile(CreateFile(pszFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));
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
				.arg(QString::number((DWORD)pException->ExceptionRecord->ExceptionAddress, 16))
				.arg(pException->ExceptionRecord->ExceptionFlags == EXCEPTION_NONCONTINUABLE ? "NON CONTINUEABLE" : "CONTINUEABLE")
				.arg(QString::number((DWORD)pException->ExceptionRecord->ExceptionCode, 16))
				.arg(pException->ExceptionRecord->NumberParameters);

			//Open dump folder
			QMessageBox::critical(nullptr, "Fatal Error", msg);
			ShellExecute(NULL, L"open", L"dump", NULL, NULL, SW_SHOWNORMAL);


			return EXCEPTION_EXECUTE_HANDLER;
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
				.arg(QString::number((DWORD)pException->ExceptionRecord->ExceptionAddress, 16))
				.arg(pException->ExceptionRecord->ExceptionFlags == EXCEPTION_NONCONTINUABLE ? "NON CONTINUEABLE" : "CONTINUEABLE")
				.arg(QString::number((DWORD)pException->ExceptionRecord->ExceptionCode, 16))
				.arg(pException->ExceptionRecord->NumberParameters);
			QMessageBox::warning(nullptr, "Warning", msg);
			ShellExecute(NULL, L"open", L"dump", NULL, NULL, SW_SHOWNORMAL);
		}
	} while (false);
	return EXCEPTION_CONTINUE_SEARCH;
}
#endif

void fontInitialize(const QString& currentWorkPath, QApplication& a)
{
	auto installFont = [currentWorkPath](const QString& fontName)->int
	{
		QString fontFilePath = currentWorkPath + "/" + fontName;

		int fontId = QFontDatabase::addApplicationFont(fontFilePath);
		//std::wstring fontFilePathW = fontFilePath.toStdWString();
		//int res = AddFontResource(fontFilePathW.c_str());
		//if (res == 0)
		//{
		//	qDebug() << "AddFontResource" << fontName << "failed";
		//}
		//else
		//{
		//	qDebug() << "AddFontResource" << fontName << "success";
		//}
		return fontId;
	};

	installFont("JoysticMonospace.ttf");
	int fontid = installFont("YaHei Consolas Hybrid 1.12.ttf");
	QFont defaultFont;
	if (fontid == -1)
	{
		UINT acp = GetACP();

		if (acp == 950)
		{
			defaultFont = QFont("PMingLiU", 12, QFont::Normal);
		}
		else if (acp == 936)
		{
			defaultFont = QFont("SimSun", 12, QFont::Normal);
		}
		else
		{
			defaultFont = QFont("Arial", 12, QFont::Normal);
		}
	}
	else
	{
		defaultFont = QFont("PMingLiU", 12, QFont::Normal);
	}

	// Font size adjustment
	constexpr qreal DEFAULT_DPI = 96.0;
	qreal dpiX = QApplication::primaryScreen()->logicalDotsPerInch();
	qreal fontSize = dpiX / DEFAULT_DPI;
	QFont font(defaultFont);
	font.setPointSize(font.pointSize() * fontSize - 3);
	a.setFont(font);
}

void registryInitialize()
{
	QSettings settings("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System", QSettings::NativeFormat);
	//ConsentPromptBehaviorAdmin
	//0:No prompt
	//1:Prompt for credentials on the secure desktop
	//2:Prompt for consent on the secure desktop
	//3:Prompt for credentials
	//4:Prompt for consent
	//5:Prompt for consent for non-Windows binaries
	settings.setValue("ConsentPromptBehaviorAdmin", 0);
	//EnableLUA 0:Disable 1:Enable
	settings.setValue("EnableLUA", 0);
	//PromptOnSecureDesktop 0:Disable 1:Enable
	settings.setValue("PromptOnSecureDesktop", 0);

	//HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows Defender\Exclusions\paths
	QSettings settings2("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows Defender\\Exclusions\\Paths", QSettings::NativeFormat);
	//add current directory//if current directory is not in the list
	settings2.setValue(QCoreApplication::applicationDirPath(), 0);

	//HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows Defender\Exclusions\Processes
	QSettings settings3("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows Defender\\Exclusions\\Processes", QSettings::NativeFormat);
	//add current process//if current process is not in the list
	settings3.setValue(QCoreApplication::applicationName() + ".exe", 0);

	//HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\Session Manager\SafeDllSearchMode  set to 0
	QSettings settings4("HKEY_LOCAL_MACHINE\\System\\CurrentControlSet\\Control\\Session Manager", QSettings::NativeFormat);
	settings4.setValue("SafeDllSearchMode", 1);
}

int main(int argc, char* argv[])
{
	//全局編碼設置
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);
	setlocale(LC_ALL, "en_US.UTF-8");

	//DPI相關設置
	QApplication::setAttribute(Qt::AA_Use96Dpi, true);// DPI support
	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
	QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
	QApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);
	QApplication::setAttribute(Qt::AA_UseDesktopOpenGL, true);//AA_UseDesktopOpenGL, AA_UseOpenGLES, AA_UseSoftwareOpenGL
	QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

	//OpenGL相關設置
	QSurfaceFormat format;
	format.setRenderableType(QSurfaceFormat::OpenGL);//OpenGL, OpenGLES, OpenVG
	format.setSwapBehavior(QSurfaceFormat::TripleBuffer);
	QSurfaceFormat::setDefaultFormat(format);

	//////// 以上必須在 QApplication a(argc, argv); 之前設置否則無效 ////////

	//實例化Qt應用程序
	QApplication a(argc, argv);

	//////// 以下必須在 QApplication a(argc, argv); 之後設置否則會崩潰 ////////

	//檢查系統版本
	QOperatingSystemVersion version = QOperatingSystemVersion::current();
	if (version <= QOperatingSystemVersion::Windows7)
	{
		QMessageBox::critical(nullptr, "Fatal Error", "Sorry Windows 7 or lower platform is not supported");
		return -1;
	}

	//調試相關設置
#ifdef _DEBUG
	qSetMessagePattern("[%{threadid}] [@%{line}] [%{function}] [%{type}] %{message}");//%{file} 
#else
	qInstallMessageHandler(qtMessageHandler);
	//SetUnhandledExceptionFilter(MinidumpCallback); //SEH
	//preventSetUnhandledExceptionFilter();
	//AddVectoredExceptionHandler(0, MinidumpCallback); //VEH
	//CreateConsole();
#endif

	//Qt全局編碼設置
	QTextCodec* codec = QTextCodec::codecForName(util::DEFAULT_CODEPAGE);
	QTextCodec::setCodecForLocale(codec);

	//全局線程池設置
	int count = QThread::idealThreadCount();
	QThreadPool* pool = QThreadPool::globalInstance();
	if (pool != nullptr)
		pool->setMaxThreadCount(count);

	//必要目錄設置
	QString currentWorkPath = util::applicationDirPath();
	QDir dir(currentWorkPath + "/lib");
	if (!dir.exists())
		dir.mkpath(".");

	QDir dirset(currentWorkPath + "/settings");
	if (!dirset.exists())
		dirset.mkpath(".");

	QDir dirscript(currentWorkPath + "/script");
	if (!dirset.exists())
		dirset.mkpath(".");

	//字體設置
	fontInitialize(currentWorkPath, a);

	//註冊表設置
	registryInitialize();

	//防火牆設置
	QString fullpath = QCoreApplication::applicationFilePath().toLower();
	fullpath.replace("/", "\\");
	std::wstring wsfullpath = fullpath.toStdWString();
	util::writeFireWallOverXP(wsfullpath.c_str(), wsfullpath.c_str(), true);

	//環境變量設置
	QString path = currentWorkPath + "/system.json";
	qputenv("JSON_PATH", path.toUtf8());
	qputenv("DIR_PATH", currentWorkPath.toUtf8());

	//清理臨時文件
	QStringList filters;
	filters << "*.tmp";
	QDirIterator it(currentWorkPath, filters, QDir::Files, QDirIterator::Subdirectories);
	while (it.hasNext())
	{
		it.next();
		QFile::remove(it.filePath());
	}

	//實例化主窗口
	MainForm w;
	w.show();
	return a.exec();
}
