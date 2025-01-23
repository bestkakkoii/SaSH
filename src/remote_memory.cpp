#include "stdafx.h"
#include <remote_memory.h>
#include <tlhelp32.h>
#include <timer.h>

namespace remote_memory
{
	static qint64 getProcessExportTable32(HANDLE hProcess, const QString& moduleName, const QSet<QString>& targetFuncs, QHash<QString, ULONG64>* ptrs)
	{
		IMAGE_DOS_HEADER dosHeader = {};
		IMAGE_NT_HEADERS32 ntHeaders = {};
		IMAGE_EXPORT_DIRECTORY exportDirectory = {};

		// 拿到目標模塊的BASE
		ULONG64 muBase = reinterpret_cast<ULONG64>(getModuleHandle(hProcess, moduleName));
		if (NULL == muBase)
		{
			return 0;
		}

		// 獲取IMAGE_DOS_HEADER
		read(hProcess, muBase, &dosHeader, sizeof(IMAGE_DOS_HEADER));
		if (dosHeader.e_magic != IMAGE_DOS_SIGNATURE)
		{
			return 0;
		}

		// 獲取IMAGE_NT_HEADERS
		read(hProcess, (muBase + dosHeader.e_lfanew), &ntHeaders, sizeof(IMAGE_NT_HEADERS32));

		if (ntHeaders.OptionalHeader.DataDirectory[0].VirtualAddress == 0)
		{
			return 0;
		}

		char strName[130] = {};
		RtlSecureZeroMemory(strName, sizeof(strName));
		// 獲取IMAGE_EXPORT_DIRECTORY
		read(hProcess, (muBase + ntHeaders.OptionalHeader.DataDirectory[0].VirtualAddress), &exportDirectory, sizeof(IMAGE_EXPORT_DIRECTORY));
		read(hProcess, (muBase + exportDirectory.Name), strName, sizeof(strName));

		if (exportDirectory.NumberOfNames < 0/* || exportDirectory.NumberOfNames > tb_info_max*/)
		{
			return 0;
		}

		qint64 count = 0;
		char bFuncName[100] = {};
		ULONG64 ulPointer = 0;
		USHORT usFuncId = 0;
		ULONG64 ulFuncAddr = 0;
		//std::string smoduleName;

		// 遍歷導出表
		for (ULONG64 i = 0; i < exportDirectory.NumberOfNames; ++i)
		{
			RtlSecureZeroMemory(bFuncName, sizeof(bFuncName));

			ulPointer = 0;
			usFuncId = 0;
			ulFuncAddr = 0;

			ulPointer = static_cast<ULONG64>(read<qint32>(hProcess, (muBase + exportDirectory.AddressOfNames + i * static_cast<ULONG64>(sizeof(DWORD)))));

			RtlSecureZeroMemory(bFuncName, sizeof(bFuncName));

			read(hProcess, (muBase + ulPointer), bFuncName, sizeof(bFuncName));

			usFuncId = read<qint16>(hProcess, (muBase + exportDirectory.AddressOfNameOrdinals + i * sizeof(qint16)));
			ulPointer = static_cast<ULONG64>(read<qint32>(hProcess, (muBase + exportDirectory.AddressOfFunctions + static_cast<ULONG64>(sizeof(DWORD)) * usFuncId)));
			ulFuncAddr = muBase + ulPointer;

			if (targetFuncs.contains(bFuncName))
			{
				ptrs->insert(bFuncName, ulFuncAddr);
				if (ptrs->size() == targetFuncs.size())
				{
					count = targetFuncs.size();
					break;
				}
			}

			//smoduleName = moduleName.toStdString();

			//tbinfo[count].Address = ulFuncAddr;
			//tbinfo[count].RecordAddr = (ULONG64)(muBase + pExport->AddressOfFunctions + static_cast<ULONG64>(sizeof(DWORD)) * usFuncId);
			//tbinfo[count].ModBase = muBase;

			//_snprintf_s(tbinfo[count].ModuleName, sizeof(tbinfo[count].ModuleName), _TRUNCATE, "%s", smoduleName.c_str());
			//_snprintf_s(tbinfo[count].FuncName, sizeof(tbinfo[count].FuncName), _TRUNCATE, "%s", bFuncName);

			++count;

			//if (count > (tb_info_max))
			//{
			//    break;
			//}
		}

		return count;
	}

	static HANDLE NtOpenProcess(DWORD dwProcess)
	{
		HANDLE ProcessHandle = NULL;
		using namespace MINT;
		OBJECT_ATTRIBUTES ObjectAttribute = {
			sizeof(OBJECT_ATTRIBUTES), 0, 0, 0, 0, 0
		};
		CLIENT_ID ClientId = {};

		InitializeObjectAttributes(&ObjectAttribute, 0, 0, 0, 0);
		ClientId.UniqueProcess = reinterpret_cast<PVOID>(static_cast<qint64>(dwProcess));
		ClientId.UniqueThread = (PVOID)0;

		BOOL ret = NT_SUCCESS(MINT::NtOpenProcess(&ProcessHandle, MAXIMUM_ALLOWED,
			&ObjectAttribute, &ClientId));

		return TRUE == ret && ProcessHandle && ((ProcessHandle) != INVALID_HANDLE_VALUE) ? ProcessHandle : nullptr;
	};

	static HANDLE ZwOpenProcess(DWORD dwProcess)
	{
		HANDLE ProcessHandle = (HANDLE)0;
		MINT::OBJECT_ATTRIBUTES ObjectAttribute = {
			sizeof(MINT::OBJECT_ATTRIBUTES), 0, 0, 0, 0, 0
		};
		ObjectAttribute.Attributes = NULL;
		MINT::CLIENT_ID ClientIds = {};
		ClientIds.UniqueProcess = reinterpret_cast<HANDLE>(static_cast<qint64>(dwProcess));
		ClientIds.UniqueThread = (HANDLE)0;
		BOOL ret = NT_SUCCESS(MINT::ZwOpenProcess(&ProcessHandle, PROCESS_ALL_ACCESS, &ObjectAttribute,
			&ClientIds));

		return TRUE == ret && ProcessHandle && ((ProcessHandle) != INVALID_HANDLE_VALUE) ? ProcessHandle : nullptr;
	};

	static HANDLE NtOpenThread(DWORD dwProcess, DWORD threadId)
	{
		using namespace MINT;
		HANDLE hThread = nullptr;
		CLIENT_ID ClientId = {};
		OBJECT_ATTRIBUTES ObjectAttribute = {
			sizeof(OBJECT_ATTRIBUTES), 0, 0, 0, 0, 0
		};
		InitializeObjectAttributes(&ObjectAttribute, 0, 0, 0, 0);
		ClientId.UniqueProcess = reinterpret_cast<PVOID>(static_cast<qint64>(dwProcess));
		ClientId.UniqueThread = reinterpret_cast<PVOID>(static_cast<qint64>(threadId));
		BOOL ret = NT_SUCCESS(MINT::NtOpenThread(&hThread, THREAD_ALL_ACCESS, &ObjectAttribute, &ClientId));

		return TRUE == ret && hThread && ((hThread) != INVALID_HANDLE_VALUE) ? hThread : nullptr;
	}

	static HANDLE openProcessToken(HANDLE ProcessHandle, ACCESS_MASK DesiredAccess)
	{
		HANDLE hToken = nullptr;
		BOOL ret = NT_SUCCESS(MINT::NtOpenProcessToken(ProcessHandle, DesiredAccess, &hToken));
		if (TRUE == ret && hToken != nullptr && ((hToken) != INVALID_HANDLE_VALUE))
		{
			return hToken;
		}

		return nullptr;
	}
}

// 提權函數：提升為DEBUG權限
bool remote_memory::enablePrivilege(HANDLE hProcess)
{
	if (!hProcess)
		return FALSE;

	BOOL fOk = FALSE;
	do
	{
		HANDLE hToken = openProcessToken(hProcess, TOKEN_ALL_ACCESS);
		if (hToken == NULL || hToken == INVALID_HANDLE_VALUE)
			break;

		TOKEN_PRIVILEGES tp = {};

		if (LookupPrivilegeValueW(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid) == FALSE)
			break;

		tp.PrivilegeCount = 1;
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		MINT::NtAdjustPrivilegesToken(hToken, FALSE, &tp, sizeof(tp), nullptr, nullptr);

		fOk = (::GetLastError() == ERROR_SUCCESS);

		MINT::NtClose(hToken);
	} while (false);

	return TRUE == fOk;
}

bool remote_memory::enablePrivilege()
{
	BOOLEAN bEnabled = FALSE;
	BOOL ret = NT_SUCCESS(MINT::RtlAdjustPrivilege(SE_DEBUG_PRIVILEGE, TRUE, FALSE, &bEnabled));

	return TRUE == ret;
}

HANDLE remote_memory::openProcess(DWORD processId)
{
	enablePrivilege();

	HANDLE hProcess = NtOpenProcess(processId);

	auto isValid = [hProcess]()->bool
		{
			return hProcess != nullptr && ((hProcess) != INVALID_HANDLE_VALUE);
		};

	do
	{
		if (isValid())
		{
			break;
		}

		hProcess = ZwOpenProcess(processId);
		if (isValid())
		{
			break;
		}

		hProcess = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
	} while (false);

	return hProcess;
}

HMODULE remote_memory::getModuleHandle(HANDLE hProcess, const QString& szModuleName)
{
	QVector<HMODULE> hMods(1024, nullptr);
	DWORD cbNeeded = 0;
	DWORD i = 0;
	wchar_t szModName[MAX_PATH] = {};

	if (K32EnumProcessModulesEx(hProcess, hMods.data(), static_cast<DWORD>(hMods.size() * sizeof(HMODULE)), &cbNeeded, LIST_MODULES_ALL) == FALSE)
	{
		return nullptr;
	}

	for (i = 0; i <= cbNeeded / sizeof(HMODULE); ++i)
	{
		RtlSecureZeroMemory(szModName, sizeof(szModName));
		if (K32GetModuleFileNameExW(hProcess, hMods.value(i), szModName, _countof(szModName)) == NULL)
		{
			continue;
		}

		QString filePath = QString::fromWCharArray(szModName);
		filePath.replace("\\", "/");
		QFileInfo file(filePath);
		QString fileName = file.fileName();
		if (fileName.compare(szModuleName, Qt::CaseInsensitive) != 0)
		{
			continue;
		}

		return hMods[i];
	}
	return nullptr;
}

bool remote_memory::getProcAddress32(HANDLE hProcess, const QString& ModuleName, const QSet<QString>& targetFuncs, QHash<QString, ULONG64>* ptrs)
{
	if (nullptr == ptrs)
	{
		return false;
	}

	qint64 count = getProcessExportTable32(hProcess, ModuleName, targetFuncs, ptrs);

	return count == static_cast<qint64>(targetFuncs.size());
}

quint64 remote_memory::alloc(HANDLE hProcess, quint64 size)
{
	if (hProcess == nullptr || size == 0)
	{
		return 0;
	}

	// Ensure size is aligned to page size
	size = (size + 4095) & ~4095ULL;

	PVOID ptr = NULL;
	SIZE_T sizet = static_cast<SIZE_T>(size);

	MINT::NTSTATUS status = MINT::NtAllocateVirtualMemory(hProcess, &ptr, NULL, &sizet, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (!NT_SUCCESS(status))
	{
		return 0;
	}

	if (nullptr == ptr)
	{
		return 0;
	}

	return reinterpret_cast<quint64>(ptr);
}

bool remote_memory::free(HANDLE hProcess, quint64 desiredAccess)
{
	if (nullptr == hProcess)
	{
		return false;
	}

	if (desiredAccess == NULL)
	{
		return false;
	}

	SIZE_T size = 0;
	BOOL ret = NT_SUCCESS(MINT::NtFreeVirtualMemory(hProcess, reinterpret_cast<PVOID*>(&desiredAccess), &size, MEM_RELEASE));

	return TRUE == ret;
}

bool remote_memory::protect(HANDLE hProcess, quint64 address, quint64 size, quint64 flNewProtect, quint64* flOldProtect)
{
	if (hProcess == nullptr || address == 0 || size == 0)
	{
		return false;
	}

	SIZE_T sizet = static_cast<SIZE_T>(size);
	BOOL ret = VirtualProtectEx(hProcess, reinterpret_cast<PVOID>(address), sizet, static_cast<DWORD>(flNewProtect), reinterpret_cast<PDWORD>(flOldProtect));
	return TRUE == ret;
}

bool remote_memory::write(HANDLE hProcess, quint64 desiredAccess, const void* buffer, quint64 dwSize)
{
	if (nullptr == hProcess)
	{
		return false;
	}

	enablePrivilege(hProcess);

	quint64 oldProtect = NULL;
	protect(hProcess, desiredAccess, dwSize, PAGE_EXECUTE_READWRITE, &oldProtect);
	BOOL ret = WriteProcessMemory(hProcess, reinterpret_cast<PVOID>(desiredAccess), buffer, dwSize, NULL);
	protect(hProcess, desiredAccess, dwSize, oldProtect, &oldProtect);

	return TRUE == ret;
}

template<typename T, typename>
bool remote_memory::write(HANDLE hProcess, quint64 desiredAccess, T data)
{
	if (nullptr == hProcess)
	{
		return false;
	}

	if (!desiredAccess)
	{
		return false;
	}

	PVOID pBuffer = &data;
	BOOL ret = write(hProcess, desiredAccess, pBuffer, sizeof(T));

	return TRUE == ret;
}

bool remote_memory::write(HANDLE hProcess, quint64 desiredAccess, const QString& str)
{
	if (nullptr == hProcess)
	{
		return false;
	}

	static QTextCodec* codec = QTextCodec::codecForName("Big5-HKSCS");

	QByteArray ba = codec->fromUnicode(str);
	ba.append('\0');

	const char* pBuffer = ba.constData();
	quint64 len = ba.size();

	constexpr size_t BUFFER_SIZE = 8192;
	QScopedArrayPointer <char> p(new char[8192]());

	if (len + 1 < BUFFER_SIZE)
		_snprintf_s(p.get(), len + 1, _TRUNCATE, "%s\0", pBuffer);
	else
		_snprintf_s(p.get(), BUFFER_SIZE, _TRUNCATE, "%s\0", pBuffer);

	BOOL ret = write(hProcess, desiredAccess, p.get(), len + 1);

	return TRUE == ret;
}

bool remote_memory::read(HANDLE hProcess, quint64 desiredAccess, void* buffer, quint64 size)
{
	if (!size)
	{
		return false;
	}

	if (!buffer)
	{
		return false;
	}

	if (nullptr == hProcess)
	{
		return false;
	}

	if (!desiredAccess)
	{
		return false;
	}

	enablePrivilege(hProcess);

	quint64 oldProtect = NULL;
	protect(hProcess, desiredAccess, size, PAGE_EXECUTE_READWRITE, &oldProtect);
	BOOL ret = NT_SUCCESS(MINT::NtReadVirtualMemory(hProcess, reinterpret_cast<PVOID>(desiredAccess), buffer, size, NULL));
	protect(hProcess, desiredAccess, size, oldProtect, &oldProtect);

	return TRUE == ret;
}

template<typename T, typename>
T remote_memory::read(HANDLE hProcess, quint64 desiredAccess)
{
	if (nullptr == hProcess)
	{
		return T();
	}

	T buffer{};
	quint64 size = sizeof(T);

	if (!size)
	{
		return T();
	}


	bool ret = read(hProcess, desiredAccess, &buffer, size);

	return ret ? buffer : T();
}

QString remote_memory::read(HANDLE hProcess, quint64 desiredAccess, quint64 size)
{
	if (nullptr == hProcess)
	{
		return "";
	}

	if (!desiredAccess)
	{
		return "";
	}

	static QTextCodec* codec = QTextCodec::codecForName("Big5-HKSCS");

	QScopedArrayPointer <char> buffer(new char[size + 1]());

	bool ret = read(hProcess, desiredAccess, buffer.get(), size);

	QString retstring = (ret) ? codec->toUnicode(buffer.get()) : "";
	return retstring;
}

bool remote_memory::injectLibrary32(
	DWORD processId,
	QString dllPath,
	HMODULE* phDllModule,
	quint64* phGameModule,
	QString* replyMessage)
{
	Timer timer;
	Timer timerShow;

	// qDebug().noquote() << QString("Inject path:\"%2\"").arg(dllPath);

	if (nullptr == replyMessage)
	{
		return false;
	}

	replyMessage->clear();

	if (!QFile::exists(dllPath))
	{
		*replyMessage = QString("Inject dll file not exists:\"%2\"").arg(dllPath);
		// qDebug().noquote() << *replyMessage;
		return false;
	}

	static constexpr uchar data[128] = {
		0x55,										//push ebp
		0x8B, 0xEC,									//mov ebp,esp
		0x56,										//push esi

		//LoadLibraryW
		0x8B, 0x75, 0x08,							//mov esi,[ebp+08]
		0xFF, 0x76, 0x0C,							//push [esi+0C]
		0x8B, 0x06,									//mov eax,[esi]
		0xFF, 0xD0,									//call eax
		0x89, 0x46, 0x10,							//mov [esi+10],eax

		//GetLastError
		0x8B, 0x46, 0x04,							//mov eax,[esi+04]
		0xFF, 0xD0,									//call eax
		0x89, 0x46, 0x14,							//mov [esi+14],eax

		//GetModuleHandleW
		0x8B, 0x46, 0x08,							//mov eax,[esi+08]
		0x6A, 0x00,									//push 00 { 0 }
		0xFF, 0xD0,									//call eax
		0x89, 0x46, 0x18,							//mov [esi+18],eax

		//return 0
		0x33, 0xC0,									//xor eax,eax

		0x5E,										//pop esi
		0x5D,										//pop ebp
		0xC2, 0x04, 0x00							//ret 0004 { 4 }
	};

	struct InjectData
	{
		DWORD loadLibraryWPtr = NULL;
		DWORD getLastErrorPtr = NULL;
		DWORD getModuleHandleWPtr = NULL;
		DWORD dllFullPathAddr = NULL;
		DWORD remoteModule = NULL;
		DWORD lastError = ERROR_SUCCESS;
		DWORD gameModule = NULL;
	}injectData = {};

	ScopedProcessHandle hProcess(processId);
	if (!hProcess.isValid())
	{
		*replyMessage = QString("Unable to get process handle with processId:%1").arg(processId);
		// qDebug().noquote() << *replyMessage;
		return false;
	}

	Page dllFullPathAddr(hProcess, dllPath, Page::kUnicode);
	if (!dllFullPathAddr.isValid())
	{
		*replyMessage = QString("Unable to allocate memory for remote dll path");
		// qDebug().noquote() << *replyMessage;
		return false;
	}

	injectData.dllFullPathAddr = dllFullPathAddr;

	// qDebug().noquote() << QString("Remote dll path address:0x%1").arg(dllFullPathAddr, 0, 16);

	QHash<QString, ULONG64> ptrs;
	static const QSet<QString> targetFuncs = { "LoadLibraryW", "GetLastError", "GetModuleHandleW" };
	bool getProcResult = getProcAddress32(hProcess, "kernel32.dll", targetFuncs, &ptrs);
	injectData.loadLibraryWPtr = ptrs.value("LoadLibraryW");
	injectData.getLastErrorPtr = ptrs.value("GetLastError");
	injectData.getModuleHandleWPtr = ptrs.value("GetModuleHandleW");

	if (!getProcResult)
	{
		QStringList errorList;
		if (injectData.loadLibraryWPtr == NULL)
		{
			errorList.append(QString("Unable to get remote LoadLibraryW address"));
		}

		if (injectData.getLastErrorPtr == NULL)
		{
			errorList.append(QString("Unable to get remote GetLastError address"));
		}

		if (injectData.getModuleHandleWPtr == NULL)
		{
			errorList.append(QString("Unable to get remote GetModuleHandleW address"));
		}

		// qDebug().noquote() << errorList.join(" | ");

		*replyMessage = errorList.join(" | ");

		return false;
	}

	// qDebug().noquote() << QString("Remote LoadLibraryW address:0x%1").arg(injectData.loadLibraryWPtr, 0, 16);

	// qDebug().noquote() << QString("Remote GetLastError address:0x%1").arg(injectData.getLastErrorPtr, 0, 16);

	// qDebug().noquote() << QString("Remote GetModuleHandleW address:0x%1").arg(injectData.getModuleHandleWPtr, 0, 16);

	//寫入待傳遞給CallBack的數據
	Page virtualInjectdata(hProcess, sizeof(InjectData));
	if (!write(hProcess, virtualInjectdata, &injectData, sizeof(InjectData)))
	{
		*replyMessage = QString("Write inject data failed with error:%1").arg(GetLastError());
		return false;
	}

	// qDebug().noquote() << QString("Inject data address:0x%1").arg(virtualInjectdata, 0, 16);

	//寫入匯編版的CallBack函數
	Page virtualFunction(hProcess, sizeof(data));
	if (!write(hProcess, virtualFunction, data, sizeof(data)))
	{
		*replyMessage = QString("Write inject function failed with error:%1").arg(GetLastError());
		return false;
	}

	// qDebug().noquote() << QString("Inject function address:0x%1").arg(virtualFunction, 0, 16);

	// qDebug().noquote() << QString("Injecting preparation cost:%1").arg(timer.elapsed());

	timer.restart();

	HANDLE threadHijacked = nullptr;
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS | TH32CS_SNAPTHREAD, 0);
	THREADENTRY32 threadEntry = {};

	threadEntry.dwSize = sizeof(THREADENTRY32);
	Thread32First(snapshot, &threadEntry);
	while (Thread32Next(snapshot, &threadEntry))
	{
		if (threadEntry.th32OwnerProcessID == processId)
		{
			threadHijacked = NtOpenThread(processId, threadEntry.th32ThreadID);
			if (threadHijacked != nullptr)
			{
				break;
			}
		}
	}

	if (threadHijacked != nullptr)
	{
		SuspendThread(threadHijacked);
	}

	//遠程執行線程
	{
		ScopedThreadHandle hThread(hProcess, (PVOID)ULONG_PTR(virtualFunction), (PVOID)ULONG_PTR(virtualInjectdata));
		if (hThread.isValid())
		{
			timer.restart();
			for (;;)
			{
				if (!read(hProcess, virtualInjectdata, &injectData, sizeof(InjectData)))
				{
					*replyMessage = QString("Read inject data failed with error:%1").arg(GetLastError());
					if (threadHijacked != nullptr)
					{
						ResumeThread(threadHijacked);
					}
					return false;
				}

				if (injectData.remoteModule != NULL)
				{
					break;
				}

				if (timer.hasExpired(30000))
				{
					*replyMessage = QString("Inject timeout.");
					break;
				}

				QThread::msleep(100);
			}
		}
		else
		{
			*replyMessage = QString("Create remote thread failed with error:%1").arg(GetLastError());
			if (threadHijacked != nullptr)
			{
				ResumeThread(threadHijacked);
			}
			return false;
		}
	}

	if (threadHijacked != nullptr)
	{
		ResumeThread(threadHijacked);
		closeHandle(threadHijacked);
	}

	if ((injectData.lastError != ERROR_SUCCESS && injectData.lastError != ERROR_INVALID_HANDLE) || injectData.gameModule <= NULL || injectData.remoteModule <= NULL)
	{
		//取得錯誤訊息
		wchar_t* lpMsgBuf = nullptr;
		FormatMessageW(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr,
			injectData.lastError,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			reinterpret_cast<LPTSTR>(&lpMsgBuf),
			0,
			nullptr);

		if (lpMsgBuf != nullptr)
		{
			*replyMessage += QString("Inject failed with error:%1(%2)").arg(QString::fromWCharArray(lpMsgBuf).simplified()).arg(injectData.lastError);
			LocalFree(lpMsgBuf);
		}
		else
		{
			*replyMessage += QString("Inject failed with unknown error:%1").arg(injectData.lastError);
		}

		return false;
	}

	if (phDllModule != nullptr)
	{
		*phDllModule = reinterpret_cast<HMODULE>(static_cast<qint64>(injectData.remoteModule));
	}

	if (phGameModule != nullptr)
	{
		*phGameModule = injectData.gameModule;
	}

	qDebug().noquote() << QString("Inject dll successed: {\"HMODULE\":\"0x%1\", \"cost\":%2}").arg(injectData.remoteModule, 0, 16).arg(timerShow.elapsed()) << Qt::endl;

	return true;
}

void remote_memory::closeHandle(HANDLE hProcess)
{
	MINT::NtClose(hProcess);
}

bool remote_memory::createRemoteThread(PHANDLE threadHandle, HANDLE processHandle, PVOID startRoutine, PVOID argument)
{
	return NT_SUCCESS(MINT::NtCreateThreadEx(threadHandle,
		THREAD_ALL_ACCESS,
		nullptr, processHandle,
		startRoutine,
		argument,
		NULL, 0, 0, 0, nullptr));
}

HANDLE remote_memory::duplicateHandle(HANDLE hSourceProcessHandle, HANDLE hSourceHandle, HANDLE hTargetProcessHandle, DWORD dwOptions)
{
	enablePrivilege();
	HANDLE hTargetHandle = nullptr;
	BOOL ret = NT_SUCCESS(MINT::NtDuplicateObject(hSourceProcessHandle, hSourceHandle, hTargetProcessHandle, &hTargetHandle, 0, FALSE, dwOptions));
	if (ret && hTargetHandle && ((hTargetHandle) != INVALID_HANDLE_VALUE))
	{
		return hTargetHandle;
	}
	else
	{
		return nullptr;
	}
}

bool remote_memory::closeMutex(HANDLE target)
{
	NTSTATUS Status = NULL;
	HANDLE hSource = nullptr;
	SIZE_T HandleCount = 0;
	SIZE_T dwHandle = 0;
	MINT::OBJECT_NAME_INFORMATION* ObjectName = nullptr;
	MINT::OBJECT_TYPE_INFORMATION* ObjectType = nullptr;
	char BufferForObjectName[1024] = {};
	char BufferForObjectType[1024] = {};
	SIZE_T count = 0;
	SIZE_T cur_count = 0;
	HANDLE hCurrent = ::GetCurrentProcess();
	wchar_t* wcs = nullptr;
	wchar_t* buf = nullptr;

	hSource = target;
	if (nullptr == hSource || hSource == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	if (!NT_SUCCESS(MINT::NtQueryInformationProcess(hSource, MINT::ProcessHandleCount, &HandleCount, sizeof(HandleCount), nullptr)))
	{
		return false;
	}

	for (quint64 i = 1; i <= HandleCount; ++i) //enum handle
	{
		if (i > 4096)
		{
			break;
		}

		dwHandle = i * 4;

		//copy handle && check isValid
		ScopedProcessHandle hDuplicate(duplicateHandle(hSource, reinterpret_cast<HANDLE>(dwHandle), hCurrent, DUPLICATE_SAME_ACCESS));
		if (!hDuplicate.isValid())
		{
			continue;
		}

		RtlSecureZeroMemory(BufferForObjectType, sizeof(BufferForObjectType));
		RtlSecureZeroMemory(BufferForObjectType, sizeof(BufferForObjectType));

		//get handle type
		Status = MINT::NtQueryObject(hDuplicate,
			MINT::_OBJECT_INFORMATION_CLASS::ObjectTypeInformation,
			BufferForObjectType,
			sizeof(BufferForObjectType),
			NULL);

		ObjectType = reinterpret_cast<MINT::OBJECT_TYPE_INFORMATION*>(BufferForObjectType);
		if (Status == STATUS_INFO_LENGTH_MISMATCH || !NT_SUCCESS(Status))
		{
			continue;
		}

		wcs = reinterpret_cast<wchar_t*>(ObjectType->TypeName.Buffer);
		_wcslwr_s(wcs, ObjectType->TypeName.Length); // to lower case

		if ((wcsnlen_s(wcs, 1024) > 0) && (wcsncmp(wcs, TEXT("mutant"), 1024) != 0))  // check handle type
		{
			continue;
		}

		//get handle name
		Status = MINT::NtQueryObject(hDuplicate,
			MINT::_OBJECT_INFORMATION_CLASS::ObjectNameInformation,
			BufferForObjectName,
			sizeof(BufferForObjectName),
			NULL);

		ObjectName = (MINT::POBJECT_NAME_INFORMATION)BufferForObjectName;
		if (STATUS_INFO_LENGTH_MISMATCH == Status || !NT_SUCCESS(Status))
		{
			continue;
		}

		buf = reinterpret_cast<wchar_t*>(ObjectName->Name.Buffer);
		if (nullptr == buf)
		{
			continue;
		}

		if (wcsnlen_s(buf, MAX_PATH) == 0)
		{
			continue;
		}

		++count;

		QString q = QString::fromWCharArray(buf);
		QByteArray ba = q.toLocal8Bit();
		//230 177 162 230 149 181 227 133 159
		//230 177 162 230 149 181 227 137 159
		//230 177 162 230 149 181 227 141 159
		if (!ba.contains("\xE6\xB1\xA2\xE6\x95\xB5\xE3") && !q.contains("汢") && !q.contains("BaseNamedObjects\\XZYDRaEvyPQC"))// || wcsnlen_s(buf, 260) > 35
		{
			continue;
		}

		//Duplicate a mutex handle with the same access rights and check whether it is valid
		ScopedProcessHandle hDuplicateClose(duplicateHandle(hSource, (HANDLE)dwHandle, hCurrent, DUPLICATE_CLOSE_SOURCE));
		if (!hDuplicateClose.isValid())
		{
			continue;
		}

		//close local handle which is duplicated from remote handle
		closeHandle(hCurrent);

		++cur_count;

		if (cur_count >= 2)
		{
			return true;
		}
	}

	if (count <= 10)
	{
		return true;
	}

	return false;
}

#pragma region Page

remote_memory::Page::Page(HANDLE hProcess, qint64 size)
	: hProcess_(hProcess)
	, lpAddress_(alloc(hProcess, size))
{
}

remote_memory::Page::Page(HANDLE hProcess, const QString& str, Encode use_unicode)
	: hProcess_(hProcess)
{
	if (kUnicode == use_unicode)
	{
		lpAddress_ = alloc(hProcess, str.size() * 2);
		std::wstring wstr = str.toStdWString();
		write(hProcess, lpAddress_, wstr.data(), wstr.size() * 2);
	}
	else
	{
		lpAddress_ = alloc(hProcess, str.size());
		write(hProcess, lpAddress_, str);
	}
}

remote_memory::Page::operator quint64() const
{
	return lpAddress_;
}

remote_memory::Page& remote_memory::Page::operator=(qint64 other)
{
	lpAddress_ = other;
	return *this;
}

remote_memory::Page* remote_memory::Page::operator&()
{
	return this;
}

remote_memory::Page::~Page()
{
	clear();
}

void remote_memory::Page::clear()
{
	if (NULL == lpAddress_)
	{
		return;
	}

	free(hProcess_, (lpAddress_));
	lpAddress_ = NULL;
}

bool remote_memory::Page::isNull() const
{
	return NULL == lpAddress_;
}

bool remote_memory::Page::isValid()const
{
	return lpAddress_ != NULL;
}

bool remote_memory::Page::isData(BYTE* data, qint64 size) const
{
	QByteArray _data(size, 0);
	read(hProcess_, lpAddress_, _data.data(), size);

	return std::memcmp(data, _data.data(), size) == 0;
}

#pragma endregion

#pragma region ScopedHandle
ScopedProcessHandle::ScopedProcessHandle(DWORD processId)
	: hProcess_(remote_memory::openProcess(processId))
{

}

ScopedProcessHandle::ScopedProcessHandle(HANDLE hProcess)
	: hProcess_(hProcess)
{

}

ScopedProcessHandle::~ScopedProcessHandle()
{
	if (hProcess_ != nullptr)
	{
		remote_memory::closeHandle(hProcess_);
	}
}

bool ScopedProcessHandle::isValid() const
{
	return hProcess_ != nullptr;
}

ScopedProcessHandle::operator HANDLE() const
{
	return hProcess_;
}

ScopedThreadHandle::ScopedThreadHandle(HANDLE processHandle, PVOID startRoutine, PVOID argument)
{
	bool result = remote_memory::createRemoteThread(&hThread_, processHandle, startRoutine, argument);
	if (!result)
	{
		hThread_ = nullptr;
	}
}

ScopedThreadHandle::~ScopedThreadHandle()
{
	if (hThread_ != nullptr)
	{
		remote_memory::closeHandle(hThread_);
	}
}

bool ScopedThreadHandle::isValid() const
{
	return hThread_ != nullptr;
}

void ScopedThreadHandle::wait() const
{
	LARGE_INTEGER pTimeout = {};
	pTimeout.QuadPart = -1ll * 10000000ll;
	MINT::NtWaitForSingleObject(hThread_, FALSE, &pTimeout);
}
#pragma endregion