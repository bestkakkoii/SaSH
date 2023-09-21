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

bool mem::read(HANDLE hProcess, quint64 desiredAccess, SIZE_T size, PVOID buffer)
{
	if (!size)return FALSE;
	if (!buffer) return FALSE;
	if (!hProcess) return FALSE;
	if (!desiredAccess) return FALSE;
	//ULONG oldProtect = NULL;
	//VirtualProtectEx(m_pi.hProcess, buffer, size, PAGE_EXECUTE_READWRITE, &oldProtect);
	BOOL ret = NT_SUCCESS(MINT::NtReadVirtualMemory(hProcess, reinterpret_cast<PVOID>(desiredAccess), buffer, size, NULL));
	//VirtualProtectEx(m_pi.hProcess, buffer, size, oldProtect, &oldProtect);
	return ret == TRUE;
}

template<typename T, typename>
T mem::read(HANDLE hProcess, quint64 desiredAccess)
{
	if (!hProcess) return T();

	T buffer{};
	SIZE_T size = sizeof(T);
	if (!size) return T();
	BOOL ret = mem::read(hProcess, desiredAccess, size, &buffer);
	return ret ? buffer : T();
}

float mem::readFloat(HANDLE hProcess, quint64 desiredAccess)
{
	if (!hProcess) return 0.0f;
	if (!desiredAccess) return 0.0f;
	float buffer = 0;
	SIZE_T size = sizeof(float);
	BOOL ret = read(hProcess, desiredAccess, size, &buffer);
	return (ret) ? (buffer) : 0.0f;
}

qreal mem::readDouble(HANDLE hProcess, quint64 desiredAccess)
{
	if (!hProcess) return 0.0;
	if (!desiredAccess) return 0.0;
	qreal buffer = 0;
	SIZE_T size = sizeof(qreal);
	BOOL ret = read(hProcess, desiredAccess, size, &buffer);
	return (ret == TRUE) ? (buffer) : 0.0;
}

QString mem::readString(HANDLE hProcess, quint64 desiredAccess, int size, bool enableTrim, bool keepOriginal)
{
	if (!hProcess) return "\0";
	if (!desiredAccess) return "\0";

	QScopedArrayPointer <char> p(q_check_ptr(new char[size + 1]));
	memset(p.get(), 0, size + 1);
	SIZE_T sizet = size;
	BOOL ret = read(hProcess, desiredAccess, sizet, p.get());
	if (!keepOriginal)
	{
		std::string s = p.get();
		QString retstring = (ret == TRUE) ? (util::toUnicode(s.c_str(), enableTrim)) : "";
		return retstring;
	}
	else
	{
		QString retstring = (ret == TRUE) ? QString(p.get()) : "";
		return retstring;
	}

}

bool mem::write(HANDLE hProcess, quint64 baseAddress, PVOID buffer, SIZE_T dwSize)
{
	if (!hProcess) return FALSE;
	ULONG oldProtect = NULL;

	VirtualProtectEx(hProcess, (LPVOID)baseAddress, dwSize, PAGE_EXECUTE_READWRITE, &oldProtect);
	BOOL ret = WriteProcessMemory(hProcess, reinterpret_cast<PVOID>(baseAddress), buffer, dwSize, NULL);
	VirtualProtectEx(hProcess, (LPVOID)baseAddress, dwSize, oldProtect, &oldProtect);
	return ret == TRUE;
}

template<typename T, typename>
bool mem::write(HANDLE hProcess, quint64 baseAddress, T data)
{
	if (!hProcess) return false;
	if (!baseAddress) return false;
	PVOID pBuffer = &data;
	BOOL ret = write(hProcess, baseAddress, pBuffer, sizeof(T));
	return ret == TRUE;
}

bool mem::writeString(HANDLE hProcess, quint64 baseAddress, const QString& str)
{
	if (!hProcess) return FALSE;
	QTextCodec* codec = QTextCodec::codecForName(util::DEFAULT_GAME_CODEPAGE);//QTextCodec::codecForMib(2025);//
	QByteArray ba = codec->fromUnicode(str);
	ba.append('\0');
	char* pBuffer = ba.data();
	SIZE_T len = ba.size();
	QScopedArrayPointer <char> p(q_check_ptr(new char[len + 1]()));
	memset(p.get(), 0, len + 1);
	_snprintf_s(p.get(), len + 1, _TRUNCATE, "%s\0", pBuffer);
	BOOL ret = write(hProcess, baseAddress, p.get(), len + 1);
	p.reset(nullptr);
	return ret == TRUE;
}

int mem::virtualAlloc(HANDLE hProcess, int size)
{
	if (!hProcess) return 0;
	DWORD ptr = NULL;
	SIZE_T sizet = static_cast<SIZE_T>(size);

	BOOL ret = NT_SUCCESS(MINT::NtAllocateVirtualMemory(hProcess, (PVOID*)&ptr, NULL, &sizet, MEM_COMMIT, PAGE_EXECUTE_READWRITE));
	if (ret == TRUE)
	{
		return static_cast<int>(ptr);
	}
	else
		return NULL;
	//return static_cast<int>(this->VirtualAllocEx(m_pi.nWnd, NULL, size, 0));
}

int mem::virtualAllocA(HANDLE hProcess, const QString& str)
{
	int ret = NULL;
	do
	{
		if (!hProcess)
			break;

		ret = virtualAlloc(hProcess, str.toLocal8Bit().size() * 2 * sizeof(char) + 1);
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

int mem::virtualAllocW(HANDLE hProcess, const QString& str)
{
	int ret = NULL;
	do
	{
		if (!hProcess)
			break;

		std::wstring wstr(str.toStdWString());
		wstr += L'\0';
		ret = virtualAlloc(hProcess, wstr.length() * sizeof(wchar_t) + 1);
		if (!ret)
			break;
		if (!write(hProcess, ret, (PVOID)wstr.c_str(), wstr.length() * sizeof(wchar_t) + 1))
		{
			virtualFree(hProcess, ret);
			ret = NULL;
			break;
		}
	} while (false);
	return ret;
}

bool mem::virtualFree(HANDLE hProcess, int baseAddress)
{
	if (!hProcess) return FALSE;
	if (baseAddress == NULL) return FALSE;
	SIZE_T size = 0;
	BOOL ret = NT_SUCCESS(MINT::NtFreeVirtualMemory(hProcess, (PVOID*)&baseAddress, &size, MEM_RELEASE));
	return ret == TRUE;
	//return static_cast<int>(this->VirtualFreeEx(m_pi.nWnd, static_cast<qlonglong>(baseAddress)));
}

DWORD mem::getRemoteModuleHandle(DWORD dwProcessId, const QString& moduleName)
{
	MODULEENTRY32 moduleEntry;
	//  获取进程快照中包含在th32ProcessID中指定的进程的所有的模块。
	ScopedHandle hSnapshot(ScopedHandle::CREATE_TOOLHELP32_SNAPSHOT, TH32CS_SNAPMODULE, dwProcessId);
	if (!hSnapshot.isValid()) return NULL;

	memset(&moduleEntry, 0, sizeof(MODULEENTRY32W));
	moduleEntry.dwSize = sizeof(MODULEENTRY32W);
	if (!Module32FirstW(hSnapshot, &moduleEntry))
		return NULL;
	else
	{
		const QString str(QString::fromWCharArray(moduleEntry.szModule));
		if (str == moduleName)
			return reinterpret_cast<DWORD>(moduleEntry.hModule);
	}

	do
	{
		const QString str(QString::fromWCharArray(moduleEntry.szModule));
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
	wchar_t szModName[MAX_PATH];
	RtlZeroMemory(szModName, sizeof(szModName));
	if (EnumProcessModulesEx(hProcess, hMods, sizeof(hMods), &cbNeeded, 3)) //http://msdn.microsoft.com/en-us/library/ms682633(v=vs.85).aspx
	{
		for (i = 0; i <= cbNeeded / sizeof(HMODULE); i++)
		{
			if (GetModuleFileNameExW(hProcess, hMods[i], szModName, sizeof(szModName)))
			{
				QString qfileName = QString::fromWCharArray(szModName);
				qfileName.replace("/", "\\");
				QFileInfo file(qfileName);
				//get file name only
				qDebug() << file.fileName();
				if (file.fileName().toLower() == szModuleName.toLower())
				{
					return hMods[i];
				}
			}
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
	RtlZeroMemory(strName, sizeof(strName));
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
		RtlZeroMemory(bFuncName, sizeof(bFuncName));
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

bool mem::inject64(qint64 index, HANDLE hProcess, QString dllPath, HMODULE* phDllModule, quint64* phGameModule)
{
	unsigned char data[128] = {
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
	};

	InjectData d;
	util::VirtualMemory dllFullPathAddr(hProcess, dllPath, util::VirtualMemory::kUnicode, true);
	d.dllFullPathAddr = dllFullPathAddr;

	d.loadLibraryWPtr = getProcAddressIn32BitProcess(hProcess, "kernel32.dll", "LoadLibraryW");
	d.getLastErrorPtr = getProcAddressIn32BitProcess(hProcess, "kernel32.dll", "GetLastError");
	d.getModuleHandleWPtr = getProcAddressIn32BitProcess(hProcess, "kernel32.dll", "GetModuleHandleW");

	util::VirtualMemory injectdata(hProcess, sizeof(InjectData), true);
	mem::write(hProcess, injectdata, &d, sizeof(InjectData));

	util::VirtualMemory remoteFunc(hProcess, sizeof(data), true);
	mem::write(hProcess, remoteFunc, data, sizeof(data));

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
		FormatMessageW( //取得錯誤訊息
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			d.lastError,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPWSTR)&d.lastError,
			0,
			NULL);

		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(index);
		emit signalDispatcher.messageBoxShow(QString::fromWCharArray(L"Inject fail, error code: %1, %2").arg(d.lastError).arg(reinterpret_cast<wchar_t*>(d.lastError)));
		return false;
	}

	if (phDllModule != nullptr)
		*phDllModule = reinterpret_cast<HMODULE>(d.remoteModule);

	if (phGameModule != nullptr)
		*phGameModule = d.gameModule;

	qDebug() << "inject OK" << "0x" + QString::number(d.remoteModule, 16);
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
	DWORD* kernel32Module = getKernel32();
	d.loadLibraryWPtr = getFunAddr(kernel32Module, "LoadLibraryW");
	d.getLastErrorPtr = getFunAddr(kernel32Module, "GetLastError");
	d.getModuleHandleWPtr = getFunAddr(kernel32Module, "GetModuleHandleW");
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
		FormatMessageW( //取得錯誤訊息
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			d.lastError,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPWSTR)&d.lastError,
			0,
			NULL);

		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(index);
		emit signalDispatcher.messageBoxShow(QString::fromWCharArray(L"Inject fail, error code: %1, %2").arg(d.lastError).arg(reinterpret_cast<wchar_t*>(d.lastError)));
		return false;
	}

	if (phDllModule != nullptr)
		*phDllModule = reinterpret_cast<HMODULE>(d.remoteModule);

	if (phGameModule != nullptr)
		*phGameModule = d.gameModule;

	qDebug() << "inject OK" << "0x" + QString::number(d.remoteModule, 16);
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
	QString key = QString::number(data.floor);
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
			size.setWidth(list.at(0).toLongLong());
			size.setHeight(list.at(1).toLongLong());
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

QFileInfoList util::loadAllFileLists(TreeWidgetItem* root, const QString& path, const QString& suffix, const QString& icon, QStringList* list)
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
	{ //將當前目錄中所有文件添加到treewidget中
		if (list)
			list->append(item.fileName());
		TreeWidgetItem* child = q_check_ptr(new TreeWidgetItem(QStringList{ item.fileName() }, 1));
		child->setIcon(0, QIcon(QPixmap(icon)));

		root->addChild(child);
	}

	QFileInfoList file_list = dir.entryInfoList(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
	const QFileInfoList folder_list = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot); //獲取當前所有目錄
	int count = folder_list.size();
	for (int i = 0; i != count; ++i) //自動遞歸添加各目錄到上一級目錄
	{
		const QString namepath = folder_list.at(i).absoluteFilePath(); //獲取路徑
		const QFileInfo folderinfo = folder_list.at(i);
		const QString name = folderinfo.fileName(); //獲取目錄名
		if (list)
			list->append(name);
		TreeWidgetItem* childroot = q_check_ptr(new TreeWidgetItem(QStringList{ name }, 0));
		childroot->setIcon(0, QIcon(QPixmap(":/image/icon_directory.png")));
		root->addChild(childroot); //將當前目錄添加成path的子項
		const QFileInfoList child_file_list = loadAllFileLists(childroot, namepath, suffix, icon, list); //遞歸添加子目錄
		file_list.append(child_file_list);
	}
	return file_list;
}

QFileInfoList util::loadAllFileLists(TreeWidgetItem* root, const QString& path, QStringList* list)
{
	/*添加path路徑文件*/
	QDir dir(path); //遍歷各級子目錄
	QDir dir_file(path); //遍歷子目錄中所有文件
	dir_file.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks); //獲取當前所有文件
	dir_file.setSorting(QDir::Size | QDir::Reversed);
	const QStringList filters = { QString("*%1").arg(util::SCRIPT_DEFAULT_SUFFIX), QString("*%1").arg(util::SCRIPT_PRIVATE_SUFFIX_DEFAULT), QString("*%1").arg(util::SCRIPT_LUA_SUFFIX_DEFAULT) };
	dir_file.setNameFilters(filters);
	QFileInfoList list_file = dir_file.entryInfoList();
	for (const QFileInfo& item : list_file)
	{ //將當前目錄中所有文件添加到treewidget中
		if (list)
			list->append(item.fileName());
		TreeWidgetItem* child = q_check_ptr(new TreeWidgetItem(QStringList{ item.fileName() }, 1));
		child->setIcon(0, QIcon(QPixmap(":/image/icon_txt.png")));

		root->addChild(child);
	}

	QFileInfoList file_list = dir.entryInfoList(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
	const QFileInfoList folder_list = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot); //獲取當前所有目錄
	int count = folder_list.size();
	for (int i = 0; i != count; ++i) //自動遞歸添加各目錄到上一級目錄
	{
		const QString namepath = folder_list.at(i).absoluteFilePath(); //獲取路徑
		const QFileInfo folderinfo = folder_list.at(i);
		const QString name = folderinfo.fileName(); //獲取目錄名
		if (list)
			list->append(name);
		TreeWidgetItem* childroot = q_check_ptr(new TreeWidgetItem(QStringList{ name }, 0));
		childroot->setIcon(0, QIcon(QPixmap(":/image/icon_directory.png")));
		root->addChild(childroot); //將當前目錄添加成path的子項
		const QFileInfoList child_file_list = loadAllFileLists(childroot, namepath); //進行遞歸
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
			if (fileInfo.fileName().contains(fileNamePart, Qt::CaseInsensitive) && fileInfo.suffix() == suffixWithDot.mid(1))
			{
				QFile file(fileInfo.absoluteFilePath());
				if (file.open(QIODevice::ReadOnly | QIODevice::Text))
				{
					QTextStream in(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
					in.setCodec(util::DEFAULT_CODEPAGE);
#else
					in.setEncoding(QStringConverter::Utf8);
#endif
					in.setGenerateByteOrderMark(true);
					//將文件名置於前方
					QString fileContent = QString("# %1\n---\n%2").arg(fileInfo.fileName()).arg(in.readAll());
					file.close();

					result->append(fileContent);
				}
			}
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
		QFileInfo fileInfo = list.at(i);
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
		HWND hwnd = windowList.at(i);

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

bool util::monitorThreadResourceUsage(quint64 threadId, double& lastCpuCost, double* pCpuUsage, double* pMemUsage, double* pMaxMemUsage)
{

	// FILETIME 是一个用两个32位字节表示时间值的结构体
	//  dwLowDateTime 低位32位时间值。
	//  dwHighDateTime 高位32位时间值
	static FILETIME preidleTime;
	static FILETIME prekernelTime;
	static FILETIME preuserTime;

	FILETIME idleTime;
	FILETIME kernelTime;
	FILETIME userTime;

	// 三个参数分别为 cpu空闲时间 内核进程占用时间 用户进程占用时间
	// 函数执行成功返回true 执行失败返回false
	bool k = GetSystemTimes(&idleTime, &kernelTime, &userTime);
	if (k)
	{
		quint64 x, y;
		double idle, kernel, user;

		x = static_cast<quint64>(preidleTime.dwHighDateTime << 31) | preidleTime.dwLowDateTime;
		y = static_cast<quint64>(idleTime.dwHighDateTime << 31) | idleTime.dwLowDateTime;
		idle = y - x;

		x = static_cast<quint64>(prekernelTime.dwHighDateTime << 31) | prekernelTime.dwLowDateTime;
		y = static_cast<quint64>(kernelTime.dwHighDateTime << 31) | kernelTime.dwLowDateTime;
		kernel = y - x;

		x = static_cast<quint64>(preuserTime.dwHighDateTime << 31) | preuserTime.dwLowDateTime;
		y = static_cast<quint64>(userTime.dwHighDateTime << 31) | userTime.dwLowDateTime;
		user = y - x;

		double cpuPercent = (kernel + user - idle) * 100 / (kernel + user);

		preidleTime = idleTime;
		prekernelTime = kernelTime;
		preuserTime = userTime;

		if (pCpuUsage != nullptr)
		{
			*pCpuUsage = cpuPercent;
		}
	}


	PROCESS_MEMORY_COUNTERS_EX memCounters = { 0 };
	if (!GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&memCounters, sizeof(memCounters))) {
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