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

constexpr const char* InjectDllName = "sadll.dll";
constexpr int MessageTimeout = 15000;

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
		instance->hGameModule_ = NULL;
		instance->hookdllModule_ = NULL;
		instance->pi_ = {  };
		instance->processHandle_.reset();
		//紀錄當前設置
		QHash<util::UserSetting, int> valueHash = instance->getValueHash();
		QHash<util::UserSetting, bool> enableHash = instance->getEnableHash();
		QHash<util::UserSetting, QString> stringHash = instance->getStringHash();

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



#define USE_NATIVE_CREATEPROCESS
#ifdef USE_NATIVE_CREATEPROCESS
	STARTUPINFOW si = { sizeof(si) };
	PROCESS_INFORMATION _pi = { 0 };
	QString directory = QFileInfo(path).absolutePath();
	if (!CreateProcessW(path.toStdWString().c_str(),
		commandList.join(" ").toStdWString().data(),
		nullptr,
		nullptr,
		TRUE,
		CREATE_UNICODE_ENVIRONMENT | INHERIT_PARENT_AFFINITY | CREATE_PRESERVE_CODE_AUTHZ_LEVEL,
		nullptr,
		directory.toStdWString().data(),
		&si,
		&_pi))
		return false;

	pi.dwThreadId = _pi.dwThreadId;
	pi.dwProcessId = _pi.dwProcessId;
	pi.hThread = _pi.hThread;
	processHandle_.reset(_pi.hProcess);

	QString fullpath = path.toLower();
	fullpath.replace("/", "\\");
	std::wstring wsfullpath = fullpath.toStdWString();
	util::writeFireWallOverXP(wsfullpath.c_str(), wsfullpath.c_str(), true);

#else
	QProcess process;
	qint64 pid = 0;
	process.setArguments(commandList);
	process.setProgram(path);
	process.setWorkingDirectory(QFileInfo(path).absolutePath());
	if (!process.startDetached(&pid) || pid == 0)
		return false;
	pi.dwProcessId = pid;
	processHandle_.reset(pi.dwProcessId);
#endif



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

	return true;
}

quint64 Injector::sendMessage(quint64 msg, quint64 wParam, qint64 lParam) const
{
	if (msg == WM_NULL)
		return 0;
	DWORD dwResult = 0L;
	SendMessageTimeoutW(pi_.hWnd, msg, wParam, lParam, SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, MessageTimeout, &dwResult);
	return static_cast<int>(dwResult);
}

bool Injector::postMessage(quint64 msg, quint64 wParam, qint64 lParam) const
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

bool Injector::injectLibrary(Injector::process_information_t& pi, unsigned short port, util::LPREMOVE_THREAD_REASON pReason)
{
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();

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

	constexpr qint64 MAX_TIMEOUT = 30000;
	bool bret = 0;
	QElapsedTimer timer;
	QString dllPath = "\0";
	qint64 parent = NULL;
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

		pi.hWnd = nullptr;

		timer.start();

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
					emit signalDispatcher.messageBoxShow(tr("EnumWindows timeout"));
					break;
				}

				QThread::msleep(100);
			}
		}

		if (nullptr == pi.hWnd)
		{
			*pReason = util::REASON_ENUM_WINDOWS_FAIL;
			continue;
		}

		qDebug() << "HWND OK";

		//紀錄線程ID(目前沒有使用到只是先記錄著)
		pi.dwThreadId = ::GetWindowThreadProcessId(pi.hWnd, nullptr);


		//嘗試打開進程句柄並檢查是否成功
		if (!isHandleValid(pi.dwProcessId))
		{
			*pReason = util::REASON_OPEN_PROCESS_FAIL;
			emit signalDispatcher.messageBoxShow(tr("OpenProcess fail"));
			continue;
		}

		if (!processHandle_.isValid())
			processHandle_.reset(pi.dwProcessId);

		mem::inject64(processHandle_, dllPath, &hookdllModule_, &hGameModule_);

		if (hookdllModule_ == 0)
		{
			*pReason = util::REASON_INJECT_LIBRARY_FAIL;
			continue;
		}

		if (NULL == hGameModule_)
		{
			*pReason = util::REASON_INJECT_LIBRARY_FAIL;
			continue;
		}

		pi_ = pi;
		parent = qgetenv("SASH_HWND").toULongLong();

		//通知客戶端初始化，並提供port端口讓客戶端連進來、另外提供本窗口句柄讓子進程反向檢查外掛是否退出
		sendMessage(kInitialize, port, parent);

		//去除改變窗口大小的屬性
		::SetWindowLongW(pi.hWnd, GWL_STYLE, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE);
		::SetWindowLongW(pi.hWnd, GWL_EXSTYLE, WS_EX_OVERLAPPEDWINDOW);

		bret = true;
	} while (false);

	if (!bret)
		pi_ = {};

	return bret;
}

#if 0
bool Injector::injectLibraryOld(Injector::process_information_t& pi, unsigned short port, util::LPREMOVE_THREAD_REASON pReason)
{
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();

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

	constexpr qint64 MAX_TIMEOUT = 30000;
	bool bret = 0;
	QElapsedTimer timer;
	DWORD* kernel32Module = nullptr;
	FARPROC loadLibraryProc = nullptr;
	quint64 nRemoteModule = 0;
	QString dllPath = "\0";
	qint64 parent = NULL;
	qint64 tryCount = 10;
	for (;;)
	{
		if (--tryCount <= 0)
		{
			break;
		}

		QString applicationDirPath = util::applicationDirPath();
		dllPath = applicationDirPath + "/" + InjectDllName;

		QFileInfo fi(dllPath);
		if (!fi.exists())
		{
			emit signalDispatcher.messageBoxShow(tr("Dll is not exist at %1").arg(dllPath));
			break;
		}

		qDebug() << "file OK";

		pi.hWnd = nullptr;

		timer.start();

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
					emit signalDispatcher.messageBoxShow(tr("EnumWindows timeout"));
					break;
				}

				QThread::msleep(100);
			}
		}

		if (nullptr == pi.hWnd)
		{
			*pReason = util::REASON_ENUM_WINDOWS_FAIL;
			continue;
		}

		qDebug() << "HWND OK";

		//紀錄線程ID(目前沒有使用到只是先記錄著)
		pi.dwThreadId = ::GetWindowThreadProcessId(pi.hWnd, nullptr);

		//取kernel32.dll入口指針
		kernel32Module = mem::getKernel32();
		if (nullptr == kernel32Module)
		{
			*pReason = util::REASON_GET_KERNEL32_FAIL;
			emit signalDispatcher.messageBoxShow(tr("Get kernel32 module handle fail"));
			continue;
		}

		//取LoadLibraryW函數指針
		loadLibraryProc = reinterpret_cast<FARPROC>(mem::getFunAddr(kernel32Module, "LoadLibraryW"));
		if (nullptr == loadLibraryProc)
		{
			*pReason = util::REASON_GET_KERNEL32_UNDOCUMENT_API_FAIL;
			emit signalDispatcher.messageBoxShow(tr("Get LoadLibraryW address fail"));
			continue;
		}

		//嘗試打開進程句柄並檢查是否成功
		if (!isHandleValid(pi.dwProcessId))
		{
			*pReason = util::REASON_OPEN_PROCESS_FAIL;
			emit signalDispatcher.messageBoxShow(tr("OpenProcess fail"));
			continue;
		}

		if (!processHandle_.isValid())
			processHandle_.reset(pi.dwProcessId);

		//在遠程進程中分配內存
		const util::VirtualMemory lpParameter(processHandle_, dllPath, util::VirtualMemory::kUnicode, true);
		if (!lpParameter.isValid())
		{
			*pReason = util::REASON_VIRTUAL_ALLOC_FAIL;
			emit signalDispatcher.messageBoxShow(tr("VirtualAllocEx fail"));
			continue;
		}

		//創建遠程線程調用LoadLibrary 
		{
			ScopedHandle hThreadHandle(
				ScopedHandle::CREATE_REMOTE_THREAD,
				processHandle_,
				reinterpret_cast<PVOID>(loadLibraryProc),
				reinterpret_cast<LPVOID>(static_cast<quint64>(lpParameter)));
			if (!hThreadHandle.isValid())
			{
				*pReason = util::REASON_INJECT_LIBRARY_FAIL;
				emit signalDispatcher.messageBoxShow(tr("CreateRemoteThread fail"));
				continue;
			}
		}

		//dll注入後嘗試取得dll的基址
		timer.restart();
		for (;;)
		{
			nRemoteModule = mem::getRemoteModuleHandle(pi.dwProcessId, InjectDllName);
			if (nRemoteModule > 0)
				break;

			if (timer.hasExpired(15000))
			{
				emit signalDispatcher.messageBoxShow(tr("timeout and unable to get remote module handle"));
				break;
			}

			if (timer.hasExpired(5000) && !IsWindow(pi.hWnd))
			{
				emit signalDispatcher.messageBoxShow(tr("timeout and window disappear while get remote module handle"));
				break;
			}

			QThread::msleep(100);
		}

		if (nRemoteModule == 0)
		{
			*pReason = util::REASON_INJECT_LIBRARY_FAIL;
			continue;
		}

		qDebug() << "inject OK";

		pi_ = pi;
		parent = qgetenv("SASH_HWND").toULongLong();

		//通知客戶端初始化，並提供port端口讓客戶端連進來、另外提供本窗口句柄讓子進程反向檢查外掛是否退出
		sendMessage(kInitialize, port, parent);

		//這裡如果能收到回傳消息代表Hook已經完成
		timer.restart();
		for (;;)
		{
			hGameModule_ = sendMessage(kGetModule, NULL, NULL);
			if (hGameModule_ != NULL)
				break;

			if (timer.hasExpired(5000))
			{
				emit signalDispatcher.messageBoxShow(tr("timeout and unable to get remote execute module handle"));
				break;
			}

			if (timer.hasExpired(5000) && !IsWindow(pi.hWnd))
			{
				emit signalDispatcher.messageBoxShow(tr("timeout and window disappear while get remote execute module handle"));
				break;
			}

			QThread::msleep(100);
		}

		if (NULL == hGameModule_)
		{
			*pReason = util::REASON_INJECT_LIBRARY_FAIL;
			continue;
		}

		qDebug() << "module OK";

		hookdllModule_ = reinterpret_cast<HMODULE>(nRemoteModule);

		//去除改變窗口大小的屬性
		::SetWindowLongW(pi.hWnd, GWL_STYLE, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE);
		::SetWindowLongW(pi.hWnd, GWL_EXSTYLE, WS_EX_OVERLAPPEDWINDOW);

		bret = true;
		break;
	}

	if (!bret)
		pi_ = {};

	return bret;
}
#endif

void Injector::remoteFreeModule()
{
	if (!processHandle_.isValid())
		processHandle_.reset(pi_.dwProcessId);

	sendMessage(kUninitialize, NULL, NULL);
	DWORD* kernel32Module = mem::getKernel32();//::GetModuleHandleW(L"kernel32.dll");
	if (kernel32Module == nullptr) return;
	FARPROC freeLibraryProc = reinterpret_cast<FARPROC>(mem::getFunAddr(kernel32Module, "FreeLibrary"));

	const util::VirtualMemory lpParameter(processHandle_, sizeof(int), true);
	if (NULL == lpParameter)
	{
		return;
	}

	mem::write<int>(processHandle_, lpParameter, reinterpret_cast<int>(hookdllModule_));

	ScopedHandle hThreadHandle(ScopedHandle::CREATE_REMOTE_THREAD, processHandle_,
		reinterpret_cast<LPVOID>(freeLibraryProc),
		reinterpret_cast<LPVOID>(static_cast<DWORD>(lpParameter)));
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

void Injector::mouseMove(int x, int y) const
{
	LPARAM data = MAKELPARAM(x, y);
	SetWindowPos(pi_.hWnd, HWND_TOP, 0, 0, 0, 0, SWP_ASYNCWINDOWPOS | SWP_NOMOVE | SWP_NOSIZE);
	sendMessage(WM_ACTIVATE, MAKEWPARAM(WA_ACTIVE, 0), NULL);
	sendMessage(WM_MOUSEMOVE, NULL, data);
}

//滑鼠移動 + 左鍵
void Injector::leftClick(int x, int y) const
{
	LPARAM data = MAKELPARAM(x, y);
	mouseMove(x, y);
	QThread::msleep(50);
	sendMessage(WM_LBUTTONDOWN, MK_LBUTTON, data);
	QThread::msleep(50);
	sendMessage(WM_LBUTTONUP, MK_LBUTTON, data);
	QThread::msleep(50);
}

void Injector::leftDoubleClick(int x, int y) const
{
	LPARAM data = MAKELPARAM(x, y);
	mouseMove(x, y);
	QThread::msleep(50);
	sendMessage(WM_LBUTTONDBLCLK, MK_LBUTTON, data);
	QThread::msleep(50);
}

void Injector::rightClick(int x, int y) const
{
	LPARAM data = MAKELPARAM(x, y);
	mouseMove(x, y);
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
	mouseMove(x1, y1);
	QThread::msleep(50);
	sendMessage(WM_LBUTTONDOWN, MK_LBUTTON, datafrom);
	QThread::msleep(50);
	mouseMove(x2, y2);
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