#include "stdafx.h"
#include "injector.h"

Injector* Injector::instance = nullptr;

bool Injector::createProcess(Injector::process_information_t& pi)
{
	const QString fileName(qgetenv("JSON_PATH"));
	if (fileName.isEmpty())
		return false;

	util::Config config(fileName);
	QString path = config.readString("System", "Command", "DirPath");
	if (path.isEmpty())
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
	int tmp = config.readInt("System", "Command", "realbin");
	if (tmp)
		nRealBin = tmp;
	else
		canSave = true;
	tmp = config.readInt("System", "Command", "adrnbin");
	if (tmp)
		nAdrnBin = tmp;
	else
		canSave = true;
	tmp = config.readInt("System", "Command", "sprbin");
	if (tmp)
		nSprBin = tmp;
	else
		canSave = true;
	tmp = config.readInt("System", "Command", "spradrnbin");
	if (tmp)
		nSprAdrnBin = tmp;
	tmp = config.readInt("System", "Command", "realtrue");
	if (tmp)
		nRealTrue = tmp;
	else
		canSave = true;
	tmp = config.readInt("System", "Command", "adrntrue");
	if (tmp)
		nAdrnTrue = tmp;
	else
		canSave = true;
	tmp = config.readInt("System", "Command", "encode");
	if (tmp)
		nEncode = tmp;
	else
		canSave = true;

	auto mkcmd = [](const QString& sec, int value)->QString
	{
		return QString("%1:%2").arg(sec).arg(value);
	};

	QStringList commandList;
	commandList.append(path);
	commandList.append("update");
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
	process.startDetached(&pid);
	process.waitForFinished();
	if (pid)
	{
		pi.dwProcessId = pid;
		if (canSave)
		{
			config.write("System", "Command", "DirPath", path);
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

bool Injector::injectLibrary(Injector::process_information_t& pi, unsigned short port, util::LPREMOVE_THREAD_REASON pReason)
{
	if (!pi.dwProcessId) return false;
	if (!pReason) return false;

	constexpr qint64 MAX_TIMEOUT = 30000;
	bool bret = 0;
	QElapsedTimer timer;
	HMODULE kernel32Module = NULL;
	FARPROC loadLibraryProc = NULL;
	DWORD hModule = NULL;
	QString dllPath = "\0";
	QFileInfo fi;
	QString fileNameOnly;
	DWORD parent = NULL;


	do
	{
#if QT_NO_DEBUG
		dllPath = QCoreApplication::applicationDirPath() + "/sadll.dll";
#else
		dllPath = R"(D:\Users\bestkakkoii\Desktop\vs_project\SaSH\Debug\sadll.dll)";
#endif
		fi.setFile(dllPath);
		fileNameOnly = fi.fileName();
		pi.hWnd = NULL;

		timer.start();

		if (!pi.hWnd)
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
				QThread::msleep(100UL);
			}
		}

		if (NULL == pi.hWnd)
		{
			if (pReason)
				*pReason = util::REASON_ENUM_WINDOWS_FAIL;
			break;
		}

		bool skip = false;

		if (skip)
			break;

		//去除改變窗口大小的屬性
		LONG dwStyle = ::GetWindowLongPtrW(pi.hWnd, GWL_STYLE);
		dwStyle &= ~(WS_SIZEBOX | WS_MAXIMIZEBOX);
		::SetWindowLongPtr(pi.hWnd, GWL_STYLE, dwStyle);

		pi.dwThreadId = ::GetWindowThreadProcessId(pi.hWnd, nullptr);


		kernel32Module = ::GetModuleHandleW(L"kernel32.dll");
		if (!kernel32Module)
		{
			if (pReason)
				*pReason = util::REASON_GET_KERNEL32_FAIL;
			break;
		}

		loadLibraryProc = ::GetProcAddress(kernel32Module, "LoadLibraryW");
		if (!loadLibraryProc)
		{
			if (pReason)
				*pReason = util::REASON_GET_KERNEL32_UNDOCUMENT_API_FAIL;
			break;
		}

		if (!isHandleValid(pi.dwProcessId))
		{
			if (pReason)
				*pReason = util::REASON_OPEN_PROCESS_FAIL;
			break;
		}

		if (!processHandle_.isValid())
			processHandle_.reset(pi.dwProcessId);

		const util::VMemory lpParameter(processHandle_, dllPath, util::VMemory::V_UNICODE, true);
		if (!lpParameter)
		{
			if (pReason)
				*pReason = util::REASON_VIRTUAL_ALLOC_FAIL;
			break;
		}

		QScopedHandle hThreadHandle(QScopedHandle::CREATE_REMOTE_THREAD, processHandle_,
			(PVOID)(LPTHREAD_START_ROUTINE)loadLibraryProc, (LPVOID)(DWORD)lpParameter);
		if (hThreadHandle.isValid())
		{
			timer.restart();
			for (;;)
			{
				hModule = mem::GetRemoteModuleHandle(pi.dwProcessId, fileNameOnly);
				if (hModule)
					break;

				if (timer.hasExpired(MAX_TIMEOUT))
					break;
				QThread::msleep(100UL);
			}
		}

		if (!hModule)
		{
			if (pReason)
				*pReason = util::REASON_INJECT_LIBRARY_FAIL;
			return false;
		}

		pi_ = pi;
		parent = qgetenv("SASH_HWND").toULong();
		sendMessage(kInitialize, port, parent);

		while (!hModule_)
			hModule_ = sendMessage(kGetModule, NULL, NULL);

		hookdllModule_ = (HMODULE)hModule;
		qDebug() << ":Inject OK!";
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
	HMODULE kernel32Module = GetModuleHandle(L"kernel32.dll");
	if (!kernel32Module) return;
	FARPROC freeLibraryProc = GetProcAddress(kernel32Module, "FreeLibrary");

	const util::VMemory lpParameter(processHandle_, sizeof(int), true);
	if (!lpParameter)
	{
		return;
	}


	mem::writeInt(processHandle_, lpParameter, (int)hookdllModule_, 0);

	QScopedHandle hThreadHandle(QScopedHandle::CREATE_REMOTE_THREAD, processHandle_,
		(PVOID)(LPTHREAD_START_ROUTINE)freeLibraryProc, (LPVOID)(DWORD)lpParameter);
	WaitForSingleObject(hThreadHandle, 1000);
}

bool Injector::isWindowAlive() const
{
	if (!isValid())
		return false;

#ifndef _DEBUG
	if (SendMessageTimeoutW(pi_.hWnd, WM_NULL, 0, 0, SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, 3000, nullptr) == 0)
		return false;
#endif

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
	int bResult = Process32First(hSnapshop, &program_info);
	if (!bResult)
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

	while (bResult)
	{
		if (program_info.th32ProcessID == pi_.dwProcessId)
			return true;

		bResult = Process32Next(hSnapshop, &program_info);
	}
	return false;
}