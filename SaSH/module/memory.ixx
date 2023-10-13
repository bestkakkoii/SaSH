module;

#include <Windows.h>
#include <QString>
#include <tlhelp32.h>
#include <psapi.h>

#include <QFileInfo>
#include <QObject>
#include <QElapsedTimer>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QtCore/QTextCodec>
#else
#include <QtCore5Compat/QTextCodec>
#endif

#include <QElapsedTimer>

#ifndef MINT_USE_SEPARATE_NAMESPACE
#define MINT_USE_SEPARATE_NAMESPACE
#include <MINT/MINT.h>
#endif

export module memory;
import <type_traits>;
import utility;
import scoped;
import string;
import global;

namespace sash
{
	inline constexpr auto MINT_SUCCESS(auto status)
	{
		return status >= 0;
	}

	export namespace memory
	{
		export bool read(HANDLE hProcess, unsigned __int64 desiredAccess, unsigned __int64 size, PVOID buffer)
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
			BOOL ret = MINT_SUCCESS(MINT::NtReadVirtualMemory(hProcess, reinterpret_cast<PVOID>(desiredAccess), buffer, size, NULL));
			//VirtualProtectEx(m_pi.hProcess, buffer, size, oldProtect, &oldProtect);

			return ret == TRUE;
		}

		export template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T> && !std::is_pointer_v<T>>>
			[[nodiscard]] T read(HANDLE hProcess, unsigned __int64 desiredAccess)
		{
			if (hProcess == nullptr)
				return T();

			T buffer{};
			SIZE_T size = sizeof(T);

			if (!size)
				return T();

			BOOL ret = read(hProcess, desiredAccess, size, &buffer);

			return ret ? buffer : T();
		}

		export template char read<char>(HANDLE hProcess, unsigned __int64 desiredAccess);
		export template short read<short>(HANDLE hProcess, unsigned __int64 desiredAccess);
		export template int read<int>(HANDLE hProcess, unsigned __int64 desiredAccess);
		export template float read<float>(HANDLE hProcess, unsigned __int64 desiredAccess);
		export template long read<long>(HANDLE hProcess, unsigned __int64 desiredAccess);
		export template long long read<long long>(HANDLE hProcess, unsigned __int64 desiredAccess);
		export template unsigned char read<unsigned char>(HANDLE hProcess, unsigned __int64 desiredAccess);
		export template unsigned short read<unsigned short>(HANDLE hProcess, unsigned __int64 desiredAccess);
		export template unsigned int read<unsigned int>(HANDLE hProcess, unsigned __int64 desiredAccess);
		export template unsigned long read<unsigned long>(HANDLE hProcess, unsigned __int64 desiredAccess);
		export template unsigned long long read<unsigned long long>(HANDLE hProcess, unsigned __int64 desiredAccess);

		export bool write(HANDLE hProcess, unsigned __int64 baseAddress, PVOID buffer, unsigned __int64 dwSize)
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

		export template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T> && !std::is_pointer_v<T>>>
			bool write(HANDLE hProcess, unsigned __int64 baseAddress, T data)
		{
			if (hProcess == nullptr)
				return false;

			if (!baseAddress)
				return false;

			PVOID pBuffer = &data;
			BOOL ret = write(hProcess, baseAddress, pBuffer, sizeof(T));

			return ret == TRUE;
		}

		export template bool write<char>(HANDLE hProcess, unsigned __int64 baseAddress, char data);
		export template bool write<short>(HANDLE hProcess, unsigned __int64 baseAddress, short data);
		export template bool write<int>(HANDLE hProcess, unsigned __int64 baseAddress, int data);
		export template bool write<float>(HANDLE hProcess, unsigned __int64 baseAddress, float data);
		export template bool write<long>(HANDLE hProcess, unsigned __int64 baseAddress, long data);
		export template bool write<long long>(HANDLE hProcess, unsigned __int64 baseAddress, long long data);
		export template bool write<unsigned char>(HANDLE hProcess, unsigned __int64 baseAddress, unsigned char data);
		export template bool write<unsigned short>(HANDLE hProcess, unsigned __int64 baseAddress, unsigned short data);
		export template bool write<unsigned int>(HANDLE hProcess, unsigned __int64 baseAddress, unsigned int data);
		export template bool write<unsigned long>(HANDLE hProcess, unsigned __int64 baseAddress, unsigned long data);
		export template bool write<unsigned long long>(HANDLE hProcess, unsigned __int64 baseAddress, unsigned long long data);

		export [[nodiscard]] QString readString(HANDLE hProcess, unsigned __int64 desiredAccess, unsigned __int64 size, bool enableTrim = true, bool keepOriginal = false)
		{
			if (hProcess == nullptr)
				return "\0";

			if (!desiredAccess)
				return "\0";

			QScopedArrayPointer <char> p(new char[size + 1]());
			memset(p.get(), 0, size + 1);

			BOOL ret = read(hProcess, desiredAccess, size, p.get());
			if (!keepOriginal)
			{
				std::string s = p.get();
				QString retstring = (ret == TRUE) ? (toQUnicode(s.c_str(), enableTrim)) : "";
				return retstring;
			}

			QString retstring = (ret == TRUE) ? QString(p.get()) : "";
			return retstring;
		}

		export bool writeString(HANDLE hProcess, unsigned __int64 baseAddress, const QString& str)
		{
			if (hProcess == nullptr)
				return false;

			QTextCodec* codec = QTextCodec::codecForName(SASH_DEFAULT_GAME_CODEPAGE);//QTextCodec::codecForMib(2025);//

			QByteArray ba = codec->fromUnicode(str);
			ba.append('\0');

			char* pBuffer = ba.data();
			quint64 len = ba.size();

			QScopedArrayPointer <char> p(new char[len + 1]());
			memset(p.get(), 0, len + 1);

			_snprintf_s(p.get(), len + 1, _TRUNCATE, "%s\0", pBuffer);

			BOOL ret = write(hProcess, baseAddress, p.get(), len + 1);
			p.reset(nullptr);

			return ret == TRUE;
		}

		export bool virtualFree(HANDLE hProcess, unsigned __int64 baseAddress)
		{
			if (hProcess == nullptr)
				return false;

			if (baseAddress == NULL)
				return false;

			SIZE_T size = 0;
			BOOL ret = MINT_SUCCESS(MINT::NtFreeVirtualMemory(hProcess, reinterpret_cast<PVOID*>(&baseAddress), &size, MEM_RELEASE));

			return ret == TRUE;
		}

		export [[nodiscard]] unsigned __int64 virtualAlloc(HANDLE hProcess, unsigned __int64 size)
		{
			if (hProcess == nullptr)
				return 0;

			quint64 ptr = NULL;
			SIZE_T sizet = static_cast<SIZE_T>(size);

			BOOL ret = MINT_SUCCESS(MINT::NtAllocateVirtualMemory(hProcess, reinterpret_cast<PVOID*>(&ptr), NULL, &sizet, MEM_COMMIT, PAGE_EXECUTE_READWRITE));
			if (ret == TRUE)
				return static_cast<int>(ptr);

			return 0;
		}

		export [[nodiscard]] unsigned __int64 virtualAllocW(HANDLE hProcess, const QString& str)
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

		export [[nodiscard]] unsigned __int64 virtualAllocA(HANDLE hProcess, const QString& str)
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

		//遠程虛擬內存申請、釋放、或申請+寫入字符串(ANSI | UNICODE)
		export class VirtualMemory
		{
		public:
			enum VirtualEncode
			{
				kAnsi,
				kUnicode,
			};

			VirtualMemory() = default;

			VirtualMemory(HANDLE h, __int64 size, bool autoclear)
				: autoclear(autoclear)
				, hProcess(h)
			{

				lpAddress = virtualAlloc(h, size);
			}

			VirtualMemory(HANDLE h, const QString& str, VirtualEncode use_unicode, bool autoclear)
				: autoclear(autoclear)
			{

				lpAddress = (use_unicode) == VirtualMemory::kUnicode ? (virtualAllocW(h, str)) : (virtualAllocA(h, str));
				hProcess = h;
			}

			inline operator quint64() const
			{
				return this->lpAddress;
			}

			VirtualMemory& operator=(__int64 other)
			{
				this->lpAddress = other;
				return *this;
			}

			VirtualMemory* operator&()
			{
				return this;
			}

			// copy constructor
			VirtualMemory(const VirtualMemory& other)
				: autoclear(other.autoclear)
				, lpAddress(other.lpAddress)
			{
			}
			//copy assignment
			VirtualMemory& operator=(const VirtualMemory& other)
			{
				this->lpAddress = other.lpAddress;
				this->autoclear = other.autoclear;
				this->hProcess = other.hProcess;
				return *this;
			}
			// move constructor
			VirtualMemory(VirtualMemory&& other) noexcept
				: autoclear(other.autoclear)
				, lpAddress(other.lpAddress)
				, hProcess(other.hProcess)

			{
				other.lpAddress = 0;
			}
			// move assignment
			VirtualMemory& operator=(VirtualMemory&& other) noexcept
			{
				this->lpAddress = other.lpAddress;
				this->autoclear = other.autoclear;
				this->hProcess = other.hProcess;
				other.lpAddress = 0;
				return *this;
			}

			friend constexpr inline bool operator==(const VirtualMemory& p1, const VirtualMemory& p2)
			{
				return p1.lpAddress == p2.lpAddress;
			}
			friend constexpr inline bool operator!=(const VirtualMemory& p1, const VirtualMemory& p2)
			{
				return p1.lpAddress != p2.lpAddress;
			}

			virtual ~VirtualMemory()
			{
				do
				{
					if ((autoclear) && (this->lpAddress) && (hProcess))
					{

						write<unsigned char>(hProcess, this->lpAddress, 0);
						virtualFree(hProcess, this->lpAddress);
						this->lpAddress = NULL;
					}
				} while (false);
			}

			[[nodiscard]] inline bool isNull() const
			{
				return !lpAddress;
			}

			[[nodiscard]] inline bool isData(BYTE* data, __int64 size) const
			{
				QScopedArrayPointer <BYTE> _data(data);
				read(hProcess, lpAddress, size, _data.data());
				return memcmp(data, _data.data(), size) == 0;
			}

			inline void clear()
			{

				if ((this->lpAddress))
				{
					virtualFree(hProcess, (this->lpAddress));
					this->lpAddress = NULL;
				}
			}

			[[nodiscard]] inline bool isValid()const
			{
				return (this->lpAddress) != NULL;
			}

		private:
			bool autoclear = false;
			quint64 lpAddress = NULL;
			HANDLE hProcess = NULL;
		};

#ifndef _WIN64
		export [[nodiscard]] DWORD getRemoteModuleHandle(DWORD dwProcessId, const QString& moduleName)
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
				const QString str(toQString(moduleEntry.szModule));
				if (str == moduleName)
					return reinterpret_cast<DWORD>(moduleEntry.hModule);
			}

			do
			{
				const QString str(toQString(moduleEntry.szModule));
				if (str == moduleName)
					return reinterpret_cast<DWORD>(moduleEntry.hModule);
			} while (Module32NextW(hSnapshot, &moduleEntry));

			return NULL;
		}
#endif
		export void freeUnuseMemory(HANDLE hProcess)
		{
			SetProcessWorkingSetSizeEx(hProcess, static_cast<SIZE_T>(-1), static_cast<SIZE_T>(-1), 0);
			K32EmptyWorkingSet(hProcess);
		}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0) && defined(_M_IX86)



		export inline __declspec(naked) DWORD* getKernel32()
		{
			__asm
			{
				mov eax, fs: [0x30]
				mov eax, [eax + 0xC]
				mov eax, [eax + 0x1C]
				mov eax, [eax]
				mov eax, [eax]
				mov eax, [eax + 8]
				ret
			}
		}
#else
		export inline DWORD* getKernel32()
		{
			return (DWORD*)GetModuleHandleW(L"kernel32.dll");
		}
#endif

#if 0
#ifdef _WIN64
		export DWORD getFunAddr(const DWORD* DllBase, const char* FunName)
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

		export HMODULE getRemoteModuleHandleByProcessHandleW(HANDLE hProcess, const QString& szModuleName)
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

					QString qfileName = toQString(szModName);
					qfileName.replace("/", "\\");
					QFileInfo file(qfileName);
					if (file.fileName().toLower() != szModuleName.toLower())
						continue;

					return hMods[i];
				}
			}

			return NULL;
		}

		typedef struct _IAT_EAT_INFO
		{
			char ModuleName[256] = {};
			char FuncName[64] = {};
			ULONG64 Address = 0;
			ULONG64 RecordAddr = 0;
			ULONG64 ModBase = 0;//just for export table
		} IAT_EAT_INFO, * PIAT_EAT_INFO;

		long getProcessExportTable32(HANDLE hProcess, const QString& ModuleName, IAT_EAT_INFO tbinfo[], int tb_info_max)
		{
			ULONG64 muBase = 0, count = 0;
			PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)new BYTE[sizeof(IMAGE_DOS_HEADER)];
			PIMAGE_NT_HEADERS32 pNtHeaders = (PIMAGE_NT_HEADERS32)new BYTE[sizeof(IMAGE_NT_HEADERS32)];
			PIMAGE_EXPORT_DIRECTORY pExport = (PIMAGE_EXPORT_DIRECTORY)new BYTE[sizeof(IMAGE_EXPORT_DIRECTORY)];

			char strName[130] = {};
			memset(strName, 0, sizeof(strName));

			//拿到目標模塊的BASE
			muBase = static_cast<ULONG>(reinterpret_cast<__int64>(getRemoteModuleHandleByProcessHandleW(hProcess, ModuleName)));
			if (!muBase)
			{
				return 0;
			}

			//獲取IMAGE_DOS_HEADER
			read(hProcess, muBase, sizeof(IMAGE_DOS_HEADER), pDosHeader);
			//獲取IMAGE_NT_HEADERS
			read(hProcess, (muBase + pDosHeader->e_lfanew), sizeof(IMAGE_NT_HEADERS32), pNtHeaders);
			if (pNtHeaders->OptionalHeader.DataDirectory[0].VirtualAddress == 0)
			{
				return 0;
			}

			read(hProcess, (muBase + pNtHeaders->OptionalHeader.DataDirectory[0].VirtualAddress), sizeof(IMAGE_EXPORT_DIRECTORY), pExport);
			read(hProcess, (muBase + pExport->Name), sizeof(strName), strName);
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
				ulPointer = static_cast<ULONG64>(read<int>(hProcess, (muBase + pExport->AddressOfNames + i * static_cast<ULONG64>(sizeof(DWORD)))));
				memset(bFuncName, 0, sizeof(bFuncName));
				read(hProcess, (muBase + ulPointer), sizeof(bFuncName), bFuncName);
				usFuncId = read<short>(hProcess, (muBase + pExport->AddressOfNameOrdinals + i * sizeof(short)));
				ulPointer = static_cast<ULONG64>(read<int>(hProcess, (muBase + pExport->AddressOfFunctions + static_cast<ULONG64>(sizeof(DWORD)) * usFuncId)));
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

		export ULONG64 getProcAddressIn32BitProcess(HANDLE hProcess, const QString& ModuleName, const QString& FuncName)
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
		export bool injectByWin7(__int64 index, DWORD dwProcessId, HANDLE hProcess, QString dllPath, HMODULE* phDllModule, unsigned __int64* phGameModule)
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

			DWORD hGameModule = getRemoteModuleHandle(dwProcessId, QString(SASH_SUPPORT_GAMENAME));
			if (phGameModule != nullptr)
				*phGameModule = static_cast<quint64>(hGameModule);

			for (__int64 i = 0; i < 2; ++i)
			{
				timer.restart();
				VirtualMemory dllFullPathAddr(hProcess, dllPath, VirtualMemory::kUnicode, true);
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
					hModule = reinterpret_cast<HMODULE>(getRemoteModuleHandle(dwProcessId, QString(SASH_INJECT_DLLNAME) + ".dll"));
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

			if (hModule != nullptr)
				qDebug() << "inject OK" << "0x" + toQString(reinterpret_cast<__int64>(hModule), 16) << "time:" << timer.elapsed() << "ms";
			return true;
		}
#endif
		export bool injectBy64(__int64 index, HANDLE hProcess, QString dllPath, HMODULE* phDllModule, unsigned __int64* phGameModule)//兼容64位注入32位
		{
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

			QElapsedTimer timer; timer.start();

			VirtualMemory dllFullPathAddr(hProcess, dllPath, VirtualMemory::kUnicode, true);
			d.dllFullPathAddr = dllFullPathAddr;

			d.loadLibraryWPtr = getProcAddressIn32BitProcess(hProcess, "kernel32.dll", "LoadLibraryW");
			d.getLastErrorPtr = getProcAddressIn32BitProcess(hProcess, "kernel32.dll", "GetLastError");
			d.getModuleHandleWPtr = getProcAddressIn32BitProcess(hProcess, "kernel32.dll", "GetModuleHandleW");

			//寫入待傳遞給CallBack的數據
			VirtualMemory injectdata(hProcess, sizeof(InjectData), true);
			write(hProcess, injectdata, &d, sizeof(InjectData));

			//寫入匯編版的CallBack函數
			VirtualMemory remoteFunc(hProcess, sizeof(data), true);
			write(hProcess, remoteFunc, data, sizeof(data));

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
					QString msg = QObject::tr("Create remote thread failed");
					std::wstring wstr = msg.toStdWString();
					MessageBoxW(NULL, wstr.c_str(), L"Inject fail", MB_OK | MB_ICONERROR);
					return false;
				}
			}

			read(hProcess, injectdata, sizeof(InjectData), &d);
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
					QString msg = QObject::tr("Inject fail, error code from client: %1, %2").arg(d.lastError).arg(toQString(p));
					std::wstring wstr = msg.toStdWString();
					MessageBoxW(NULL, wstr.c_str(), L"Inject fail", MB_OK | MB_ICONERROR);
					LocalFree(p);
				}
				return false;
			}

			if (phDllModule != nullptr)
				*phDllModule = reinterpret_cast<HMODULE>(static_cast<__int64>(d.remoteModule));

			if (phGameModule != nullptr)
				*phGameModule = d.gameModule;

			qDebug() << "inject OK" << "0x" + toQString(d.remoteModule, 16) << "time:" << timer.elapsed() << "ms";
			return d.gameModule > 0 && d.remoteModule > 0;
		}
#if 0
		export bool inject(__int64 index, HANDLE hProcess, QString dllPath, HMODULE* phDllModule, unsigned __int64* phGameModule)//32注入32
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
			VirtualMemory dllfullpathaddr(hProcess, dllPath, VirtualMemory::kUnicode, true);
			d.dllFullPathAddr = dllfullpathaddr;

			VirtualMemory injectdata(hProcess, sizeof(InjectData), true);
			write(hProcess, injectdata, &d, sizeof(InjectData));

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

			VirtualMemory remoteFunc(hProcess, 100, true);
			write(hProcess, remoteFunc, const_cast<char*>("\x8B\x44\x24\x04\x8B\xD8\x8B\x48\x0C\x51\xFF\x13\x89\x43\x10\x8B\x43\x04\xFF\xD0\x89\x43\x14\x8B\x43\x08\x6A\x00\xFF\xD0\x89\x43\x18\xC3"), 36);

			{
				ScopedHandle hThreadHandle(
					ScopedHandle::CREATE_REMOTE_THREAD,
					hProcess,
					reinterpret_cast<PVOID>(static_cast<quint64>(remoteFunc)),
					reinterpret_cast<LPVOID>(static_cast<quint64>(injectdata)));
			}

			read(hProcess, injectdata, sizeof(InjectData), &d);
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
					emit signalDispatcher.messageBoxShow(QObject::tr("Inject fail, error code: %1, %2").arg(d.lastError).arg(toQString(p)), QMessageBox::Icon::Critical);
					LocalFree(p);
				}
				return false;
			}

			if (phDllModule != nullptr)
				*phDllModule = reinterpret_cast<HMODULE>(d.remoteModule);

			if (phGameModule != nullptr)
				*phGameModule = d.gameModule;

			qDebug() << "inject OK" << "0x" + toQString(d.remoteModule, 16);
			return d.gameModule > 0 && d.remoteModule > 0;
		}
#endif
		export bool enumProcess(QVector<__int64>* pprocesses, const QString& moduleName)
		{
			// 创建一个进程快照
			ScopedHandle hSnapshot(ScopedHandle::CREATE_TOOLHELP32_SNAPSHOT, TH32CS_SNAPPROCESS, 0);
			if (!hSnapshot.isValid())
				return false;

			PROCESSENTRY32W pe32 = { 0 };
			pe32.dwSize = sizeof(PROCESSENTRY32W);

			// 遍历进程快照
			if (Process32First(hSnapshot, &pe32))
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

					for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
					{
						TCHAR szModule[MAX_PATH];
						if (K32GetModuleBaseNameW(hProcess, hModules[i], szModule, sizeof(szModule) / sizeof(TCHAR)) == 0)
							continue;

						QString moduleNameStr = QString::fromWCharArray(szModule);
						if (!moduleNameStr.contains(moduleName, Qt::CaseInsensitive))
							continue;

						// 模块名称包含指定名称，将进程PID添加到QVector中
						if (pprocesses != nullptr)
							pprocesses->append(static_cast<__int64>(pe32.th32ProcessID));
					}
				} while (Process32Next(hSnapshot, &pe32));
			}

			return true;
		}
	}
}
