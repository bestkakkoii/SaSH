/*
				GNU GENERAL PUBLIC LICENSE
				   Version 2, June 1991
COPYRIGHT (C) Bestkakkoii 2024 All Rights Reserved.
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
#include <gamedevice.h>
#include "model/listview.h"

Server GameDevice::server;
safe::hash<long long, GameDevice*> GameDevice::instances;

constexpr long long MessageTimeout = 8000;

GameDevice::GameDevice(long long index)
	: Indexer(index)
	, log(index, nullptr)
	, autil(index)
	, isWin7OrLower_(QOperatingSystemVersion::current() <= QOperatingSystemVersion::Windows7)
{
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(index);
	connect(&signalDispatcher, &SignalDispatcher::nodifyAllStop, this, &GameDevice::gameRequestInterruption);
}

GameDevice::~GameDevice()
{
	qDebug() << "GameDevice is destroyed!!";
}

void GameDevice::reset()
{
	QList<long long> keys = instances.keys();
	for (long long key : keys)
	{
		reset(key);
	}
}

void GameDevice::reset(long long index)//static
{
	if (!instances.contains(index))
		return;

	GameDevice* instance = instances.value(index);
	if (instance == nullptr)
		return;

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
	instance->scriptThreadId = 0;

	instance->IS_TCP_CONNECTION_OK_TO_USE.off();
	instance->IS_INJECT_OK.off();
	instance->setEnableHash(util::kLogOutEnable, false);
	instance->setEnableHash(util::kEchoEnable, false);
	instance->setEnableHash(util::kLogBackEnable, false);
	instance->resetGameInterruptionFlag();
}

GameDevice::CreateProcessResult GameDevice::createProcess(GameDevice::process_information_t& pi)
{
	QString path = currentGameExePath;
	if (path.isEmpty() || !QFile::exists(path))
	{
		return CreateProcessResult::CreateFail;
	}

	QVector<long long> pids;
	if (mem::enumProcess(&pids, SASH_INJECT_DLLNAME))//枚舉已經注入過sadll但狀態值為0的進程
	{
		for (long long pid : pids)
		{
			ScopedHandle hProcess(pid);
			if (mem::read<int>(hProcess, hGameModule_ + sa::kOffsetGameInjectState) == 0)
			{
				pi.dwProcessId = pid;
				processHandle_.reset(pi.dwProcessId);
				return CreateProcessResult::CreateWithExistingProcess;
			}
		}
	}
	else if (mem::enumProcess(&pids, SASH_SUPPORT_GAMENAME, SASH_INJECT_DLLNAME))//枚舉未注入過sadll且封包KEY包含帳號或帳號相同的進程
	{
		QString userName = getStringHash(util::kGameAccountString);
		if (!userName.isEmpty())
		{
			for (long long pid : pids)
			{
				ScopedHandle hProcess(pid);
				QString remoteUserNameKey = mem::readString(hProcess, hGameModule_ + sa::kOffsetPersonalKey, 32);
				QString remoteUserName = mem::readString(hProcess, hGameModule_ + sa::kOffsetAccount, 32);

				QString remoteUserNameEx = ecbDecrypt(sa::kOffsetAccountECB);

				if (!remoteUserNameKey.isEmpty() &&
					(remoteUserNameKey.contains(userName)
						|| (!remoteUserName.isEmpty() && remoteUserName == userName)
						|| (!remoteUserNameEx.isEmpty() && remoteUserNameEx == userName)))
				{
					pi.dwProcessId = pid;
					processHandle_.reset(pi.dwProcessId);
					return CreateProcessResult::CreateWithExistingProcessNoDll;
				}


			}
		}
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

long long GameDevice::sendMessage(long long msg, long long wParam, long long lParam) const
{
	if (WM_NULL == msg)
		return 0;

	DWORD_PTR dwResult = 0L;
	SendMessageTimeoutW(pi_.hWnd, static_cast<UINT>(msg), static_cast<WPARAM>(wParam), static_cast<LPARAM>(lParam),
		SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, static_cast<UINT>(MessageTimeout), &dwResult);
	return static_cast<long long>(dwResult);
}

bool GameDevice::postMessage(long long msg, long long wParam, long long lParam) const
{
	if (WM_NULL == msg)
		return false;

	BOOL ret = PostMessageW(pi_.hWnd, static_cast<UINT>(msg), static_cast<WPARAM>(wParam), static_cast<LPARAM>(lParam));
	return  ret == TRUE;
}

bool GameDevice::isHandleValid(long long pid)
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

bool GameDevice::capture(const QString& fileName) const
{
	if (!isValid())
		return false;

	QFileInfo fileInfo(fileName);
	//check is a valid path
	if (fileInfo.absolutePath().isEmpty() || fileInfo.fileName().isEmpty())
		return false;

	//check png suffix
	if (fileInfo.suffix().toLower() != "png")
		return false;

	long long currentIndex = getIndex();

	//取桌面指針
	QScreen* screen = QGuiApplication::primaryScreen();
	if (nullptr == screen)
	{
		return false;
	}

	//遊戲後臺滑鼠一移動到左上
	LPARAM data = MAKELPARAM(0, 0);
	sendMessage(WM_MOUSEMOVE, NULL, data);

	//根據HWND擷取窗口後臺圖像
	QPixmap pixmap = screen->grabWindow(reinterpret_cast<WId>(getProcessWindow()));

	//轉存為QImage
	QImage image = pixmap.toImage();

	//only take middle part of the image
	image = image.copy(269, 226, 102, 29);//368,253

	if (QFile::exists(fileName))
		QFile::remove(fileName);

	return image.save(fileName, "PNG");
}

QString GameDevice::ecbDecrypt(long long address) const
{
	mem::VirtualMemory lpDst(processHandle_, 32, true);
	sendMessage(kECBCrypt, getProcessModule() + address, lpDst);
	return mem::readString(processHandle_, lpDst, 32);
}

QByteArray GameDevice::ecbEncrypt(const QString& str) const
{
	mem::VirtualMemory lpSrc(processHandle_, str, mem::VirtualMemory::kAnsi, true);
	mem::VirtualMemory lpDst(processHandle_, 32, true);

	sendMessage(kECBEncrypt, lpSrc, lpDst);
	QByteArray result(32, '\0');
	mem::read(processHandle_, lpDst, 32, result.data());
	return result;
}

void GameDevice::paused()
{
	if (isScriptPaused())
		return;

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(getIndex());
	emit signalDispatcher.scriptPaused();

	{
		std::unique_lock<std::mutex> lock(scriptPausedMutex_);
		isScriptPaused_.on();
	}
}

void GameDevice::resumed()
{
	if (!isScriptPaused())
		return;

	{
		std::unique_lock<std::mutex> lock(scriptPausedMutex_);
		isScriptPaused_.off();
	}

	scriptPausedCondition_.notify_all();
}

void GameDevice::checkScriptPause()
{
	if (isScriptPaused())
	{
		std::unique_lock<std::mutex> lock(scriptPausedMutex_);
		scriptPausedCondition_.wait(lock);
	}
}

#if 0
DWORD WINAPI GameDevice::getFunAddr(const DWORD* DllBase, const char* FunName)
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

bool GameDevice::remoteInitialize(GameDevice::process_information_t& pi, unsigned short port, util::LPREMOVE_THREAD_REASON pReason)
{
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

	injectdate.index = getIndex();
	injectdate.parentHWnd = reinterpret_cast<long long>(getParentWidget());
	injectdate.port = port;
	injectdate.type = kIPv6;

	if (!mem::isProcessExist(pi.dwProcessId))
	{
		*pReason = util::REASON_INJECT_LIBRARY_FAIL;
		return false;
	}

	if (IsWindow(pi.hWnd) == FALSE)
	{
		*pReason = util::REASON_INJECT_LIBRARY_FAIL;
		return false;
	}

	const mem::VirtualMemory lpStruct(processHandle_, sizeof(InitialData), true);
	if (!lpStruct.isValid())
	{
		//emit signalDispatcher.messageBoxShow(QObject::tr("Remote virtualmemory alloc failed"), QMessageBox::Icon::Critical);
		*pReason = util::REASON_INJECT_LIBRARY_FAIL;
		return false;
	}

	if (!mem::write(processHandle_, lpStruct, &injectdate, sizeof(InitialData)))
	{
		//emit signalDispatcher.messageBoxShow(QObject::tr("Remote virtualmemory write failed"), QMessageBox::Icon::Critical);
		*pReason = util::REASON_INJECT_LIBRARY_FAIL;
		return false;
	}

	util::timer timer;
	DWORD_PTR dwResult = 0L;
	for (;;)
	{
		if (IsWindowVisible(pi_.hWnd) == TRUE)
		{
			if (SendMessageTimeoutW(pi_.hWnd, kInitialize, lpStruct, NULL, SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT | SMTO_BLOCK, MessageTimeout, &dwResult) != 0)
			{
				if (dwResult == 0)
				{
					//emit signalDispatcher.messageBoxShow(QObject::tr("Remote dll initialize failed"), QMessageBox::Icon::Critical);
					*pReason = util::REASON_INJECT_LIBRARY_FAIL;
					break;
				}
				break;
			}
		}

		if (!mem::isProcessExist(pi.dwProcessId))
		{
			*pReason = util::REASON_INJECT_LIBRARY_FAIL;
			return false;
		}

		if (IsWindow(pi.hWnd) == FALSE)
		{
			*pReason = util::REASON_INJECT_LIBRARY_FAIL;
			return false;
		}

		if (timer.hasExpired(sa::MAX_TIMEOUT))
		{
			//emit signalDispatcher.messageBoxShow(QObject::tr("SendMessageTimeoutW failed"), QMessageBox::Icon::Critical);
			*pReason = util::REASON_INJECT_LIBRARY_FAIL;
			break;
		}

		QThread::msleep(10);
	}

	return dwResult != 0;
}

bool GameDevice::injectLibrary(GameDevice::process_information_t& pi, unsigned short port, util::LPREMOVE_THREAD_REASON pReason, bool bConnectOnly)
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
	util::timer timer;
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

			if (!mem::isProcessExist(pi.dwProcessId))
			{
				*pReason = util::REASON_INJECT_LIBRARY_FAIL;
				return false;
			}

			if (timer.hasExpired(sa::MAX_TIMEOUT))
			{
				//emit signalDispatcher.messageBoxShow(QObject::tr("EnumWindows timeout"), QMessageBox::Icon::Critical);
				break;
			}

			QThread::msleep(10);
		}

		if (nullptr == pi.hWnd)
		{
			//emit signalDispatcher.messageBoxShow(QObject::tr("EnumWindows failed"), QMessageBox::Icon::Critical);
			*pReason = util::REASON_ENUM_WINDOWS_FAIL;
			break;
		}

		qDebug() << "HWND OK, cost:" << timer.cost() << "ms";

		//紀錄線程ID(目前沒有使用到只是先記錄著)
		pi.dwThreadId = ::GetWindowThreadProcessId(pi.hWnd, nullptr);

		//嘗試打開進程句柄並檢查是否成功
		if (!isHandleValid(pi.dwProcessId))
		{
			//emit signalDispatcher.messageBoxShow(QObject::tr("OpenProcess failed"), QMessageBox::Icon::Critical);
			*pReason = util::REASON_OPEN_PROCESS_FAIL;
			break;
		}

		if (!processHandle_.isValid())
			processHandle_.reset(pi.dwProcessId);

		if (bConnectOnly)
		{
			pi_ = pi;
			return remoteInitialize(pi, port, pReason);
		}

		timer.restart();
		for (;;)
		{
			if (static_cast<long long>(mem::read<int>(processHandle_, hGameModule_ + sa::kOffsetWorldStatus)) == 1
				&& static_cast<long long>(mem::read<int>(processHandle_, hGameModule_ + sa::kOffsetGameStatus)) == 2)
				break;

			if (!mem::isProcessExist(pi.dwProcessId))
			{
				*pReason = util::REASON_INJECT_LIBRARY_FAIL;
				return false;
			}

			if (IsWindow(pi.hWnd) == FALSE)
			{
				*pReason = util::REASON_INJECT_LIBRARY_FAIL;
				return false;
			}

			if (timer.hasExpired(sa::MAX_TIMEOUT))
				break;

			QThread::msleep(200);
		}

		timer.restart();

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
		if (mem::injectByWin7(currentIndex, pi.dwProcessId, processHandle_, dllPath, &hookdllModule_, &hGameModule_, pi.hWnd))
			qDebug() << "inject cost:" << timer.cost() << "ms";
		else
		{
			*pReason = util::REASON_INJECT_LIBRARY_FAIL;
			return false;
		}
#else
		if (mem::injectBy64(currentIndex, pi.dwProcessId, processHandle_, dllPath, &hookdllModule_, &hGameModule_, pi.hWnd))
			qDebug() << "inject cost:" << timer.cost() << "ms";
		else
		{
			*pReason = util::REASON_INJECT_LIBRARY_FAIL;
			return false;
		}
#endif

		if (nullptr == hookdllModule_)
		{
			//emit signalDispatcher.messageBoxShow(QObject::tr("Injected dll module is null"), QMessageBox::Icon::Critical);
			*pReason = util::REASON_INJECT_LIBRARY_FAIL;
			break;
		}

		if (0 == hGameModule_)
		{
			//emit signalDispatcher.messageBoxShow(QObject::tr("Game module is null"), QMessageBox::Icon::Critical);
			*pReason = util::REASON_INJECT_LIBRARY_FAIL;
			break;
		}

		pi_ = pi;

		if (!remoteInitialize(pi, port, pReason))
			break;

		//去除改變窗口大小的屬性
		LONG dwStyle = ::GetWindowLongW(pi.hWnd, GWL_STYLE);
		LONG tempStyle = dwStyle;

		if (util::checkAND(dwStyle, WS_SIZEBOX))
			dwStyle &= ~WS_SIZEBOX;

		if (util::checkAND(dwStyle, WS_MAXIMIZEBOX))
			dwStyle &= ~WS_MAXIMIZEBOX;

		if (tempStyle != dwStyle)
			::SetWindowLongW(pi.hWnd, GWL_STYLE, dwStyle);

		bret = true;
		IS_INJECT_OK.on();
	} while (false);

	if (!bret)
		pi_ = {};

	return bret;
}

// 判斷特定進程的UI線程是否無響應
bool GameDevice::IsUIThreadHung() const
{
	ULONG length = 0;
	NTSTATUS status = MINT::NtQuerySystemInformation(MINT::SystemProcessInformation, nullptr, 0, &length);

	if (status != STATUS_INFO_LENGTH_MISMATCH)
	{
		return false; // 無法獲取所需的長度
	}

	QScopedArrayPointer<BYTE> buffer(new BYTE[length]);

	MINT::PSYSTEM_PROCESS_INFORMATION spi = reinterpret_cast<MINT::PSYSTEM_PROCESS_INFORMATION>(buffer.data());
	if (!spi)
	{
		return false; // 內存分配失敗
	}

	status = MINT::NtQuerySystemInformation(MINT::SystemProcessInformation, spi, length, &length);
	if (!NT_SUCCESS(status))
	{
		return false; // API 調用失敗
	}

	MINT::PSYSTEM_PROCESS_INFORMATION current = spi;
	for (;;)
	{
		if (current->UniqueProcessId == (HANDLE)(ULONG_PTR)pi_.dwProcessId)
		{
			// 找到了指定的進程，檢查它的所有線程
			for (ULONG i = 0; i < current->NumberOfThreads; i++)
			{
				MINT::SYSTEM_THREAD_INFORMATION& thread = current->Threads[i];
				//如果thread id不正確
				if (pi_.dwThreadId != (long long)thread.ClientId.UniqueThread)
					continue;

				// 檢查線程狀態和等待原因，這里以檢查是否為等待用戶輸入為例
				if (thread.ThreadState == MINT::KTHREAD_STATE::Waiting
					&& (thread.WaitReason == MINT::KWAIT_REASON::Suspended
						|| thread.WaitReason == MINT::KWAIT_REASON::WrSuspended
						|| thread.WaitReason == MINT::KWAIT_REASON::WrMutex
						|| thread.WaitReason == MINT::KWAIT_REASON::WrFastMutex
						|| thread.WaitReason == MINT::KWAIT_REASON::WrGuardedMutex
						))
				{
					return true; // 線程可能無響應
				}
			}

			break; // 已處理指定進程
		}

		if (current->NextEntryOffset == 0)
		{
			break;
		}

		current = (MINT::PSYSTEM_PROCESS_INFORMATION)((LPBYTE)current + current->NextEntryOffset);
	}

	return false; // 沒有檢測到無響應的UI線程
}


bool GameDevice::isWindowAlive() const
{
	if (!isValid())
		return false;

	do
	{
		DWORD_PTR dwResult = 0L;
		if (IsWindow(pi_.hWnd) == FALSE)
		{
			break;
		}

		if (SendMessageTimeoutW(pi_.hWnd, WM_NULL, 0, 0, SMTO_ERRORONEXIT | SMTO_ABORTIFHUNG, MessageTimeout, &dwResult) <= 0 || dwResult != 1234)
		{
			break;
		}

		if (IsHungAppWindow(pi_.hWnd) == TRUE)
		{
			break;
		}

		if (IsUIThreadHung())
		{
			break;
		}

		return true;
	} while (false);

	close();
	return false;
}

void GameDevice::mouseMove(long long x, long long y) const
{
	//LPARAM data = MAKELPARAM(x, y);
	//sendMessage(WM_MOUSEMOVE, NULL, data);
	mem::write<int>(processHandle_, hGameModule_ + sa::kOffestMouseX, x);
	mem::write<int>(processHandle_, hGameModule_ + sa::kOffestMouseY, y);
}

void GameDevice::leftClick(long long x, long long y) const
{
	//LPARAM data = MAKELPARAM(x, y);
	//sendMessage(WM_MOUSEMOVE, NULL, data);
	//QThread::msleep(50);
	//sendMessage(WM_LBUTTONDOWN, MK_LBUTTON, data);
	//QThread::msleep(50);
	//sendMessage(WM_LBUTTONUP, MK_LBUTTON, data);
	//QThread::msleep(50);
	mouseMove(x, y);
	QThread::msleep(50);
	mem::write<int>(processHandle_, hGameModule_ + sa::kOffestMouseClick, 1);
}

void GameDevice::leftDoubleClick(long long x, long long y) const
{
	//LPARAM data = MAKELPARAM(x, y);
	//sendMessage(WM_MOUSEMOVE, NULL, data);
	//QThread::msleep(50);
	//sendMessage(WM_LBUTTONDBLCLK, MK_LBUTTON, data);
	//QThread::msleep(50);
	mouseMove(x, y);
	QThread::msleep(50);
	LPARAM data = MAKELPARAM(x, y);
	postMessage(WM_LBUTTONDBLCLK, MK_LBUTTON, data);
}

void GameDevice::rightClick(long long x, long long y) const
{
	//LPARAM data = MAKELPARAM(x, y);
	//sendMessage(WM_MOUSEMOVE, NULL, data);
	//QThread::msleep(50);
	//sendMessage(WM_RBUTTONDOWN, MK_RBUTTON, data);
	//QThread::msleep(50);
	//sendMessage(WM_RBUTTONUP, MK_RBUTTON, data);
	//QThread::msleep(50);
	mouseMove(x, y);
	QThread::msleep(50);
	mem::write<int>(processHandle_, hGameModule_ + sa::kOffestMouseClick, 2);
}

void GameDevice::dragto(long long x1, long long y1, long long x2, long long y2) const
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

void GameDevice::hide(long long mode) const
{
	HWND hWnd = getProcessWindow();
	if (hWnd == nullptr)
		return;

	LONG_PTR exstyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);

	//retore before hide
	ShowWindow(hWnd, SW_RESTORE);

	//add tool window style to hide from taskbar

	if (!isWin7OrLower())
	{
		if (!util::checkAND(exstyle, WS_EX_TOOLWINDOW))
			exstyle |= WS_EX_TOOLWINDOW;
	}
	else
	{
		//add tool window style to hide from taskbar
		if (!util::checkAND(exstyle, WS_EX_APPWINDOW))
			exstyle |= WS_EX_APPWINDOW;
	}

	//添加透明化屬性
	if (!util::checkAND(exstyle, WS_EX_LAYERED))
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

void GameDevice::show() const
{
	HWND hWnd = getProcessWindow();
	if (hWnd == nullptr)
		return;

	sendMessage(kEnableWindowHide, false, NULL);

	LONG_PTR exstyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);

	if (!isWin7OrLower())
	{
		//remove tool window style to show from taskbar
		if (util::checkAND(exstyle, WS_EX_TOOLWINDOW))
			exstyle &= ~WS_EX_TOOLWINDOW;
	}
	else
	{
		//remove tool window style to show from taskbar
		if (util::checkAND(exstyle, WS_EX_APPWINDOW))
			exstyle &= ~WS_EX_APPWINDOW;
	}

	//移除透明化屬性
	if (util::checkAND(exstyle, WS_EX_LAYERED))
		exstyle &= ~WS_EX_LAYERED;
	SetWindowLongPtr(hWnd, GWL_EXSTYLE, exstyle);

	//active once
	ShowWindow(hWnd, SW_RESTORE);
	ShowWindow(hWnd, SW_SHOW);

	//bring to top once
	SetForegroundWindow(hWnd);
}

QString GameDevice::getPointFileName() const
{
	const QString dirPath(QString("%1/lib/map/%2").arg(util::applicationDirPath()).arg(currentServerListIndex.get()));
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