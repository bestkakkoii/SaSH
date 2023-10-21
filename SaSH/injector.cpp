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

Server Injector::server;
util::SafeHash<long long, Injector*> Injector::instances;

constexpr long long MessageTimeout = 3000;
constexpr long long MAX_TIMEOUT = 30000;

Injector::Injector(long long index)
	: Indexer(index)
	, log(index, nullptr)
	, autil(index)
{

}

Injector::~Injector()
{
	qDebug() << "Injector is destroyed!!";
}

void Injector::reset()
{
	QList<long long> keys = instances.keys();
	for (long long key : keys)
	{
		reset(key);
	}
}

void Injector::reset(long long index)//static
{
	if (!instances.contains(index))
		return;

	Injector* instance = instances.value(index);
	if (!instance->worker.isNull())
	{
		instance->worker.reset(nullptr);
	}
	instance->hGameModule_ = 0x400000ULL;
	instance->hookdllModule_ = nullptr;
	instance->pi_ = {};
	instance->processHandle_.reset();

	instance->autil.util_Clear();
	instance->currentGameExePath = "";//當前使用的遊戲進程完整路徑
	instance->IS_SCRIPT_FLAG.store(false, std::memory_order_release);//主腳本是否運行
	instance->IS_SCRIPT_INTERRUPT.store(false, std::memory_order_release);
	instance->IS_TCP_CONNECTION_OK_TO_USE.store(false, std::memory_order_release);
	instance->currentServerListIndex = 0;
	instance->scriptThreadId = 0;
	instance->IS_INJECT_OK = false;
#if 0
	//紀錄當前設置
	QHash<util::UserSetting, bool> enableHash = instance->getEnablesHash();
	QHash<util::UserSetting, long long> valueHash = instance->getValuesHash();
	QHash<util::UserSetting, QString> stringHash = instance->getStringsHash();

	QStringList _serverNameList = instance->serverNameList.get();

	QStringList _subServerNameList = instance->subServerNameList.get();

	long long _currentServerListIndex = instance->currentServerListIndex;

	QString _currentScriptFileName = instance->currentScriptFileName;

	HWND _hWnd = instance->getParentWidget();

	instance->deleteLater();
	instance = new Injector(index);
	if (instance != nullptr)
	{
		//恢復設置
		instance->setValuesHash(valueHash);
		instance->setEnablesHash(enableHash);
		instance->setStringsHash(stringHash);

		instance->serverNameList.set(_serverNameList);

		instance->subServerNameList.set(_subServerNameList);

		instance->currentServerListIndex = _currentServerListIndex;

		instance->currentScriptFileName = _currentScriptFileName;

		instance->setParentWidget(_hWnd);

		instances.insert(index, instance);
	}
#endif
}

Injector::CreateProcessResult Injector::createProcess(Injector::process_information_t& pi)
{
	QString path = currentGameExePath;
	if (path.isEmpty() || !QFile::exists(path))
	{
		return CreateProcessResult::CreateFail;
	}

	long long nRealBin = 138;
	long long nAdrnBin = 138;
	long long nSprBin = 116;
	long long nSprAdrnBin = 116;
	long long nRealTrue = 13;
	long long nAdrnTrue = 5;
	long long nEncode = 0;

	bool canSave = false;

	util::Config config(QString("%1|%2").arg(__FUNCTION__).arg(__LINE__));
	long long tmp = config.read<long long>("System", "Command", "realbin");
	if (tmp)
		nRealBin = tmp;
	else
		canSave = true;

	tmp = config.read<long long>("System", "Command", "adrnbin");
	if (tmp)
		nAdrnBin = tmp;
	else
		canSave = true;

	tmp = config.read<long long>("System", "Command", "sprbin");
	if (tmp)
		nSprBin = tmp;
	else
		canSave = true;

	tmp = config.read<long long>("System", "Command", "spradrnbin");
	if (tmp)
		nSprAdrnBin = tmp;
	tmp = config.read<long long>("System", "Command", "realtrue");
	if (tmp)
		nRealTrue = tmp;
	else
		canSave = true;

	tmp = config.read<long long>("System", "Command", "adrntrue");
	if (tmp)
		nAdrnTrue = tmp;
	else
		canSave = true;

	tmp = config.read<long long>("System", "Command", "encode");
	if (tmp)
		nEncode = tmp;
	else
		canSave = true;

	QString customCommand = config.read<QString>("System", "Command", "custom");

	auto mkcmd = [](const QString& sec, long long value)->QString
		{
			return QString("%1:%2").arg(sec).arg(value);
		};

	QStringList commandList;
	//啟動參數
	//updated realbin:138 adrnbin:138 sprbin:116 spradrnbin:116 adrntrue:5 realtrue:13 encode:0 windowmode
	if (customCommand.isEmpty())
	{
		commandList.append(path);
		commandList.append("updated");
		commandList.append(mkcmd("realbin", nRealBin));
		commandList.append(mkcmd("adrnbin", nAdrnBin));
		commandList.append(mkcmd("sprbin", nSprBin));
		commandList.append(mkcmd("spradrnbin", nSprAdrnBin));
		commandList.append(mkcmd("realtrue", nRealTrue));
		commandList.append(mkcmd("adrntrue", nAdrnTrue));
		commandList.append(mkcmd("encode", nEncode));
		commandList.append("windowmode");
	}
	else
	{
		commandList.append(customCommand);
	}

	auto save = [&config, nRealBin, nAdrnBin, nSprBin, nSprAdrnBin, nRealTrue, nAdrnTrue, nEncode]()
		{
			//保存啟動參數
			config.write("System", "Command", "realbin", nRealBin);
			config.write("System", "Command", "adrnbin", nAdrnBin);
			config.write("System", "Command", "sprbin", nSprBin);
			config.write("System", "Command", "spradrnbin", nSprAdrnBin);
			config.write("System", "Command", "realtrue", nRealTrue);
			config.write("System", "Command", "adrntrue", nAdrnTrue);
			config.write("System", "Command", "encode", nEncode);
		};

	QProcess process;
	long long pid = 0;
	process.setArguments(commandList);
	process.setProgram(path);
	process.setWorkingDirectory(QFileInfo(path).absolutePath());

	if (process.startDetached(&pid) && pid)
	{

		pi.dwProcessId = pid;
		if (canSave)
			save();

		processHandle_.reset(pi.dwProcessId);
		return CreateProcessResult::CreateAboveWindow8Success;
	}
	return CreateProcessResult::CreateAboveWindow8Failed;
}

long long Injector::sendMessage(long long msg, long long wParam, long long lParam) const
{
	if (WM_NULL == msg)
		return 0;

	DWORD_PTR dwResult = 0L;
	SendMessageTimeoutW(pi_.hWnd, static_cast<UINT>(msg), static_cast<WPARAM>(wParam), static_cast<LPARAM>(lParam),
		SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, static_cast<UINT>(MessageTimeout), &dwResult);
	return static_cast<long long>(dwResult);
}

bool Injector::postMessage(long long msg, long long wParam, long long lParam) const
{
	if (WM_NULL == msg)
		return false;

	BOOL ret = PostMessageW(pi_.hWnd, static_cast<UINT>(msg), static_cast<WPARAM>(wParam), static_cast<LPARAM>(lParam));
	return  ret == TRUE;
}

bool Injector::isHandleValid(long long pid)
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

#if 0
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
#endif

bool Injector::injectLibrary(Injector::process_information_t& pi, unsigned short port, util::LPREMOVE_THREAD_REASON pReason)
{
	long long currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);

	if (pi.dwProcessId == 0)
	{
		emit signalDispatcher.messageBoxShow(QObject::tr("dwProcessId is null!"));
		return false;
	}

	if (nullptr == pReason)
	{
		emit signalDispatcher.messageBoxShow(QObject::tr("pReason is null!"));
		return false;
	}

	bool bret = 0;
	QElapsedTimer timer; timer.start();
	QString dllPath = "\0";
	pi.hWnd = nullptr;

	do
	{
		QString applicationDirPath = util::applicationDirPath();
		QStringList files;
		util::searchFiles(applicationDirPath, SASH_INJECT_DLLNAME, ".dll", &files, false);
		if (files.isEmpty())
		{
			emit signalDispatcher.messageBoxShow(QObject::tr("Dll is not exist at %1").arg(applicationDirPath));
			break;
		}

		dllPath = files.first();
		if (dllPath.isEmpty())
		{
			emit signalDispatcher.messageBoxShow(QObject::tr("Dll is not exist at %1").arg(applicationDirPath));
			break;
		}

		QFileInfo fi(dllPath);
		if (!fi.exists())
		{
			emit signalDispatcher.messageBoxShow(QObject::tr("Dll is not exist at %1").arg(dllPath));
			break;
		}

		qDebug() << "file OK";

		timer.restart();
		//查找窗口句炳
		for (;;)
		{
			::EnumWindows(EnumWindowsCallback, reinterpret_cast<LPARAM>(&pi));
			if (pi.hWnd != nullptr)
			{
				DWORD dwProcessId = 0;
				::GetWindowThreadProcessId(pi.hWnd, &dwProcessId);
				if (dwProcessId == pi.dwProcessId)
					break;
			}

			if (timer.hasExpired(MAX_TIMEOUT))
			{
				emit signalDispatcher.messageBoxShow(QObject::tr("EnumWindows timeout"), QMessageBox::Icon::Critical);
				break;
			}

			QThread::msleep(10);
		}

		if (nullptr == pi.hWnd)
		{
			emit signalDispatcher.messageBoxShow(QObject::tr("EnumWindows failed"), QMessageBox::Icon::Critical);
			*pReason = util::REASON_ENUM_WINDOWS_FAIL;
			break;
		}

		qDebug() << "HWND OK, cost:" << timer.elapsed() << "ms";

		//紀錄線程ID(目前沒有使用到只是先記錄著)
		pi.dwThreadId = ::GetWindowThreadProcessId(pi.hWnd, nullptr);

		//嘗試打開進程句柄並檢查是否成功
		if (!isHandleValid(pi.dwProcessId))
		{
			emit signalDispatcher.messageBoxShow(QObject::tr("OpenProcess failed"), QMessageBox::Icon::Critical);
			*pReason = util::REASON_OPEN_PROCESS_FAIL;
			break;
		}

		if (!processHandle_.isValid())
			processHandle_.reset(pi.dwProcessId);

		timer.restart();
		for (;;)
		{
			if (static_cast<long long>(mem::read<int>(processHandle_, hGameModule_ + kOffsetWorldStatus)) == 1
				&& static_cast<long long>(mem::read<int>(processHandle_, hGameModule_ + kOffsetGameStatus)) == 2)
				break;

			if (timer.hasExpired(MAX_TIMEOUT))
				break;
		}

		timer.restart();
		QOperatingSystemVersion version = QOperatingSystemVersion::current();
		//Win7

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
		if (mem::injectByWin7(currentIndex, pi.dwProcessId, processHandle_, dllPath, &hookdllModule_, &hGameModule_))
			qDebug() << "inject cost:" << timer.elapsed() << "ms";
#else
		if (mem::injectBy64(currentIndex, processHandle_, dllPath, &hookdllModule_, &hGameModule_))
			qDebug() << "inject cost:" << timer.elapsed() << "ms";
#endif

		if (nullptr == hookdllModule_)
		{
			emit signalDispatcher.messageBoxShow(QObject::tr("Injected dll module is null"), QMessageBox::Icon::Critical);
			*pReason = util::REASON_INJECT_LIBRARY_FAIL;
			break;
		}

		if (0 == hGameModule_)
		{
			emit signalDispatcher.messageBoxShow(QObject::tr("Game module is null"), QMessageBox::Icon::Critical);
			*pReason = util::REASON_INJECT_LIBRARY_FAIL;
			break;
		}

		pi_ = pi;

		//通知客戶端初始化，並提供port端口讓客戶端連進來、另外提供本窗口句柄讓子進程反向檢查外掛是否退出
		struct InitialData
		{
			long long parentHWnd = 0i64;
			long long index = 0i64;
			long long port = 0i64;
			long long type = 0i64;
		}injectdate;

		enum
		{
			kIPv4,
			kIPv6,
		};

		injectdate.index = currentIndex;
		injectdate.parentHWnd = reinterpret_cast<__int64>(getParentWidget());
		injectdate.port = port;
		if (version > QOperatingSystemVersion::Windows7)
			injectdate.type = kIPv6;
		else
			injectdate.type = kIPv4;

		const util::VirtualMemory lpStruct(processHandle_, sizeof(InitialData), true);
		if (!lpStruct.isValid())
		{
			emit signalDispatcher.messageBoxShow(QObject::tr("Remote virtualmemory alloc failed"), QMessageBox::Icon::Critical);
			*pReason = util::REASON_INJECT_LIBRARY_FAIL;
			break;
		}

		if (!mem::write(processHandle_, lpStruct, &injectdate, sizeof(InitialData)))
		{
			emit signalDispatcher.messageBoxShow(QObject::tr("Remote virtualmemory write failed"), QMessageBox::Icon::Critical);
			*pReason = util::REASON_INJECT_LIBRARY_FAIL;
			break;
		}

		timer.restart();
		DWORD_PTR dwResult = 0L;
		for (;;)
		{
			if (IsWindowVisible(pi_.hWnd))
			{
				if (SendMessageTimeoutW(pi_.hWnd, kInitialize, lpStruct, NULL, SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT | SMTO_BLOCK, MessageTimeout, &dwResult) != 0)
				{
					if (dwResult == 0)
					{
						emit signalDispatcher.messageBoxShow(QObject::tr("Remote dll initialize failed"), QMessageBox::Icon::Critical);
						*pReason = util::REASON_INJECT_LIBRARY_FAIL;
						break;
					}
					break;
				}
			}

			if (timer.hasExpired(5000))
			{
				emit signalDispatcher.messageBoxShow(QObject::tr("SendMessageTimeoutW failed"), QMessageBox::Icon::Critical);
				*pReason = util::REASON_INJECT_LIBRARY_FAIL;
				break;
			}

			QThread::msleep(10);
		}

		if (dwResult == 0)
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

		bret = true;
		IS_INJECT_OK = true;
	} while (false);

	if (!bret)
		pi_ = {};

	return bret;
}

bool Injector::isWindowAlive() const
{
	if (!isValid())
		return false;

	//#ifndef _DEBUG
	//	if (SendMessageTimeoutW(pi_.hWnd, WM_NULL, 0, 0, SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, MessageTimeout, nullptr) <= 0)
	//		return false;
	//#endif

	if (IsWindow(pi_.hWnd))
		return true;

	DWORD dwProcessId = NULL;
	ScopedHandle hSnapshop(ScopedHandle::CREATE_TOOLHELP32_SNAPSHOT, TH32CS_SNAPPROCESS, dwProcessId);
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

void Injector::mouseMove(long long x, long long y) const
{
	//LPARAM data = MAKELPARAM(x, y);
	//sendMessage(WM_MOUSEMOVE, NULL, data);
	mem::write<int>(processHandle_, hGameModule_ + kOffestMouseX, x);
	mem::write<int>(processHandle_, hGameModule_ + kOffestMouseY, y);
}

//滑鼠移動 + 左鍵
void Injector::leftClick(long long x, long long y) const
{
	//LPARAM data = MAKELPARAM(x, y);
	//sendMessage(WM_MOUSEMOVE, NULL, data);
	//QThread::msleep(50);
	//sendMessage(WM_LBUTTONDOWN, MK_LBUTTON, data);
	//QThread::msleep(50);
	//sendMessage(WM_LBUTTONUP, MK_LBUTTON, data);
	//QThread::msleep(50);
	mouseMove(x, y);
	mem::write<int>(processHandle_, hGameModule_ + kOffestMouseClick, 1);
}

void Injector::leftDoubleClick(long long x, long long y) const
{
	//LPARAM data = MAKELPARAM(x, y);
	//sendMessage(WM_MOUSEMOVE, NULL, data);
	//QThread::msleep(50);
	//sendMessage(WM_LBUTTONDBLCLK, MK_LBUTTON, data);
	//QThread::msleep(50);
	mouseMove(x, y);
	LPARAM data = MAKELPARAM(x, y);
	sendMessage(WM_LBUTTONDBLCLK, MK_LBUTTON, data);
}

void Injector::rightClick(long long x, long long y) const
{
	//LPARAM data = MAKELPARAM(x, y);
	//sendMessage(WM_MOUSEMOVE, NULL, data);
	//QThread::msleep(50);
	//sendMessage(WM_RBUTTONDOWN, MK_RBUTTON, data);
	//QThread::msleep(50);
	//sendMessage(WM_RBUTTONUP, MK_RBUTTON, data);
	//QThread::msleep(50);
	mouseMove(x, y);
	mem::write<int>(processHandle_, hGameModule_ + kOffestMouseClick, 2);
}

void Injector::dragto(long long x1, long long y1, long long x2, long long y2) const
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

void Injector::hide(long long mode)
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
	const QString dirPath(QString("%1/lib/map/%2").arg(util::applicationDirPath()).arg(currentServerListIndex));
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
		return (dirPath + QString("/point_en_US.json"));
	}
}