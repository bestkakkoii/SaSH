#include "stdafx.h"
#include <injector.h>
#include "model/listview.h"

Injector* Injector::instance = nullptr;

constexpr const char* InjectDllName = u8"sadll.dll";
constexpr int MessageTimeout = 3000;

Injector::Injector()
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
		instance->server.reset();
		instance->hModule_ = NULL;
		instance->hookdllModule_ = NULL;
		instance->pi_ = {  };
		instance->processHandle_.reset();
		util::SafeHash<util::UserSetting, int> valueHash = instance->getValueHash();
		util::SafeHash<util::UserSetting, bool> enableHash = instance->getEnableHash();
		util::SafeHash<util::UserSetting, QString> stringHash = instance->getStringHash();
		delete instance;
		instance = new Injector();
		if (instance)
		{
			instance->setValueHash(valueHash);
			instance->setEnableHash(enableHash);
			instance->setStringHash(stringHash);
		}
	}
}

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
	//啟動參數

	//updated realbin:138 adrnbin:138 sprbin:116 spradrnbin:116 adrntrue:5 realtrue:13 encode:0 windowmode
	commandList.append("update");
	commandList.append(mkcmd("realbin", nRealBin));
	commandList.append(mkcmd("adrnbin", nAdrnBin));
	commandList.append(mkcmd("sprbin", nSprBin));
	commandList.append(mkcmd("spradrnbin", nSprAdrnBin));
	commandList.append(mkcmd("adrntrue", nAdrnTrue));
	commandList.append(mkcmd("realtrue", nRealTrue));
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

int Injector::sendMessage(int msg, int wParam, int lParam) const
{
	if (msg < 0) return 0;
#ifdef _WIN64
	DWORD_PTR dwResult = NULL;
#else
	DWORD dwResult = NULL;
#endif
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

DWORD WINAPI getFunAddr(const DWORD* DllBase, const char* FunName)
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
	if (!pi.dwProcessId) return false;
	if (nullptr == pReason) return false;

	constexpr qint64 MAX_TIMEOUT = 15000;
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
#if QT_NO_DEBUG
		dllPath = QCoreApplication::applicationDirPath() + "/" + InjectDllName;
#else
		dllPath = R"(D:\Users\bestkakkoii\Desktop\vs_project\SaSH\Debug\sadll.dll)";
#endif
		fi.setFile(dllPath);
		fileNameOnly = fi.fileName();

		//檢查dll生成日期必須與當前exe相同日或更早
		QFileInfo exeInfo(QCoreApplication::applicationFilePath());

		if (fi.exists() && fi.lastModified() > exeInfo.lastModified())
		{
			break;
		}


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
				QThread::msleep(100UL);
			}
		}

		if (nullptr == pi.hWnd)
		{
			*pReason = util::REASON_ENUM_WINDOWS_FAIL;
			break;
		}

		bool skip = false;

		if (skip)
			break;

		//去除改變窗口大小的屬性
		LONG dwStyle = ::GetWindowLongW(pi.hWnd, GWL_STYLE);
		dwStyle &= ~(WS_SIZEBOX | WS_MAXIMIZEBOX);
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
			static_cast<PVOID>(reinterpret_cast<LPTHREAD_START_ROUTINE>(loadLibraryProc)),
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
				QThread::msleep(100UL);
			}
		}

		if (NULL == hModule)
		{
			*pReason = util::REASON_INJECT_LIBRARY_FAIL;
			break;
		}

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
		}

		if (NULL == hModule_)
		{
			*pReason = util::REASON_INJECT_LIBRARY_FAIL;
			break;
		}

		hookdllModule_ = reinterpret_cast<HMODULE>(hModule);
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
	DWORD* kernel32Module = getKernel32();//::GetModuleHandleW(L"kernel32.dll");
	if (kernel32Module == nullptr) return;
	FARPROC freeLibraryProc = reinterpret_cast<FARPROC>(getFunAddr(kernel32Module, u8"FreeLibrary"));

	const util::VirtualMemory lpParameter(processHandle_, sizeof(int), true);
	if (NULL == lpParameter)
	{
		return;
	}

	mem::writeInt(processHandle_, lpParameter, (int)hookdllModule_, 0);

	QScopedHandle hThreadHandle(QScopedHandle::CREATE_REMOTE_THREAD, processHandle_,
		static_cast<LPVOID>(reinterpret_cast<LPTHREAD_START_ROUTINE>(freeLibraryProc)),
		reinterpret_cast<LPVOID>(static_cast<DWORD>(lpParameter)));
	WaitForSingleObject(hThreadHandle, 1000);
}

bool Injector::isWindowAlive() const
{
	if (!isValid())
		return false;

	//#ifndef _DEBUG
	//	//CE下斷點時會自動關閉遊戲所以DEBUG時不檢查
	//	if (SendMessageTimeoutW(pi_.hWnd, WM_NULL, 0, 0, SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, MessageTimeout, nullptr) == 0)
	//		return false;
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