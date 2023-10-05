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
#include "util.h"

#pragma region mem

bool mem::read(HANDLE hProcess, quint64 desiredAccess, quint64 size, PVOID buffer)
{
	if (!size)
		return false;

	if (!buffer)
		return false;

	if (hProcess == nullptr)
		return false;

	if (!desiredAccess)
		return false;

	ScopedHandle::enablePrivilege(::GetCurrentProcess());

	//ULONG oldProtect = NULL;
	//VirtualProtectEx(m_pi.hProcess, buffer, size, PAGE_EXECUTE_READWRITE, &oldProtect);
	BOOL ret = NT_SUCCESS(MINT::NtReadVirtualMemory(hProcess, reinterpret_cast<PVOID>(desiredAccess), buffer, size, NULL));
	//VirtualProtectEx(m_pi.hProcess, buffer, size, oldProtect, &oldProtect);

	return ret == TRUE;
}

template<typename T, typename>
T mem::read(HANDLE hProcess, quint64 desiredAccess)
{
	if (hProcess == nullptr)
		return T();

	T buffer{};
	SIZE_T size = sizeof(T);

	if (!size)
		return T();

	BOOL ret = mem::read(hProcess, desiredAccess, size, &buffer);

	return ret ? buffer : T();
}

float mem::readFloat(HANDLE hProcess, quint64 desiredAccess)
{
	if (hProcess == nullptr)
		return 0.0f;

	if (!desiredAccess)
		return 0.0f;

	float buffer = 0;
	SIZE_T size = sizeof(float);
	BOOL ret = read(hProcess, desiredAccess, size, &buffer);

	return (ret) ? (buffer) : 0.0f;
}

qreal mem::readDouble(HANDLE hProcess, quint64 desiredAccess)
{
	if (hProcess == nullptr)
		return 0.0;

	if (!desiredAccess)
		return 0.0;

	qreal buffer = 0;
	SIZE_T size = sizeof(qreal);
	BOOL ret = read(hProcess, desiredAccess, size, &buffer);

	return (ret == TRUE) ? (buffer) : 0.0;
}

QString mem::readString(HANDLE hProcess, quint64 desiredAccess, quint64 size, bool enableTrim, bool keepOriginal)
{
	if (hProcess == nullptr)
		return "\0";

	if (!desiredAccess)
		return "\0";

	QScopedArrayPointer <char> p(q_check_ptr(new char[size + 1]()));
	memset(p.get(), 0, size + 1);

	BOOL ret = read(hProcess, desiredAccess, size, p.get());
	if (!keepOriginal)
	{
		std::string s = p.get();
		QString retstring = (ret == TRUE) ? (util::toUnicode(s.c_str(), enableTrim)) : "";
		return retstring;
	}

	QString retstring = (ret == TRUE) ? QString(p.get()) : "";
	return retstring;
}

bool mem::write(HANDLE hProcess, quint64 baseAddress, PVOID buffer, quint64 dwSize)
{
	if (hProcess == nullptr)
		return false;

	ULONG oldProtect = NULL;

	ScopedHandle::enablePrivilege(::GetCurrentProcess());

	VirtualProtectEx(hProcess, (LPVOID)baseAddress, dwSize, PAGE_EXECUTE_READWRITE, &oldProtect);
	BOOL ret = WriteProcessMemory(hProcess, reinterpret_cast<PVOID>(baseAddress), buffer, dwSize, NULL);
	VirtualProtectEx(hProcess, (LPVOID)baseAddress, dwSize, oldProtect, &oldProtect);

	return ret == TRUE;
}

template<typename T, typename>
bool mem::write(HANDLE hProcess, quint64 baseAddress, T data)
{
	if (hProcess == nullptr)
		return false;

	if (!baseAddress)
		return false;

	PVOID pBuffer = &data;
	BOOL ret = write(hProcess, baseAddress, pBuffer, sizeof(T));

	return ret == TRUE;
}

bool mem::writeString(HANDLE hProcess, quint64 baseAddress, const QString& str)
{
	if (hProcess == nullptr)
		return false;

	QTextCodec* codec = QTextCodec::codecForName(util::DEFAULT_GAME_CODEPAGE);//QTextCodec::codecForMib(2025);//

	QByteArray ba = codec->fromUnicode(str);
	ba.append('\0');

	char* pBuffer = ba.data();
	quint64 len = ba.size();

	QScopedArrayPointer <char> p(q_check_ptr(new char[len + 1]()));
	memset(p.get(), 0, len + 1);

	_snprintf_s(p.get(), len + 1, _TRUNCATE, "%s\0", pBuffer);

	BOOL ret = write(hProcess, baseAddress, p.get(), len + 1);
	p.reset(nullptr);

	return ret == TRUE;
}

quint64 mem::virtualAlloc(HANDLE hProcess, quint64 size)
{
	if (hProcess == nullptr)
		return 0;

	quint64 ptr = NULL;
	SIZE_T sizet = static_cast<SIZE_T>(size);

	BOOL ret = NT_SUCCESS(MINT::NtAllocateVirtualMemory(hProcess, reinterpret_cast<PVOID*>(&ptr), NULL, &sizet, MEM_COMMIT, PAGE_EXECUTE_READWRITE));
	if (ret == TRUE)
		return static_cast<int>(ptr);

	return 0;
}

quint64 mem::virtualAllocA(HANDLE hProcess, const QString& str)
{
	quint64 ret = NULL;
	do
	{
		if (hProcess == nullptr)
			break;

		ret = virtualAlloc(hProcess, static_cast<quint64>(str.toLocal8Bit().size()) * 2 * sizeof(char) + 1);
		if (ret == FALSE)
			break;
		if (!writeString(hProcess, ret, str))
		{
			virtualFree(hProcess, ret);
			ret = NULL;
			break;
		}
	} while (false);
	return ret;
}

quint64 mem::virtualAllocW(HANDLE hProcess, const QString& str)
{
	quint64 ret = NULL;
	do
	{
		if (hProcess == nullptr)
			break;

		std::wstring wstr(str.toStdWString());
		wstr += L'\0';

		ret = virtualAlloc(hProcess, static_cast<quint64>(wstr.length()) * sizeof(wchar_t) + 1);
		if (!ret)
			break;

		if (!write(hProcess, ret, const_cast<wchar_t*>(wstr.c_str()), static_cast<quint64>(wstr.length()) * sizeof(wchar_t) + 1))
		{
			virtualFree(hProcess, ret);
			ret = NULL;
			break;
		}
	} while (false);
	return ret;
}

bool mem::virtualFree(HANDLE hProcess, quint64 baseAddress)
{
	if (hProcess == nullptr)
		return false;

	if (baseAddress == NULL)
		return false;

	SIZE_T size = 0;
	BOOL ret = NT_SUCCESS(MINT::NtFreeVirtualMemory(hProcess, reinterpret_cast<PVOID*>(&baseAddress), &size, MEM_RELEASE));

	return ret == TRUE;
}

DWORD mem::getRemoteModuleHandle(DWORD dwProcessId, const QString& moduleName)
{
	//获取进程快照中包含在th32ProcessID中指定的进程的所有的模块。
	ScopedHandle hSnapshot(ScopedHandle::CREATE_TOOLHELP32_SNAPSHOT, TH32CS_SNAPMODULE, dwProcessId);
	if (!hSnapshot.isValid())
		return NULL;

	MODULEENTRY32W moduleEntry = {};
	memset(&moduleEntry, 0, sizeof(MODULEENTRY32W));
	moduleEntry.dwSize = sizeof(MODULEENTRY32W);
	if (!Module32FirstW(hSnapshot, &moduleEntry))
		return NULL;
	else
	{
		const QString str(util::toQString(moduleEntry.szModule));
		if (str == moduleName)
			return reinterpret_cast<DWORD>(moduleEntry.hModule);
	}

	do
	{
		const QString str(util::toQString(moduleEntry.szModule));
		if (str == moduleName)
			return reinterpret_cast<DWORD>(moduleEntry.hModule);
	} while (Module32NextW(hSnapshot, &moduleEntry));

	return NULL;
}

void mem::freeUnuseMemory(HANDLE hProcess)
{
	SetProcessWorkingSetSizeEx(hProcess, static_cast<SIZE_T>(-1), static_cast<SIZE_T>(-1), 0);
	K32EmptyWorkingSet(hProcess);
}

DWORD mem::getFunAddr(const DWORD* DllBase, const char* FunName)
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

HMODULE mem::getRemoteModuleHandleByProcessHandleW(HANDLE hProcess, const QString& szModuleName)
{
	HMODULE hMods[1024] = {};
	DWORD cbNeeded = 0, i = 0;
	wchar_t szModName[MAX_PATH] = {};
	memset(szModName, 0, sizeof(szModName));

	if (K32EnumProcessModulesEx(hProcess, hMods, sizeof(hMods), &cbNeeded, 3)) //http://msdn.microsoft.com/en-us/library/ms682633(v=vs.85).aspx
	{
		for (i = 0; i <= cbNeeded / sizeof(HMODULE); i++)
		{
			if (K32GetModuleFileNameExW(hProcess, hMods[i], szModName, _countof(szModName)) == NULL)
				continue;

			QString qfileName = util::toQString(szModName);
			qfileName.replace("/", "\\");
			QFileInfo file(qfileName);
			if (file.fileName().toLower() != szModuleName.toLower())
				continue;

			return hMods[i];
		}
	}

	return NULL;
}

long mem::getProcessExportTable32(HANDLE hProcess, const QString& ModuleName, IAT_EAT_INFO tbinfo[], int tb_info_max)
{
	ULONG64 muBase = 0, count = 0;
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)new BYTE[sizeof(IMAGE_DOS_HEADER)];
	PIMAGE_NT_HEADERS32 pNtHeaders = (PIMAGE_NT_HEADERS32)new BYTE[sizeof(IMAGE_NT_HEADERS32)];
	PIMAGE_EXPORT_DIRECTORY pExport = (PIMAGE_EXPORT_DIRECTORY)new BYTE[sizeof(IMAGE_EXPORT_DIRECTORY)];

	char strName[130] = {};
	memset(strName, 0, sizeof(strName));

	//拿到目標模塊的BASE
	muBase = (ULONG)getRemoteModuleHandleByProcessHandleW(hProcess, ModuleName);
	if (!muBase)
	{
		return 0;
	}

	//獲取IMAGE_DOS_HEADER
	mem::read(hProcess, muBase, sizeof(IMAGE_DOS_HEADER), pDosHeader);
	//獲取IMAGE_NT_HEADERS
	mem::read(hProcess, (muBase + pDosHeader->e_lfanew), sizeof(IMAGE_NT_HEADERS32), pNtHeaders);
	if (pNtHeaders->OptionalHeader.DataDirectory[0].VirtualAddress == 0)
	{
		return 0;
	}

	mem::read(hProcess, (muBase + pNtHeaders->OptionalHeader.DataDirectory[0].VirtualAddress), sizeof(IMAGE_EXPORT_DIRECTORY), pExport);
	mem::read(hProcess, (muBase + pExport->Name), sizeof(strName), strName);
	ULONG64 i = 0;

	if (pExport->NumberOfNames < 0 || pExport->NumberOfNames > 8192)
	{
		return 0;
	}

	for (i = 0; i < pExport->NumberOfNames; i++)
	{
		char bFuncName[100] = {};
		ULONG64 ulPointer;
		USHORT usFuncId;
		ULONG64 ulFuncAddr;
		ulPointer = static_cast<ULONG64>(mem::read<int>(hProcess, (muBase + pExport->AddressOfNames + i * static_cast<ULONG64>(sizeof(DWORD)))));
		memset(bFuncName, 0, sizeof(bFuncName));
		mem::read(hProcess, (muBase + ulPointer), sizeof(bFuncName), bFuncName);
		usFuncId = mem::read<short>(hProcess, (muBase + pExport->AddressOfNameOrdinals + i * sizeof(short)));
		ulPointer = static_cast<ULONG64>(mem::read<int>(hProcess, (muBase + pExport->AddressOfFunctions + static_cast<ULONG64>(sizeof(DWORD)) * usFuncId)));
		ulFuncAddr = muBase + ulPointer;
		_snprintf_s(tbinfo[count].ModuleName, sizeof(tbinfo[count].ModuleName), _TRUNCATE, "%s", ModuleName.toUtf8().constData());
		_snprintf_s(tbinfo[count].FuncName, sizeof(tbinfo[count].FuncName), _TRUNCATE, "%s", bFuncName);
		tbinfo[count].Address = ulFuncAddr;
		tbinfo[count].RecordAddr = (ULONG64)(muBase + pExport->AddressOfFunctions + static_cast<ULONG64>(sizeof(DWORD)) * usFuncId);
		tbinfo[count].ModBase = muBase;
		count++;
		if (count > (ULONG)tb_info_max)
			break;
	}

	delete[]pDosHeader;
	delete[]pExport;
	delete[]pNtHeaders;
	return count;
}

//獲得32位進程中某個DLL導出函數的地址
ULONG64 mem::getProcAddressIn32BitProcess(HANDLE hProcess, const QString& ModuleName, const QString& FuncName)
{
	ULONG64 RetAddr = 0;
	PIAT_EAT_INFO pInfo = (PIAT_EAT_INFO)malloc(4096 * sizeof(IAT_EAT_INFO));
	if (!pInfo)
		return NULL;

	long count = getProcessExportTable32(hProcess, ModuleName, pInfo, 2048);
	if (!count)
		return NULL;

	for (long i = 0; i < count; i++)
	{
		if (QString(pInfo[i].FuncName).toLower() == FuncName.toLower())
		{
			RetAddr = pInfo[i].Address;
			break;
		}
	}

	free(pInfo);
	return RetAddr;
}

bool mem::injectByWin7(qint64 index, DWORD dwProcessId, HANDLE hProcess, QString dllPath, HMODULE* phDllModule, quint64* phGameModule)
{
	HMODULE hModule = nullptr;
	QElapsedTimer timer; timer.start();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(index);

	HMODULE kernel32Module = GetModuleHandleW(L"kernel32.dll");
	if (nullptr == kernel32Module)
	{
		emit signalDispatcher.messageBoxShow(QObject::tr("GetModuleHandleW failed"), QMessageBox::Icon::Critical);
		return false;
	}

	FARPROC loadLibraryProc = GetProcAddress(kernel32Module, "LoadLibraryW");
	if (nullptr == loadLibraryProc)
	{
		emit signalDispatcher.messageBoxShow(QObject::tr("GetProcAddress failed"), QMessageBox::Icon::Critical);
		return false;
	}

	DWORD hGameModule = mem::getRemoteModuleHandle(dwProcessId, "sa_8001.exe");
	if (phGameModule != nullptr)
		*phGameModule = static_cast<quint64>(hGameModule);

	for (qint64 i = 0; i < 2; ++i)
	{
		timer.restart();
		util::VirtualMemory dllFullPathAddr(hProcess, dllPath, util::VirtualMemory::kUnicode, true);
		if (!dllFullPathAddr.isValid())
		{
			emit signalDispatcher.messageBoxShow(QObject::tr("VirtualAllocEx failed"), QMessageBox::Icon::Critical);
			return false;
		}

		//遠程執行線程
		ScopedHandle hThreadHandle(
			ScopedHandle::CREATE_REMOTE_THREAD,
			hProcess,
			reinterpret_cast<PVOID>(loadLibraryProc),
			reinterpret_cast<LPVOID>(static_cast<quint64>(dllFullPathAddr)));

		if (!hThreadHandle.isValid())
		{
			emit signalDispatcher.messageBoxShow(QObject::tr("Create remote thread failed"), QMessageBox::Icon::Critical);
			return false;
		}

		timer.restart();
		for (;;)
		{
			hModule = reinterpret_cast<HMODULE>(mem::getRemoteModuleHandle(dwProcessId, "sadll.dll"));
			if (hModule != nullptr)
				break;

			if (timer.hasExpired(3000))
				break;

			QThread::msleep(100);
		}

		if (phDllModule != nullptr)
			*phDllModule = hModule;

		if (hModule != nullptr)
			break;
	}

	qDebug() << "inject OK" << "0x" + util::toQString(reinterpret_cast<qint64>(hModule), 16) << "time:" << timer.elapsed() << "ms";
	return true;
}

bool mem::injectBy64(qint64 index, HANDLE hProcess, QString dllPath, HMODULE* phDllModule, quint64* phGameModule)
{
	QElapsedTimer timer; timer.start();
	static unsigned char data[128] = {
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
		DWORD loadLibraryWPtr = 0;
		DWORD getLastErrorPtr = 0;
		DWORD getModuleHandleWPtr = 0;
		DWORD dllFullPathAddr = 0;
		DWORD remoteModule = 0;
		DWORD lastError = 0;
		DWORD gameModule = 0;
	}d;

	util::VirtualMemory dllFullPathAddr(hProcess, dllPath, util::VirtualMemory::kUnicode, true);
	d.dllFullPathAddr = dllFullPathAddr;

	d.loadLibraryWPtr = getProcAddressIn32BitProcess(hProcess, "kernel32.dll", "LoadLibraryW");
	d.getLastErrorPtr = getProcAddressIn32BitProcess(hProcess, "kernel32.dll", "GetLastError");
	d.getModuleHandleWPtr = getProcAddressIn32BitProcess(hProcess, "kernel32.dll", "GetModuleHandleW");

	//寫入待傳遞給CallBack的數據
	util::VirtualMemory injectdata(hProcess, sizeof(InjectData), true);
	mem::write(hProcess, injectdata, &d, sizeof(InjectData));

	//寫入匯編版的CallBack函數
	util::VirtualMemory remoteFunc(hProcess, sizeof(data), true);
	mem::write(hProcess, remoteFunc, data, sizeof(data));

	qDebug() << "time:" << timer.elapsed() << "ms";
	timer.restart();

	//遠程執行線程
	{
		ScopedHandle hThreadHandle(
			ScopedHandle::CREATE_REMOTE_THREAD,
			hProcess,
			reinterpret_cast<PVOID>(static_cast<quint64>(remoteFunc)),
			reinterpret_cast<LPVOID>(static_cast<quint64>(injectdata)));

		if (!hThreadHandle.isValid())
		{
			SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(index);
			emit signalDispatcher.messageBoxShow(QObject::tr("Create remote thread failed"), QMessageBox::Icon::Critical);
			return false;
		}
	}

	mem::read(hProcess, injectdata, sizeof(InjectData), &d);
	if (d.lastError != 0)
	{
		//取得錯誤訊息
		wchar_t* p = nullptr;
		FormatMessageW(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr,
			d.lastError,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			p,
			0,
			nullptr);

		if (p != nullptr)
		{
			SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(index);
			emit signalDispatcher.messageBoxShow(QObject::tr("Inject fail, error code from client: %1, %2").arg(d.lastError).arg(util::toQString(p)), QMessageBox::Icon::Critical);
			LocalFree(p);
		}
		return false;
	}

	if (phDllModule != nullptr)
		*phDllModule = reinterpret_cast<HMODULE>(d.remoteModule);

	if (phGameModule != nullptr)
		*phGameModule = d.gameModule;

	qDebug() << "inject OK" << "0x" + util::toQString(d.remoteModule, 16) << "time:" << timer.elapsed() << "ms";
	return d.gameModule > 0 && d.remoteModule > 0;
}

bool mem::inject(qint64 index, HANDLE hProcess, QString dllPath, HMODULE* phDllModule, quint64* phGameModule)
{
	struct InjectData
	{
		DWORD loadLibraryWPtr = 0;
		DWORD getLastErrorPtr = 0;
		DWORD getModuleHandleWPtr = 0;
		DWORD dllFullPathAddr = 0;
		DWORD remoteModule = 0;
		DWORD lastError = 0;
		DWORD gameModule = 0;

	};

	InjectData d;
	HMODULE kernel32Module = GetModuleHandleW(L"kernel32.dll");
	if (nullptr == kernel32Module)
	{
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(index);
		emit signalDispatcher.messageBoxShow(QObject::tr("GetModuleHandleW failed"), QMessageBox::Icon::Critical);
		return false;
	}

	d.loadLibraryWPtr = reinterpret_cast<DWORD>(GetProcAddress(kernel32Module, "LoadLibraryW"));
	d.getLastErrorPtr = reinterpret_cast<DWORD>(GetProcAddress(kernel32Module, "GetLastError"));
	d.getModuleHandleWPtr = reinterpret_cast<DWORD>(GetProcAddress(kernel32Module, "GetModuleHandleW"));
	util::VirtualMemory dllfullpathaddr(hProcess, dllPath, util::VirtualMemory::kUnicode, true);
	d.dllFullPathAddr = dllfullpathaddr;

	util::VirtualMemory injectdata(hProcess, sizeof(InjectData), true);
	mem::write(hProcess, injectdata, &d, sizeof(InjectData));

	/*
	08130019 - 8B 44 24 04           - mov eax,[esp+04]
	0813001D - 8B D8                 - mov ebx,eax
	0813001F - 8B 48 0C              - mov ecx,[eax+0C]
	08130022 - 51                    - push ecx
	08130023 - FF 13                 - call dword ptr [ebx]
	08130025 - 89 43 10              - mov [ebx+10],eax
	08130028 - 8B 43 04              - mov eax,[ebx+04]
	0813002B - FF D0                 - call eax
	0813002D - 89 43 14              - mov [ebx+14],eax
	08130030 - 8B 43 08              - mov eax,[ebx+08]
	08130033 - 6A 00                 - push 00 { 0 }
	08130035 - FF D0                 - call eax
	08130037 - 89 43 18              - mov [ebx+18],eax
	0813003A - C3                    - ret
	*/

	util::VirtualMemory remoteFunc(hProcess, 100, true);
	mem::write(hProcess, remoteFunc, const_cast<char*>("\x8B\x44\x24\x04\x8B\xD8\x8B\x48\x0C\x51\xFF\x13\x89\x43\x10\x8B\x43\x04\xFF\xD0\x89\x43\x14\x8B\x43\x08\x6A\x00\xFF\xD0\x89\x43\x18\xC3"), 36);

	{
		ScopedHandle hThreadHandle(
			ScopedHandle::CREATE_REMOTE_THREAD,
			hProcess,
			reinterpret_cast<PVOID>(static_cast<quint64>(remoteFunc)),
			reinterpret_cast<LPVOID>(static_cast<quint64>(injectdata)));
	}

	mem::read(hProcess, injectdata, sizeof(InjectData), &d);
	if (d.lastError != 0)
	{
		wchar_t* p = nullptr;
		FormatMessageW( //取得錯誤訊息
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr,
			d.lastError,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			p,
			0,
			nullptr);

		if (p != nullptr)
		{
			SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(index);
			emit signalDispatcher.messageBoxShow(QObject::tr("Inject fail, error code: %1, %2").arg(d.lastError).arg(util::toQString(p)), QMessageBox::Icon::Critical);
			LocalFree(p);
		}
		return false;
	}

	if (phDllModule != nullptr)
		*phDllModule = reinterpret_cast<HMODULE>(d.remoteModule);

	if (phGameModule != nullptr)
		*phGameModule = d.gameModule;

	qDebug() << "inject OK" << "0x" + util::toQString(d.remoteModule, 16);
	return d.gameModule > 0 && d.remoteModule > 0;
}
#pragma endregion

#pragma region Config
QReadWriteLock g_fileLock;

util::Config::Config(const QString& fileName)
	: fileName_(fileName)
	, file_(fileName)
{
	g_fileLock.lockForWrite();//附加文件寫鎖
	isVaild = open();
	//g_fileLock.unlock();
}

util::Config::~Config()
{
	//g_fileLock.lockForWrite();//附加文件寫鎖
	sync();//同步數據
	g_fileLock.unlock();//解鎖
}

bool util::Config::open()
{
	bool enableReopen = false;
	if (!file_.exists())
	{
		//文件不存在，以全新文件複寫方式打開
		if (!file_.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text))
		{
			return false;
		}
	}
	else
	{
		//文件存在，以普通讀寫方式打開
		if (!file_.open(QIODevice::ReadWrite | QIODevice::Text))
		{
			return false;
		}
		enableReopen = true;
	}

	QTextStream in(&file_);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	in.setCodec(util::DEFAULT_CODEPAGE);
#else
	in.setEncoding(QStringConverter::Utf8);
#endif
	in.setGenerateByteOrderMark(true);
	QString text = in.readAll();
	QByteArray allData = text.toUtf8();

	if (allData.simplified().isEmpty())
	{
		return true;
	}

	//解析json
	QJsonParseError jsonError;
	document_ = QJsonDocument::fromJson(allData, &jsonError);
	if (jsonError.error != QJsonParseError::NoError)
	{
		// 解析错误
		return false;
	}

	//把json轉換成qvariantmap
	QJsonObject root = document_.object();
	cache_ = root.toVariantMap();

	return true;
}

void util::Config::sync()
{
	if (isVaild && hasChanged_)
	{
		QJsonObject root = QJsonObject::fromVariantMap(cache_);
		document_.setObject(root);
		QByteArray data = document_.toJson(QJsonDocument::Indented);

		if (file_.isOpen())
		{
			file_.close();
		}

		//總是以全新寫入的方式
		if (!file_.open(QIODevice::ReadWrite | QIODevice::Truncate))
		{
			return;
		}

		file_.write(data);
		file_.flush();
	}
	file_.close();
}

void util::Config::removeSec(const QString sec)
{
	if (cache_.contains(sec))
	{
		cache_.remove(sec);
		if (!hasChanged_)
			hasChanged_ = true;
	}
}

void util::Config::write(const QString& sec, const QString& key, const QString& sub, const QVariant& value)
{
	if (!cache_.contains(sec))
	{
		QJsonObject subjson;
		subjson.insert(sub, QJsonValue::fromVariant(value));
		QJsonObject json;
		json.insert(key, QJsonValue(subjson));
		cache_.insert(sec, QJsonValue(json));
	}
	else
	{
		QJsonObject json = cache_.value(sec).toJsonObject();

		if (!json.contains(key))
		{
			QJsonObject subjson;
			subjson.insert(sub, QJsonValue::fromVariant(value));
			json.insert(key, QJsonValue(subjson));
			cache_.insert(sec, QJsonValue(json));
		}
		else
		{
			QJsonObject subjson = json.value(key).toObject();
			subjson.insert(sub, QJsonValue::fromVariant(value));
			json.insert(key, QJsonValue(subjson));
			cache_.insert(sec, QJsonValue(json));
		}
	}
	if (!hasChanged_)
		hasChanged_ = true;
}

void util::Config::write(const QString& sec, const QString& key, const QVariant& value)
{
	if (!cache_.contains(sec))
	{
		QJsonObject json;
		json.insert(key, QJsonValue::fromVariant(value));
		cache_.insert(sec, QJsonValue(json));
	}
	else
	{
		QJsonObject json = cache_.value(sec).toJsonObject();
		json.insert(key, QJsonValue::fromVariant(value));
		cache_.insert(sec, QJsonValue(json));
	}
	if (!hasChanged_)
		hasChanged_ = true;
}

void util::Config::write(const QString& key, const QVariant& value)
{
	cache_.insert(key, value);
	if (!hasChanged_)
		hasChanged_ = true;
}

void util::Config::WriteHash(const QString& sec, const QString& key, QMap<QString, QPair<bool, QString>>& hash)
{
	//將將多餘的都刪除
	QJsonObject json = cache_.value(sec).toJsonObject();
	if (json.contains(key))
	{
		QJsonObject subjson = json.value(key).toObject();
		for (auto it = subjson.begin(); it != subjson.end();)
		{
			if (!hash.contains(it.key()))
			{
				it = subjson.erase(it);
			}
			else
			{
				++it;
			}
		}

		json.insert(key, QJsonValue(subjson));
		cache_.insert(sec, QJsonValue(json));
	}

	// hash key is json subkey
	if (!cache_.contains(sec))
	{
		QJsonObject j;
		QJsonObject subjson;
		for (auto it = hash.cbegin(); it != hash.cend(); ++it)
		{
			QJsonObject subsubjson;
			subsubjson.insert("enable", it.value().first);
			subsubjson.insert("value", it.value().second);
			subjson.insert(it.key(), subsubjson);
		}

		j.insert(key, subjson);
		cache_.insert(sec, QJsonValue(j));
	}
	else
	{
		QJsonObject j = cache_.value(sec).toJsonObject();
		if (!j.contains(key))
		{
			QJsonObject subjson;
			for (auto it = hash.cbegin(); it != hash.cend(); ++it)
			{
				QJsonObject subsubjson;
				subsubjson.insert("enable", it.value().first);
				subsubjson.insert("value", it.value().second);
				subjson.insert(it.key(), subsubjson);
			}

			j.insert(key, subjson);
			cache_.insert(sec, QJsonValue(j));
		}
		else
		{
			QJsonObject subjson = json.value(key).toObject();
			for (auto it = hash.cbegin(); it != hash.cend(); ++it)
			{
				QJsonObject subsubjson;
				subsubjson.insert("enable", it.value().first);
				subsubjson.insert("value", it.value().second);
				subjson.insert(it.key(), subsubjson);
			}

			json.insert(key, subjson);
			cache_.insert(sec, QJsonValue(json));
		}
	}
}

QMap<QString, QPair<bool, QString>> util::Config::EnumString(const QString& sec, const QString& key) const
{
	QMap<QString, QPair<bool, QString>> ret;
	if (!cache_.contains(sec))
	{
		return ret;
	}

	QJsonObject json = cache_.value(sec).toJsonObject();
	if (!json.contains(key))
	{
		return ret;
	}

	QJsonObject subjson = json.value(key).toObject();
	for (auto it = subjson.begin(); it != subjson.end(); ++it)
	{
		QPair<bool, QString> pair;
		//key == enable
		QJsonObject  child = it.value().toObject();
		if (child.contains("enable") && child.contains("value") && child["enable"].isBool() && child["value"].isString())
		{
			pair.first = child["enable"].toBool();
			pair.second = child["value"].toString();
			ret.insert(it.key(), pair);
		}
	}
	return ret;
}

void util::Config::writeMapData(const QString&, const util::MapData& data)
{
	QString key = util::toQString(data.floor);
	QJsonArray jarray;
	if (cache_.contains(key))
	{
		jarray = cache_[key].toJsonArray();
	}

	QString text = QString("%1|%2,%3").arg(data.name).arg(data.x).arg(data.y);

	if (!jarray.contains(text))
	{
		jarray.append(text);
	}

	cache_.insert(key, jarray);

	if (!hasChanged_)
		hasChanged_ = true;
}

// 读取数据
QList<util::MapData> util::Config::readMapData(const QString& key) const
{
	QList<MapData> result;

	if (!cache_.contains(key))
		return result;

	QJsonArray jarray = cache_.value(key).toJsonArray();
	if (jarray.isEmpty())
		return result;

	for (const QJsonValue& value : jarray)
	{
		QStringList list = value.toString().split(util::rexOR);
		if (list.size() != 2)
			continue;

		MapData data;
		data.name = list[0].simplified();
		if (data.name.isEmpty())
			continue;

		QString pos = list[1].simplified();
		if (pos.isEmpty())
			continue;

		if (pos.count(",") != 1)
			continue;

		QStringList posList = pos.split(",");
		if (posList.size() != 2)
			continue;

		data.x = posList[0].simplified().toLongLong();
		data.y = posList[1].simplified().toLongLong();
		if (data.x == 0 && data.y == 0)
			continue;

		result.append(data);
	}

	return result;
}
#pragma endregion

void util::FormSettingManager::loadSettings()
{
	util::Crypt crypt;
	const QString fileName(qgetenv("JSON_PATH"));
	if (fileName.isEmpty())
		return;

	if (!QFile::exists(fileName))
		return;

	Config config(fileName);

	QString ObjectName;
	if (mainwindow_ != nullptr)
	{
		ObjectName = mainwindow_->objectName();
	}
	else
	{
		ObjectName = widget_->objectName();
	}


	QString strSize = config.read<QString>(ObjectName, "Size");
	QSize size;
	if (!strSize.isEmpty())
	{
		QStringList list = strSize.split(util::rexComma, Qt::SkipEmptyParts);
		if (list.size() == 2)
		{
			size.setWidth(list.value(0).toLongLong());
			size.setHeight(list.value(1).toLongLong());
		}
	}

	QByteArray geometry;
	QString qstrGeometry = config.read<QString>("Form", ObjectName, "Geometry");
	if (!qstrGeometry.isEmpty())
		geometry = crypt.decryptToByteArray(qstrGeometry);//DECODE

	if (mainwindow_ != nullptr)
	{
		QByteArray state;
		QString qstrState = config.read<QString>("Form", ObjectName, "State");
		if (!qstrState.isEmpty())
			state = crypt.decryptToByteArray(qstrState);//DECODE
		mainwindow_->resize(size);
		if (!geometry.isEmpty())
			mainwindow_->restoreGeometry(geometry);
		if (!state.isEmpty())
			mainwindow_->restoreState(state, 3);
	}
	else
	{
		widget_->resize(size);
		if (!geometry.isEmpty())
			widget_->restoreGeometry(geometry);
	}

}
void util::FormSettingManager::saveSettings()
{
	util::Crypt crypt;
	const QString fileName(qgetenv("JSON_PATH"));
	if (fileName.isEmpty())
		return;

	Config config(fileName);
	QString ObjectName;
	QString qstrGeometry;
	QString qstrState;
	QSize size;
	if (mainwindow_ != nullptr)
	{
		ObjectName = mainwindow_->objectName();
		qstrGeometry = crypt.encryptFromByteArray(mainwindow_->saveGeometry());//ENCODE
		qstrState = crypt.encryptFromByteArray(mainwindow_->saveState(3));//ENCODE
		size = mainwindow_->size();
		config.write("Form", ObjectName, "State", qstrState);
	}
	else
	{
		ObjectName = widget_->objectName();
		qstrGeometry = crypt.encryptFromByteArray(widget_->saveGeometry());//ENCODE
		size = widget_->size();
	}

	config.write("Form", ObjectName, "Geometry", qstrGeometry);
	config.write("Form", ObjectName, "Size", QString("%1,%2").arg(size.width()).arg(size.height()));
}

QFileInfoList util::loadAllFileLists(TreeWidgetItem* root, const QString& path, const QString& suffix
	, const QString& icon
	, QStringList* list
	, const QString& folderIcon)
{
	/*添加path路徑文件*/
	QDir dir(path); //遍歷各級子目錄
	QDir dir_file(path); //遍歷子目錄中所有文件

	dir_file.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks); //獲取當前所有文件
	dir_file.setSorting(QDir::Size | QDir::Reversed);

	const QStringList filters = { QString("*%1").arg(suffix) };

	dir_file.setNameFilters(filters);

	QFileInfoList list_file = dir_file.entryInfoList();

	for (const QFileInfo& item : list_file)
	{
		//將當前目錄中所有文件添加到treewidget中
		if (list != nullptr)
			list->append(item.fileName());

		QString content;
		if (!readFile(item.absoluteFilePath(), &content))
			continue;

		TreeWidgetItem* child = q_check_ptr(new TreeWidgetItem(QStringList{ item.fileName() }, 1));
		if (child == nullptr)
			continue;

		child->setToolTip(0, QString("===== %1 =====\n\n%2").arg(item.absoluteFilePath()).arg(content.left(256)));
		child->setIcon(0, QIcon(QPixmap(icon)));

		root->addChild(child);
	}

	QFileInfoList file_list = dir.entryInfoList(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
	const QFileInfoList folder_list = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot); //獲取當前所有目錄
	int count = folder_list.size();
	for (int i = 0; i != count; ++i) //自動遞歸添加各目錄到上一級目錄
	{
		const QString namepath = folder_list.value(i).absoluteFilePath(); //獲取路徑
		const QFileInfo folderinfo = folder_list.value(i);
		const QString name = folderinfo.fileName(); //獲取目錄名
		if (list != nullptr)
			list->append(name);

		TreeWidgetItem* childroot = new TreeWidgetItem(QStringList{ name }, 0);
		if (childroot == nullptr)
			continue;

		childroot->setIcon(0, QIcon(QPixmap(folderIcon)));
		root->addChild(childroot); //將當前目錄添加成path的子項
		const QFileInfoList child_file_list = loadAllFileLists(childroot, namepath, suffix, icon, list, folderIcon); //遞歸添加子目錄
		file_list.append(child_file_list);
	}
	return file_list;
}

QFileInfoList util::loadAllFileLists(TreeWidgetItem* root, const QString& path, QStringList* list, const QString& fileIcon, const QString& folderIcon)
{
	/*添加path路徑文件*/
	QDir dir(path); //遍歷各級子目錄
	QDir dir_file(path); //遍歷子目錄中所有文件
	dir_file.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks); //獲取當前所有文件
	dir_file.setSorting(QDir::Size | QDir::Reversed);

	const QStringList filters = {
		QString("*%1").arg(util::SCRIPT_DEFAULT_SUFFIX),
		QString("*%1").arg(util::SCRIPT_PRIVATE_SUFFIX_DEFAULT),
		QString("*%1").arg(util::SCRIPT_LUA_SUFFIX_DEFAULT)
	};

	dir_file.setNameFilters(filters);
	QFileInfoList list_file = dir_file.entryInfoList();

	for (const QFileInfo& item : list_file)
	{
		//將當前目錄中所有文件添加到treewidget中
		if (list != nullptr)
			list->append(item.fileName());

		TreeWidgetItem* child = new TreeWidgetItem(QStringList{ item.fileName() }, 1);
		if (child == nullptr)
			continue;

		QString content;
		if (!readFile(item.absoluteFilePath(), &content))
			continue;

		child->setToolTip(0, QString("===== %1 =====\n\n%2").arg(item.absoluteFilePath()).arg(content.left(256)));
		child->setIcon(0, QIcon(QPixmap(fileIcon)));
		root->addChild(child);
	}

	QFileInfoList file_list = dir.entryInfoList(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
	const QFileInfoList folder_list = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot); //獲取當前所有目錄
	int count = folder_list.size();

	//自動遞歸添加各目錄到上一級目錄
	for (int i = 0; i != count; ++i)
	{
		const QString namepath = folder_list.value(i).absoluteFilePath(); //獲取路徑
		const QFileInfo folderinfo = folder_list.value(i);
		const QString name = folderinfo.fileName(); //獲取目錄名

		if (list != nullptr)
			list->append(name);

		TreeWidgetItem* childroot = new TreeWidgetItem(QStringList{ name }, 0);
		if (childroot == nullptr)
			continue;

		childroot->setIcon(0, QIcon(QPixmap(folderIcon)));

		//將當前目錄添加成path的子項
		root->addChild(childroot);

		const QFileInfoList child_file_list(loadAllFileLists(childroot, namepath, list, fileIcon, folderIcon)); //遞歸添加子目錄
		file_list.append(child_file_list);
	}

	return file_list;
}

void util::searchFiles(const QString& dir, const QString& fileNamePart, const QString& suffixWithDot, QList<QString>* result)
{
	QDir d(dir);
	if (!d.exists())
		return;

	QFileInfoList list = d.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
	for (const QFileInfo& fileInfo : list)
	{
		if (fileInfo.isFile())
		{
			if (!fileInfo.fileName().contains(fileNamePart, Qt::CaseInsensitive) || fileInfo.suffix() != suffixWithDot.mid(1))
				continue;

			QString content;
			if (!readFile(fileInfo.absoluteFilePath(), &content))
				continue;

			//將文件名置於前方
			QString fileContent = QString("# %1\n---\n%2").arg(fileInfo.fileName()).arg(content);

			result->append(fileContent);
		}
		else if (fileInfo.isDir())
		{
			searchFiles(fileInfo.absoluteFilePath(), fileNamePart, suffixWithDot, result);
		}
	}
}

bool util::enumAllFiles(const QString dir, const QString suffix, QVector<QPair<QString, QString>>* result)
{
	QDir directory(dir);

	if (!directory.exists())
	{
		return false; // 目錄不存在，返回失敗
	}

	QFileInfoList fileList = directory.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
	QFileInfoList dirList = directory.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);

	// 遍歷並匹配文件
	for (const QFileInfo& fileInfo : fileList)
	{
		QString fileName = fileInfo.fileName();
		QString filePath = fileInfo.filePath();

		// 如果suffix不為空且文件名不以suffix結尾，則跳過
		if (!suffix.isEmpty() && !fileName.endsWith(suffix, Qt::CaseInsensitive))
		{
			continue;
		}

		// 將匹配的文件信息添加到結果中
		result->append(QPair<QString, QString>(fileName, filePath));
	}

	// 遞歸遍歷子目錄
	for (const QFileInfo& dirInfo : dirList)
	{
		QString subDir = dirInfo.filePath();
		bool success = enumAllFiles(subDir, suffix, result);
		if (!success)
		{
			return false; // 遞歸遍歷失敗，返回失敗
		}
	}

	return true; // 遍歷成功，返回成功
}

//自身進程目錄 遞歸遍查找指定文件
QString util::findFileFromName(const QString& fileName, const QString& dirpath)
{
	QDir dir(dirpath);
	if (!dir.exists())
		return QString();

	dir.setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
	dir.setSorting(QDir::DirsFirst);
	QFileInfoList list = dir.entryInfoList();
	for (int i = 0; i < list.size(); ++i)
	{
		QFileInfo fileInfo = list.value(i);
		if (fileInfo.fileName() == fileName)
			return fileInfo.absoluteFilePath();
		if (fileInfo.isDir())
		{
			QString ret = util::findFileFromName(fileName, fileInfo.absoluteFilePath());
			if (!ret.isEmpty())
				return ret;
		}
	}
	return QString();
}


void util::sortWindows(const QVector<HWND>& windowList, bool alignLeft)
{
	if (windowList.isEmpty())
	{
		return;
	}

	// 獲取桌面分辨率
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	//int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	// 窗口移動的初始位置
	int x = 0;
	int y = 0;

	// 遍歷窗口列表，移動窗口
	int size = windowList.size();
	for (int i = 0; i < size; ++i)
	{
		HWND hwnd = windowList.value(i);
		if (hwnd == nullptr)
		{
			continue;
		}

		// 設置窗口位置
		ShowWindow(hwnd, SW_RESTORE);                             // 先恢覆窗口
		SetForegroundWindow(hwnd);                                // 然後將窗口激活

		// 獲取窗口大小
		RECT windowRect;
		GetWindowRect(hwnd, &windowRect);
		int windowWidth = windowRect.right - windowRect.left;
		int windowHeight = windowRect.bottom - windowRect.top;

		// 根據對齊方式設置窗口位置
		if (alignLeft)
		{
			SetWindowPos(hwnd, HWND_TOP, x, y, 0, 0, SWP_ASYNCWINDOWPOS | SWP_NOSIZE); // 左對齊
		}
		else
		{
			int xPos = screenWidth - (x + windowWidth);
			SetWindowPos(hwnd, HWND_TOP, xPos, y, 0, 0, SWP_ASYNCWINDOWPOS | SWP_NOSIZE);// 右對齊
		}

		// 更新下一個窗口的位置
		x += windowWidth - 15;
		if (x + windowWidth > screenWidth)
		{
			x = 0;
			y += windowHeight;
		}
	}
}

bool util::writeFireWallOverXP(const LPCTSTR& ruleName, const LPCTSTR& appPath, bool NoopIfExist)
{
	bool bret = true;
	HRESULT hrComInit = S_OK;
	HRESULT hr = S_OK;

	INetFwPolicy2* pNetFwPolicy2 = NULL;
	INetFwRules* pFwRules = NULL;
	INetFwRule* pFwRule = NULL;

	BSTR bstrRuleName = SysAllocString(ruleName);
	BSTR bstrAppName = SysAllocString(appPath);
	BSTR bstrDescription = SysAllocString(ruleName);

	// Initialize COM.
	hrComInit = CoInitializeEx(
		0,
		COINIT_APARTMENTTHREADED);

	do
	{
		// Ignore RPC_E_CHANGED_MODE; this just means that COM has already been
	// initialized with a different mode. Since we don't care what the mode is,
	// we'll just use the existing mode.
		if (hrComInit != RPC_E_CHANGED_MODE)
		{
			if (FAILED(hrComInit))
			{
				//print << "CoInitializeEx failed: " << hrComInit << Qt::endl;
				bret = false;
				break;
			}
		}

		// Retrieve INetFwPolicy2
		hr = CoCreateInstance(
			__uuidof(NetFwPolicy2),
			NULL,
			CLSCTX_INPROC_SERVER,
			__uuidof(INetFwPolicy2),
			(void**)&pNetFwPolicy2);

		if (FAILED(hr))
		{
			//print << "CoCreateInstance for INetFwPolicy2 failed : " << hr << Qt::endl;
			bret = false;
			break;
		}

		// Retrieve INetFwRules
		hr = pNetFwPolicy2->get_Rules(&pFwRules);
		if (FAILED(hr))
		{
			//print << "get_Rules failed: " << hr << Qt::endl;
			bret = false;
			break;
		}

		// see if existed
		if (NoopIfExist)
		{
			hr = pFwRules->Item(bstrRuleName, &pFwRule);
			if (hr == S_OK)
			{
				//print << "Firewall Item existed" << hr << Qt::endl;
				bret = false;
				break;
			}
		}

		// Create a new Firewall Rule object.
		hr = CoCreateInstance(
			__uuidof(NetFwRule),
			NULL,
			CLSCTX_INPROC_SERVER,
			__uuidof(INetFwRule),
			(void**)&pFwRule);
		if (FAILED(hr))
		{
			// printf("CoCreateInstance for Firewall Rule failed: 0x%08lx\n", hr << Qt::endl;
			bret = false;
			break;
		}

		// Populate the Firewall Rule object

		pFwRule->put_Name(bstrRuleName);
		pFwRule->put_ApplicationName(bstrAppName);
		pFwRule->put_Description(bstrDescription);
		pFwRule->put_Protocol(NET_FW_IP_PROTOCOL_ANY);
		pFwRule->put_Action(NET_FW_ACTION_ALLOW);
		pFwRule->put_Enabled(VARIANT_TRUE);
		pFwRule->put_Profiles(NET_FW_PROFILE2_ALL);

		// Add the Firewall Rule
		hr = pFwRules->Add(pFwRule);
		if (FAILED(hr))
		{
			// printf("Firewall Rule Add failed: 0x%08lx\n", hr << Qt::endl;
			bret = false;
			break;
		}
	} while (false);

	// Free BSTR's
	SysFreeString(bstrRuleName);
	SysFreeString(bstrAppName);
	SysFreeString(bstrDescription);

	// Release the INetFwRule object
	if (pFwRule != NULL)
	{
		pFwRule->Release();
	}

	// Release the INetFwRules object
	if (pFwRules != NULL)
	{
		pFwRules->Release();
	}

	// Release the INetFwPolicy2 object
	if (pNetFwPolicy2 != NULL)
	{
		pNetFwPolicy2->Release();
	}

	// Uninitialize COM.
	if (SUCCEEDED(hrComInit))
	{
		CoUninitialize();
	}
	return bret;
}

bool util::monitorThreadResourceUsage(quint64 threadId, FILETIME& preidleTime, FILETIME& prekernelTime, FILETIME& preuserTime, double* pCpuUsage, double* pMemUsage, double* pMaxMemUsage)
{
	FILETIME idleTime = { 0 };
	FILETIME kernelTime = { 0 };
	FILETIME userTime = { 0 };

	if (GetSystemTimes(&idleTime, &kernelTime, &userTime) == TRUE)
	{
		ULARGE_INTEGER x = { 0 }, y = { 0 };

		double idle, kernel, user;

		x.LowPart = preidleTime.dwLowDateTime;
		x.HighPart = preidleTime.dwHighDateTime;
		y.LowPart = idleTime.dwLowDateTime;
		y.HighPart = idleTime.dwHighDateTime;
		idle = static_cast<double>(y.QuadPart - x.QuadPart) / 10000000.0; // 转换为秒

		x.LowPart = prekernelTime.dwLowDateTime;
		x.HighPart = prekernelTime.dwHighDateTime;
		y.LowPart = kernelTime.dwLowDateTime;
		y.HighPart = kernelTime.dwHighDateTime;
		kernel = static_cast<double>(y.QuadPart - x.QuadPart) / 10000000.0; // 转换为秒

		x.LowPart = preuserTime.dwLowDateTime;
		x.HighPart = preuserTime.dwHighDateTime;
		y.LowPart = userTime.dwLowDateTime;
		y.HighPart = userTime.dwHighDateTime;
		user = static_cast<double>(y.QuadPart - x.QuadPart) / 10000000.0; // 转换为秒

		double totalTime = kernel + user;

		if (totalTime > 0.0)
		{
			*pCpuUsage = (kernel + user - idle) / totalTime * 100.0;
		}

		preidleTime = idleTime;
		prekernelTime = kernelTime;
		preuserTime = userTime;
	}

	PROCESS_MEMORY_COUNTERS_EX memCounters = { 0 };
	if (!K32GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memCounters), sizeof(memCounters)))
	{
		qDebug() << "Failed to get memory info: " << GetLastError();
		return false;
	}

	if (pMemUsage != nullptr)
	{
		*pMemUsage = memCounters.WorkingSetSize / (1024.0 * 1024.0); // 內存使用量，以兆字節為單位
	}

	if (pMaxMemUsage != nullptr)
	{
		*pMaxMemUsage = memCounters.PrivateUsage / (1024.0 * 1024.0); // 內存使用量，以兆字節為單位
	}

	return true;
}

QFont util::getFont()
{
	QFont font;
	font.setBold(false); // 加粗
	font.setCapitalization(QFont::Capitalization::MixedCase); //混合大小寫
	//font.setFamilies(const QStringList & families);
	font.setFamily("SimSun");// 宋体
	font.setFixedPitch(true); // 固定間距
	font.setHintingPreference(QFont::HintingPreference::PreferFullHinting/*QFont::HintingPreference::PreferFullHinting*/);
	font.setItalic(false); // 斜體
	font.setKerning(false); //禁止調整字距
	font.setLetterSpacing(QFont::SpacingType::AbsoluteSpacing, 0.0);
	font.setOverline(false);
	font.setPointSize(12);
	font.setPointSizeF(12.0);
	font.setPixelSize(12);
	font.setStretch(0);
	font.setStrikeOut(false);
	font.setStyle(QFont::Style::StyleNormal);
	font.setStyleHint(QFont::StyleHint::System/*SansSerif*/, QFont::StyleStrategy::NoAntialias/*QFont::StyleStrategy::PreferAntialias*/);
	//font.setStyleName(const QString & styleName);
	font.setStyleStrategy(QFont::StyleStrategy::NoAntialias/*QFont::StyleStrategy::PreferAntialias*/);
	font.setUnderline(false);
	font.setWeight(60);
	font.setWordSpacing(0.0);
	return font;
}

void util::asyncRunBat(const QString& path, QString data)
{
	const QString batfile = QString("%1/%2.bat").arg(path).arg(QDateTime::currentDateTime().toString("sash_yyyyMMdd"));
	if (util::writeFile(batfile, data))
		ShellExecuteW(NULL, L"open", (LPCWSTR)batfile.utf16(), NULL, NULL, SW_HIDE);
}

QString util::applicationFilePath()
{
	static bool init = false;
	if (!init)
	{
		WCHAR buffer[MAX_PATH] = {};
		DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
		if (length == 0)
			return "";

		QString path(QString::fromWCharArray(buffer, length));
		if (path.isEmpty())
			return "";

		qputenv("CURRENT_PATH", path.toUtf8());
		init = true;
	}
	return QString::fromUtf8(qgetenv("CURRENT_PATH"));
}

QString util::applicationDirPath()
{
	static bool init = false;
	if (!init)
	{
		QString path(applicationFilePath());
		if (path.isEmpty())
			return "";

		QFileInfo fileInfo(path);
		path = fileInfo.absolutePath();
		qputenv("CURRENT_DIR", path.toUtf8());
		init = true;
	}
	return QString::fromUtf8(qgetenv("CURRENT_DIR"));
}

QString util::applicationName()
{
	static bool init = false;
	if (!init)
	{
		QString path(applicationFilePath());
		if (path.isEmpty())
			return "";

		QFileInfo fileInfo(path);
		path = fileInfo.fileName();
		qputenv("CURRENT_NAME", path.toUtf8());
		init = true;
	}
	return QString::fromUtf8(qgetenv("CURRENT_NAME"));
}

qint64 __vectorcall util::percent(qint64 value, qint64 total)
{
	if (value == 1 && total > 0)
		return value;
	if (value == 0)
		return 0;

	double d = std::floor(static_cast<double>(value) * 100.0 / static_cast<double>(total));
	if ((value > 0) && (d < 1.0))
		return 1;
	else
		return static_cast<qint64>(d);
}

bool util::createFileDialog(const QString& name, QString* retstring, QWidget* parent)
{
	QFileDialog dialog(parent);
	dialog.setModal(true);
	dialog.setFileMode(QFileDialog::ExistingFile);
	dialog.setViewMode(QFileDialog::Detail);
	dialog.setOption(QFileDialog::ReadOnly, true);
	dialog.setAcceptMode(QFileDialog::AcceptOpen);


	//check suffix
	if (!name.isEmpty())
	{
		QStringList filters;
		filters << name;
		dialog.setNameFilters(filters);
	}

	//directory
	//自身目錄往上一層
	QString directory = util::applicationDirPath();
	directory = QDir::toNativeSeparators(directory);
	directory = QDir::cleanPath(directory + QDir::separator() + "..");
	dialog.setDirectory(directory);

	if (dialog.exec() == QDialog::Accepted)
	{
		QStringList fileNames = dialog.selectedFiles();
		if (fileNames.size() > 0)
		{
			QString fileName = fileNames.value(0);

			QTextCodec* codec = nullptr;
			UINT acp = GetACP();
			if (acp == 936)
				codec = QTextCodec::codecForName(util::DEFAULT_GAME_CODEPAGE);
			else if (acp == 950)
				codec = QTextCodec::codecForName("big5");
			else
				codec = QTextCodec::codecForName(util::DEFAULT_CODEPAGE);

			std::string str = codec->fromUnicode(fileName).data();
			fileName = codec->toUnicode(str.c_str());

			if (retstring)
				*retstring = fileName;

			return true;
		}
	}
	return false;
}

bool util::customStringCompare(const QString& str1, const QString& str2)
{
	//中文locale
	static const QLocale locale;
	static const QCollator collator(locale);

	return collator.compare(str1, str2) < 0;
}

QString util::formatMilliseconds(qint64 milliseconds)
{
	qint64 totalSeconds = milliseconds / 1000ll;
	qint64 days = totalSeconds / (24ll * 60ll * 60ll);
	qint64 hours = (totalSeconds % (24ll * 60ll * 60ll)) / (60ll * 60ll);
	qint64 minutes = (totalSeconds % (60ll * 60ll)) / 60ll;
	qint64 seconds = totalSeconds % 60ll;
	qint64 remainingMilliseconds = milliseconds % 1000ll;

	return QString(QObject::tr("%1 day %2 hour %3 min %4 sec %5 msec"))
		.arg(days).arg(hours).arg(minutes).arg(seconds).arg(remainingMilliseconds);
}

QString util::formatSeconds(qint64 seconds)
{
	qint64 day = seconds / 86400ll;
	qint64 hours = (seconds % 86400ll) / 3600ll;
	qint64 minutes = (seconds % 3600ll) / 60ll;
	qint64 remainingSeconds = seconds % 60ll;

	return QString(QObject::tr("%1 day %2 hour %3 min %4 sec")).arg(day).arg(hours).arg(minutes).arg(remainingSeconds);
};

bool util::readFileFilter(const QString& fileName, QString& content, bool* pisPrivate)
{
	if (pisPrivate != nullptr)
		*pisPrivate = false;

	if (fileName.endsWith(util::SCRIPT_PRIVATE_SUFFIX_DEFAULT))
	{
#ifdef CRYPTO_H
		Crypto crypto;
		if (!crypto.decodeScript(fileName, c))
			return false;

		if (pisPrivate != nullptr)
			*pisPrivate = true;
#else
		return false;
#endif
	}
	content.replace("\r\n", "\n");
	return true;
}

bool util::readFile(const QString& fileName, QString* pcontent, bool* pisPrivate)
{
	QFileInfo fi(fileName);
	if (!fi.exists())
		return false;

	if (fi.isDir())
		return false;

	util::ScopedFile f(fileName, QIODevice::ReadOnly | QIODevice::Text);
	if (!f.isOpen())
		return false;

	TextStream in(&f);
	QString content = in.readAll();

	if (readFileFilter(fileName, content, pisPrivate))
	{
		if (pcontent)
			*pcontent = content;
		return true;
	}

	return false;
}

bool util::writeFile(const QString& fileName, const QString& content)
{
	util::ScopedFile f(fileName, QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
	if (!f.isOpen())
		return false;

	TextStream out(&f);
	out << content;
	out.flush();
	f.flush();
	return true;
}