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

bool __fastcall mem::read(HANDLE hProcess, unsigned long long desiredAccess, unsigned long long size, PVOID buffer)
{
	if (!size)
		return false;

	if (!buffer)
		return false;

	if (hProcess == nullptr)
		return false;

	if (!desiredAccess)
		return false;

	ScopedHandle::enablePrivilege();

	//ULONG oldProtect = NULL;
	//VirtualProtectEx(m_pi.hProcess, buffer, size, PAGE_EXECUTE_READWRITE, &oldProtect);
	BOOL ret = NT_SUCCESS(MINT::NtReadVirtualMemory(hProcess, reinterpret_cast<PVOID>(desiredAccess), buffer, size, NULL));
	//VirtualProtectEx(m_pi.hProcess, buffer, size, oldProtect, &oldProtect);

	return ret == TRUE;
}

template<typename T, typename>
T __fastcall mem::read(HANDLE hProcess, unsigned long long desiredAccess)
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

float __fastcall mem::readFloat(HANDLE hProcess, unsigned long long desiredAccess)
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

qreal __fastcall mem::readDouble(HANDLE hProcess, unsigned long long desiredAccess)
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

QString __fastcall mem::readString(HANDLE hProcess, unsigned long long desiredAccess, unsigned long long size, bool enableTrim, bool keepOriginal)
{
	if (hProcess == nullptr)
		return "\0";

	if (!desiredAccess)
		return "\0";

	QScopedArrayPointer <char> p(q_check_ptr(new char[size + 1]()));
	sash_assume(!p.isNull());
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

bool __fastcall mem::write(HANDLE hProcess, unsigned long long baseAddress, PVOID buffer, unsigned long long dwSize)
{
	if (hProcess == nullptr)
		return false;

	ULONG oldProtect = NULL;

	ScopedHandle::enablePrivilege();

	VirtualProtectEx(hProcess, (LPVOID)baseAddress, dwSize, PAGE_EXECUTE_READWRITE, &oldProtect);
	BOOL ret = WriteProcessMemory(hProcess, reinterpret_cast<PVOID>(baseAddress), buffer, dwSize, NULL);
	VirtualProtectEx(hProcess, (LPVOID)baseAddress, dwSize, oldProtect, &oldProtect);

	return ret == TRUE;
}

template<typename T, typename>
bool __fastcall mem::write(HANDLE hProcess, unsigned long long baseAddress, T data)
{
	if (hProcess == nullptr)
		return false;

	if (!baseAddress)
		return false;

	PVOID pBuffer = &data;
	BOOL ret = write(hProcess, baseAddress, pBuffer, sizeof(T));

	return ret == TRUE;
}

bool __fastcall mem::writeString(HANDLE hProcess, unsigned long long baseAddress, const QString& str)
{
	if (hProcess == nullptr)
		return false;

	QTextCodec* codec = QTextCodec::codecForName(util::DEFAULT_GAME_CODEPAGE);//QTextCodec::codecForMib(2025);//

	QByteArray ba = codec->fromUnicode(str);
	ba.append('\0');

	const char* pBuffer = ba.constData();
	unsigned long long len = ba.size();

	//QScopedArrayPointer <char> p(q_check_ptr(new char[len + 1]()));
	char p[8192] = {};

	if (len + 1 < sizeof(p))
		_snprintf_s(p, len + 1, _TRUNCATE, "%s\0", pBuffer);
	else
		_snprintf_s(p, sizeof(p), _TRUNCATE, "%s\0", pBuffer);

	BOOL ret = write(hProcess, baseAddress, p, len + 1);

	return ret == TRUE;
}

class RemoteMemoryPool
{
public:
	RemoteMemoryPool(HANDLE hProcess, SIZE_T size)
		: hProcess(hProcess), poolSize(size), allocatedSize(0)
	{
		PVOID baseAddress = NULL;
		NTSTATUS status = MINT::NtAllocateVirtualMemory(hProcess, &baseAddress, 0, &poolSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		if (!NT_SUCCESS(status))
		{
			throw std::runtime_error("Failed to allocate memory pool");
		}
		poolBase = reinterpret_cast<unsigned char*>(baseAddress);
	}

	~RemoteMemoryPool()
	{
		if (poolBase)
		{
			SIZE_T size = 0;
			MINT::NtFreeVirtualMemory(hProcess, reinterpret_cast<PVOID*>(&poolBase), &size, MEM_RELEASE);
		}
	}

	void* __fastcall allocate(SIZE_T size)
	{
		std::unique_lock<std::shared_mutex> lock(mutex);

		// Check free list first
		for (auto it = freeBlocks.begin(); it != freeBlocks.end(); ++it)
		{
			if (it->second >= size)
			{
				void* allocatedMemory = it->first;
				if (it->second > size)
				{
					it->first = static_cast<unsigned char*>(it->first) + size;
					it->second -= size;
				}
				else
				{
					freeBlocks.erase(it);
				}
				allocationMap[allocatedMemory] = size;
				return allocatedMemory;
			}
		}

		// Allocate from the remaining pool space
		if (allocatedSize + size > poolSize)
		{
			return nullptr; // Out of memory
		}
		void* allocatedMemory = poolBase + allocatedSize;
		allocatedSize += size;
		allocationMap[allocatedMemory] = size;
		return allocatedMemory;
	}

	void deallocate(void* ptr)
	{
		std::unique_lock<std::shared_mutex> lock(mutex);

		auto allocationEntry = allocationMap.find(ptr);
		if (allocationEntry == allocationMap.end())
		{
			// Handle error: attempting to deallocate memory that was not allocated
			return;
		}

		SIZE_T size = allocationEntry->second;
		allocationMap.erase(allocationEntry);

		// Zero out the memory in the remote process before deallocating
		unsigned char* zeroBuffer = new unsigned char[size]();
		SIZE_T bytesWritten;
		BOOL result = WriteProcessMemory(hProcess, ptr, zeroBuffer, size, &bytesWritten);
		delete[] zeroBuffer;

		if (!result || bytesWritten != size)
		{
			// Handle error: memory write failed
			return;
		}

		// Attempt to merge this block with existing free blocks
		mergeFreeBlock(ptr, size);
	}

private:
	HANDLE hProcess;
	unsigned char* poolBase;
	SIZE_T poolSize;
	SIZE_T allocatedSize;
	std::unordered_map<void*, SIZE_T> allocationMap;
	std::list<std::pair<void*, SIZE_T>> freeBlocks;
	std::shared_mutex mutex;

	void mergeFreeBlock(void* ptr, SIZE_T size)
	{
		unsigned char* blockStart = reinterpret_cast<unsigned char*>(ptr);
		unsigned char* blockEnd = blockStart + size;

		for (auto it = freeBlocks.begin(); it != freeBlocks.end(); ++it)
		{
			unsigned char* currentBlockStart = reinterpret_cast<unsigned char*>(it->first);
			unsigned char* currentBlockEnd = currentBlockStart + it->second;

			if (blockEnd == currentBlockStart)
			{
				it->first = blockStart;
				it->second += size;
				mergeAdjacentFreeBlocks(it);
				return;
			}
			else if (blockStart == currentBlockEnd)
			{
				it->second += size;
				mergeAdjacentFreeBlocks(it);
				return;
			}
		}

		freeBlocks.emplace_back(ptr, size);
	}

	void mergeAdjacentFreeBlocks(std::list<std::pair<void*, SIZE_T>>::iterator startIt)
	{
		auto it = startIt;
		auto nextIt = std::next(it);

		while (nextIt != freeBlocks.end())
		{
			unsigned char* currentBlockEnd = reinterpret_cast<unsigned char*>(it->first) + it->second;
			unsigned char* nextBlockStart = reinterpret_cast<unsigned char*>(nextIt->first);

			if (currentBlockEnd == nextBlockStart)
			{
				it->second += nextIt->second;
				nextIt = freeBlocks.erase(nextIt);
			}
			else
			{
				++it;
				++nextIt;
			}
		}
	}
};

namespace mem
{
	std::unordered_map<HANDLE, std::unique_ptr<RemoteMemoryPool>> memoryPoolMap;
	std::shared_mutex mutex;
}

unsigned long long __fastcall mem::virtualAlloc(HANDLE hProcess, unsigned long long size)
{
	//if (hProcess == nullptr || size == 0)
	//	throw std::invalid_argument("Invalid handle or size");

	//// Ensure size is aligned to page size
	//size = (size + 4095) & ~4095ULL;

	//PVOID ptr = NULL;
	//SIZE_T sizet = static_cast<SIZE_T>(size);

	//NTSTATUS status = MINT::NtAllocateVirtualMemory(hProcess, &ptr, NULL, &sizet, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	//if (!NT_SUCCESS(status))
	//	throw std::runtime_error("Memory allocation failed");

	//return reinterpret_cast<unsigned long long>(ptr);

	std::shared_lock<std::shared_mutex> lock(mutex);
	auto it = memoryPoolMap.find(hProcess);
	if (it == memoryPoolMap.end())
	{
		lock.unlock();
		std::unique_lock<std::shared_mutex> lock(mutex);
		it = memoryPoolMap.find(hProcess);
		if (it == memoryPoolMap.end())
		{
			std::unique_ptr<RemoteMemoryPool> memoryPool(new RemoteMemoryPool(hProcess, 4096 * 32));
			it = memoryPoolMap.insert(std::make_pair(hProcess, std::move(memoryPool))).first;
		}
	}

	return reinterpret_cast<unsigned long long>(it->second->allocate(size));
}

unsigned long long __fastcall mem::virtualAllocA(HANDLE hProcess, const QString& str)
{
	unsigned long long ret = NULL;
	do
	{
		if (hProcess == nullptr)
			break;

		ret = virtualAlloc(hProcess, static_cast<unsigned long long>(str.toLocal8Bit().size()) * 2 * sizeof(char) + 1);
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

unsigned long long __fastcall mem::virtualAllocW(HANDLE hProcess, const QString& str)
{
	unsigned long long ret = NULL;
	do
	{
		if (hProcess == nullptr)
			break;

		std::wstring wstr(str.toStdWString());
		wstr += L'\0';

		ret = virtualAlloc(hProcess, static_cast<unsigned long long>(wstr.length()) * sizeof(wchar_t) + 1);
		if (!ret)
			break;

		if (!write(hProcess, ret, const_cast<wchar_t*>(wstr.c_str()), static_cast<unsigned long long>(wstr.length()) * sizeof(wchar_t) + 1))
		{
			virtualFree(hProcess, ret);
			ret = NULL;
			break;
		}
	} while (false);
	return ret;
}

bool __fastcall mem::virtualFree(HANDLE hProcess, unsigned long long baseAddress)
{
	//if (hProcess == nullptr)
	//	return false;

	//if (baseAddress == NULL)
	//	return false;

	//SIZE_T size = 0;
	//BOOL ret = NT_SUCCESS(MINT::NtFreeVirtualMemory(hProcess, reinterpret_cast<PVOID*>(&baseAddress), &size, MEM_RELEASE));

	//return ret == TRUE;

	std::shared_lock<std::shared_mutex> lock(mutex);
	auto it = memoryPoolMap.find(hProcess);
	if (it != memoryPoolMap.end())
	{
		lock.unlock();
		std::unique_lock<std::shared_mutex> lock(mutex);
		it = memoryPoolMap.find(hProcess);
		if (it != memoryPoolMap.end())
		{
			it->second->deallocate(reinterpret_cast<void*>(baseAddress));
			return true;
		}
	}

	return false;
}

#ifndef _WIN64
DWORD __fastcall mem::getRemoteModuleHandle(DWORD dwProcessId, const QString& moduleName)
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
#endif

void __fastcall mem::freeUnuseMemory(HANDLE hProcess)
{
	SetProcessWorkingSetSizeEx(hProcess, static_cast<SIZE_T>(-1), static_cast<SIZE_T>(-1), 0);
	K32EmptyWorkingSet(hProcess);
}

#if 0
#ifdef _WIN64
DWORD __fastcall mem::getFunAddr(const DWORD* DllBase, const char* FunName)
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
#endif

HMODULE __fastcall mem::getRemoteModuleHandleByProcessHandleW(HANDLE hProcess, const QString& szModuleName)
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

			QString fileName = util::toQString(szModName);
			fileName.replace("/", "\\");
			QFileInfo file(fileName);
			if (file.fileName().toLower() != szModuleName.toLower())
				continue;

			return hMods[i];
		}
	}

	return NULL;
}

long __fastcall mem::getProcessExportTable32(HANDLE hProcess, const QString& ModuleName, IAT_EAT_INFO tbinfo[], int tb_info_max)
{
	ULONG64 muBase = 0, count = 0;
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)q_check_ptr(new BYTE[sizeof(IMAGE_DOS_HEADER)]);
	sash_assume(pDosHeader != nullptr);
	PIMAGE_NT_HEADERS32 pNtHeaders = (PIMAGE_NT_HEADERS32)q_check_ptr(new BYTE[sizeof(IMAGE_NT_HEADERS32)]);
	sash_assume(pNtHeaders != nullptr);
	PIMAGE_EXPORT_DIRECTORY pExport = (PIMAGE_EXPORT_DIRECTORY)q_check_ptr(new BYTE[sizeof(IMAGE_EXPORT_DIRECTORY)]);
	sash_assume(pExport != nullptr);

	char strName[130] = {};
	memset(strName, 0, sizeof(strName));

	//拿到目標模塊的BASE
	muBase = static_cast<ULONG>(reinterpret_cast<long long>(getRemoteModuleHandleByProcessHandleW(hProcess, ModuleName)));
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
		std::string smoduleName = util::toConstData(ModuleName);
		_snprintf_s(tbinfo[count].ModuleName, sizeof(tbinfo[count].ModuleName), _TRUNCATE, "%s", smoduleName.c_str());
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
ULONG64 __fastcall mem::getProcAddressIn32BitProcess(HANDLE hProcess, const QString& ModuleName, const QString& FuncName)
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

#ifndef _WIN64
bool __fastcall mem::injectByWin7(long long index, DWORD dwProcessId, HANDLE hProcess, QString dllPath, HMODULE* phDllModule, unsigned long long* phGameModule, HWND hWnd)
{
	HMODULE hModule = nullptr;
	util::timer timer;
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

	DWORD hGameModule = mem::getRemoteModuleHandle(dwProcessId, QString(SASH_SUPPORT_GAMENAME));
	if (phGameModule != nullptr)
		*phGameModule = static_cast<unsigned long long>(hGameModule);

	for (long long i = 0; i < 2; ++i)
	{
		timer.restart();
		mem::VirtualMemory dllFullPathAddr(hProcess, dllPath, mem::VirtualMemory::kUnicode, true);
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
			reinterpret_cast<LPVOID>(static_cast<unsigned long long>(dllFullPathAddr)));

		if (!hThreadHandle.isValid())
		{
			emit signalDispatcher.messageBoxShow(QObject::tr("Create remote thread failed"), QMessageBox::Icon::Critical);
			return false;
		}

		timer.restart();
		for (;;)
		{
			hModule = reinterpret_cast<HMODULE>(mem::getRemoteModuleHandle(dwProcessId, QString(SASH_INJECT_DLLNAME) + ".dll"));
			if (hModule != nullptr)
				break;

			if (timer.hasExpired(3000))
				break;

			if (!mem::isProcessExist(dwProcessId))
				return false;

			if (hWnd != nullptr)
			{
				if (!IsWindow(hWnd))
					return false;
			}

			QThread::msleep(200);
		}

		if (phDllModule != nullptr)
			*phDllModule = hModule;

		if (hModule != nullptr)
			break;
	}

	if (hModule != nullptr)
		qDebug() << "inject OK" << "0x" + util::toQString(reinterpret_cast<long long>(hModule), 16) << "time:" << timer.cost() << "ms";
	return true;
}
#endif

bool __fastcall mem::injectBy64(long long index, DWORD dwProcessId, HANDLE hProcess, QString dllPath, HMODULE* phDllModule, unsigned long long* phGameModule, HWND hWnd)
{
	util::timer timer;
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

	mem::VirtualMemory dllFullPathAddr(hProcess, dllPath, mem::VirtualMemory::kUnicode, true);
	d.dllFullPathAddr = dllFullPathAddr;

	d.loadLibraryWPtr = getProcAddressIn32BitProcess(hProcess, "kernel32.dll", "LoadLibraryW");
	d.getLastErrorPtr = getProcAddressIn32BitProcess(hProcess, "kernel32.dll", "GetLastError");
	d.getModuleHandleWPtr = getProcAddressIn32BitProcess(hProcess, "kernel32.dll", "GetModuleHandleW");

	//寫入待傳遞給CallBack的數據
	mem::VirtualMemory injectdata(hProcess, sizeof(InjectData), true);
	mem::write(hProcess, injectdata, &d, sizeof(InjectData));

	//寫入匯編版的CallBack函數
	mem::VirtualMemory remoteFunc(hProcess, sizeof(data), true);
	mem::write(hProcess, remoteFunc, data, sizeof(data));

	qDebug() << "time:" << timer.cost() << "ms";
	timer.restart();

	//遠程執行線程
	{
		ScopedHandle hThreadHandle(
			ScopedHandle::CREATE_REMOTE_THREAD,
			hProcess,
			reinterpret_cast<PVOID>(static_cast<unsigned long long>(remoteFunc)),
			reinterpret_cast<LPVOID>(static_cast<unsigned long long>(injectdata)));

		if (!hThreadHandle.isValid())
		{
			SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(index);
			emit signalDispatcher.messageBoxShow(QObject::tr("Create remote thread failed"), QMessageBox::Icon::Critical);
			return false;
		}

		util::timer timer;
		for (;;)
		{
			mem::read(hProcess, injectdata, sizeof(InjectData), &d);

			if (d.remoteModule != NULL)
				break;

			if (timer.hasExpired(3000))
				break;

			if (!mem::isProcessExist(dwProcessId))
				return false;

			if (hWnd != nullptr)
			{
				if (!IsWindow(hWnd))
					return false;
			}

			QThread::msleep(200);
		}
	}

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
		*phDllModule = reinterpret_cast<HMODULE>(static_cast<long long>(d.remoteModule));

	if (phGameModule != nullptr)
		*phGameModule = d.gameModule;

	qDebug() << "inject OK" << "0x" + util::toQString(d.remoteModule, 16) << "time:" << timer.cost() << "ms";
	return d.gameModule > 0 && d.remoteModule > 0;
}

#if 0
bool __fastcall mem::inject(long long index, HANDLE hProcess, QString dllPath, HMODULE* phDllModule, unsigned long long* phGameModule)
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
	mem::VirtualMemory dllfullpathaddr(hProcess, dllPath, mem::VirtualMemory::kUnicode, true);
	d.dllFullPathAddr = dllfullpathaddr;

	mem::VirtualMemory injectdata(hProcess, sizeof(InjectData), true);
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

	mem::VirtualMemory remoteFunc(hProcess, 100, true);
	mem::write(hProcess, remoteFunc, const_cast<char*>("\x8B\x44\x24\x04\x8B\xD8\x8B\x48\x0C\x51\xFF\x13\x89\x43\x10\x8B\x43\x04\xFF\xD0\x89\x43\x14\x8B\x43\x08\x6A\x00\xFF\xD0\x89\x43\x18\xC3"), 36);

	{
		ScopedHandle hThreadHandle(
			ScopedHandle::CREATE_REMOTE_THREAD,
			hProcess,
			reinterpret_cast<PVOID>(static_cast<unsigned long long>(remoteFunc)),
			reinterpret_cast<LPVOID>(static_cast<unsigned long long>(injectdata)));
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
#endif

bool __fastcall mem::enumProcess(QVector<long long>* pprocesses, const QString& moduleName, const QString& withoutModuleName)
{
	// 创建一个进程快照
	ScopedHandle hSnapshot(ScopedHandle::CREATE_TOOLHELP32_SNAPSHOT, TH32CS_SNAPPROCESS, 0);
	if (!hSnapshot.isValid())
		return false;

	PROCESSENTRY32W pe32 = {};
	pe32.dwSize = sizeof(PROCESSENTRY32W);

	// 遍历进程快照
	if (Process32FirstW(hSnapshot, &pe32))
	{
		do
		{
			// 打开进程以获取模块信息
			ScopedHandle hProcess(pe32.th32ProcessID);
			if (!hProcess.isValid())
				continue;

			// 获取进程模块信息
			HMODULE hModules[1024];
			DWORD cbNeeded;
			if (K32EnumProcessModules(hProcess, hModules, sizeof(hModules), &cbNeeded) == FALSE)
				continue;

			bool bret = false;
			for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
			{
				TCHAR szModule[MAX_PATH];
				if (K32GetModuleBaseNameW(hProcess, hModules[i], szModule, sizeof(szModule) / sizeof(TCHAR)) == 0)
					continue;

				QString moduleNameStr = QString::fromWCharArray(szModule);
				if (!moduleName.isEmpty())
				{
					if (!moduleNameStr.contains(moduleName, Qt::CaseInsensitive))
						continue;

					bret = true;
				}

				if (!withoutModuleName.isEmpty())
				{
					if (moduleNameStr.contains(withoutModuleName, Qt::CaseInsensitive))
					{
						bret = false;
						break;
					}
				}
			}

			if (bret)
			{
				if (pprocesses != nullptr)
					pprocesses->append(static_cast<long long>(pe32.th32ProcessID));
			}

		} while (Process32NextW(hSnapshot, &pe32));
	}

	if (pprocesses != nullptr)
		return !pprocesses->isEmpty();
	else
		return false;
}

bool __fastcall mem::isProcessExist(long long pid)
{
	ScopedHandle hProcess(pid);
	return hProcess.isValid();
}
#pragma endregion

#pragma region Config
QHash<QString, QMutex*> g_fileLockHash;

static void __fastcall tryLock(const QString& fileName)
{
	if (!g_fileLockHash.contains(fileName))
	{
		std::unique_ptr<QMutex> mutex(new QMutex());
		sash_assume(mutex != nullptr);
		if (mutex == nullptr)
			return;
		g_fileLockHash.insert(fileName, mutex.release());
	}

	g_fileLockHash.value(fileName)->lock();
}

static void __fastcall releaseLock(const QString& fileName)
{
	if (g_fileLockHash.contains(fileName))
	{
		g_fileLockHash.value(fileName)->unlock();
	}
}

util::Config::Config(const QString& callBy)
	: fileName_(QString::fromUtf8(qgetenv("JSON_PATH")))
{
	callby = callBy;
	//qDebug() << "============= callBy::::" << callBy;
	tryLock(fileName_);
	file_.setFileName(fileName_);
	isVaild = open();
}

util::Config::Config(const QString& fileName, const QString& callBy)
	: fileName_(fileName)
{
	callby = callBy;
	//qDebug() << "============= callBy::::" << callBy;
	tryLock(fileName_);
	file_.setFileName(fileName_);
	isVaild = open();
}

util::Config::Config(const QByteArray& fileName, const QString& callBy)
	: fileName_(QString::fromUtf8(fileName))
{
	callby = callBy;
	//qDebug() << "============= callBy::::" << callBy;
	tryLock(fileName_);
	file_.setFileName(fileName_);
	isVaild = open();
}

util::Config::~Config()
{
	sync();//同步數據
	//qDebug() << "============= callBy::::" << callby << "has unlock";
	releaseLock(fileName_);
}

bool util::Config::open()
{
	//緩存存在則直接從緩存讀取
	if (cacheHash_.contains(fileName_))
	{
		cache_ = cacheHash_.value(fileName_);
		return true;
	}

	if (!file_.exists())
	{
		//文件不存在，以全新文件複寫方式打開
		if (file_.openWriteNew())
		{
			//成功直接返回因為是全新的不需要往下讀
			return true;
		}
		else
			return false;

	}
	else
	{
		//文件存在，以普通讀寫方式打開
		if (!file_.openReadWrite())
		{
			return false;
		}
	}

	QByteArray allData = file_.readAll();

	//第一次使用所以是空的不需要解析成json
	if (allData.simplified().isEmpty())
	{
		return true;
	}

	//解析json
	QJsonParseError jsonError;
	document_ = QJsonDocument::fromJson(allData, &jsonError);
	if (jsonError.error != QJsonParseError::NoError)
	{
		//解析錯誤則直接複寫新的
		if (file_.isOpen())
			file_.close();

		if (file_.openWriteNew())
			return true;//成功直接返回因為是全新的不需要往下讀
		else
			return false;
	}

	//把json轉換成qvariantmap
	QJsonObject root = document_.object();
	cache_ = root.toVariantMap();

	//緩存起來
	cacheHash_.insert(fileName_, cache_);

	return true;
}

void util::Config::sync()
{
	if (!hasChanged_)
	{
		return;
	}

	cacheHash_.insert(fileName_, cache_);

	QJsonObject root = QJsonObject::fromVariantMap(cache_);
	document_.setObject(root);
	QByteArray data = document_.toJson(QJsonDocument::Indented);

	if (!file_.openWriteNew())
	{
		return;
	}

	file_.write(data);
	file_.flush();
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

void util::Config::writeMapData(const QString&, const MapData& data)
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
QList<util::Config::MapData> util::Config::readMapData(const QString& key) const
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

void __fastcall util::FormSettingManager::loadSettings()
{
	Config config(QString("%1|%2").arg(__FUNCTION__).arg(__LINE__));

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
		geometry = util::hexStringToByteArray(qstrGeometry);//DECODE

	if (mainwindow_ != nullptr)
	{
		QByteArray state;
		QString qstrState = config.read<QString>("Form", ObjectName, "State");
		if (!qstrState.isEmpty())
			state = util::hexStringToByteArray(qstrState);//DECODE
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

void __fastcall util::FormSettingManager::saveSettings()
{
	Config config(QString("%1|%2").arg(__FUNCTION__).arg(__LINE__));
	QString ObjectName;
	QString qstrGeometry;
	QString qstrState;
	QSize size;
	if (mainwindow_ != nullptr)
	{
		ObjectName = mainwindow_->objectName();
		qstrGeometry = util::byteArrayToHexString(mainwindow_->saveGeometry());//ENCODE
		qstrState = util::byteArrayToHexString(mainwindow_->saveState(3));//ENCODE
		size = mainwindow_->size();
		config.write("Form", ObjectName, "State", qstrState);
	}
	else
	{
		ObjectName = widget_->objectName();
		qstrGeometry = util::byteArrayToHexString(widget_->saveGeometry());//ENCODE
		size = widget_->size();
	}

	config.write("Form", ObjectName, "Geometry", qstrGeometry);
	config.write("Form", ObjectName, "Size", QString("%1,%2").arg(size.width()).arg(size.height()));
}

QFileInfoList __fastcall util::loadAllFileLists(
	TreeWidgetItem* root,
	const QString& path,
	const QString& suffix,
	const QString& icon,
	QStringList* list,
	const QString& folderIcon)
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
		QString content;
		if (!readFile(item.absoluteFilePath(), &content))
			continue;

		std::unique_ptr<TreeWidgetItem> child(q_check_ptr(new TreeWidgetItem(QStringList{ item.fileName() }, 1)));
		sash_assume(child != nullptr);
		if (child == nullptr)
			continue;

		child->setData(0, Qt::UserRole, "file");
		child->setToolTip(0, QString("===== %1 =====\n\n%2").arg(item.absoluteFilePath()).arg(content.left(256)));
		child->setIcon(0, QIcon(icon));

		if (root != nullptr)
			root->addChild(child.release());

		if (list != nullptr)
			list->append(item.fileName());
	}

	QFileInfoList file_list = dir.entryInfoList(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
	const QFileInfoList folder_list = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot); //獲取當前所有目錄
	long long count = folder_list.size();
	for (long long i = 0; i != count; ++i) //自動遞歸添加各目錄到上一級目錄
	{
		const QString namepath = folder_list.value(i).absoluteFilePath(); //獲取路徑
		const QFileInfo folderinfo = folder_list.value(i);
		const QString name = folderinfo.fileName(); //獲取目錄名
		if (list != nullptr)
			list->append(name);

		std::unique_ptr<TreeWidgetItem> childroot(q_check_ptr(new TreeWidgetItem(QStringList{ name }, 1)));
		sash_assume(childroot != nullptr);
		if (childroot == nullptr)
			continue;

		childroot->setToolTip(0, namepath);
		childroot->setData(0, Qt::UserRole, "folder");
		childroot->setIcon(0, QIcon(folderIcon));

		TreeWidgetItem* item = childroot.release();

		if (root != nullptr)
			root->addChild(item); //將當前目錄添加成path的子項

		const QFileInfoList child_file_list = loadAllFileLists(item, namepath, suffix, icon, list, folderIcon); //遞歸添加子目錄
		file_list.append(child_file_list);
	}
	return file_list;
}

QFileInfoList __fastcall util::loadAllFileLists(
	TreeWidgetItem* root,
	const QString& path,
	QStringList* list,
	const QString& fileIcon,
	const QString& folderIcon)
{
	/*添加path路徑文件*/
	QDir dir(path); //遍歷各級子目錄
	QDir dir_file(path); //遍歷子目錄中所有文件
	dir_file.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks); //獲取當前所有文件
	dir_file.setSorting(QDir::Size | QDir::Reversed);

	const QStringList filters = {
		QString("*%1").arg(util::SCRIPT_DEFAULT_SUFFIX),
		QString("*%1").arg(util::SCRIPT_PRIVATE_SUFFIX_DEFAULT),
	};

	dir_file.setNameFilters(filters);
	QFileInfoList list_file = dir_file.entryInfoList();

	for (const QFileInfo& item : list_file)
	{
		std::unique_ptr<TreeWidgetItem> child(q_check_ptr(new TreeWidgetItem(QStringList{ item.fileName() }, 1)));
		sash_assume(child != nullptr);
		if (child == nullptr)
			continue;

		QString content;
		if (!readFile(item.absoluteFilePath(), &content))
			break;

		child->setText(0, item.fileName());
		child->setData(0, Qt::UserRole, "file");
		child->setToolTip(0, QString("===== %1 =====\n\n%2").arg(item.absoluteFilePath()).arg(content.left(256)));
		child->setIcon(0, QIcon(fileIcon));

		if (root != nullptr)
			root->addChild(child.release());

		if (list != nullptr)
			list->append(item.fileName());
	}


	QFileInfoList file_list = dir.entryInfoList(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
	const QFileInfoList folder_list = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot); //獲取當前所有目錄
	long long count = folder_list.size();

	//自動遞歸添加各目錄到上一級目錄
	for (long long i = 0; i != count; ++i)
	{
		const QString namepath = folder_list.value(i).absoluteFilePath(); //獲取路徑
		const QFileInfo folderinfo = folder_list.value(i);
		const QString name = folderinfo.fileName(); //獲取目錄名

		std::unique_ptr<TreeWidgetItem> childroot(q_check_ptr(new TreeWidgetItem()));
		sash_assume(childroot != nullptr);
		if (childroot == nullptr)
			continue;

		childroot->setText(0, name);
		childroot->setToolTip(0, namepath);
		childroot->setData(0, Qt::UserRole, "folder");
		childroot->setIcon(0, QIcon(folderIcon));

		TreeWidgetItem* item = childroot.release();

		//將當前目錄添加成path的子項
		if (root != nullptr)
			root->addChild(item);

		if (list != nullptr)
			list->append(name);

		const QFileInfoList child_file_list(loadAllFileLists(item, namepath, list, fileIcon, folderIcon)); //遞歸添加子目錄
		file_list.append(child_file_list);
	}

	return file_list;
}

void __fastcall util::searchFiles(const QString& dir, const QString& fileNamePart, const QString& suffixWithDot, QStringList* presult, bool withcontent, bool isExact)
{
	QDir d(dir);
	if (!d.exists())
		return;

	QFileInfoList list = d.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
	for (const QFileInfo& fileInfo : list)
	{
		if (fileInfo.isFile())
		{
			// Simplified extension check
			QString suffix = suffixWithDot.startsWith('.') ? suffixWithDot.mid(1).toLower() : suffixWithDot.toLower();
			QString fileName = fileInfo.fileName().toLower();
			//remove suffix from fileName
			if (fileName.endsWith(suffix))
			{
				fileName = fileName.left(fileName.length() - suffix.length());
				if (fileName.endsWith('.'))
					fileName = fileName.left(fileName.length() - 1);
			}

			if (!suffix.isEmpty() && fileInfo.suffix().toLower() != suffix)
				continue;

			if ((!fileName.isEmpty() && !isExact && !fileName.contains(fileNamePart, Qt::CaseInsensitive))
				|| (!fileName.isEmpty() && isExact && fileName.toLower() != fileNamePart.toLower()))
				continue;

			if (withcontent)
			{
				QString content;
				// Assuming readFile is a function that reads the file content
				if (!util::readFile(fileInfo.absoluteFilePath(), &content))
					continue;

				QString fileContent = QString("# %1\n---\n%2").arg(fileInfo.fileName()).arg(content);
				presult->append(fileContent);
			}
			else
			{
				presult->append(fileInfo.absoluteFilePath());
			}
		}
		else if (fileInfo.isDir())
		{
			// Recursively search in subdirectories
			searchFiles(fileInfo.absoluteFilePath(), fileNamePart, suffixWithDot, presult, withcontent, isExact);
		}
	}
}

bool __fastcall util::enumAllFiles(const QString& dir, const QString& suffix, QVector<QPair<QString, QString>>* result)
{
	if (result == nullptr)
	{
		return false;
	}

	QDir directory(dir);
	if (!directory.exists())
	{
		return false; // 目錄不存在時返回 false
	}

	QFileInfoList fileInfoList = directory.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDir::DirsFirst);
	for (const QFileInfo& fileInfo : fileInfoList)
	{
		if (fileInfo.isFile())
		{
			QString fileName = fileInfo.fileName();
			QString filePath = fileInfo.filePath();

			if (!suffix.isEmpty() && !fileName.endsWith(suffix, Qt::CaseInsensitive))
			{
				continue; // 跳過不匹配後綴的文件
			}

			result->append(qMakePair(fileName, filePath));
		}
		else if (fileInfo.isDir())
		{
			// 遞歸處理子目錄
			enumAllFiles(fileInfo.filePath(), suffix, result); // 不檢查返回值
		}
	}

	return true;
}

//自身進程目錄 遞歸遍查找指定文件
QString __fastcall util::findFileFromName(const QString& fileName, const QString& dirpath)
{
	QDir dir(dirpath);
	if (!dir.exists())
		return QString();

	dir.setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
	dir.setSorting(QDir::DirsFirst);
	QFileInfoList list = dir.entryInfoList();
	long long size = list.size();
	for (long long i = 0; i < size; ++i)
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

void __fastcall util::sortWindows(const QVector<HWND>& windowList, bool alignLeft)
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

bool __fastcall util::writeFireWallOverXP(const LPCTSTR& ruleName, const LPCTSTR& appPath, bool NoopIfExist)
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

bool __fastcall util::monitorThreadResourceUsage(FILETIME& preidleTime, FILETIME& prekernelTime, FILETIME& preuserTime, double* pCpuUsage, double* pMemUsage, double* pMaxMemUsage)
{
	FILETIME idleTime = {};
	FILETIME kernelTime = {};
	FILETIME userTime = {};

	if (GetSystemTimes(&idleTime, &kernelTime, &userTime) == TRUE)
	{
		ULARGE_INTEGER x = {};
		ULARGE_INTEGER y = {};

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

	PROCESS_MEMORY_COUNTERS_EX memCounters = {};
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

QFont __fastcall util::getFont()
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
	font.setWeight(QFont::Weight::Medium);
	font.setWordSpacing(0.0);
	return font;
}

void __fastcall util::asyncRunBat(const QString& path, QString data)
{
	QDateTime dt = QDateTime::currentDateTime();
	//to number only datetime string
	const QString batfile = QString("%1/sash_%2.bat").arg(path).arg(QDateTime::currentDateTime().toString("yyyyMMdd"));
	if (QFile::exists(batfile))
		QFile::remove(batfile);

	qDebug() << "asyncRunBat: " << batfile;
	if (util::writeFile(batfile, data, false))
		ShellExecuteW(NULL, L"open", (LPCWSTR)batfile.utf16(), NULL, NULL, SW_HIDE);
	else
		qDebug() << "asyncRunBat error: " << batfile;
}

QString __fastcall util::applicationFilePath()
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

		if (path.contains("\\"))
			path.replace("\\", "/");

		qputenv("CURRENT_PATH", path.toUtf8());
		init = true;
	}
	return QString::fromUtf8(qgetenv("CURRENT_PATH"));
}

QString __fastcall util::applicationDirPath()
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

QString __fastcall util::applicationName()
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

long long __fastcall util::percent(long long value, long long total)
{
	if (value == 1 && total > 0)
		return value;
	if (value == 0)
		return 0;

	double d = std::floor(static_cast<double>(value) * 100.0 / static_cast<double>(total));
	if ((value > 0) && (static_cast<long long>(d) == 0))
		return 1;
	else
		return static_cast<long long>(d);
}

QString __fastcall util::formatMilliseconds(long long milliseconds, bool noSpace)
{
	long long totalSeconds = milliseconds / 1000ll;
	long long days = totalSeconds / (24ll * 60ll * 60ll);
	long long hours = (totalSeconds % (24ll * 60ll * 60ll)) / (60ll * 60ll);
	long long minutes = (totalSeconds % (60ll * 60ll)) / 60ll;
	long long seconds = totalSeconds % 60ll;
	long long remainingMilliseconds = milliseconds % 1000ll;

	if (!noSpace)
	{
		return QString(QObject::tr("%1 day %2 hour %3 min %4 sec %5 msec"))
			.arg(days).arg(hours).arg(minutes).arg(seconds).arg(remainingMilliseconds);
	}
	else
	{
		return QString(QObject::tr("%1d%2h%3m%4s"))
			.arg(days).arg(hours).arg(minutes).arg(seconds);
	}
}

QString __fastcall util::formatSeconds(long long seconds)
{
	long long day = seconds / 86400ll;
	long long hours = (seconds % 86400ll) / 3600ll;
	long long minutes = (seconds % 3600ll) / 60ll;
	long long remainingSeconds = seconds % 60ll;

	return QString(QObject::tr("%1 day %2 hour %3 min %4 sec")).arg(day).arg(hours).arg(minutes).arg(remainingSeconds);
};

bool __fastcall util::readFileFilter(const QString& fileName, QString& content, bool* pisPrivate)
{
	if (pisPrivate != nullptr)
		*pisPrivate = false;

	if (fileName.endsWith(util::SCRIPT_PRIVATE_SUFFIX_DEFAULT))
	{
#ifdef CRYPTO_H
		Crypto crypto;
		if (!crypto.decodeScript(fileName, content))
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

bool __fastcall util::readFile(const QString& fileName, QString* pcontent, bool* pisPrivate, QString* originalData)
{
	QFileInfo fi(fileName);
	if (!fi.exists())
		return false;

	if (fi.isDir())
		return false;

	util::ScopedFile f(fileName);
	if (!f.openRead())
		return false;

	QByteArray ba = f.readAll();
	QString content = QString::fromUtf8(ba);

	if (originalData != nullptr)
		*originalData = content;

	if (readFileFilter(fileName, content, pisPrivate))
	{
		if (pcontent != nullptr)
			*pcontent = content;
		return true;
	}

	return false;
}

bool __fastcall util::writeFile(const QString& fileName, const QString& content, bool isLocal)
{
	{
		util::ScopedFile f(fileName);
		if (!f.openWriteNew())
			return false;

		if (!isLocal)
		{
			QByteArray ba = content.toUtf8();
			f.write(ba);
			f.flush();
		}
		else
		{
			QTextStream stream(&f);
			UINT acp = GetACP();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
			if (acp == 950)
				stream.setCodec("Big5");
			else if (acp == 936)
				stream.setCodec("GBK");
			else
				stream.setCodec("UTF-8");
#else
			stream.setEncoding(QStringConverter::Encoding::System);
#endif

			stream << content;
			stream.flush();
		}

		if (f.error() != QFile::NoError)
		{
			qDebug() << "writeFile error: " << f.errorString();
			return false;
		}
	}

	return QFileInfo::exists(fileName);
}

// 將二進制數據轉換為16進制字符串
QString __fastcall util::byteArrayToHexString(const QByteArray& data)
{
	QString hexString;
	for (char byte : data)
	{
		hexString.append(QString("%1").arg(static_cast<unsigned char>(byte), 2, 16, QChar('0')));
	}
	return hexString;
}

// 將16進制字符串轉換為二進制數據
QByteArray __fastcall util::hexStringToByteArray(const QString& hexString)
{
	QByteArray byteArray;
	long long length = hexString.length();
	for (long long i = 0; i < length; i += 2)
	{
		bool ok;
		QString byteString = hexString.mid(i, 2);
		char byte = static_cast<char>(byteString.toInt(&ok, 16));
		if (!ok)
		{
			// 處理轉換失敗的情況
			return QByteArray();
		}
		byteArray.append(byte);
	}
	return byteArray;
}

bool __fastcall util::fileDialogShow(const QString& name, long long acceptType, QString* retstring, QWidget* pparent)
{
	if (retstring != nullptr)
		retstring->clear();

	QFileDialog dialog(pparent);

	dialog.setAttribute(Qt::WA_QuitOnClose);
	dialog.setModal(false);
	dialog.setAcceptMode(static_cast<QFileDialog::AcceptMode>(acceptType));

	QFileInfo fileInfo(name);
	dialog.setDefaultSuffix(fileInfo.suffix());

	dialog.setFileMode(QFileDialog::AnyFile);
	//dialog->setFilter(QDir::Filters::
	//dialog->setHistory(const QStringList & paths)
	//dialog->setIconProvider(QFileIconProvider * provider)
	//dialog->setItemDelegate(QAbstractItemDelegate * delegate)
	dialog.setLabelText(QFileDialog::LookIn, QObject::tr("Look in:"));
	dialog.setLabelText(QFileDialog::FileName, QObject::tr("File name:"));
	dialog.setLabelText(QFileDialog::FileType, QObject::tr("File type:"));
	dialog.setLabelText(QFileDialog::Accept, QObject::tr("Open"));
	dialog.setLabelText(QFileDialog::Reject, QObject::tr("Cancel"));

	dialog.setNameFilter("*.txt *.lua *.json *.exe");

	if (!name.isEmpty())
	{
		QStringList filters;
		filters << name;
		dialog.setNameFilters(filters);
	}

	dialog.setOption(QFileDialog::ShowDirsOnly, false);
	dialog.setOption(QFileDialog::DontResolveSymlinks, true);
	dialog.setOption(QFileDialog::DontConfirmOverwrite, true);
	dialog.setOption(QFileDialog::DontUseNativeDialog, true);
	dialog.setOption(QFileDialog::ReadOnly, true);
	dialog.setOption(QFileDialog::HideNameFilterDetails, false);
	dialog.setOption(QFileDialog::DontUseCustomDirectoryIcons, true);

	//dialog->setProxyModel(QAbstractProxyModel * proxyModel)
	QList<QUrl> urls;
	urls << QUrl::fromLocalFile(util::applicationDirPath())
		<< QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::MusicLocation).first());

	dialog.setSidebarUrls(urls);
	dialog.setSupportedSchemes(QStringList());
	dialog.setViewMode(QFileDialog::ViewMode::List);

	//directory
	//自身目錄往上一層
	QString directory = util::applicationDirPath();
	directory = QDir::toNativeSeparators(directory);
	directory = QDir::cleanPath(directory + QDir::separator() + "..");
	dialog.setDirectory(directory);

	do
	{
		if (dialog.exec() != QDialog::Accepted)
			break;

		QStringList fileNames = dialog.selectedFiles();
		if (fileNames.isEmpty())
			break;

		QString fileName = fileNames.value(0);
		if (fileName.isEmpty())
			break;

		if (retstring != nullptr)
			*retstring = fileName;

		return true;
	} while (false);

	return false;
}