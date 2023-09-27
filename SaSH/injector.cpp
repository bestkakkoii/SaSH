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

util::SafeHash<qint64, Injector*> Injector::instances;

constexpr const char* InjectDllName = u8"sadll.dll";
constexpr qint64 MessageTimeout = 3000;
constexpr qint64 MAX_TIMEOUT = 30000;

Injector::Injector(qint64 index)
	: index_(index)
	, autil(index)
{
	scriptLogModel.reset(new StringListModel);
	chatLogModel.reset(new StringListModel);
}

Injector::~Injector()
{
	qDebug() << "Injector is destroyed!!";
}

void Injector::reset()
{
	QList<qint64> keys = instances.keys();
	for (qint64 key : keys)
	{
		reset(key);
	}
}

void Injector::reset(qint64 index)//static
{
	if (!instances.contains(index))
		return;

	Injector* instance = instances.take(index);

	instance->server.reset(nullptr);
	instance->hGameModule_ = 0ULL;
	instance->hookdllModule_ = nullptr;
	instance->pi_ = {};
	instance->processHandle_.reset();

	//紀錄當前設置
	QHash<util::UserSetting, bool> enableHash = instance->getEnablesHash();
	QHash<util::UserSetting, qint64> valueHash = instance->getValuesHash();
	QHash<util::UserSetting, QString> stringHash = instance->getStringsHash();

	QStringList _serverNameList = instance->serverNameList.get();

	QStringList _subServerNameList = instance->subServerNameList.get();

	qint64 _currentServerListIndex = instance->currentServerListIndex;

	QString _currentScriptFileName = instance->currentScriptFileName;

	HWND _hWnd = instance->getParentWidget();

	delete instance;
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

}

Injector::CreateProcessResult Injector::createProcess(Injector::process_information_t& pi)
{
	const QString fileName(qgetenv("JSON_PATH"));
	if (fileName.isEmpty())
		return CreateProcessResult::CreateFail;

	util::Config config(fileName);

	QString path = currentGameExePath;
	if (path.isEmpty() || !QFile::exists(path))
	{
		return CreateProcessResult::CreateFail;
	}

	qint64 nRealBin = 138;
	qint64 nAdrnBin = 138;
	qint64 nSprBin = 116;
	qint64 nSprAdrnBin = 116;
	qint64 nRealTrue = 13;
	qint64 nAdrnTrue = 5;
	qint64 nEncode = 0;

	bool canSave = false;
	qint64 tmp = config.read<qint64>("System", "Command", "realbin");
	if (tmp)
		nRealBin = tmp;
	else
		canSave = true;

	tmp = config.read<qint64>("System", "Command", "adrnbin");
	if (tmp)
		nAdrnBin = tmp;
	else
		canSave = true;

	tmp = config.read<qint64>("System", "Command", "sprbin");
	if (tmp)
		nSprBin = tmp;
	else
		canSave = true;

	tmp = config.read<qint64>("System", "Command", "spradrnbin");
	if (tmp)
		nSprAdrnBin = tmp;
	tmp = config.read<qint64>("System", "Command", "realtrue");
	if (tmp)
		nRealTrue = tmp;
	else
		canSave = true;

	tmp = config.read<qint64>("System", "Command", "adrntrue");
	if (tmp)
		nAdrnTrue = tmp;
	else
		canSave = true;

	tmp = config.read<qint64>("System", "Command", "encode");
	if (tmp)
		nEncode = tmp;
	else
		canSave = true;

	auto mkcmd = [](const QString& sec, qint64 value)->QString
	{
		return QString("%1:%2").arg(sec).arg(value);
	};

	QStringList commandList;
	//啟動參數
	//updated realbin:138 adrnbin:138 sprbin:116 spradrnbin:116 adrntrue:5 realtrue:13 encode:0 windowmode
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

#if 1
	QProcess process;
	qint64 pid = 0;
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
#endif

#if 0
	do
	{
		qint64 currentIndex = getIndex();
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);

		QString command = commandList.join(" ");
		QString currentDir = QFileInfo(path).absolutePath();
		QString applicationDirPath = util::applicationDirPath();
		QString dllPath = applicationDirPath + "/" + InjectDllName;

		QFileInfo fi(dllPath);
		if (!fi.exists())
		{
			emit signalDispatcher.messageBoxShow(tr("Dll is not exist at %1").arg(dllPath));
			break;
		}

		STARTUPINFOW si = {};
		PROCESS_INFORMATION pi2 = {};
		si.cb = sizeof(si);

		QElapsedTimer timer; timer.start();

		if (DetourCreateProcessWithDllExW(
			path.toStdWString().c_str(),
			const_cast<LPWSTR>(command.toStdWString().c_str()),
			NULL,
			NULL,
			FALSE,
			NULL,
			NULL,
			currentDir.toStdWString().c_str(),
			&si,
			&pi2,
			dllPath.toStdString().c_str(),
			::CreateProcessW) == FALSE)
		{
			break;
		}

		pi.dwProcessId = pi2.dwProcessId;
		if (!processHandle_.isValid())
			processHandle_.reset(pi2.hProcess);

		if (canSave)
			save();

		if (nullptr == pi.hWnd)
		{
			//查找窗口句炳
			for (;;)
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
				{
					emit signalDispatcher.messageBoxShow(tr("EnumWindows timeout"), QMessageBox::Icon::Critical);
					break;
				}

				QThread::msleep(100);
			}
		}

		if (nullptr == pi.hWnd)
		{
			emit signalDispatcher.messageBoxShow(QObject::tr("EnumWindows failed"), QMessageBox::Icon::Critical);
			break;
		}

		qDebug() << "HWND OK";

		//紀錄線程ID(目前沒有使用到只是先記錄著)
		pi.dwThreadId = ::GetWindowThreadProcessId(pi.hWnd, nullptr);

		hookdllModule_ = mem::getRemoteModuleHandleByProcessHandleW(processHandle_, InjectDllName);
		if (nullptr == hookdllModule_)
		{
			emit signalDispatcher.messageBoxShow(QObject::tr("Game module is null"), QMessageBox::Icon::Critical);
			break;
		}

		hGameModule_ = reinterpret_cast<quint64>(mem::getRemoteModuleHandleByProcessHandleW(processHandle_, "sa_8001.exe"));
		if (NULL == hGameModule_)
		{
			hGameModule_ = 0x400000ULL;
		}

		pi_ = pi;

		//通知客戶端初始化，並提供port端口讓客戶端連進來、另外提供本窗口句柄讓子進程反向檢查外掛是否退出
		struct InitialData
		{
			__int64 parentHWnd = 0i64;
			__int64 index = 0i64;
			__int64 port = 0i64;
			__int64 type = 0i64;
		}injectdate;

		enum
		{
			kIPv4,
			kIPv6,
		};

		QOperatingSystemVersion version = QOperatingSystemVersion::current();
		injectdate.index = currentIndex;
		injectdate.parentHWnd = reinterpret_cast<__int64>(getParentWidget());
		injectdate.port = server->getPort();
		if (version > QOperatingSystemVersion::Windows7)
			injectdate.type = kIPv6;
		else
			injectdate.type = kIPv4;

		const util::VirtualMemory lpStruct(processHandle_, sizeof(InitialData), true);
		if (!lpStruct.isValid())
		{
			emit signalDispatcher.messageBoxShow(QObject::tr("Remote virtualmemory alloc failed"), QMessageBox::Icon::Critical);
			break;
		}

		mem::write(processHandle_, lpStruct, &injectdate, sizeof(InitialData));
		sendMessage(kInitialize, lpStruct, NULL);

		//去除改變窗口大小的屬性
		LONG dwStyle = ::GetWindowLongW(pi.hWnd, GWL_STYLE);
		LONG tempStyle = dwStyle;

		if (dwStyle & WS_SIZEBOX)
			dwStyle &= ~WS_SIZEBOX;

		if (dwStyle & WS_MAXIMIZEBOX)
			dwStyle &= ~WS_MAXIMIZEBOX;

		if (tempStyle != dwStyle)
			::SetWindowLongW(pi.hWnd, GWL_STYLE, dwStyle);

		return CreateBelowWindow8Success;
	} while (false);


	return CreateBelowWindow8Failed;
#endif
}

qint64 Injector::sendMessage(qint64 msg, qint64 wParam, qint64 lParam) const
{
	if (msg == WM_NULL)
		return 0;

	DWORD_PTR dwResult = 0L;
	SendMessageTimeoutW(pi_.hWnd, msg, wParam, lParam, SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, MessageTimeout, &dwResult);
	return static_cast<qint64>(dwResult);
}

bool Injector::postMessage(qint64 msg, qint64 wParam, qint64 lParam) const
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
	qint64 currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);

	if (!pi.dwProcessId)
	{
		emit signalDispatcher.messageBoxShow(tr("dwProcessId is null!"));
		return false;
	}

	if (nullptr == pReason)
	{
		emit signalDispatcher.messageBoxShow(tr("pReason is null!"));
		return false;
	}

	bool bret = 0;
	QElapsedTimer timer; timer.start();
	QString dllPath = "\0";
	pi.hWnd = nullptr;

	do
	{
		QString applicationDirPath = util::applicationDirPath();
		dllPath = applicationDirPath + "/" + InjectDllName;

		QFileInfo fi(dllPath);
		if (!fi.exists())
		{
			emit signalDispatcher.messageBoxShow(tr("Dll is not exist at %1").arg(dllPath));
			break;
		}

		qDebug() << "file OK";

		timer.restart();
		if (nullptr == pi.hWnd)
		{
			//查找窗口句炳
			for (;;)
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
				{
					emit signalDispatcher.messageBoxShow(tr("EnumWindows timeout"), QMessageBox::Icon::Critical);
					break;
				}

				QThread::msleep(100);
			}
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
		QOperatingSystemVersion version = QOperatingSystemVersion::current();
		if (version > QOperatingSystemVersion::Windows7)
		{
			mem::inject(currentIndex, processHandle_, dllPath, &hookdllModule_, &hGameModule_);
		}
		else
		{
			//Win7
			mem::injectByWin7(currentIndex, processHandle_, dllPath, &hookdllModule_, &hGameModule_);
		}

		qDebug() << "inject cost:" << timer.elapsed() << "ms";

		if (hookdllModule_ == 0)
		{
			emit signalDispatcher.messageBoxShow(QObject::tr("Injected dll module is null"), QMessageBox::Icon::Critical);
			*pReason = util::REASON_INJECT_LIBRARY_FAIL;
			break;
		}

		if (NULL == hGameModule_)
		{
			emit signalDispatcher.messageBoxShow(QObject::tr("Game module is null"), QMessageBox::Icon::Critical);
			*pReason = util::REASON_INJECT_LIBRARY_FAIL;
			break;
		}

		pi_ = pi;

		//通知客戶端初始化，並提供port端口讓客戶端連進來、另外提供本窗口句柄讓子進程反向檢查外掛是否退出
		struct InitialData
		{
			__int64 parentHWnd = 0i64;
			__int64 index = 0i64;
			__int64 port = 0i64;
			__int64 type = 0i64;
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

		mem::write(processHandle_, lpStruct, &injectdate, sizeof(InitialData));
		SendMessageTimeoutW(pi_.hWnd, kInitialize, lpStruct, NULL, SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT | SMTO_BLOCK, MessageTimeout, nullptr);

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

void Injector::mouseMove(qint64 x, qint64 y) const
{
	LPARAM data = MAKELPARAM(x, y);
	sendMessage(WM_MOUSEMOVE, NULL, data);
}

//滑鼠移動 + 左鍵
void Injector::leftClick(qint64 x, qint64 y) const
{
	LPARAM data = MAKELPARAM(x, y);
	sendMessage(WM_MOUSEMOVE, NULL, data);
	QThread::msleep(50);
	sendMessage(WM_LBUTTONDOWN, MK_LBUTTON, data);
	QThread::msleep(50);
	sendMessage(WM_LBUTTONUP, MK_LBUTTON, data);
	QThread::msleep(50);
}

void Injector::leftDoubleClick(qint64 x, qint64 y) const
{
	LPARAM data = MAKELPARAM(x, y);
	sendMessage(WM_MOUSEMOVE, NULL, data);
	QThread::msleep(50);
	sendMessage(WM_LBUTTONDBLCLK, MK_LBUTTON, data);
	QThread::msleep(50);
}

void Injector::rightClick(qint64 x, qint64 y) const
{
	LPARAM data = MAKELPARAM(x, y);
	sendMessage(WM_MOUSEMOVE, NULL, data);
	QThread::msleep(50);
	sendMessage(WM_RBUTTONDOWN, MK_RBUTTON, data);
	QThread::msleep(50);
	sendMessage(WM_RBUTTONUP, MK_RBUTTON, data);
	QThread::msleep(50);
}

void Injector::dragto(qint64 x1, qint64 y1, qint64 x2, qint64 y2) const
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

void Injector::hide(qint64 mode)
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