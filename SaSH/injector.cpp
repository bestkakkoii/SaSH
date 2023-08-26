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
#include <injector.h>
#include "model/listview.h"

Injector* Injector::instance = nullptr;

constexpr const char* InjectDllName = u8"sadll.dll";
constexpr int MessageTimeout = 3000;

inline __declspec(naked) DWORD* getKernel32()
{
	__asm
	{
		mov eax, fs: [0x30] ;
		mov eax, [eax + 0xC];
		mov eax, [eax + 0x1C];
		mov eax, [eax];
		mov eax, [eax];
		mov eax, [eax + 8];
		ret;
	}
}

Injector::Injector()
	: globalMutex(QMutex::NonRecursive)
{
	scriptLogModel.reset(new StringListModel);
	chatLogModel.reset(new StringListModel);
}

Injector::~Injector()
{
	qDebug() << "Injector is destroyed";
}

void Injector::clear()//static
{
	if (instance != nullptr)
	{
		instance->server.reset(nullptr);
		instance->hModule_ = NULL;
		instance->hookdllModule_ = NULL;
		instance->pi_ = {  };
		instance->processHandle_.reset();
		//紀錄當前設置
		util::SafeHash<util::UserSetting, int> valueHash = instance->getValueHash();
		util::SafeHash<util::UserSetting, bool> enableHash = instance->getEnableHash();
		util::SafeHash<util::UserSetting, QString> stringHash = instance->getStringHash();

		QStringList _serverNameList = instance->serverNameList.get();

		QStringList _subServerNameList = instance->subServerNameList.get();

		int _currentServerListIndex = instance->currentServerListIndex;

		QString _currentScriptFileName = instance->currentScriptFileName;

		delete instance;
		instance = new Injector();
		if (instance)
		{
			//恢復設置
			instance->setValueHash(valueHash);
			instance->setEnableHash(enableHash);
			instance->setStringHash(stringHash);

			instance->serverNameList.set(_serverNameList);

			instance->subServerNameList.set(_subServerNameList);

			instance->currentServerListIndex = _currentServerListIndex;

			instance->currentScriptFileName = _currentScriptFileName;
		}
	}
}

bool Injector::createProcess(Injector::process_information_t& pi)
{
	const QString fileName(qgetenv("JSON_PATH"));
	if (fileName.isEmpty())
		return false;

	util::Config config(fileName);

	QString path = currentGameExePath;
	if (path.isEmpty() || !QFile::exists(path))
	{
		return false;
	}

	int nRealBin = 138;
	int nAdrnBin = 138;
	int nSprBin = 116;
	int nSprAdrnBin = 116;
	int nRealTrue = 13;
	int nAdrnTrue = 5;
	int nEncode = 0;

	bool canSave = false;
	int tmp = config.read<int>("System", "Command", "realbin");
	if (tmp)
		nRealBin = tmp;
	else
		canSave = true;

	tmp = config.read<int>("System", "Command", "adrnbin");
	if (tmp)
		nAdrnBin = tmp;
	else
		canSave = true;

	tmp = config.read<int>("System", "Command", "sprbin");
	if (tmp)
		nSprBin = tmp;
	else
		canSave = true;

	tmp = config.read<int>("System", "Command", "spradrnbin");
	if (tmp)
		nSprAdrnBin = tmp;
	tmp = config.read<int>("System", "Command", "realtrue");
	if (tmp)
		nRealTrue = tmp;
	else
		canSave = true;

	tmp = config.read<int>("System", "Command", "adrntrue");
	if (tmp)
		nAdrnTrue = tmp;
	else
		canSave = true;

	tmp = config.read<int>("System", "Command", "encode");
	if (tmp)
		nEncode = tmp;
	else
		canSave = true;

	auto mkcmd = [](const QString& sec, int value)->QString
	{
		return QString("%1:%2").arg(sec).arg(value);
	};

	QStringList commandList;
	//啟動參數

	//updated realbin:138 adrnbin:138 sprbin:116 spradrnbin:116 adrntrue:5 realtrue:13 encode:0 windowmode
	commandList.append("updated");
	commandList.append(mkcmd("realbin", nRealBin));
	commandList.append(mkcmd("adrnbin", nAdrnBin));
	commandList.append(mkcmd("sprbin", nSprBin));
	commandList.append(mkcmd("spradrnbin", nSprAdrnBin));
	commandList.append(mkcmd("realtrue", nRealTrue));
	commandList.append(mkcmd("adrntrue", nAdrnTrue));
	commandList.append(mkcmd("encode", nEncode));
	commandList.append("windowmode");

	QProcess process;
	qint64 pid = 0;
	process.setArguments(commandList);
	process.setProgram(path);
	process.setWorkingDirectory(QFileInfo(path).absolutePath());

	if (process.startDetached(&pid) && pid)
	{

		pi.dwProcessId = pid;
		if (canSave)
		{
			//保存啟動參數
			config.write("System", "Command", "realbin", nRealBin);
			config.write("System", "Command", "adrnbin", nAdrnBin);
			config.write("System", "Command", "sprbin", nSprBin);
			config.write("System", "Command", "spradrnbin", nSprAdrnBin);
			config.write("System", "Command", "realtrue", nRealTrue);
			config.write("System", "Command", "adrntrue", nAdrnTrue);
			config.write("System", "Command", "encode", nEncode);
		}

		processHandle_.reset(pi.dwProcessId);
		return true;
	}
	return false;
}

int Injector::sendMessage(int msg, int wParam, int lParam) const
{
	if (msg == WM_NULL)
		return 0;

	DWORD dwResult = 0L;
	SendMessageTimeoutW(pi_.hWnd, msg, wParam, lParam, SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, MessageTimeout, &dwResult);
	return static_cast<int>(dwResult);
}

bool Injector::postMessage(int msg, int wParam, int lParam) const
{
	if (msg < 0) return false;
	BOOL ret = PostMessageW(pi_.hWnd, msg, wParam, lParam);
	return  ret == TRUE;
}

bool Injector::isHandleValid(qint64 pid)
{
	if (NULL == pid)
		pid = pi_.dwProcessId;

	if (NULL == pid)
		return false;

	if (!processHandle_.isValid())
	{
		processHandle_.reset(pid);
		return processHandle_.isValid();
	}
	return true;
}

DWORD WINAPI Injector::getFunAddr(const DWORD* DllBase, const char* FunName)
{
	// 遍歷導出表
	PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)DllBase;
	PIMAGE_NT_HEADERS pNt = (PIMAGE_NT_HEADERS)(pDos->e_lfanew + (DWORD)pDos);
	PIMAGE_OPTIONAL_HEADER pOt = (PIMAGE_OPTIONAL_HEADER)&pNt->OptionalHeader;
	PIMAGE_EXPORT_DIRECTORY pExport = (PIMAGE_EXPORT_DIRECTORY)(pOt->DataDirectory[0].VirtualAddress + (DWORD)DllBase);
	// 獲取到ENT、EOT、EAT
	DWORD* pENT = (DWORD*)(pExport->AddressOfNames + (DWORD)DllBase);
	WORD* pEOT = (WORD*)(pExport->AddressOfNameOrdinals + (DWORD)DllBase);
	DWORD* pEAT = (DWORD*)(pExport->AddressOfFunctions + (DWORD)DllBase);

	for (DWORD i = 0; i < pExport->NumberOfNames; ++i)
	{
		char* Name = (char*)(pENT[i] + (DWORD)DllBase);
		if (!strncmp(Name, FunName, MAX_PATH))
			return pEAT[pEOT[i]] + (DWORD)DllBase;
	}
	return (DWORD)NULL;
}

bool Injector::injectLibrary(Injector::process_information_t& pi, unsigned short port, util::LPREMOVE_THREAD_REASON pReason)
{
	if (!pi.dwProcessId)
		return false;

	if (nullptr == pReason)
		return false;

	constexpr qint64 MAX_TIMEOUT = 10000;
	bool bret = 0;
	QElapsedTimer timer;
	DWORD* kernel32Module = nullptr;
	FARPROC loadLibraryProc = nullptr;
	DWORD hModule = NULL;
	QString dllPath = "\0";
	QFileInfo fi;
	QString fileNameOnly;
	DWORD parent = NULL;

	do
	{
		QString applicationDirPath = util::applicationDirPath();
		dllPath = applicationDirPath + "/" + InjectDllName;

		fi.setFile(dllPath);
		fileNameOnly = fi.fileName();

		//檢查dll生成日期必須與當前exe相同日或更早
		QFileInfo exeInfo(applicationDirPath);

		if (fi.exists() && fi.lastModified() > exeInfo.lastModified())
		{
			break;
		}

		qDebug() << "file OK";

		pi.hWnd = NULL;

		timer.start();

		if (nullptr == pi.hWnd)
		{
			for (;;)//超過N秒自動退出
			{
				::EnumWindows(EnumWindowsCallback, reinterpret_cast<LPARAM>(&pi));
				if (pi.hWnd)
				{
					DWORD dwProcessId = 0;
					::GetWindowThreadProcessId(pi.hWnd, &dwProcessId);
					if (dwProcessId == pi.dwProcessId)
						break;
				}
				if (timer.hasExpired(MAX_TIMEOUT))
					break;
				QThread::msleep(100);
			}
		}

		if (nullptr == pi.hWnd)
		{
			*pReason = util::REASON_ENUM_WINDOWS_FAIL;
			break;
		}

		qDebug() << "HWND OK";

		bool skip = false;

		if (skip)
			break;

		//去除改變窗口大小的屬性
		LONG dwStyle = ::GetWindowLongW(pi.hWnd, GWL_STYLE);
		LONG tempStyle = dwStyle;

		if (dwStyle & WS_SIZEBOX)
			dwStyle &= ~WS_SIZEBOX;

		if (dwStyle & WS_MAXIMIZEBOX)
			dwStyle &= ~WS_MAXIMIZEBOX;

		if (tempStyle != dwStyle)
			::SetWindowLongW(pi.hWnd, GWL_STYLE, dwStyle);

		pi.dwThreadId = ::GetWindowThreadProcessId(pi.hWnd, nullptr);


		kernel32Module = getKernel32();//::GetModuleHandleW(L"kernel32.dll");
		if (nullptr == kernel32Module)
		{
			*pReason = util::REASON_GET_KERNEL32_FAIL;
			break;
		}

		loadLibraryProc = reinterpret_cast<FARPROC>(getFunAddr(kernel32Module, u8"LoadLibraryW"));
		if (nullptr == loadLibraryProc)
		{
			*pReason = util::REASON_GET_KERNEL32_UNDOCUMENT_API_FAIL;
			break;
		}

		if (!isHandleValid(pi.dwProcessId))
		{
			*pReason = util::REASON_OPEN_PROCESS_FAIL;
			break;
		}

		if (!processHandle_.isValid())
			processHandle_.reset(pi.dwProcessId);

		const util::VirtualMemory lpParameter(processHandle_, dllPath, util::VirtualMemory::kUnicode, true);
		if (NULL == lpParameter)
		{
			*pReason = util::REASON_VIRTUAL_ALLOC_FAIL;
			break;
		}

		QScopedHandle hThreadHandle(QScopedHandle::CREATE_REMOTE_THREAD, processHandle_,
			reinterpret_cast<PVOID>(loadLibraryProc),
			reinterpret_cast<LPVOID>(static_cast<DWORD>(lpParameter)));
		if (hThreadHandle.isValid())
		{
			timer.restart();
			for (;;)
			{
				hModule = mem::getRemoteModuleHandle(pi.dwProcessId, fileNameOnly);
				if (hModule)
					break;

				if (timer.hasExpired(MAX_TIMEOUT))
					break;

				if (!IsWindow(pi.hWnd))
					break;

				if (SendMessageTimeoutW(pi.hWnd, WM_NULL, 0, 0, SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, MessageTimeout, nullptr) == 0)
					break;

				QThread::msleep(100);
			}
		}

		if (NULL == hModule)
		{
			*pReason = util::REASON_INJECT_LIBRARY_FAIL;
			break;
		}

		qDebug() << "inject OK";

		pi_ = pi;
		parent = qgetenv("SASH_HWND").toULong();
		sendMessage(kInitialize, port, parent);

		timer.restart();
		for (;;)
		{
			if ((hModule_ = sendMessage(kGetModule, NULL, NULL)) != NULL)
				break;

			if (timer.hasExpired(MAX_TIMEOUT))
				break;

			if (!IsWindow(pi.hWnd))
				break;

			if (sendMessage(WM_NULL, 0, 0) == 0)
				break;

			QThread::msleep(100);
		}

		if (NULL == hModule_)
		{
			*pReason = util::REASON_INJECT_LIBRARY_FAIL;
			break;
		}

		qDebug() << "module OK";

		hookdllModule_ = reinterpret_cast<HMODULE>(hModule);
		bret = true;
	} while (false);

	if (!bret)
		pi_ = { 0 };
	return bret;
}

void Injector::remoteFreeModule()
{
	if (!processHandle_.isValid())
		processHandle_.reset(pi_.dwProcessId);

	sendMessage(kUninitialize, NULL, NULL);
	DWORD* kernel32Module = getKernel32();//::GetModuleHandleW(L"kernel32.dll");
	if (kernel32Module == nullptr) return;
	FARPROC freeLibraryProc = reinterpret_cast<FARPROC>(getFunAddr(kernel32Module, u8"FreeLibrary"));

	const util::VirtualMemory lpParameter(processHandle_, sizeof(int), true);
	if (NULL == lpParameter)
	{
		return;
	}

	mem::write<int>(processHandle_, lpParameter, reinterpret_cast<int>(hookdllModule_));

	QScopedHandle hThreadHandle(QScopedHandle::CREATE_REMOTE_THREAD, processHandle_,
		reinterpret_cast<LPVOID>(freeLibraryProc),
		reinterpret_cast<LPVOID>(static_cast<DWORD>(lpParameter)));
	WaitForSingleObject(hThreadHandle, 1000);
}

bool Injector::isWindowAlive() const
{
	if (!isValid())
		return false;

	//#ifndef _DEBUG
	//if (SendMessageTimeoutW(pi_.hWnd, WM_NULL, 0, 0, SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, MessageTimeout, nullptr) == 0)
	//	return false;
	//#endif

	if (IsWindow(pi_.hWnd))
		return true;

	DWORD dwProcessId = NULL;
	QScopedHandle hSnapshop(QScopedHandle::CREATE_TOOLHELP32_SNAPSHOT, TH32CS_SNAPPROCESS, dwProcessId);
	if (!hSnapshop.isValid())
	{
		return false;
	}

	PROCESSENTRY32W program_info = {};
	program_info.dwSize = sizeof(PROCESSENTRY32W);
	BOOL bResult = Process32FirstW(hSnapshop, &program_info);
	if (FALSE == bResult)
	{
		return false;
	}
	else
	{
		if (program_info.th32ProcessID == pi_.dwProcessId)
		{
			return true;
		}
	}

	while (TRUE == bResult)
	{
		if (program_info.th32ProcessID == pi_.dwProcessId)
			return true;

		bResult = Process32NextW(hSnapshop, &program_info);
	}
	return false;
}

void Injector::mouseMove(int x, int y) const
{
	LPARAM data = MAKELPARAM(x, y);
	sendMessage(WM_MOUSEMOVE, NULL, data);
}

//滑鼠移動 + 左鍵
void Injector::leftClick(int x, int y) const
{
	LPARAM data = MAKELPARAM(x, y);
	sendMessage(WM_MOUSEMOVE, NULL, data);
	QThread::msleep(50);
	sendMessage(WM_LBUTTONDOWN, MK_LBUTTON, data);
	QThread::msleep(50);
	sendMessage(WM_LBUTTONUP, MK_LBUTTON, data);
	QThread::msleep(50);
}

void Injector::leftDoubleClick(int x, int y) const
{
	LPARAM data = MAKELPARAM(x, y);
	sendMessage(WM_MOUSEMOVE, NULL, data);
	QThread::msleep(50);
	sendMessage(WM_LBUTTONDBLCLK, MK_LBUTTON, data);
	QThread::msleep(50);
}

void Injector::rightClick(int x, int y) const
{
	LPARAM data = MAKELPARAM(x, y);
	sendMessage(WM_MOUSEMOVE, NULL, data);
	QThread::msleep(50);
	sendMessage(WM_RBUTTONDOWN, MK_RBUTTON, data);
	QThread::msleep(50);
	sendMessage(WM_RBUTTONUP, MK_RBUTTON, data);
	QThread::msleep(50);
}

void Injector::dragto(int x1, int y1, int x2, int y2) const
{
	LPARAM datafrom = MAKELPARAM(x1, y1);
	LPARAM datato = MAKELPARAM(x2, y2);
	sendMessage(WM_MOUSEMOVE, NULL, datafrom);
	QThread::msleep(50);
	sendMessage(WM_LBUTTONDOWN, MK_LBUTTON, datafrom);
	QThread::msleep(50);
	sendMessage(WM_MOUSEMOVE, NULL, datato);
	QThread::msleep(50);
	sendMessage(WM_LBUTTONUP, MK_LBUTTON, datato);
	QThread::msleep(50);
}

void Injector::hide(int mode)
{
	HWND hWnd = getProcessWindow();
	if (hWnd == nullptr)
		return;

	bool isWin7 = false;
	//get windows version
	QOperatingSystemVersion version = QOperatingSystemVersion::current();
	if (version <= QOperatingSystemVersion::Windows7)
		isWin7 = true;

	LONG_PTR exstyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);

	//retore before hide
	ShowWindow(hWnd, SW_RESTORE);

	//add tool window style to hide from taskbar

	if (!isWin7)
	{
		if (!(exstyle & WS_EX_TOOLWINDOW))
			exstyle |= WS_EX_TOOLWINDOW;
	}
	else
	{
		//add tool window style to hide from taskbar
		if (!(exstyle & WS_EX_APPWINDOW))
			exstyle |= WS_EX_APPWINDOW;
	}

	//添加透明化屬性
	if (!(exstyle & WS_EX_LAYERED))
		exstyle |= WS_EX_LAYERED;
	SetWindowLongPtr(hWnd, GWL_EXSTYLE, exstyle);

	//設置透明度
	SetLayeredWindowAttributes(hWnd, 0, 0, LWA_ALPHA);

	if (mode == 1)
	{
		//minimize
		ShowWindow(hWnd, SW_MINIMIZE);
		//hide
		ShowWindow(hWnd, SW_HIDE);
	}

	sendMessage(kEnableWindowHide, true, NULL);

	mem::freeUnuseMemory(getProcess());
}

void Injector::show()
{
	HWND hWnd = getProcessWindow();
	if (hWnd == nullptr)
		return;

	sendMessage(kEnableWindowHide, false, NULL);

	bool isWin7 = false;
	//get windows version
	QOperatingSystemVersion version = QOperatingSystemVersion::current();
	if (version <= QOperatingSystemVersion::Windows7)
		isWin7 = true;

	LONG_PTR exstyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);

	if (!isWin7)
	{
		//remove tool window style to show from taskbar
		if (exstyle & WS_EX_TOOLWINDOW)
			exstyle &= ~WS_EX_TOOLWINDOW;
	}
	else
	{
		//remove tool window style to show from taskbar
		if (exstyle & WS_EX_APPWINDOW)
			exstyle &= ~WS_EX_APPWINDOW;
	}

	//移除透明化屬性
	if (exstyle & WS_EX_LAYERED)
		exstyle &= ~WS_EX_LAYERED;
	SetWindowLongPtr(hWnd, GWL_EXSTYLE, exstyle);

	//active once
	ShowWindow(hWnd, SW_RESTORE);
	ShowWindow(hWnd, SW_SHOW);

	//bring to top once
	SetForegroundWindow(hWnd);
}

QString Injector::getPointFileName()
{

	const QString dirPath(QString("%1/map/%2").arg(util::applicationDirPath()).arg(currentServerListIndex));
	QDir dir(dirPath);
	if (!dir.exists())
		dir.mkdir(dirPath);

	UINT acp = ::GetACP();
	if (acp == 950)
	{
		return (dirPath + QString("/point_zh_TW.json"));
	}
	else if (acp == 936)
	{
		return (dirPath + QString("/point_zh_CN.json"));
	}
	else
	{
		return (dirPath + QString("/point.json"));
	}
}