#include "pch.h"
#include "sadll.h"
//#include "autil.h"
#ifdef USE_ASYNC_TCP
#include "asyncclient.h"
#else
#include "syncclient.h"
#endif

#define USE_MINIDUMP

#pragma comment(lib, "detours.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")

constexpr const char* IPV6_DEFAULT = "::1";

WNDPROC g_OldWndProc = nullptr;
HWND g_MainHwnd = nullptr;
HMODULE g_hGameModule = nullptr;
HMODULE g_hDllModule = nullptr;
WCHAR g_szGameModulePath[MAX_PATH] = { 0 };
DWORD g_MainThreadId = 0;
HANDLE g_MainThreadHandle = nullptr;
HWND g_ParenthWnd = nullptr;

template<typename T>
inline T CONVERT_GAMEVAR(DWORD offset) { return (T)((reinterpret_cast<ULONG_PTR>(g_hGameModule) + offset)); }

//用於隱藏指定模塊
void GameService::hideModule(HMODULE hLibrary)
{
	using namespace MINT;
	PPEB_LDR_DATA pLdr = nullptr;
	PLDR_DATA_TABLE_ENTRY FirstModule = nullptr;
	PLDR_DATA_TABLE_ENTRY GurrentModule = nullptr;
	__try
	{
		__asm {
			mov esi, fs: [0x30]
			mov esi, [esi + 0x0C]
			mov pLdr, esi
		}

		if (pLdr == nullptr)
			return;

		FirstModule = reinterpret_cast<PLDR_DATA_TABLE_ENTRY>(pLdr->InLoadOrderModuleList.Flink);
		GurrentModule = FirstModule;
		while (!(GurrentModule->DllBase == hLibrary))
		{
			GurrentModule = reinterpret_cast<PLDR_DATA_TABLE_ENTRY>(GurrentModule->InLoadOrderLinks.Blink);
			if (GurrentModule == FirstModule)
				break;
		}
		if (GurrentModule->DllBase != hLibrary)
			return;

		(reinterpret_cast<PLDR_DATA_TABLE_ENTRY>((GurrentModule->InLoadOrderLinks.Flink))->InLoadOrderLinks.Blink) = GurrentModule->InLoadOrderLinks.Blink;
		(reinterpret_cast<PLDR_DATA_TABLE_ENTRY>((GurrentModule->InLoadOrderLinks.Blink))->InLoadOrderLinks.Flink) = GurrentModule->InLoadOrderLinks.Flink;

		memset(GurrentModule->FullDllName.Buffer, 0, GurrentModule->FullDllName.Length);
		memset(GurrentModule, 0, sizeof(PLDR_DATA_TABLE_ENTRY));
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return;
	}
}

#ifdef AUTIL_H
constexpr size_t LINEBUFSIZ = 8192;
int net_readbuflen = 0;
char net_readbuf[LINEBUFSIZ] = {};
char rpc_linebuffer[Autil::NETBUFSIZ] = {};

void clearNetBuffer()
{
	net_readbuflen = 0;
	memset(net_readbuf, 0, LINEBUFSIZ);
	memset(rpc_linebuffer, 0, Autil::NETBUFSIZ);
	Autil::util_Release();
}

int appendReadBuf(char* buf, int size)
{
	if ((net_readbuflen + size) > Autil::NETBUFSIZ)
		return -1;

	memcpy_s(net_readbuf + net_readbuflen, Autil::NETBUFSIZ, buf, size);
	net_readbuflen += size;
	return 0;
}

int shiftReadBuf(int size)
{
	int i;

	if (size > net_readbuflen)
		return -1;

	for (i = size; i < net_readbuflen; ++i)
	{
		net_readbuf[i - size] = net_readbuf[i];
	}
	net_readbuflen -= size;
	return 0;
}

int GameService::getLineFromReadBuf(char* output, int maxlen)
{
	int i, j;
	if (net_readbuflen >= Autil::NETBUFSIZ)
		return -1;

	for (i = 0; i < net_readbuflen && i < (maxlen - 1); ++i)
	{
		if (net_readbuf[i] == '\n')
		{
			memcpy_s(output, Autil::NETBUFSIZ, net_readbuf, i);
			output[i] = '\0';

			for (j = i + 1; j >= 0; j--)
			{
				if (j >= Autil::NETBUFSIZ)
					continue;
				if (output[j] == '\r')
				{
					output[j] = '\0';
					break;
				}
			}

			shiftReadBuf(i + 1);

			if (net_readbuflen < LINEBUFSIZ)
				net_readbuf[net_readbuflen] = '\0';
			return 0;
		}
	}
	return -1;
}
#endif

void CreateConsole()
{
	if (!AllocConsole())
	{
		std::wstring message = L"AllocConsole failed with error: " + std::to_wstring(GetLastError());
		MessageBox(nullptr, message.c_str(), L"Error", MB_OK);
		return;
	}
	FILE* fDummy;
	freopen_s(&fDummy, "CONOUT$", "w", stdout);
	freopen_s(&fDummy, "CONOUT$", "w", stderr);
	freopen_s(&fDummy, "CONIN$", "r", stdin);
	std::cout.clear();
	std::clog.clear();
	std::cerr.clear();
	std::cin.clear();

	// std::wcout, std::wclog, std::wcerr, std::wcin
	HANDLE hConOut = CreateFile(TEXT("CONOUT$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	HANDLE hConIn = CreateFile(TEXT("CONIN$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	SetStdHandle(STD_OUTPUT_HANDLE, hConOut);
	SetStdHandle(STD_ERROR_HANDLE, hConOut);
	SetStdHandle(STD_INPUT_HANDLE, hConIn);
	std::wcout.clear();
	std::wclog.clear();
	std::wcerr.clear();
	std::wcin.clear();

}

#ifdef USE_MINIDUMP
#include <DbgHelp.h>
#pragma comment(lib, "dbghelp.lib")

#include <DbgHelp.h>
#pragma comment(lib, "dbghelp.lib")

#if defined _M_X64 || defined _M_IX86
LPTOP_LEVEL_EXCEPTION_FILTER WINAPI
dummySetUnhandledExceptionFilter(
	LPTOP_LEVEL_EXCEPTION_FILTER)
{
	return NULL;
}
#else
#error "This code works only for x86 and x64!"
#endif

BOOL preventSetUnhandledExceptionFilter()
{
	HMODULE hKernel32 = LoadLibraryW(L"kernel32.dll");
	if (hKernel32 == nullptr)
		return FALSE;

	void* pOrgEntry = GetProcAddress(hKernel32, "SetUnhandledExceptionFilter");
	if (pOrgEntry == nullptr)
		return FALSE;

	DWORD dwOldProtect = 0;
	SIZE_T jmpSize = 5;
#ifdef _M_X64
	jmpSize = 13;
#endif
	BOOL bProt = VirtualProtect(pOrgEntry, jmpSize, PAGE_EXECUTE_READWRITE, &dwOldProtect);

	BYTE newJump[20];
	memset(newJump, 0, sizeof(newJump));
	void* pNewFunc = &dummySetUnhandledExceptionFilter;
#ifdef _M_IX86
	DWORD dwOrgEntryAddr = (DWORD)pOrgEntry;
	dwOrgEntryAddr += jmpSize; // add 5 for 5 op-codes for jmp rel32
	DWORD dwNewEntryAddr = (DWORD)pNewFunc;
	DWORD dwRelativeAddr = dwNewEntryAddr - dwOrgEntryAddr;
	// JMP rel32: Jump near, relative, displacement relative to next instruction.
	newJump[0] = 0xE9;  // JMP rel32
	memcpy(&newJump[1], &dwRelativeAddr, sizeof(pNewFunc));
#elif _M_X64
	// We must use R10 or R11, because these are "scratch" registers 
	// which need not to be preserved accross function calls
	// For more info see: Register Usage for x64 64-Bit
	// http://msdn.microsoft.com/en-us/library/ms794547.aspx
	// Thanks to Matthew Smith!!!
	newJump[0] = 0x49;  // MOV R11, ...
	newJump[1] = 0xBB;  // ...
	memcpy(&newJump[2], &pNewFunc, sizeof(pNewFunc));
	//pCur += sizeof (ULONG_PTR);
	newJump[10] = 0x41;  // JMP R11, ...
	newJump[11] = 0xFF;  // ...
	newJump[12] = 0xE3;  // ...
#endif
	SIZE_T bytesWritten;
	BOOL bRet = WriteProcessMemory(GetCurrentProcess(),
		pOrgEntry, newJump, jmpSize, &bytesWritten);

	if (bProt != FALSE)
	{
		DWORD dwBuf;
		VirtualProtect(pOrgEntry, jmpSize, dwOldProtect, &dwBuf);
	}
	return bRet;
}

LONG CALLBACK MinidumpCallback(PEXCEPTION_POINTERS pException)
{
	do
	{
		if (!pException)
			break;

		auto PathFileExists = [](const wchar_t* name)->BOOL
		{
			DWORD dwAttrib = GetFileAttributesW(name);
			return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
		};

		// Check if dump directory exists
		if (!PathFileExists(L"dump"))
		{
			CreateDirectoryW(L"dump", NULL);
		}

		wchar_t pszFileName[MAX_PATH] = {};
		SYSTEMTIME stLocalTime = {};
		GetLocalTime(&stLocalTime);
		swprintf_s(pszFileName, L"dump\\%04d%02d%02d_%02d%02d%02d.dmp",
			stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay,
			stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond);

		HANDLE hDumpFile = CreateFileW(pszFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hDumpFile == INVALID_HANDLE_VALUE)
			break;

		MINIDUMP_EXCEPTION_INFORMATION dumpInfo = {};
		dumpInfo.ExceptionPointers = pException;
		dumpInfo.ThreadId = GetCurrentThreadId();
		dumpInfo.ClientPointers = TRUE;

		MiniDumpWriteDump(
			GetCurrentProcess(),
			GetCurrentProcessId(),
			hDumpFile,
			(MINIDUMP_TYPE)(
				MiniDumpNormal
				| MiniDumpWithFullMemory
				| MiniDumpWithHandleData
				| MiniDumpWithThreadInfo
				| MiniDumpWithUnloadedModules
				| MiniDumpWithProcessThreadData
				),
			&dumpInfo,
			NULL,
			NULL
		);

		if (pException->ExceptionRecord->ExceptionFlags == EXCEPTION_NONCONTINUABLE)
		{
			throw;
			return EXCEPTION_EXECUTE_HANDLER;
		}
	} while (false);
	return EXCEPTION_CONTINUE_SEARCH;
}
#endif

LRESULT CALLBACK WndProc(HWND hWnd, DWORD message, LPARAM wParam, LPARAM lParam);
//
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID)
{
	if (DLL_PROCESS_ATTACH == ul_reason_for_call)
	{
		DWORD ThreadId = GetCurrentThreadId();
		if (g_MainThreadId != 0 && g_MainThreadId != ThreadId)
			return TRUE;

		GetModuleFileNameW(nullptr, g_szGameModulePath, MAX_PATH);
		g_hDllModule = hModule;
		g_hGameModule = GetModuleHandleW(nullptr);
		g_MainThreadId = ThreadId;
		g_MainThreadHandle = GetCurrentThread();
		HWND hWnd = util::GetCurrentWindowHandle();
		if (hWnd != nullptr)
		{
			g_MainHwnd = hWnd;
			g_OldWndProc = reinterpret_cast<WNDPROC>(GetWindowLongA(hWnd, GWL_WNDPROC));
			SetWindowLongA(hWnd, GWL_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc));
#ifdef USE_MINIDUMP
			SetUnhandledExceptionFilter(MinidumpCallback);
			preventSetUnhandledExceptionFilter();
#endif

#ifdef _DEBUG
			CreateConsole();
			std::cout << "DllMain:::  hModule:" << std::hex << hModule << " ul_reason_for_call:" << std::dec << ul_reason_for_call << std::endl;
			std::cout << "DllMain:::  g_hGameModule:" << std::hex << g_hGameModule << " g_MainThreadId:" << std::dec << g_MainThreadId << std::endl;
			std::cout << "DllMain:::  g_MainThreadHandle:" << std::hex << g_MainThreadHandle << " g_MainHwnd:" << std::dec << g_MainHwnd << std::endl;
			std::cout << "DllMain:::  g_OldWndProc:" << std::hex << g_OldWndProc << std::endl;
			std::cout << "DllMain:::  newWndProc:" << std::hex << reinterpret_cast<LONG_PTR>(WndProc) << std::endl;
#endif
		}
		DisableThreadLibraryCalls(hModule);
	}
	else if (DLL_PROCESS_DETACH == ul_reason_for_call)
	{
		GameService& g_GameService = GameService::getInstance();
		g_GameService.uninitialize();
		SetWindowLongA(g_MainHwnd, GWL_WNDPROC, reinterpret_cast<LONG_PTR>(g_OldWndProc));
	}

	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, DWORD message, LPARAM wParam, LPARAM lParam)
{
	using namespace util;
	GameService& g_GameService = GameService::getInstance();
	switch (message)
	{
	case WM_NULL:
		return 1;
	case WM_CLOSE:
	{
		MINT::NtTerminateProcess(GetCurrentProcess(), 0);
		return 1;
	}
	case WM_MOUSEMOVE:
	{
		//通知外掛更新當前鼠標坐標顯示
		PostMessageW(g_ParenthWnd, message + WM_USER, wParam, lParam);
		break;
	}
	case WM_KEYDOWN:
	{
		//按下CTRL+V
		if ((wParam == 'V' && GetKeyState(VK_CONTROL) < 0))
		{
			if (((1 == *g_GameService.g_world_status && 2 == *g_GameService.g_game_status)
				|| (1 == *g_GameService.g_world_status && 3 == *g_GameService.g_game_status))
				&& OpenClipboard(hWnd))
			{
				HANDLE hClipboardData = GetClipboardData(CF_TEXT);
				if (hClipboardData)
				{
					char* pszText = static_cast<char*>(GlobalLock(hClipboardData));
					if (pszText != nullptr)
					{
						int index = *CONVERT_GAMEVAR<int*>(0x415EF50);
						if (index == 0 || strlen(CONVERT_GAMEVAR<char*>(0x414F278)) == 0)
						{
							memset(CONVERT_GAMEVAR<char*>(0x414F278), 0, 20u);
							_snprintf_s(CONVERT_GAMEVAR<char*>(0x414F278), 20u, _TRUNCATE, "%s", pszText);
						}
						else
						{
							memset(CONVERT_GAMEVAR<char*>(0x415AA58), 0, 20u);
							_snprintf_s(CONVERT_GAMEVAR<char*>(0x415AA58), 20u, _TRUNCATE, "%s", pszText);
						}
						GlobalUnlock(hClipboardData);
					}
				}
				CloseClipboard();
				return 1;
			}
			SendMessage(hWnd, WM_PASTE, 0, 0);
			return 1;
		}
		break;
	}
	case WM_KEYUP:
	{
		//檢查是否為delete
		if (wParam == VK_DELETE)
		{
			PostMessageW(g_ParenthWnd, message + WM_USER + VK_DELETE, wParam, lParam);
		}
		break;
	}
	//case 0xAE:
	//{
	//	std::cout << "0xAE wparam: " << wParam << " lparam: " << lParam << std::endl;
	//	break;
	//}
	case kInitialize:
	{
#ifdef _DEBUG
		std::cout << "kInitialize" << std::endl;
#endif
		g_ParenthWnd = reinterpret_cast<HWND>(lParam);
		g_GameService.initialize(static_cast<unsigned short>(wParam));
		return 1;
	}
	case kUninitialize:
	{
#ifdef _DEBUG
		std::cout << "kUninitialize" << std::endl;
#endif
		g_GameService.uninitialize();
		SetWindowLongW(g_MainHwnd, GWL_WNDPROC, reinterpret_cast<LONG_PTR>(g_OldWndProc));
		FreeLibraryAndExitThread(g_hDllModule, 0);
		return 1;
	}
	case kGetModule:
	{
		return reinterpret_cast<int>(GetModuleHandleW(nullptr));
	}
	case kSendPacket:
	{
		//從外掛送來的封包
		if ((*g_GameService.g_sockfd == INVALID_SOCKET) || (*g_GameService.g_sockfd == NULL) || (lParam == NULL))
			return 0;

		DWORD dwBytesSent = 0;
		//int nRet = g_GameService.psend(*g_GameService.g_sockfd, reinterpret_cast<char*>(wParam), static_cast<int>(lParam), NULL);
		int nRet = g_GameService.psend(*g_GameService.g_sockfd, reinterpret_cast<char*>(wParam), static_cast<int>(lParam), NULL);
#ifdef _DEBUG
		std::cout << "kSendPacket:::  sockfd:" << std::to_string(*g_GameService.g_sockfd)
			<< " address:0x" << std::hex << wParam
			<< " size:" << std::to_string(lParam)
			<< " errorcode:" << WSAGetLastError() << std::endl;
#endif
		return nRet;
	}
	///////////////////////////////////////////////////////////
	case kEnableEffect://關閉特效
	{
		g_GameService.WM_EnableEffect(wParam);
		return 1;
	}
	case kEnablePlayerShow://隱藏人物
	{
		g_GameService.WM_EnablePlayerShow(wParam);
		return 1;
	}
	case kSetTimeLock://鎖定時間
	{
		g_GameService.WM_SetTimeLock(wParam, lParam);
		return 1;
	}
	case kEnableSound://關閉音效
	{
		g_GameService.WM_EnableSound(wParam);
		return 1;
	}
	case kEnableImageLock://鎖定畫面
	{
		g_GameService.WM_EnableImageLock(wParam);
		return 1;
	}
	case kEnablePassWall://橫衝直撞
	{
		g_GameService.WM_EnablePassWall(wParam);
		return 1;
	}
	case kEnableFastWalk://快速走路
	{
		g_GameService.WM_EnableFastWalk(wParam);
		return 1;
	}
	case kSetBoostSpeed://加速
	{
		g_GameService.WM_SetBoostSpeed(wParam, lParam);
		return 1;
	}
	case kEnableMoveLock://鎖定原地
	{
		g_GameService.WM_EnableMoveLock(wParam);
		return 1;
	}
	case kMuteSound://靜音
	{
		g_GameService.WM_MuteSound(wParam);
		return 1;
	}
	case kEnableBattleDialog://關閉戰鬥對話框
	{
		g_GameService.WM_EnableBattleDialog(wParam);
		return 1;
	}
	case kSetGameStatus://設置遊戲動畫狀態
	{
		g_GameService.WM_SetGameStatus(wParam);
		return 1;
	}
	case kSetBlockPacket://戰鬥封包阻擋
	{
		g_GameService.WM_SetBLockPacket(wParam);
		return 1;
	}
	case kBattleTimeExtend://戰鬥時間延長
	{
		g_GameService.WM_BattleTimeExtend(wParam);
		return 1;
	}
	case kEnableOptimize:
	{
		g_GameService.WM_SetOptimize(wParam);
		return 1;
	}
	case kEnableWindowHide:
	{
		g_GameService.WM_SetWindowHide(wParam);
		return 1;
	}
	//action
	case kSendAnnounce://公告
	{
		g_GameService.WM_Announce(reinterpret_cast<char*>(wParam), lParam);
		return 1;
	}
	case kSetMove://移動
	{
		g_GameService.WM_Move(wParam, lParam);
		return 1;
	}
	case kDistoryDialog://銷毀對話框
	{
		g_GameService.WM_DistoryDialog();
		return 1;
	}
	case kCleanChatHistory://清空聊天緩存
	{
		g_GameService.WM_CleanChatHistory();
		return 1;
	}
	case kCreateDialog:
	{
		g_GameService.WM_CreateDialog(wParam, reinterpret_cast<char*>(lParam));
		return 1;
	}
	default:
		break;
	}
	return CallWindowProcA(g_OldWndProc, g_MainHwnd, message, wParam, lParam);
}

extern "C"
{
	//new socket
	SOCKET WSAAPI New_socket(int af, int type, int protocol)
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_socket(af, type, protocol);
	}

	//new send
	int WSAAPI New_send(SOCKET s, const char* buf, int len, int flags)
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_send(s, buf, len, flags);
	}

	//new recv
	int WSAAPI New_recv(SOCKET s, char* buf, int len, int flags)
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_recv(s, buf, len, flags);
	}

	//new closesocket
	int WSAAPI New_closesocket(SOCKET s)
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_closesocket(s);
	}

	//new connect
	int WSAAPI New_connect(SOCKET s, const struct sockaddr* name, int namelen)
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_connect(s, name, namelen);
	}

	//new inet_addr
	unsigned long WSAAPI New_inet_addr(const char* cp)
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_inet_addr(cp);
	}

	//new ntohs
	u_short WSAAPI New_ntohs(u_short netshort)
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_ntohs(netshort);
	}

	//new SetWindowTextA
	BOOL WINAPI New_SetWindowTextA(HWND hWnd, LPCSTR lpString)
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_SetWindowTextA(hWnd, lpString);
	}

	DWORD WINAPI New_GetTickCount()
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_GetTickCount();
	}

	BOOL WINAPI New_QueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount)
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_QueryPerformanceCounter(lpPerformanceCount);
	}

	DWORD WINAPI New_TimeGetTime()
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_TimeGetTime();
	}

	void WINAPI New_Sleep(DWORD dwMilliseconds)
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_Sleep(dwMilliseconds);
	}

	BOOL WINAPI New_IsIconic(HWND hWnd)
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_IsIconic(hWnd);
	}

	/////// game client ///////

	//PlaySound
	void __cdecl New_PlaySound(int a, int b, int c)
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_PlaySound(a, b, c);
	}

	//BattleProc
	void __cdecl New_BattleProc()
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_BattleProc();
	}

	//BattleCommandReady
	void __cdecl New_BattleCommandReady()
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_BattleCommandReady();
	}

	//TimeProc
	void __cdecl New_TimeProc(int fd)
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_TimeProc(fd);
	}

	//lssproto_EN_recv
	void __cdecl New_lssproto_EN_recv(int fd, int result, int field)
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_lssproto_EN_recv(fd, result, field);
	}

	//lssproto_B_recv
	void __cdecl New_lssproto_B_recv(int fd, char* command)
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_lssproto_B_recv(fd, command);
	}

	//lssproto_WN_send
	void _cdecl New_lssproto_WN_send(int fd, int x, int y, int dialogid, int unitid, int select, char* data)
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_lssproto_WN_send(fd, x, y, dialogid, unitid, select, data);
	}

	void __cdecl New_lssproto_TK_send(int fd, int x, int y, const char* message, int color, int area)
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_lssproto_TK_send(fd, x, y, message, color, area);
	}
}

void GameService::initialize(unsigned short port)
{
	if (isInitialized_.load(std::memory_order_acquire))
		return;

	isInitialized_.store(true, std::memory_order_release);

	UINT acp = GetACP();
	if (acp != 936)
	{
		//將環境設置回簡體 8001端 99.99%都是簡體
		SetConsoleCP(936);
		SetConsoleOutputCP(936);
		setlocale(LC_ALL, "Chinese.GB2312");
	}

	g_sockfd = CONVERT_GAMEVAR<int*>(0x421B404);//套接字


	g_world_status = CONVERT_GAMEVAR<int*>(0x4230DD8);
	g_game_status = CONVERT_GAMEVAR<int*>(0x4230DF0);
#ifdef AUTIL_H
	Autil::PersonalKey = CONVERT_GAMEVAR<char*>(0x4AC0898);//封包解密密鑰
#endif

	//pBattleCommandReady = CONVERT_GAMEVAR<pfnBattleCommandReady>(0x6B9A0);//戰鬥面板生成call裡面的一個小function
	pBattleCommandReady = CONVERT_GAMEVAR<DWORD*>(0xA8CA);
	pPlaySound = CONVERT_GAMEVAR<pfnPlaySound>(0x88190);//播放音效
	pBattleProc = CONVERT_GAMEVAR<pfnBattleProc>(0x3940);//戰鬥循環
	pTimeProc = CONVERT_GAMEVAR<pfnTimeProc>(0x1E6D0);//刷新時間循環
	pLssproto_EN_recv = CONVERT_GAMEVAR<pfnLssproto_EN_recv>(0x64E10);//進戰鬥封包
	pLssproto_B_recv = CONVERT_GAMEVAR<pfnLssproto_B_recv>(0x64EF0);//戰鬥封包
	pLssproto_WN_send = CONVERT_GAMEVAR<pfnLssproto_WN_send>(0x8FDC0);//對話框發送封包
	pLssproto_TK_send = CONVERT_GAMEVAR<pfnLssproto_TK_send>(0x8F7C0);//喊話發送封包

	/*
		sa_8001.exe+91710 - FF 25 08C04900        - jmp dword ptr [sa_8001.exe+9C008] { ->DINPUT.DirectInputCreateA }

		sa_8001.exe+91722 - FF 25 A4C24900        - jmp dword ptr [sa_8001.exe+9C2A4] { ->->KERNELBASE.GetLastError }

		sa_8001.exe+9172E - FF 25 A8C24900        - jmp dword ptr [sa_8001.exe+9C2A8] { ->WS2_32._WSAFDIsSet }
		sa_8001.exe+91734 - FF 25 B0C24900        - jmp dword ptr [sa_8001.exe+9C2B0] { ->WS2_32.select }
		sa_8001.exe+9173A - FF 25 B4C24900        - jmp dword ptr [sa_8001.exe+9C2B4] { ->WS2_32.WSAStartup }
		sa_8001.exe+91740 - FF 25 B8C24900        - jmp dword ptr [sa_8001.exe+9C2B8] { ->WS2_32.WSACleanup }
		sa_8001.exe+91746 - FF 25 BCC24900        - jmp dword ptr [sa_8001.exe+9C2BC] { ->WS2_32.connect }
		sa_8001.exe+9174C - FF 25 C0C24900        - jmp dword ptr [sa_8001.exe+9C2C0] { ->WS2_32.gethostbyname }
		sa_8001.exe+91752 - FF 25 C4C24900        - jmp dword ptr [sa_8001.exe+9C2C4] { ->WS2_32.inet_addr }
		sa_8001.exe+91758 - FF 25 C8C24900        - jmp dword ptr [sa_8001.exe+9C2C8] { ->WS2_32.ntohs }
		sa_8001.exe+9175E - FF 25 CCC24900        - jmp dword ptr [sa_8001.exe+9C2CC] { ->WS2_32.ioctlsocket }
		sa_8001.exe+91764 - E9 A70E8962           - jmp 62D22610
		sa_8001.exe+91769 - CC                    - int 3
		sa_8001.exe+9176A - FF 25 DCC24900        - jmp dword ptr [sa_8001.exe+9C2DC] { ->WSOCK32.setsockopt }
		sa_8001.exe+91770 - FF 25 D0C24900        - jmp dword ptr [sa_8001.exe+9C2D0] { ->WSOCK32.recvfrom }
		sa_8001.exe+91776 - FF 25 D4C24900        - jmp dword ptr [sa_8001.exe+9C2D4] { ->WS2_32.sendto }

	*/

	/*WSAAPI*/
	psocket = CONVERT_GAMEVAR<pfnsocket>(0x91764);//::socket;
	psend = CONVERT_GAMEVAR<pfnsend>(0x9171C); //::send;
	precv = CONVERT_GAMEVAR<pfnrecv>(0x91728);//這裡直接勾::recv會無效，遊戲通常會複寫另一個call dword ptr指向recv
	pclosesocket = CONVERT_GAMEVAR<pfnclosesocket>(0x91716);//::closesocket;
	//pinet_addr = CONVERT_GAMEVAR<pfninet_addr>(0x9C2C4); //::inet_addr; //
	//pntohs = ::ntohs; //CONVERT_GAMEVAR<pfnntohs>(0x9C2C8);//
	pconnect = ::connect;//CONVERT_GAMEVAR<pfnconnect>(0x9C2BC);

	/*WINAPI*/
	pSetWindowTextA = ::SetWindowTextA;//防止部分私服調用A類函數，導致其他語系系統的窗口標題亂碼
	pGetTickCount = ::GetTickCount;
	pQueryPerformanceCounter = ::QueryPerformanceCounter;
	pTimeGetTime = ::timeGetTime;
	pSleep = ::Sleep;

	//DINPUT.dll+76FE - FF 15 7812477C        - call dword ptr [DINPUT.dll+21278] { ->USER32.IsIconic }
	//pIsIconic = ::IsIconic;


	//禁止遊戲內切AI SE切AI會崩潰
	DWORD paddr = CONVERT_GAMEVAR <DWORD>(0x1DF82);
	util::MemoryMove(paddr, "\xB8\x00\x00\x00\x00", 5);//只要點pgup或pgdn就會切0
	paddr = CONVERT_GAMEVAR <DWORD>(0x1DFE6);
	util::MemoryMove(paddr, "\xBA\x00\x00\x00\x00", 5);//只要點pgup或pgdn就會切0
	//paddr = CONVERT_GAMEVAR <DWORD>(0x1F2EF);
	//util::MemoryMove(paddr, "\xB9\x00\x00\x00\x00", 5);
	int* pEnableAI = CONVERT_GAMEVAR<int*>(0xD9050);
	*pEnableAI = 0;

	//sa_8001sf.exe+A8CA - FF 05 F0 0D 63 04        - inc [sa_8001sf.exe+4230DF0] { (2) }
	BYTE newByte[6] = { 0x90, 0xE8, 0x90, 0x90, 0x90, 0x90 }; //新數據
	util::detour(::New_BattleCommandReady, reinterpret_cast<DWORD>(pBattleCommandReady), oldBattleCommandReadyByte, newByte, sizeof(oldBattleCommandReadyByte), 0);

#if 0
	//禁止開頭那隻寵物亂跑
	paddr = CONVERT_GAMEVAR<DWORD>(0x79A0F);
	//sa_8001.exe+79A0F - E8 DCFAFBFF           - call sa_8001.exe+394F0s
	util::MemoryMove(paddr, "\x90\x90\x90\x90\x90", 5);
	//開頭那隻寵物會不會動
	paddr = CONVERT_GAMEVAR<DWORD>(0x79A14);
	//sa_8001.exe+79A14 - E8 5777F8FF           - call sa_8001.exe+1170
	util::MemoryMove(paddr, "\x90\x90\x90\x90\x90", 5);
#endif
	////使遊戲開頭那隻亂竄的寵物不可見
	paddr = CONVERT_GAMEVAR<DWORD>(0x79A19);
	//sa_8001.exe+79A19 - E8 224A0000           - call sa_8001.exe+7E440
	util::MemoryMove(paddr, "\x90\x90\x90\x90\x90", 5);

	//開始下勾子
	DetourRestoreAfterWith();
	DetourTransactionBegin();
	DetourUpdateThread(g_MainThreadHandle);

	DetourAttach(&(PVOID&)psocket, ::New_socket);
	DetourAttach(&(PVOID&)psend, ::New_send);
	DetourAttach(&(PVOID&)precv, ::New_recv);
	DetourAttach(&(PVOID&)pclosesocket, ::New_closesocket);
	//DetourAttach(&(PVOID&)pinet_addr, ::New_inet_addr);
	//DetourAttach(&(PVOID&)pntohs, ::New_ntohs);
#ifdef _DEBUG
	DetourAttach(&(PVOID&)pconnect, ::New_connect);
#endif

	DetourAttach(&(PVOID&)pSetWindowTextA, ::New_SetWindowTextA);
	DetourAttach(&(PVOID&)pGetTickCount, ::New_GetTickCount);
	DetourAttach(&(PVOID&)pQueryPerformanceCounter, ::New_QueryPerformanceCounter);
	DetourAttach(&(PVOID&)pTimeGetTime, ::New_TimeGetTime);
	DetourAttach(&(PVOID&)pSleep, ::New_Sleep);
	//DetourAttach(&(PVOID&)pIsIconic, ::New_IsIconic);

	DetourAttach(&(PVOID&)pPlaySound, ::New_PlaySound);
	DetourAttach(&(PVOID&)pBattleProc, ::New_BattleProc);
	DetourAttach(&(PVOID&)pTimeProc, ::New_TimeProc);
	DetourAttach(&(PVOID&)pLssproto_EN_recv, ::New_lssproto_EN_recv);
	DetourAttach(&(PVOID&)pLssproto_B_recv, ::New_lssproto_B_recv);
	DetourAttach(&(PVOID&)pLssproto_WN_send, ::New_lssproto_WN_send);
	DetourAttach(&(PVOID&)pLssproto_TK_send, ::New_lssproto_TK_send);

	DetourTransactionCommit();

	WM_SetOptimize(true);

#ifdef USE_ASYNC_TCP
	if (nullptr == asyncClient_)
	{
		//與外掛連線
		asyncClient_.reset(new AsyncClient());
		if (asyncClient_ != nullptr && asyncClient_->Connect(IPV6_DEFAULT, port))
		{
			std::cout << "connect success" << std::endl;
			asyncClient_->Start();
			//#ifdef NDEBUG
			//			if (g_hDllModule != nullptr)
			//				hideModule(g_hDllModule);
			//#endif
		}
	}
#else
	if (nullptr == syncClient_)
	{
		//與外掛連線
		syncClient_.reset(new SyncClient());
		if (syncClient_ != nullptr && syncClient_->Connect(IPV6_DEFAULT, port))
		{
			//#ifdef NDEBUG
			//			if (g_hDllModule != nullptr)
			//			{
			//
			//				hideModule(g_hDllModule);
			//				HMODULE hm = ::GetModuleHandleW(L"sqsq.dll");
			//				if (hm != nullptr)
			//					hideModule(hm);
			//			}
			//#endif
		}
	}
#endif

	//SOCKET fd = INVALID_SOCKET;
	//if (connectServer(fd, "103.36.167.114", 9078) == 1)
	//	*g_sockfd = fd;
}

//這裡的東西其實沒有太大必要，一般外掛斷開就直接關遊戲了，如果需要設計能重連才需要
void GameService::uninitialize()
{
	if (!isInitialized_.load(std::memory_order_acquire))
		return;

	isInitialized_.store(false, std::memory_order_release);

#ifdef USE_ASYNC_TCP
	if (asyncClient_)
		asyncClient_.reset();
#else
	if (syncClient_)
		syncClient_.reset();
#endif

	DetourRestoreAfterWith();
	DetourTransactionBegin();
	DetourUpdateThread(g_MainThreadHandle);

	DetourDetach(&(PVOID&)psocket, ::New_socket);
	DetourDetach(&(PVOID&)psend, ::New_send);
	DetourDetach(&(PVOID&)pclosesocket, ::New_closesocket);
	//DetourDetach(&(PVOID&)pinet_addr, ::New_inet_addr);
	//DetourDetach(&(PVOID&)pntohs, ::New_ntohs);
#ifdef _DEBUG
	DetourDetach(&(PVOID&)pconnect, ::New_connect);
#endif

	DetourDetach(&(PVOID&)pSetWindowTextA, ::New_SetWindowTextA);
	DetourDetach(&(PVOID&)pGetTickCount, ::New_GetTickCount);
	DetourDetach(&(PVOID&)pQueryPerformanceCounter, ::New_QueryPerformanceCounter);
	DetourDetach(&(PVOID&)pTimeGetTime, ::New_TimeGetTime);
	DetourDetach(&(PVOID&)pSleep, ::New_Sleep);
	//DetourDetach(&(PVOID&)pIsIconic, ::New_IsIconic);

	DetourDetach(&(PVOID&)pPlaySound, ::New_PlaySound);
	DetourDetach(&(PVOID&)pBattleProc, ::New_BattleProc);
	DetourDetach(&(PVOID&)pTimeProc, ::New_TimeProc);
	DetourDetach(&(PVOID&)pLssproto_EN_recv, ::New_lssproto_EN_recv);
	DetourDetach(&(PVOID&)pLssproto_B_recv, ::New_lssproto_B_recv);
	DetourDetach(&(PVOID&)pLssproto_WN_send, ::New_lssproto_WN_send);
	DetourDetach(&(PVOID&)pLssproto_TK_send, ::New_lssproto_TK_send);

	DetourTransactionCommit();

	util::undetour(pBattleCommandReady, oldBattleCommandReadyByte, sizeof(oldBattleCommandReadyByte));
}

void GameService::Send(const std::string& str)
{
#ifdef USE_ASYNC_TCP
	//轉發給外掛
	if (asyncClient_)
		asyncClient_->Send(str.c_str(), str.size());
#else
	//轉發給外掛
	if (syncClient_)
		syncClient_->Send(str.c_str(), str.size());
#endif
}

#ifdef AUTIL_H
//解密跟分析封包流程一般都在外掛端處理，這裡只是為了要在快戰打開時攔截轉場封包，但其實直接攔截EN函數也是一樣的
int SaDispatchMessage(int fd, char* encoded)
{
	//int		func = 0, fieldcount = 0;
	//int		iChecksum = 0, iChecksumrecv = 0;
	//std::unique_ptr<char[]> raw(new char[Autil::NETBUFSIZ]());

	//Autil::util_DecodeMessage(raw.get(), Autil::NETBUFSIZ, encoded);
	//Autil::util_SplitMessage(raw.get(), Autil::NETBUFSIZ, const_cast<char*>(Autil::SEPARATOR));
	//if (Autil::util_GetFunctionFromSlice(&func, &fieldcount) != 1)
	//{
	//	return 0;
	//}

	////std::cout << "fun: " << std::to_string(func) << " fieldcount: " << std::to_string(fieldcount) << std::endl;

	//auto CHECKFUN = [&func](int cmpfunc) -> bool { return (func == cmpfunc); };

	//case Autil::LSSPROTO_EN_RECV) /*Battle EncountFlag //開始戰鬥 7*/)
	//{
	//	Autil::SliceCount = 0;
	//	return 2;
	//}
	//else case Autil::LSSPROTO_CLIENTLOGIN_RECV)/*選人畫面 72*/)
	//{
	//	return 0;
	//}
	//else case Autil::LSSPROTO_CHARLIST_RECV)/*選人頁面資訊 80*/)
	//{
	//	return 0;
	//}

	//Autil::SliceCount = 0;
	//return 1;
}
#endif

//hooks
SOCKET WSAAPI GameService::New_socket(int af, int type, int protocol)
{
	SOCKET ret = psocket(af, type, protocol);
	return ret;
}

int WSAAPI GameService::New_send(SOCKET s, const char* buf, int len, int flags)
{
	int ret = psend(s, buf, len, flags);
	return ret;
}

int WSAAPI GameService::New_connect(SOCKET s, const struct sockaddr* name, int namelen)
{
	if (s && name != nullptr)
	{
		if (name->sa_family == AF_INET)
		{
			struct sockaddr_in* sockaddr_ipv4 = (struct sockaddr_in*)name;
			char ipStr[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &(sockaddr_ipv4->sin_addr), ipStr, sizeof(ipStr));
			uint16_t port = ntohs(sockaddr_ipv4->sin_port);

			std::cout << "IPv4 Address: " << ipStr << " Port: " << port << std::endl;
		}
	}
	return pconnect(s, name, namelen);
}

unsigned long WSAAPI GameService::New_inet_addr(const char* cp)
{
	std::cout << "inet_addr: " << std::string(cp) << std::endl;
	return pinet_addr(cp);
}

u_short WSAAPI GameService::New_ntohs(u_short netshort)
{
	std::cout << "ntohs: " << std::to_string(netshort) << std::endl;
	return pntohs(netshort);
}

//hook recv將封包全部轉發給外掛，本來準備完全由外掛處理好再發回來，但效果不盡人意
int WSAAPI GameService::New_recv(SOCKET s, char* buf, int len, int flags)
{
#ifdef AUTIL_H
	using namespace Autil;
	//將客戶端緩存內容原封不動複製到新的堆 (其實沒什麼必要)
	std::unique_ptr <char[]> raw(new char[len + 1]());
	memcpy_s(raw.get(), len, buf, len);
	raw.get()[len] = '\0';

	//正常收包
	clearNetBuffer();
	int recvlen = precv(s, rpc_linebuffer, sizeof(rpc_linebuffer), flags);//LINEBUFSIZ
#else
	int recvlen = precv(s, buf, len, flags);
#endif
	if (recvlen > 0)
	{
#ifdef AUTIL_H
		//將收到的數據複製到緩存
		memcpy_s(raw.get(), len, rpc_linebuffer, recvlen);
#endif

#ifdef USE_ASYNC_TCP
		//轉發給外掛
		if (asyncClient_)
		{
			std::cout << "send recv: " << std::to_string(recvlen) << std::endl;
			asyncClient_->Send(buf, recvlen);
		}
#else
		//轉發給外掛
		if (syncClient_)
			syncClient_->Send(buf, recvlen);//raw.get()
#endif

#ifdef AUTIL_H
		//將封包內容壓入全局緩存
		appendReadBuf(rpc_linebuffer, recvlen);

		//解析封包，這裡主要是為了實現快速戰鬥，在必要的時候讓客戶端不會收到戰鬥進場封包
		//前面已經攔截lssproto_EN_recv了所以這邊暫時沒有用處
		while ((recvlen != SOCKET_ERROR) && (net_readbuflen > 0))
		{
			memset(rpc_linebuffer, 0, Autil::NETBUFSIZ);
			if (!getLineFromReadBuf(rpc_linebuffer, Autil::NETBUFSIZ))
			{
				int ret = SaDispatchMessage(s, rpc_linebuffer);
				if (ret < 0)
				{
#ifdef _DEBUG
					std::cout << "************************* DONE *************************" << std::endl;
#endif
					//代表此段數據已到結尾
					clearNetBuffer();
					break;
				}
				else if (ret == 0)
				{
#ifdef _DEBUG
					std::cout << "************************* CLEAN *************************" << std::endl;
#endif
					//錯誤的數據 或 需要清除緩存
					clearNetBuffer();
					break;
				}
				else if (ret == 2)
				{
					if (IS_ENCOUNT_BLOCK_FLAG)
					{
#ifdef _DEBUG
						std::cout << "************************* Block *************************" << std::endl;
#endif

						//需要移除不傳給遊戲的數據
						clearNetBuffer();
						SetLastError(0);
						return 1;
					}
				}
			}
			else
			{
#ifdef _DEBUG
				std::cout << "************************* END *************************" << std::endl;
#endif
				//數據讀完了
				util_Release();
				break;
			}
		}
#endif
	}

#ifdef AUTIL_H
	//將封包還給遊戲客戶端(A_A)
	memcpy_s(buf, len, rpc_linebuffer, len);
#endif

	return recvlen;
}

int WSAAPI GameService::New_closesocket(SOCKET s)
{
	if (g_sockfd != nullptr)
	{
		SOCKET cmps = *g_sockfd;
		if ((s != INVALID_SOCKET) && (s > 0)
			&& (cmps != INVALID_SOCKET) && (cmps > 0)
			&& (s == cmps))
		{
			Send("dc 1");
		}
	}

	int ret = pclosesocket(s);
	return ret;
}

//防止其他私服使用A類函數導致標題亂碼
BOOL WSAAPI GameService::New_SetWindowTextA(HWND hWnd, LPCSTR lpString)
{
	return TRUE;
}

DWORD WINAPI GameService::New_GetTickCount()
{
	static DWORD g_dwRealTick = pGetTickCount();
	static DWORD g_dwHookTick = pGetTickCount();
	DWORD tmpTick = pGetTickCount();
	g_dwHookTick = g_dwHookTick + speedBoostValue * (tmpTick - g_dwRealTick);
	g_dwRealTick = tmpTick;
	return g_dwHookTick;
}

BOOL WINAPI GameService::New_QueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount)
{
	BOOL result = pQueryPerformanceCounter(lpPerformanceCount);
	lpPerformanceCount->QuadPart *= speedBoostValue; // 修改返回值为原始值的两倍
	return result;
}

DWORD WINAPI GameService::New_TimeGetTime()
{
	static DWORD g_dwRealTime = pTimeGetTime();
	static DWORD g_dwHookTime = pTimeGetTime();
	DWORD tmpTime = pTimeGetTime();
	g_dwHookTime = g_dwHookTime + speedBoostValue * (tmpTime - g_dwRealTime);
	g_dwRealTime = tmpTime;
	return g_dwHookTime;
}

void WINAPI GameService::New_Sleep(DWORD dwMilliseconds)
{
	if (enableSleepAdjust.load(std::memory_order_acquire) && dwMilliseconds == 0)
		dwMilliseconds = 1;

	pSleep(dwMilliseconds);
}

BOOL WINAPI GameService::New_IsIconic(HWND hWnd)
{
	return FALSE;
}

//////////////////////////////////////////////////////////////////////

//音效
void GameService::New_PlaySound(int a, int b, int c)
{
	if (!IS_SOUND_MUTE_FLAG)
	{
		pPlaySound(a, b, c);
	}
}

//戰鬥循環
void GameService::New_BattleProc()
{
	if (IS_BATTLE_PROC_FLAG)
	{
		pBattleProc();
	}
}

//每回合開始顯示戰鬥面板 (這裡只是切換面板時會順便調用到的函數)
void GameService::New_BattleCommandReady()
{
	int* p = CONVERT_GAMEVAR<int*>(0x4230DF0);
	++(*p);
	Send("bPK 1");
}

//遊戲時間刷新循環 (早上,下午....)
void GameService::New_TimeProc(int fd)
{
	if (!IS_TIME_LOCK_FLAG)
		pTimeProc(fd);
}

//EN封包攔截，戰鬥進場
void GameService::New_lssproto_EN_recv(int fd, int result, int field)
{
	if (!IS_ENCOUNT_BLOCK_FLAG)
		pLssproto_EN_recv(fd, result, field);
}

//B封包攔截，戰鬥封包
void GameService::New_lssproto_B_recv(int fd, char* command)
{
	if (!IS_ENCOUNT_BLOCK_FLAG)
		pLssproto_B_recv(fd, command);
}

//WN對話框發包攔截
void GameService::New_lssproto_WN_send(int fd, int x, int y, int dialogid, int unitid, int select, const char* data)
{
	if (unitid == 1234 && dialogid == 4321)
	{
		std::string str = "dk|";
		str += std::to_string(x) + "|";
		str += std::to_string(y) + "|";
		str += std::to_string(select) + "|";
		str += std::string(data);
		std::cout << "send: " << str << std::endl;
		Send(str);
		return;
	}

	pLssproto_WN_send(fd, x, y, dialogid, unitid, select, data);
}

//TK對話框收包攔截
void GameService::New_lssproto_TK_send(int fd, int x, int y, const char* message, int color, int area)
{
	std::string msg = message;
	//查找是否包含'//'
	if (!msg.empty() && msg.find("//") != std::string::npos)
	{
		std::string str = "tk|";
		str += std::to_string(x) + "|";
		str += std::to_string(y) + "|";
		str += std::to_string(color) + "|";
		str += std::to_string(area) + "|";
		str += msg.substr(2);
		Send(str);
		return;
	}

	pLssproto_TK_send(fd, x, y, message, color, area);
}
////////////////////////////////////////////////////

//設置遊戲畫面狀態值
void GameService::WM_SetGameStatus(int status)
{
	if (g_game_status == nullptr)
		return;

	int G = *g_game_status;
	if (G != status)
		*g_game_status = status;
}

//資源優化
void GameService::WM_SetOptimize(bool enable)
{
	if (g_hGameModule == nullptr)
		return;

	DWORD optimizeAddr = CONVERT_GAMEVAR<DWORD>(0x129E7);
	if (!enable)
	{
		/*
		//sa_8001.exe+129E9 - A1 0CA95400           - mov eax,[sa_8001.exe+14A90C] { (05205438) }
		//sa_8001.exe+129EE - 6A 00                 - push 00 { 0 }
		util::MemoryMove(optimizeAddr, "\xA1\x0C\xA9\x54\x00\x6A\x00", 7);

		//sa_8001.exe+129E7 - 75 37                 - jne sa_8001sf.exe+12A20
		util::MemoryMove(optimizeAddr, "\x75\x37", 2);
		*/

		*CONVERT_GAMEVAR<int*>(0xAB7C8) = 14;
	}
	else
	{
		/*
		//sa_8001.exe+129E9 - EB 10                 - jmp sa_8001.exe+129FB
		util::MemoryMove(optimizeAddr, "\xEB\x10\x90\x90\x90\x90\x90", 7);

		//sa_8001.exe+129E7 - EB 12                 - jmp sa_8001sf.exe+129FB
		util::MemoryMove(optimizeAddr, "\xEB\x12", 2);
		*/

		*CONVERT_GAMEVAR<int*>(0xAB7C8) = 0;
	}
}

//隱藏窗口
void GameService::WM_SetWindowHide(bool enable)
{
	if (g_hGameModule == nullptr)
		return;

	//聊天紀錄顯示行數數量設為0
	nowChatRowCount_ = *CONVERT_GAMEVAR<int*>(0xA2674);
	if (nowChatRowCount_ < 20)
		*CONVERT_GAMEVAR<int*>(0xA2674) = 20;

	if (!enable)
	{
		//sa_8001sf.exe+129E7 - 75 37                 - jne sa_8001sf.exe+12A20 資源優化關閉
		util::MemoryMove(CONVERT_GAMEVAR<DWORD>(0x129E7), "\xEB\x12", 2);

		//sa_8001sf.exe+1DFFD - 74 31                 - je sa_8001sf.exe+1E030
		util::MemoryMove(CONVERT_GAMEVAR<DWORD>(0x1DFFD), "\x74\x31", 2);

		//sa_8001sf.exe+1DEE4 - 83 F9 0E              - cmp ecx,0E { 14 } 加速相關
		util::MemoryMove(CONVERT_GAMEVAR<DWORD>(0x1DEE4), "\x83\xF9\x0E", 3);

		//Sleep停止更改
		enableSleepAdjust.store(false, std::memory_order_release);
	}
	else
	{
		*CONVERT_GAMEVAR<int*>(0xA2674) = 0;

		//sa_8001sf.exe+129E7 - EB 37                 - jmp sa_8001sf.exe+12A20 資源優化開啟
		util::MemoryMove(CONVERT_GAMEVAR<DWORD>(0x129E7), "\xEB\x37", 2);

		//sa_8001sf.exe+1DFFD - EB 31                 - jmp sa_8001sf.exe+1E030 強制跳過整個背景繪製
		util::MemoryMove(CONVERT_GAMEVAR<DWORD>(0x1DFFD), "\xEB\x31", 2);

		//sa_8001sf.exe+1DEE4 - 83 F9 0E              - cmp ecx,05 { 14 } 加速相關
		util::MemoryMove(CONVERT_GAMEVAR<DWORD>(0x1DEE4), "\x83\xF9\x7F", 3);

		//Sleep強制更改
		enableSleepAdjust.store(true, std::memory_order_release);
	}
}

//靜音
void GameService::WM_MuteSound(bool enable)
{
	IS_SOUND_MUTE_FLAG = enable;
}

//戰鬥時間延長(99秒)
void GameService::WM_BattleTimeExtend(bool enable)
{
	if (g_hGameModule == nullptr)
		return;
	DWORD* timerAddr = CONVERT_GAMEVAR<DWORD*>(0x9854);
	//sa_8001.exe+9853 - 05 30750000           - add eax,00007530 { 30000 }
	if (!enable)
	{
		*timerAddr = 30UL * 1000UL;
	}
	else
	{
		*timerAddr = 100UL * 1000UL;
	}
}

//允許戰鬥面板
void GameService::WM_EnableBattleDialog(bool enable)
{
	if (g_hGameModule == nullptr)
		return;
	//DWORD addr = CONVERT_GAMEVAR<DWORD>(0x98C9);
	//DWORD addr2 = CONVERT_GAMEVAR<DWORD>(0x9D07);
	//DWORD addrSoundCharOpen = CONVERT_GAMEVAR<DWORD>(0x98DA);
	//DWORD addrSoundCharOpen2 = CONVERT_GAMEVAR<DWORD>(0x9D13);
	//DWORD addrSoundCharClose = CONVERT_GAMEVAR<DWORD>(0x667D);
	//DWORD addrSoundCharClose2 = CONVERT_GAMEVAR<DWORD>(0x93C9);
	//int* timerAddr = CONVERT_GAMEVAR<int*>(0xE21E0);
	//DWORD timerAddr2 = CONVERT_GAMEVAR<DWORD>(0x9841);
	//DWORD timerAddr3 = CONVERT_GAMEVAR<DWORD>(0x9DB6);
	DWORD battleLoopAddr = CONVERT_GAMEVAR<DWORD>(0xA914);
	if (enable)
	{
		////sa_8001sf.exe+98C9 - A3 E4214E00           - mov [sa_8001sf.exe+E21E4],eax { (0) }
		//util::MemoryMove(addr, "\xA3\xE4\x21\x4E\x00", 5);
		////sa_8001sf.exe+9D07 - 89 15 E4214E00        - mov [sa_8001sf.exe+E21E4],edx { (0) }
		//util::MemoryMove(addr2, "\x89\x15\xE4\x21\x4E\x00", 6);
		////sa_8001sf.exe+98DA - E8 B1E80700           - call sa_8001sf.exe+88190
		//util::MemoryMove(addrSoundCharOpen, "\xE8\xB1\xE8\x07\x00", 5);
		////sa_8001sf.exe+9D13 - E8 78E40700           - call sa_8001sf.exe+88190
		//util::MemoryMove(addrSoundCharOpen2, "\xE8\x78\xE4\x07\x00", 5);
		////sa_8001sf.exe+667D - E8 0E1B0800           - call sa_8001sf.exe+88190
		//util::MemoryMove(addrSoundCharClose, "\xE8\x0E\x1B\x08\x00", 5);
		////sa_8001sf.exe+93C9 - E8 C2ED0700           - call sa_8001sf.exe+88190
		//util::MemoryMove(addrSoundCharClose2, "\xE8\xC2\xED\x07\x00", 5);
		////sa_8001sf.exe+954F - A1 E0214E00           - mov eax,[sa_8001sf.exe+E21E0] { (0) }
		//*timerAddr = 1;
		////sa_8001sf.exe+9841 - 89 1D E0214E00        - mov [sa_8001sf.exe+E21E0],ebx { (1) }
		//util::MemoryMove(timerAddr2, "\x89\x1D\xE0\x21\x4E\x00", 6);
		////sa_8001sf.exe+9DB6 - 89 2D E0214E00        - mov [sa_8001sf.exe+E21E0],ebp { (0) }
		//util::MemoryMove(timerAddr3, "\x89\x2D\xE0\x21\x4E\x00", 6);
		////////////////////////////////////////////////////////////////////////////////////////////

		//sa_8001sf.exe+A914 - 75 0C                 - jne sa_8001sf.exe+A922 //原始
		//sa_8001sf.exe+A914 - EB 0C                 - jmp sa_8001sf.exe+A922 //最重要的
		util::MemoryMove(battleLoopAddr, "\xEB\x0C", 2);
		IS_BATTLE_PROC_FLAG = true;

	}
	else
	{
		//util::MemoryMove(addr, "\x90\x90\x90\x90\x90", 5);
		//util::MemoryMove(addr2, "\x90\x90\x90\x90\x90\x90", 6);
		//util::MemoryMove(addrSoundCharOpen, "\x90\x90\x90\x90\x90", 5);
		//util::MemoryMove(addrSoundCharOpen2, "\x90\x90\x90\x90\x90", 5);
		//util::MemoryMove(addrSoundCharClose, "\x90\x90\x90\x90\x90", 5);
		//util::MemoryMove(addrSoundCharClose2, "\x90\x90\x90\x90\x90", 5);
		//*timerAddr = 0;
		//util::MemoryMove(timerAddr2, "\x90\x90\x90\x90\x90\x90", 6);
		//util::MemoryMove(timerAddr3, "\x90\x90\x90\x90\x90\x90", 6);
		////////////////////////////////////////////////////////////////////////////////////////////

		util::MemoryMove(battleLoopAddr, "\x90\x90", 2);
		IS_BATTLE_PROC_FLAG = false;
	}
}

//顯示特效
void GameService::WM_EnableEffect(bool enable)
{
	if (g_hGameModule == nullptr)
		return;
	DWORD effectAddr = CONVERT_GAMEVAR<DWORD>(0x434DD);
	DWORD effectAddr2 = CONVERT_GAMEVAR<DWORD>(0x482F0);
	DWORD effectAddr3 = CONVERT_GAMEVAR<DWORD>(0x48DE6);
	DWORD effectAddr4 = CONVERT_GAMEVAR<DWORD>(0x49029);
	DWORD effectAddr5 = CONVERT_GAMEVAR<DWORD>(0x7BDF2);
	if (enable)
	{
		//sa_8001sf.exe+434DD - C7 05 F0 0D 63 04 C8 00 00 00 - mov [sa_8001sf.exe+4230DF0],000000C8 { (3),200 }
		util::MemoryMove(effectAddr, "\xC7\x05\xF0\x0D\x63\x04\xC8\x00\x00\x00", 10);


		///*
		//	sa_8001sf.exe+482F0 - 0F8C FA020000         - jl sa_8001sf.exe+485F0
		//	sa_8001sf.exe+482F6 - 8B 76 28              - mov esi,[esi+28]
		//	sa_8001sf.exe+482F9 - 83 FE FF              - cmp esi,-01 { 255 }
		//	sa_8001sf.exe+482FC - 75 12                 - jne sa_8001sf.exe+48310

		//*/
		//util::MemoryMove(effectAddr2, "\x0F\x8C\xFA\x02\x00\x00\x8B\x76\x28\x83\xFE\xFF\x75\x12", 14);
		// 
		// 
		//sa_8001sf.exe+48DE6 - 7C 42                 - jl sa_8001sf.exe+48E2A
		util::MemoryMove(effectAddr3, "\x7C\x42", 2);
		//sa_8001sf.exe+49029 - 8B 4D 34              - mov ecx,[ebp+34]
		//sa_8001sf.exe + 4902C - 3B C1 - cmp eax, ecx
		util::MemoryMove(effectAddr4, "\x8B\x4D\x34\x3B\xC1", 5);
		//sa_8001sf.exe+7BDF2 - 0F87 20010000         - ja sa_8001sf.exe+7BF18
		util::MemoryMove(effectAddr5, "\x0F\x87\x20\x01\x00\x00", 6);

	}
	else
	{
		//sa_8001sf.exe+434DD - EB 08                 - jmp sa_8001sf.exe+434E7
		util::MemoryMove(effectAddr, "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90", 10);


		///*
		//sa_8001sf.exe+482F0 - EB 04                 - jmp sa_8001sf.exe+482F6
		//sa_8001sf.exe+482F2 - FA                    - cli
		//sa_8001sf.exe+482F3 - 02 00                 - add al,[eax]
		//sa_8001sf.exe+482F5 - 00 8B 762883FE        - add [ebx-sa_8001sf.exe+13CD78A],cl
		//sa_8001sf.exe+482FB - FF 75 12              - push [ebp+12]

		//*/
		//util::MemoryMove(effectAddr2, "\xEB\x04\xFA\x02\x00\x00\x00\x8B\x76\x28\x83\xFE\xFF\x75\x12", 14);


		//sa_8001sf.exe+48DE6 - EB 42                 - jmp sa_8001sf.exe+48E2A
		util::MemoryMove(effectAddr3, "\xEB\x42", 2);
		//sa_8001sf.exe+49029 - 3D 00000000           - cmp eax,00000000 { 0 }
		util::MemoryMove(effectAddr4, "\x3D\x00\x00\x00\x00", 5);
		/*
		sa_8001sf.exe+7BDF2 - 90                    - nop
		sa_8001sf.exe+7BDF3 - E9 20010000           - jmp sa_8001sf.exe+7BF18
		*/
		util::MemoryMove(effectAddr5, "\x90\xE9\x20\x01\x00\x00", 6);
	}
}

//顯示人物
void GameService::WM_EnablePlayerShow(bool enable)
{
	if (g_hGameModule == nullptr)
		return;
	DWORD playerShowAddr = CONVERT_GAMEVAR<DWORD>(0xEA30);
	DWORD playerShowAddr2 = CONVERT_GAMEVAR<DWORD>(0xEFD0);
	DWORD playerShowAddr3 = CONVERT_GAMEVAR<DWORD>(0xF180);
	if (enable)
	{
		//sa_8001sf.exe+EA30 - 51                    - push ecx
		util::MemoryMove(playerShowAddr, "\x51", 1);
		//sa_8001sf.exe+EFD0 - 8B 44 24 04           - mov eax,[esp+04]
		util::MemoryMove(playerShowAddr2, "\x8B\x44\x24\x04", 4);
		//sa_8001sf.exe+F180 - 8B 44 24 04           - mov eax,[esp+04]
		util::MemoryMove(playerShowAddr3, "\x8B\x44\x24\x04", 4);
	}
	else
	{
		//sa_8001sf.exe+EA30 - C3                    - ret 
		util::MemoryMove(playerShowAddr, "\xC3", 1);
		//sa_8001sf.exe+EFD0 - C3                    - ret 
		util::MemoryMove(playerShowAddr2, "\xC3\x90\x90\x90", 4);
		//sa_8001sf.exe+F180 - C3                    - ret
		util::MemoryMove(playerShowAddr3, "\xC3\x90\x90\x90", 4);
	}
}

//鎖定時間
void GameService::WM_SetTimeLock(bool enable, int time)
{
	if (g_hGameModule == nullptr)
		return;
	DWORD timerefreshAddr = CONVERT_GAMEVAR<DWORD>(0x1E6D0);
	if (!enable)
	{
		//sa_8001sf.exe+1E6D0 - 56                    - push esi
		//util::MemoryMove(timerefreshAddr, "\x56", 1);
		IS_TIME_LOCK_FLAG = false;
	}
	else
	{
		if (time >= 0 && time <= 4)
		{
			int* pcurrentTime = CONVERT_GAMEVAR<int*>(0x14CD00);
			int* pa = CONVERT_GAMEVAR<int*>(0x14CD68);
			int* pb = CONVERT_GAMEVAR<int*>(0x14D170);
			int* pc = CONVERT_GAMEVAR<int*>(0x1AB740);
			int* pd = CONVERT_GAMEVAR<int*>(0x1AB74C);

			constexpr int set[5 * 5] = {
				832, 3, 0, 0,    0, //下午
				64,  0, 1, 256,  1, //黃昏
				320, 1, 2, 512,  2, //午夜
				576, 2, 3, 768,  3, //早晨
				832, 3, 0, 1024, 0  //中午
			};

			//sa_8001sf.exe+1E6D0 - C3                    - ret 
			//util::MemoryMove(timerefreshAddr, "\xC3", 1);//把刷新時間的循環ret掉
			IS_TIME_LOCK_FLAG = true;

			*pcurrentTime = set[time * 5 + 0];
			*pa = set[time * 5 + 1];
			*pb = set[time * 5 + 2];
			*pc = set[time * 5 + 3];
			*pd = set[time * 5 + 4];
		}
	}
}

//打開聲音
void GameService::WM_EnableSound(bool enable)
{
	if (g_hGameModule == nullptr)
		return;
	int* pBackgroundMusic = CONVERT_GAMEVAR<int*>(0xD36E0);
	int* pSoundEffect = CONVERT_GAMEVAR<int*>(0xD36DC);
	auto pSetSound = CONVERT_GAMEVAR<void(__cdecl*)()>(0x880F0);
	if (enable)
	{
		*pBackgroundMusic = currentMusic_;
		*pSoundEffect = currentSound_;
		pSetSound();
	}
	else
	{
		currentMusic_ = *pBackgroundMusic;
		currentSound_ = *pSoundEffect;
		*pBackgroundMusic = 0;
		*pSoundEffect = 0;
		pSetSound();
	}
}

//鎖定畫面
void GameService::WM_EnableImageLock(bool enable)
{
	if (g_hGameModule == nullptr)
		return;
	DWORD pImageLock = CONVERT_GAMEVAR<DWORD>(0x42A7B);
	if (!enable)
	{
		/*
		sa_8001sf.exe+42A7B - E8 F0 FD FF FF           - call sa_8001sf.exe+42870
		sa_8001sf.exe+42A80 - 8B 15 44 A6 56 04        - mov edx,[sa_8001sf.exe+416A644] { (1280.00) }
		sa_8001sf.exe+42A86 - A1 48 A6 56 04           - mov eax,[sa_8001sf.exe+416A648] { (896.00) }
		sa_8001sf.exe+42A8B - 89 15 98 29 58 04        - mov [sa_8001sf.exe+4182998],edx { (1280.00) }
		sa_8001sf.exe+42A91 - A3 94 29 58 04           - mov [sa_8001sf.exe+4182994],eax { (896.00) }
		*/
		util::MemoryMove(pImageLock, "\xE8\xF0\xFD\xFF\xFF\x8B\x15\x44\xA6\x56\x04\xA1\x48\xA6\x56\x04\x89\x15\x98\x29\x58\x04\xA3\x94\x29\x58\x04", 27);
	}
	else
	{
		//27 nop
		util::MemoryMove(pImageLock, "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90", 27);
	}
}

//橫衝直撞
void GameService::WM_EnablePassWall(bool enable)
{
	if (g_hGameModule == nullptr)
		return;
	DWORD pPassWall = CONVERT_GAMEVAR<DWORD>(0x42051);
	if (!enable)
	{
		//sa_8001sf.exe+42051 - 66 83 3C 55 749B5604 01 - cmp word ptr [edx*2+sa_8001sf.exe+4169B74],01 { 1 }
		util::MemoryMove(pPassWall, "\x66\x83\x3C\x55\x74\x9B\x56\x04\x01", 9);
	}
	else
	{
		//sa_8001sf.exe+42051 - ret nop...
		util::MemoryMove(pPassWall, "\xC3\x90\x90\x90\x90\x90\x90\x90\x90", 9);
	}
}

//快速走路
void GameService::WM_EnableFastWalk(bool enable)
{
	if (g_hGameModule == nullptr)
		return;
	DWORD pFastWalk = CONVERT_GAMEVAR<DWORD>(0x42EB8);
	if (!enable)
	{
		//sa_8001sf.exe+42EB8 - 75 33                 - jne sa_8001sf.exe+42EED
		util::MemoryMove(pFastWalk, "\x75\x33", 2);
	}
	else
	{
		util::MemoryMove(pFastWalk, "\x90\x90", 2);
	}
}

//加速
void GameService::WM_SetBoostSpeed(bool enable, int speed)
{
	if (g_hGameModule == nullptr)
		return;
	DWORD pBoostSpeed = CONVERT_GAMEVAR<DWORD>(0x1DEE4);
	int* pSpeed = CONVERT_GAMEVAR<int*>(0xAB7CC);
	if (!enable || (speed <= 0))
	{
		//sa_8001sf.exe+1DEE4 - 83 F9 02              - cmp ecx,02 { 2 }
		*pSpeed = 14;
		util::MemoryMove(pBoostSpeed, "\x83\xF9\x02", 3);
		speedBoostValue = 1;
	}
	else
	{
		if (speed >= 1 && speed <= 14)
		{
			*pSpeed = 14 - speed + 1;
			//sa_8001sf.exe+1DEE4 - 83 F9 0E              - cmp ecx,0E { 14 }
			util::MemoryMove(pBoostSpeed, "\x83\xF9\x0E", 3);
			speedBoostValue = 1;
		}
		else if (speed > 14 && speed <= 125)
		{
			*pSpeed = 1;
			BYTE* pBoostSpeedValue = reinterpret_cast<BYTE*>(pBoostSpeed + 2);
			*pBoostSpeedValue = static_cast<BYTE>(speed + 2);
			speedBoostValue = speed - 13;
		}
	}
}

//鎖定原地
void GameService::WM_EnableMoveLock(bool enable)
{
	if (g_hGameModule == nullptr)
		return;
	//DWORD pMoveLock = CONVERT_GAMEVAR<DWORD>(0x42773);
	if (enable != IS_MOVE_LOCK)
	{
		IS_MOVE_LOCK = enable;
		DWORD* pMoveStart = CONVERT_GAMEVAR<DWORD*>(0x42795 + 0x6);
		*pMoveStart = !enable ? 1UL : 0UL;

	}

	if (!enable)
	{
		DWORD* pMoveStart = CONVERT_GAMEVAR<DWORD*>(0x4181198);
		*pMoveStart = 0UL;
		pMoveStart = CONVERT_GAMEVAR<DWORD*>(0x41829FC);
		*pMoveStart = 0UL;
	}

	//DWORD* pMoveLock = CONVERT_GAMEVAR<DWORD*>(0x4275E);
	//if (!enable)
	//{
	//	//00442773 - 77 2A                 - ja 0044279F
	//	util::MemoryMove(pMoveLock, "\x74\x3F", 2);

	//	//sa_8001.exe+42795 - C7 05 E0295804 01000000 - mov [sa_8001.exe+41829E0],00000001 { (0),1 }
	//	//util::MemoryMove(pMoveLock, "\x01", 1);
	//}
	//else
	//{
	//	//sa_8001sf.exe+42773 - EB 2A                 - jmp sa_8001sf.exe+4279F  //直接改跳轉會有機率完全卡住恢復後不能移動
	//	util::MemoryMove(pMoveLock, "\xEB\x3F", 2);

	//	//fill \x90  取代的是把切換成1會觸發移動的值移除掉
	//	//util::MemoryMove(pMoveLock, "\x00", 1);
	//}
}

//公告
void GameService::WM_Announce(char* str, int color)
{
	if (g_hGameModule == nullptr)
		return;
	auto pAnnounce = CONVERT_GAMEVAR<void(_cdecl*)(char*, int)>(0x115C0);
	if (pAnnounce != nullptr)
		pAnnounce(str, color);
}

//移動
void GameService::WM_Move(int x, int y)
{
	/*
	sa_8001sf.exe+4277A - 8B 15 38A65604        - mov edx,[sa_8001sf.exe+416A638] { (98) }
	sa_8001sf.exe+42780 - A3 54B74B00           - mov [sa_8001sf.exe+BB754],eax { (580410503) }
	sa_8001sf.exe+42785 - A1 3CA65604           - mov eax,[sa_8001sf.exe+416A63C] { (0.01) }
	sa_8001sf.exe+4278A - 89 15 4C025604        - mov [sa_8001sf.exe+416024C],edx { (100) }
	sa_8001sf.exe+42790 - A3 50025604           - mov [sa_8001sf.exe+4160250],eax { (100) }
	sa_8001sf.exe+42795 - C7 05 E0295804 01000000 - mov [sa_8001sf.exe+41829E0],00000001 { (0),1 }

	*/
	if (g_hGameModule == nullptr)
		return;
	int* goalX = CONVERT_GAMEVAR<int*>(0x416024C);
	int* goalY = CONVERT_GAMEVAR<int*>(0x4160250);
	int* walking = CONVERT_GAMEVAR<int*>(0x41829E0);
	if (g_hGameModule != nullptr)
	{
		*goalX = x;
		*goalY = y;
		*walking = 1;
	}
}

//銷毀NPC對話框
void GameService::WM_DistoryDialog()
{
	if (g_hGameModule == nullptr)
		return;

	auto a = CONVERT_GAMEVAR<int*>(0xB83EC);
	if (a != nullptr)
		*a = -1;
	auto distory = CONVERT_GAMEVAR<void(_cdecl*)(int)>(0x11C0);
	if (distory != nullptr)
	{
		int* pDialog = CONVERT_GAMEVAR<int*>(0x415EF98);
		if (pDialog != nullptr)
		{
			if (*pDialog)
			{
				distory(*pDialog);
				*pDialog = 0;
			}
		}
	}

}

//清空聊天緩存
void GameService::WM_CleanChatHistory()
{
	if (g_hGameModule == nullptr)
		return;

	//sa_8001.exe + 117C0 - B8 884D5400 - mov eax, sa_8001.exe + 144D88 { (0) }
	//sa_8001.exe + 117C5 - C6 00 00 - mov byte ptr[eax], 00 { 0 }
	//sa_8001.exe + 117C8 - 05 0C010000 - add eax, 0000010C { 268 }
	//sa_8001.exe + 117CD - 3D 78625400 - cmp eax, sa_8001.exe + 146278 { (652401777) }
	//sa_8001.exe + 117D2 - 7C F1 - jl sa_8001.exe + 117C5
	//sa_8001.exe + 117D4 - C7 05 F8A45400 00000000 - mov[sa_8001.exe + 14A4F8], 00000000 { (0), 0 }
	//sa_8001.exe + 117DE - C3 - ret
	char* chatbuffer = CONVERT_GAMEVAR<char*>(0x144D88);
	char* pmaxchatbuffer = CONVERT_GAMEVAR<char*>(0x146278);
	int* pchatsize = CONVERT_GAMEVAR<int*>(0x14A4F8);
	constexpr size_t bufsize = 268;
	const size_t count = pmaxchatbuffer - chatbuffer;
	for (size_t i = 0; i < count; i += bufsize)
	{
		*chatbuffer = '\0';
		chatbuffer += bufsize;
	}
	*pchatsize = 0;
}

//創建對話框
void GameService::WM_CreateDialog(int button, const char* data)
{
	if (g_hGameModule == nullptr)
		return;
	//push 45ADF50
	//push 4D2  //自訂對話框編號
	//push 10E1 //自訂NPCID
	//mov edx, 0x1
	//or edx, 0x2
	//or edx, 0x4
	//push edx //按鈕
	//push 2 //對話框類型
	//push 0 //sockfd
	//call 00464AC0
	//add esp, 18
	using CreateDialog_t = void(_cdecl*)(int, int, int, int, int, const char*);
	CreateDialog_t createDialog = CONVERT_GAMEVAR<CreateDialog_t>(0x64AC0);
	if (createDialog != nullptr)
	{
		createDialog(0, 2, button, 0x10E1, 0x4D2, data);
	}
}

//lssproto_EN_recv
//ASSA 是在 sa_8001sf.exe+64E2B - E9 5FC9A019           - jmp 19E7178F 下勾子
//然後處理完跳轉回 0FA91778 - E9 B4369DF0           - jmp sa_8001sf.exe+64E31  或直接ret 
//sa_8001sf.exe+64E31 - 83 F8 04              - cmp eax,04 { 4 }
/*
sa_8001sf.exe+8D516 - 83 F8 07              - cmp eax,07 { 7 }
sa_8001sf.exe+8D519 - 75 65                 - jne sa_8001sf.exe+8D580
sa_8001sf.exe+8D51B - 8D 4C 24 1C           - lea ecx,[esp+1C]
sa_8001sf.exe+8D51F - 51                    - push ecx
sa_8001sf.exe+8D520 - 6A 02                 - push 02 { 2 }
sa_8001sf.exe+8D522 - E8 69FCFFFF           - call sa_8001sf.exe+8D190
sa_8001sf.exe+8D527 - 8D 54 24 20           - lea edx,[esp+20]
sa_8001sf.exe+8D52B - 8B F0                 - mov esi,eax
sa_8001sf.exe+8D52D - 52                    - push edx
sa_8001sf.exe+8D52E - 6A 03                 - push 03 { 3 }
sa_8001sf.exe+8D530 - E8 5BFCFFFF           - call sa_8001sf.exe+8D190
sa_8001sf.exe+8D535 - 03 F0                 - add esi,eax
sa_8001sf.exe+8D537 - 8D 44 24 14           - lea eax,[esp+14]
sa_8001sf.exe+8D53B - 50                    - push eax
sa_8001sf.exe+8D53C - 6A 04                 - push 04 { 4 }
sa_8001sf.exe+8D53E - E8 4DFCFFFF           - call sa_8001sf.exe+8D190
sa_8001sf.exe+8D543 - 8B 44 24 1C           - mov eax,[esp+1C]
sa_8001sf.exe+8D547 - 83 C4 18              - add esp,18 { 24 }
sa_8001sf.exe+8D54A - 3B F0                 - cmp esi,eax
sa_8001sf.exe+8D54C - 0F85 2E190000         - jne sa_8001sf.exe+8EE80
sa_8001sf.exe+8D552 - 8B 4C 24 18           - mov ecx,[esp+18]
sa_8001sf.exe+8D556 - 8B 54 24 1C           - mov edx,[esp+1C]
sa_8001sf.exe+8D55A - 8B 84 24 34C00000     - mov eax,[esp+0000C034]
sa_8001sf.exe+8D561 - 51                    - push ecx
sa_8001sf.exe+8D562 - 52                    - push edx
sa_8001sf.exe+8D563 - 50                    - push eax
sa_8001sf.exe+8D564 - E8 A778FDFF           - call sa_8001sf.exe+64E10
sa_8001sf.exe+8D569 - 83 C4 0C              - add esp,0C { 12 }
sa_8001sf.exe+8D56C - C7 05 B808EC04 00000000 - mov [sa_8001sf.exe+4AC08B8],00000000 { (0),0 }
sa_8001sf.exe+8D576 - 33 C0                 - xor eax,eax
sa_8001sf.exe+8D578 - 5E                    - pop esi
sa_8001sf.exe+8D579 - 81 C4 2CC00000        - add esp,0000C02C { 49196 }
sa_8001sf.exe+8D57F - C3                    - ret
*/

////接收伺服器發來已經送出戰鬥封包則切換標誌
//void GameService::WM_ServerNotifyBattleCommandEnd()
//{
//	/*
//	sa_8001sf.exe+4608 - 8B 0D CC214E00        - mov ecx,[sa_8001sf.exe+E21CC] { (0) }
//	sa_8001sf.exe+460F - 8B 04 8D 28E4EB04     - mov eax,[ecx*4+sa_8001sf.exe+4ABE428]
//	sa_8001sf.exe+4616 - B9 40000000           - mov ecx,00000040 { 64 }
//	sa_8001sf.exe+461B - 8B B0 A0000000        - mov esi,[eax+000000A0]
//	sa_8001sf.exe+4621 - 0B F1                 - or esi,ecx
//	sa_8001sf.exe+4623 - 89 B0 A0000000        - mov [eax+000000A0],esi
//	sa_8001sf.exe+4629 - 8B 15 CC214E00        - mov edx,[sa_8001sf.exe+E21CC] { (0) }
//	sa_8001sf.exe+4631 - 8B 04 95 3CE4EB04     - mov eax,[edx*4+sa_8001sf.exe+4ABE43C]
//	sa_8001sf.exe+4639 - 8B 90 A0000000        - mov edx,[eax+000000A0]
//	sa_8001sf.exe+463F - 0B D1                 - or edx,ecx
//	sa_8001sf.exe+4641 - 89 90 A0000000        - mov [eax+000000A0],edx
//	sa_8001sf.exe+4647 - A1 F00D6304           - mov eax,[sa_8001sf.exe+4230DF0] { (3) }
//	sa_8001sf.exe+464C - 40                    - inc eax
//	sa_8001sf.exe+464D - A3 F00D6304           - mov [sa_8001sf.exe+4230DF0],eax { (3) }
//	*/
//
//	__asm
//	{
//		mov ecx, [0x04E21CC]
//		mov eax, [ecx * 0x4 + 0x4EBE428]
//		mov esi, [eax + 0x00000A0]
//
//		mov ecx, 0x0000040
//		or esi, ecx
//		mov[eax + 0x00000A0], esi
//
//		mov edx, [0x04E21CC]
//		mov eax, [edx * 0x4 + 0x4EBE43C]
//		mov edx, [eax + 0x00000A0]
//
//		or edx, ecx
//		mov[eax + 0x00000A0], edx
//
//		mov eax, [0x4630DF0]
//		inc eax
//		mov ecx, 0x4630DF0
//		mov[ecx], eax
//	}
//
//	//auto update = [](int* addr)
//	//{
//	//	int* pE21CC = CONVERT_GAMEVAR<int*>(0xE21CC);
//	//	int E21CCValue = *pE21CC;
//	//	int value = E21CCValue;
//
//	//	int* ptr = ((E21CCValue) * 4 + addr);
//	//	int* temp = (ptr + 0xA0);
//	//	(*temp) |= 0x40;
//	//};
//
//
//	//g_battleNotifyHasSend = false;
//
//	//update(CONVERT_GAMEVAR<int*>(0x4ABE428));
//	//update(CONVERT_GAMEVAR<int*>(0x4ABE43C));
//
//	//int* addr4230DF0 = CONVERT_GAMEVAR<int*>(0x4230DF0);
//	//(*addr4230DF0)++;
//}

////

int GameService::connectServer(SOCKET& rsocket, const char* ip, unsigned short port)
{
	int nRet = -1;
	BOOL bConnected = FALSE;

	rsocket = psocket(AF_INET, SOCK_STREAM, 0);
	//套接字創建失敗！
	if (rsocket == INVALID_SOCKET)
		return -1;

	sockaddr_in serverAddr{};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);

	if (inet_pton(AF_INET, ip, &(serverAddr.sin_addr)) <= 0)
	{
		std::cout << "Invalid address/ Address not supported \n";
		return -1;
	}

	//設置接收超時時間為35秒
	int nTimeout = 35000;
	if (setsockopt(rsocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&nTimeout, sizeof(nTimeout)) == SOCKET_ERROR)
	{
		std::cout << "setsockopt failed with error code : " << WSAGetLastError() << std::endl;
		return -2;
	}

	//把當前套接字設為非阻塞模式
	unsigned long nFlag = 1;
	nRet = ioctlsocket(rsocket, FIONBIO, (unsigned long*)&nFlag);
	if (nRet == SOCKET_ERROR)//把當前套接字設為非阻塞模式失敗!
	{
		std::cout << "ioctlsocket failed with error code : " << WSAGetLastError() << std::endl;
		return -3;
	}

	//非阻塞模式下執行I/O操作時，Winsock函數會立即返回並交出控制權。這種模式使用起來比較覆雜，
	//因為函數在沒有運行完成就進行返回，會不斷地返回WSAEWOULDBLOCK錯誤。
	if (pconnect(rsocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		struct timeval timeout = { 0 };
		timeout.tv_sec = 10;	//連接超時時間為10秒,此值過小會造成多線程同時連接服務端時因無法建立連接而通信失敗
		timeout.tv_usec = 0;	//產生此情況的原因是線程的執行順序是不可預知的

		fd_set fdWrite;
		FD_ZERO(&fdWrite);
		FD_SET(rsocket, &fdWrite);

		int nError = -1;
		int nLen = sizeof(int);
		nRet = select(rsocket, 0, &fdWrite, 0, &timeout);
		if (nRet > 0)
		{
			getsockopt(rsocket, SOL_SOCKET, SO_ERROR, (char*)&nError, &nLen);
			if (nError != 0)
				bConnected = FALSE;
			else
				bConnected = TRUE;
		}
		else
			bConnected = FALSE;
	}

	//再設置回阻塞模式
	nFlag = 0;
	ioctlsocket(rsocket, FIONBIO, (unsigned long*)&nFlag);
	//若連接失敗則返回
	if (bConnected == FALSE)//與服務器建立連接失敗！
	{
		std::cout << "connect failed with error code : " << WSAGetLastError() << std::endl;
		return -4;
	}

	return 1;
}