#include "pch.h"
#include "sadll.h"

#ifdef USE_ASYNC_TCP
#include "asyncclient.h"
#else
#include "syncclient.h"
#endif

//#define USE_MINIDUMP

#pragma comment(lib, "detours.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")

WNDPROC g_OldWndProc = nullptr;
HWND g_MainHwnd = nullptr;
HMODULE g_hGameModule = nullptr;
HMODULE g_hDllModule = nullptr;
WCHAR g_szGameModulePath[MAX_PATH] = {};
DWORD g_MainThreadId = 0;
HANDLE g_MainThreadHandle = nullptr;
HWND g_ParenthWnd = nullptr;

template<typename T>
inline static T CONVERT_GAMEVAR(ULONG_PTR offset) { return (T)((reinterpret_cast<ULONG_PTR>(g_hGameModule) + offset)); }

#pragma region Debug
#ifdef USE_MINIDUMP
#include <DbgHelp.h>
#include <assert.h>
#include <cassert>
#pragma comment(lib, "dbghelp.lib")
static LONG CALLBACK MinidumpCallback(PEXCEPTION_POINTERS pException)
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
#pragma endregion

#pragma region New
extern "C"
{
	//new send
	static int WSAAPI New_send(SOCKET s, const char* buf, int len, int flags)
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_send(s, buf, len, flags);
	}
#if 0
	//new socket
	static SOCKET WSAAPI New_socket(int af, int type, int protocol)
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_socket(af, type, protocol);
	}

	//new connect
	static int WSAAPI New_connect(SOCKET s, const struct sockaddr* name, int namelen)
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_connect(s, name, namelen);
	}

	//new inet_addr
	static unsigned long WSAAPI New_inet_addr(const char* cp)
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_inet_addr(cp);
	}

	//new ntohs
	static u_short WSAAPI New_ntohs(u_short netshort)
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_ntohs(netshort);
	}

#endif

	//new recv
	static int WSAAPI New_recv(SOCKET s, char* buf, int len, int flags)
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_recv(s, buf, len, flags);
	}

	//new closesocket
	static int WSAAPI New_closesocket(SOCKET s)
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_closesocket(s);
	}

	//new SetWindowTextA
	static BOOL WINAPI New_SetWindowTextA(HWND hWnd, LPCSTR lpString)
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_SetWindowTextA(hWnd, lpString);
	}

	static DWORD WINAPI New_GetTickCount()
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_GetTickCount();
	}

	static BOOL WINAPI New_QueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount)
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_QueryPerformanceCounter(lpPerformanceCount);
	}

	static DWORD WINAPI New_TimeGetTime()
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_TimeGetTime();
	}

	static void WINAPI New_Sleep(DWORD dwMilliseconds)
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_Sleep(dwMilliseconds);
	}

	/////// game client ///////

	//PlaySound
	static void __cdecl New_PlaySound(int a, int b, int c)
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_PlaySound(a, b, c);
	}

	//BattleProc
	static void __cdecl New_BattleProc()
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_BattleProc();
	}

	//BattleCommandReady
	static void __cdecl New_BattleCommandReady()
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_BattleCommandReady();
	}

	//TimeProc
	static void __cdecl New_TimeProc(int fd)
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_TimeProc(fd);
	}

	//lssproto_EN_recv
	static void __cdecl New_lssproto_EN_recv(int fd, int result, int field)
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_lssproto_EN_recv(fd, result, field);
	}

	//lssproto_B_recv
	static void __cdecl New_lssproto_B_recv(int fd, char* command)
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_lssproto_B_recv(fd, command);
	}

	//lssproto_WN_send
	static void _cdecl New_lssproto_WN_send(int fd, int x, int y, int dialogid, int unitid, int select, char* data)
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_lssproto_WN_send(fd, x, y, dialogid, unitid, select, data);
	}

	static void __cdecl New_lssproto_TK_send(int fd, int x, int y, const char* message, int color, int area)
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_lssproto_TK_send(fd, x, y, message, color, area);
	}

	static void __cdecl New_lssproto_W2_send(int fd, int x, int y, const char* dir)
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_lssproto_W2_send(fd, x, y, dir);
	}

	static void __cdecl New_CreateDialog(int unk, int type, int button, int unitid, int dialogid, const char* data)
	{
		GameService& g_GameService = GameService::getInstance();
		return g_GameService.New_CreateDialog(unk, type, button, unitid, dialogid, data);
	}
}

static void setSocket(SOCKET fd)
{
	int option_value = 1; //禁用Nagle
	setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&option_value, sizeof(option_value));
	BOOL keepAlive = TRUE; //Keep-Alive
	setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (const char*)&keepAlive, sizeof(BOOL));
	struct tcp_keepalive in_keep_alive = {};
	unsigned long ul_in_len = sizeof(struct tcp_keepalive);
	struct tcp_keepalive out_keep_alive = {};
	unsigned long ul_out_len = sizeof(struct tcp_keepalive);
	unsigned long ul_bytes_return = 0;
	in_keep_alive.onoff = 1; /*打開keepalive*/
	in_keep_alive.keepaliveinterval = 5000; /*發送keepalive心跳時間間隔-單位為毫秒*/
	in_keep_alive.keepalivetime = 1000; /*多長時間沒有報文開始發送keepalive心跳包-單位為毫秒*/

	WSAIoctl(fd, SIO_KEEPALIVE_VALS, (LPVOID)&in_keep_alive, ul_in_len, (LPVOID)&out_keep_alive, ul_out_len, &ul_bytes_return, NULL, NULL);

	constexpr int dscpValue = 0xe0 | 0x10 | 0x04;  // 0x3F DSCP 63
	setsockopt(fd, IPPROTO_IP, IP_TOS, (const char*)&dscpValue, sizeof(dscpValue));

	//int nSendBuf = 0;
	//setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (const char*)&nSendBuf, sizeof(int));

	//int nRecvBuf = 0;
	//setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (const char*)&nRecvBuf, sizeof(int));

	//int receiveTimeout = 100;
	//setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&receiveTimeout, sizeof(int));

	//int sendTimeout = 30000;
	//setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&sendTimeout, sizeof(int));
}

//hooks
SOCKET GameService::New_socket(int af, int type, int protocol) const
{
	SOCKET fd = psocket(af, type, protocol);
	if (INVALID_SOCKET != fd)
		setSocket(fd);
	return fd;
}

int GameService::New_send(SOCKET s, const char* buf, int len, int flags)
{
	int snedlen = psend(s, buf, len, flags);
	if (snedlen > 0)
	{
		std::string str = std::string(buf, snedlen);
		//prepend a header SEND|
		str = "SEND|" + str;
		sendToServer(str + "\n");
	}
	return snedlen;
}

int GameService::New_connect(SOCKET s, const struct sockaddr* name, int namelen) const
{
	//if (s && name != nullptr)
	//{
	//	if (AF_INET == name->sa_family)
	//	{
	//		struct sockaddr_in* sockaddr_ipv4 = (struct sockaddr_in*)name;
	//		char ipStr[INET_ADDRSTRLEN] = {};
	//		if (inet_ntop(AF_INET, &(sockaddr_ipv4->sin_addr), ipStr, sizeof(ipStr)) != nullptr)
	//		{
	//			unsigned short port = ntohs(sockaddr_ipv4->sin_port);
	//			std::cout << "IPv4 Address: " << ipStr << " Port: " << port << std::endl;
	//		}
	//	}
	//}
	return pconnect(s, name, namelen);
}

unsigned long GameService::New_inet_addr(const char* cp) const
{
	std::cout << L"inet_addr: " << std::string(cp) << std::endl;
	return pinet_addr(cp);
}

u_short GameService::New_ntohs(u_short netshort) const
{
	std::wcout << L"ntohs: " << std::to_wstring(netshort) << std::endl;
	return pntohs(netshort);
}

//hook recv將封包全部轉發給外掛，本來準備完全由外掛處理好再發回來，但效果不盡人意
int GameService::New_recv(SOCKET s, char* buf, int len, int flags)
{
	int recvlen = precv(s, buf, len, flags);

	if ((recvlen > 0) && (recvlen <= len))
	{
		sendToServer(buf, static_cast<size_t>(recvlen));
	}

	return recvlen;
}

int GameService::New_closesocket(SOCKET s)
{
	if (g_sockfd != nullptr)
	{
		SOCKET cmps = static_cast<SOCKET>(*g_sockfd);
		if ((s != INVALID_SOCKET) && (s > 0u)
			&& (cmps != INVALID_SOCKET) && (cmps > 0u)
			&& (s == cmps))
		{
			sendToServer("dc|1\n");
		}
	}

	int ret = pclosesocket(s);
	return ret;
}

//防止其他私服使用A類函數導致標題亂碼
BOOL GameService::New_SetWindowTextA(HWND, LPCSTR)
{
	return TRUE;
}

DWORD GameService::New_GetTickCount() const
{
	static DWORD g_dwRealTick = pGetTickCount();
	static DWORD g_dwHookTick = pGetTickCount();

	DWORD tmpTick = pGetTickCount();

	g_dwHookTick = g_dwHookTick + speedBoostValue * (tmpTick - g_dwRealTick);
	g_dwRealTick = tmpTick;

	return g_dwHookTick;
}

BOOL GameService::New_QueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount) const
{
	BOOL result = pQueryPerformanceCounter(lpPerformanceCount);

	lpPerformanceCount->QuadPart *= static_cast<LONGLONG>(speedBoostValue); // 修改返回值為原始值的兩倍

	return result;
}

DWORD GameService::New_TimeGetTime() const
{
	static DWORD g_dwRealTime = pTimeGetTime();
	static DWORD g_dwHookTime = pTimeGetTime();

	DWORD tmpTime = pTimeGetTime();

	g_dwHookTime = g_dwHookTime + speedBoostValue * (tmpTime - g_dwRealTime);
	g_dwRealTime = tmpTime;

	return g_dwHookTime;
}

void GameService::New_Sleep(DWORD dwMilliseconds)
{
	if ((TRUE == enableSleepAdjust.load(std::memory_order_acquire)) && (0UL == dwMilliseconds))
		dwMilliseconds = 1UL;

	pSleep(dwMilliseconds);
}

//////////////////////////////////////////////////////////////////////

//音效
void GameService::New_PlaySound(int a, int b, int c) const
{
	if (FALSE == IS_SOUND_MUTE_FLAG)
	{
		pPlaySound(a, b, c);
	}
}

//戰鬥循環
void GameService::New_BattleProc() const
{
	if (TRUE == IS_BATTLE_PROC_FLAG)
	{
		pBattleProc();
	}
}

//每回合開始顯示戰鬥面板 (這裡只是切換面板時會順便調用到的函數)
void GameService::New_BattleCommandReady()
{
	sendToServer("bpk|1\n");

	int* p = CONVERT_GAMEVAR<int*>(0x4230DF0ul);
	++(*p);
}

//遊戲時間刷新循環 (早上,下午....)
void GameService::New_TimeProc(int fd) const
{
	if (FALSE == IS_TIME_LOCK_FLAG)
		pTimeProc(fd);
}

//EN封包攔截，戰鬥進場
void GameService::New_lssproto_EN_recv(int fd, int result, int field) const
{
	if (FALSE == IS_ENCOUNT_BLOCK_FLAG)
		pLssproto_EN_recv(fd, result, field);
}

//B封包攔截，戰鬥封包
void GameService::New_lssproto_B_recv(int fd, char* command) const
{
	if (FALSE == IS_ENCOUNT_BLOCK_FLAG)
		pLssproto_B_recv(fd, command);
}

//WN對話框發包攔截
void GameService::New_lssproto_WN_send(int fd, int x, int y, int dialogid, int unitid, int select, const char* data)
{
	*CONVERT_GAMEVAR<int*>(0x4200004ul) = 0;
	if ((1234 == unitid) && (4321 == dialogid))
	{
		std::string str = "dk|";

		str += std::to_string(x) + "|";
		str += std::to_string(y) + "|";
		str += std::to_string(select) + "|";

		str += std::string(data);

		std::cout << "send: " << str << std::endl;

		sendToServer(str + "\n");

		return;
	}

	pLssproto_WN_send(fd, x, y, dialogid, unitid, select, data);
}

//TK對話框收包攔截
void GameService::New_lssproto_TK_send(int fd, int x, int y, const char* message, int color, int area)
{
	const std::string msg = message;

	//查找是否包含'//'
	if (!msg.empty() && msg.find("//") != std::string::npos)
	{
		if (msg.find("//console"))
		{
			if (g_consoleHwnd == nullptr)
				g_consoleHwnd = util::createConsole();
			else
			{
				//is visible
				if (IsWindowVisible(g_consoleHwnd))
					ShowWindow(g_consoleHwnd, SW_HIDE);
				else
					ShowWindow(g_consoleHwnd, SW_SHOW);
			}
			return;
		}

		std::string str = "tk|";
		str += std::to_string(x) + "|";
		str += std::to_string(y) + "|";
		str += std::to_string(color) + "|";
		str += std::to_string(area) + "|";

		str += msg.substr(2u);

		sendToServer(str + "\n");

		return;
	}

	pLssproto_TK_send(fd, x, y, message, color, area);
}

//W2移動收包攔截
void GameService::New_lssproto_W2_send(int fd, int x, int y, const char* message) const
{
	//PostMessageW(g_ParenthWnd, kSetMove, NULL, MAKELPARAM(x, y));
	pLssproto_W2_send(fd, x, y, message);
}

void GameService::New_CreateDialog(int unk, int type, int button, int unitid, int dialogid, const char* data) const
{
	pCreateDialog(unk, type, button, unitid, dialogid, data);
	*CONVERT_GAMEVAR<int*>(0x4200004ul) = 1;

}
#pragma endregion

#pragma region WM
//設置遊戲畫面狀態值
BOOL GameService::WM_SetGameStatus(int status)
{
	std::unique_lock<std::shared_mutex> lock(g_statusLock);

	if (!isInitialized_.load(std::memory_order_acquire))
		return FALSE;

	if (nullptr == g_game_status)
		return FALSE;

	int G = *g_game_status;

	if (G != status)
	{
		*g_game_status = status;
		return TRUE;
	}

	return FALSE;
}

//設置遊戲世界狀態值
BOOL GameService::WM_SetWorldStatus(int status)
{
	std::unique_lock<std::shared_mutex> lock(g_statusLock);

	if (!isInitialized_.load(std::memory_order_acquire))
		return FALSE;

	if (nullptr == g_world_status)
		return FALSE;

	int W = *g_world_status;

	if (W != status)
	{
		*g_world_status = status;
		return TRUE;
	}

	return FALSE;
}

//資源優化
BOOL GameService::WM_SetOptimize(BOOL enable)
{
	if (!isInitialized_.load(std::memory_order_acquire))
		return FALSE;

	if (nullptr == g_hGameModule)
		return FALSE;

	//DWORD optimizeAddr = CONVERT_GAMEVAR<DWORD>(0x129E7);
	if (FALSE == enable)
	{
		/*
		//sa_8001.exe+129E9 - A1 0CA95400           - mov eax,[sa_8001.exe+14A90C] { (05205438) }
		//sa_8001.exe+129EE - 6A 00                 - push 00 { 0 }
		util::MemoryMove(optimizeAddr, "\xA1\x0C\xA9\x54\x00\x6A\x00", 7);

		//sa_8001.exe+129E7 - 75 37                 - jne sa_8001sf.exe+12A20
		util::MemoryMove(optimizeAddr, "\x75\x37", 2);
		*/

		*CONVERT_GAMEVAR<int*>(0xAB7C8ul) = 14;
	}
	else
	{
		/*
		//sa_8001.exe+129E9 - EB 10                 - jmp sa_8001.exe+129FB
		util::MemoryMove(optimizeAddr, "\xEB\x10\x90\x90\x90\x90\x90", 7);

		//sa_8001.exe+129E7 - EB 12                 - jmp sa_8001sf.exe+129FB
		util::MemoryMove(optimizeAddr, "\xEB\x12", 2);
		*/

		*CONVERT_GAMEVAR<int*>(0xAB7C8ul) = 0;
	}

	return TRUE;
}

//隱藏窗口
BOOL GameService::WM_SetWindowHide(BOOL enable)
{
	if (!isInitialized_.load(std::memory_order_acquire))
		return FALSE;

	if (nullptr == g_hGameModule)
		return FALSE;

	//聊天紀錄顯示行數數量設為0
	nowChatRowCount_ = *CONVERT_GAMEVAR<int*>(0xA2674ul);
	if (nowChatRowCount_ < 20)
		*CONVERT_GAMEVAR<int*>(0xA2674ul) = 20;

	if (FALSE == enable)
	{
		//sa_8001sf.exe+129E7 - 75 37                 - jne sa_8001sf.exe+12A20 資源優化關閉
		util::MemoryMove(CONVERT_GAMEVAR<DWORD>(0x129E7ul), "\xEB\x12", 2u);

		//sa_8001sf.exe+1DFFD - 74 31                 - je sa_8001sf.exe+1E030
		util::MemoryMove(CONVERT_GAMEVAR<DWORD>(0x1DFFDul), "\x74\x31", 2u);

		//sa_8001sf.exe+1DEE4 - 83 F9 0E              - cmp ecx,0E { 14 } 加速相關
		//util::MemoryMove(CONVERT_GAMEVAR<DWORD>(0x1DEE4ul), "\x83\xF9\x0E", 3u);
		*reinterpret_cast<BYTE*>(CONVERT_GAMEVAR<DWORD>(0x1DEE4ul) + 2) = 14;
	}
	else
	{
		*CONVERT_GAMEVAR<int*>(0xA2674ul) = 0;

		//sa_8001sf.exe+129E7 - EB 37                 - jmp sa_8001sf.exe+12A20 資源優化開啟
		util::MemoryMove(CONVERT_GAMEVAR<DWORD>(0x129E7ul), "\xEB\x37", 2u);

		//sa_8001sf.exe+1DFFD - EB 31                 - jmp sa_8001sf.exe+1E030 強制跳過整個背景繪製
		util::MemoryMove(CONVERT_GAMEVAR<DWORD>(0x1DFFDul), "\xEB\x31", 2u);

		//sa_8001sf.exe+1DEE4 - 83 F9 0E              - cmp ecx,05 { 14 } 加速相關
		//util::MemoryMove(CONVERT_GAMEVAR<DWORD>(0x1DEE4ul), "\x83\xF9\x7F", 3u);
		*reinterpret_cast<BYTE*>(CONVERT_GAMEVAR<DWORD>(0x1DEE4ul) + 2) = 127;
	}

	enableSleepAdjust.store(enable, std::memory_order_release);

	return TRUE;
}

//靜音
BOOL GameService::WM_MuteSound(BOOL enable)
{
	if (!isInitialized_.load(std::memory_order_acquire))
		return FALSE;

	IS_SOUND_MUTE_FLAG = enable;

	return TRUE;
}

//戰鬥時間延長(99秒)
BOOL GameService::WM_BattleTimeExtend(BOOL enable)
{
	if (!isInitialized_.load(std::memory_order_acquire))
		return FALSE;

	if (nullptr == g_hGameModule)
		return FALSE;

	DWORD* timerAddr = CONVERT_GAMEVAR<DWORD*>(0x9854ul);

	//sa_8001.exe+9853 - 05 30750000           - add eax,00007530 { 30000 }
	if (FALSE == enable)
	{
		*timerAddr = 30UL * 1000UL;
	}
	else
	{
		*timerAddr = 100UL * 1000UL;
	}

	return TRUE;
}

//允許戰鬥面板
BOOL GameService::WM_EnableBattleDialog(BOOL enable)
{
	if (!isInitialized_.load(std::memory_order_acquire))
		return FALSE;

	if (nullptr == g_hGameModule)
		return FALSE;

	//DWORD addr = CONVERT_GAMEVAR<DWORD>(0x98C9ul);
	//DWORD addr2 = CONVERT_GAMEVAR<DWORD>(0x9D07ul);
	//DWORD addrSoundCharOpen = CONVERT_GAMEVAR<DWORD>(0x98DAul);
	//DWORD addrSoundCharOpen2 = CONVERT_GAMEVAR<DWORD>(0x9D13ul);
	//DWORD addrSoundCharClose = CONVERT_GAMEVAR<DWORD>(0x667Dul);
	//DWORD addrSoundCharClose2 = CONVERT_GAMEVAR<DWORD>(0x93C9ul);
	//int* timerAddr = CONVERT_GAMEVAR<int*>(0xE21E0ul);
	//DWORD timerAddr2 = CONVERT_GAMEVAR<DWORD>(0x9841ul);
	//DWORD timerAddr3 = CONVERT_GAMEVAR<DWORD>(0x9DB6ul);
	DWORD battleLoopAddr = CONVERT_GAMEVAR<DWORD>(0xA914ul);
	if (TRUE == enable)
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
		util::MemoryMove(battleLoopAddr, "\xEB\x0C", 2u);
	}
	else
	{
		//util::MemoryMove(addr, "\x90\x90\x90\x90\x90", 5u);
		//util::MemoryMove(addr2, "\x90\x90\x90\x90\x90\x90", 6u);
		//util::MemoryMove(addrSoundCharOpen, "\x90\x90\x90\x90\x90", 5u);
		//util::MemoryMove(addrSoundCharOpen2, "\x90\x90\x90\x90\x90", 5u);
		//util::MemoryMove(addrSoundCharClose, "\x90\x90\x90\x90\x90", 5u);
		//util::MemoryMove(addrSoundCharClose2, "\x90\x90\x90\x90\x90", 5u);
		//*timerAddr = 0;
		//util::MemoryMove(timerAddr2, "\x90\x90\x90\x90\x90\x90", 6u);
		//util::MemoryMove(timerAddr3, "\x90\x90\x90\x90\x90\x90", 6u);
		////////////////////////////////////////////////////////////////////////////////////////////

		util::MemoryMove(battleLoopAddr, "\x90\x90", 2u);
	}

	IS_BATTLE_PROC_FLAG = enable;

	return TRUE;
}

//顯示特效
BOOL GameService::WM_EnableEffect(BOOL enable)
{
	if (!isInitialized_.load(std::memory_order_acquire))
		return FALSE;

	if (nullptr == g_hGameModule)
		return FALSE;

	DWORD effectAddr = CONVERT_GAMEVAR<DWORD>(0x434DDul);
	//DWORD effectAddr2 = CONVERT_GAMEVAR<DWORD>(0x482F0ul);
	DWORD effectAddr3 = CONVERT_GAMEVAR<DWORD>(0x48DE6ul);
	DWORD effectAddr4 = CONVERT_GAMEVAR<DWORD>(0x49029ul);
	DWORD effectAddr5 = CONVERT_GAMEVAR<DWORD>(0x7BDF2ul);
	DWORD effectAddr6 = CONVERT_GAMEVAR<DWORD>(0x60013);
	//DWORD effectAddr7 = CONVERT_GAMEVAR<DWORD>(0x21163);

	if (TRUE == enable)
	{
		//sa_8001sf.exe+434DD - C7 05 F0 0D 63 04 C8 00 00 00 - mov [sa_8001sf.exe+4230DF0],000000C8 { (3),200 }
		util::MemoryMove(effectAddr, "\xC7\x05\xF0\x0D\x63\x04\xC8\x00\x00\x00", 10u);

		//sa_8001.exe.text+5F013 - C7 05 F00D6304 C8000000 - mov [sa_8001.exe+4230DF0],000000C8 { (3),200 }
		util::MemoryMove(effectAddr6, "\xC7\x05\xF0\x0D\x63\x04\xC8\x00\x00\x00", 10u);

		//sa_8001.exe.text+20163 - C7 05 F00D6304 C8000000 - mov [sa_8001.exe+4230DF0],000000C8 { (3),200 }
		//util::MemoryMove(effectAddr7, "\xC7\x05\xF0\x0D\x63\x04\xC8\x00\x00\x00", 10u);

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
		util::MemoryMove(effectAddr3, "\x7C\x42", 2u);
		//sa_8001sf.exe+49029 - 8B 4D 34              - mov ecx,[ebp+34]
		//sa_8001sf.exe + 4902C - 3B C1 - cmp eax, ecx
		util::MemoryMove(effectAddr4, "\x8B\x4D\x34\x3B\xC1", 5u);
		//sa_8001sf.exe+7BDF2 - 0F87 20010000         - ja sa_8001sf.exe+7BF18
		util::MemoryMove(effectAddr5, "\x0F\x87\x20\x01\x00\x00", 6u);
	}
	else
	{
		//sa_8001sf.exe+434DD - EB 08                 - jmp sa_8001sf.exe+434E7
		util::MemoryMove(effectAddr, "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90", 10u);

		//sa_8001.exe.text+5F013 - C7 05 F00D6304 C8000000 - mov [sa_8001.exe+4230DF0],000000C8 { (3),200 }
		util::MemoryMove(effectAddr6, "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90", 10u);

		//sa_8001.exe.text+20163 - C7 05 F00D6304 C8000000 - mov [sa_8001.exe+4230DF0],000000C8 { (3),200 }
		//util::MemoryMove(effectAddr7, "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90", 10u);

		///*
		//sa_8001sf.exe+482F0 - EB 04                 - jmp sa_8001sf.exe+482F6
		//sa_8001sf.exe+482F2 - FA                    - cli
		//sa_8001sf.exe+482F3 - 02 00                 - add al,[eax]
		//sa_8001sf.exe+482F5 - 00 8B 762883FE        - add [ebx-sa_8001sf.exe+13CD78A],cl
		//sa_8001sf.exe+482FB - FF 75 12              - push [ebp+12]

		//*/
		//util::MemoryMove(effectAddr2, "\xEB\x04\xFA\x02\x00\x00\x00\x8B\x76\x28\x83\xFE\xFF\x75\x12", 14);

		//sa_8001sf.exe+48DE6 - EB 42                 - jmp sa_8001sf.exe+48E2A
		util::MemoryMove(effectAddr3, "\xEB\x42", 2u);
		//sa_8001sf.exe+49029 - 3D 00000000           - cmp eax,00000000 { 0 }
		util::MemoryMove(effectAddr4, "\x3D\x00\x00\x00\x00", 5u);
		/*
		sa_8001sf.exe+7BDF2 - 90                    - nop
		sa_8001sf.exe+7BDF3 - E9 20010000           - jmp sa_8001sf.exe+7BF18
		*/
		util::MemoryMove(effectAddr5, "\x90\xE9\x20\x01\x00\x00", 6u);


	}

	return TRUE;
}

//顯示人物
BOOL GameService::WM_EnableCharShow(BOOL enable)
{
	if (!isInitialized_.load(std::memory_order_acquire))
		return FALSE;

	if (nullptr == g_hGameModule)
		return FALSE;

	DWORD playerShowAddr = CONVERT_GAMEVAR<DWORD>(0xEA30ul);
	DWORD playerShowAddr2 = CONVERT_GAMEVAR<DWORD>(0xEFD0ul);
	DWORD playerShowAddr3 = CONVERT_GAMEVAR<DWORD>(0xF180ul);

	if (TRUE == enable)
	{
		//sa_8001sf.exe+EA30 - 51                    - push ecx
		util::MemoryMove(playerShowAddr, "\x51", 1u);
		//sa_8001sf.exe+EFD0 - 8B 44 24 04           - mov eax,[esp+04]
		util::MemoryMove(playerShowAddr2, "\x8B\x44\x24\x04", 4u);
		//sa_8001sf.exe+F180 - 8B 44 24 04           - mov eax,[esp+04]
		util::MemoryMove(playerShowAddr3, "\x8B\x44\x24\x04", 4u);
	}
	else
	{
		//sa_8001sf.exe+EA30 - C3                    - ret 
		util::MemoryMove(playerShowAddr, "\xC3", 1u);
		//sa_8001sf.exe+EFD0 - C3                    - ret 
		util::MemoryMove(playerShowAddr2, "\xC3\x90\x90\x90", 4u);
		//sa_8001sf.exe+F180 - C3                    - ret
		util::MemoryMove(playerShowAddr3, "\xC3\x90\x90\x90", 4u);
	}

	return TRUE;
}

//鎖定時間
BOOL GameService::WM_SetTimeLock(BOOL enable, unsigned int time)
{
	if (!isInitialized_.load(std::memory_order_acquire))
		return FALSE;

	if (nullptr == g_hGameModule)
		return FALSE;

	//DWORD timerefreshAddr = CONVERT_GAMEVAR<DWORD>(0x1E6D0);
	//sa_8001sf.exe+1E6D0 - 56                    - push esi
		//util::MemoryMove(timerefreshAddr, "\x56", 1);

	if (TRUE == enable)
	{
		if (time <= 4u)
		{
			int* pcurrentTime = CONVERT_GAMEVAR<int*>(0x14CD00ul);
			int* pa = CONVERT_GAMEVAR<int*>(0x14CD68ul);
			int* pb = CONVERT_GAMEVAR<int*>(0x14D170ul);
			int* pc = CONVERT_GAMEVAR<int*>(0x1AB740ul);
			int* pd = CONVERT_GAMEVAR<int*>(0x1AB74Cul);

			constexpr int set[5 * 5] = {
				832, 3, 0, 0,    0, //下午
				64,  0, 1, 256,  1, //黃昏
				320, 1, 2, 512,  2, //午夜
				576, 2, 3, 768,  3, //早晨
				832, 3, 0, 1024, 0  //中午
			};

			//sa_8001sf.exe+1E6D0 - C3                    - ret 
			//util::MemoryMove(timerefreshAddr, "\xC3", 1);//把刷新時間的循環ret掉
			IS_TIME_LOCK_FLAG = TRUE;

			*pcurrentTime = set[time * 5u + 0u];
			*pa = set[time * 5u + 1u];
			*pb = set[time * 5u + 2u];
			*pc = set[time * 5u + 3u];
			*pd = set[time * 5u + 4u];
		}
	}

	IS_TIME_LOCK_FLAG = enable;

	return TRUE;
}

//打開聲音
BOOL GameService::WM_EnableSound(BOOL enable)
{
	if (!isInitialized_.load(std::memory_order_acquire))
		return FALSE;

	if (nullptr == g_hGameModule)
		return FALSE;

	int* pBackgroundMusic = CONVERT_GAMEVAR<int*>(0xD36E0ul);
	int* pSoundEffect = CONVERT_GAMEVAR<int*>(0xD36DCul);
	auto pSetSound = CONVERT_GAMEVAR<void(__cdecl*)()>(0x880F0ul);

	if (TRUE == enable)
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

	return TRUE;
}

//鎖定畫面
BOOL GameService::WM_EnableImageLock(BOOL enable)
{
	if (!isInitialized_.load(std::memory_order_acquire))
		return FALSE;

	if (nullptr == g_hGameModule)
		return FALSE;

	DWORD pImageLock = CONVERT_GAMEVAR<DWORD>(0x42A7Bul);

	if (FALSE == enable)
	{
		/*
		sa_8001sf.exe+42A7B - E8 F0 FD FF FF           - call sa_8001sf.exe+42870
		sa_8001sf.exe+42A80 - 8B 15 44 A6 56 04        - mov edx,[sa_8001sf.exe+416A644] { (1280.00) }
		sa_8001sf.exe+42A86 - A1 48 A6 56 04           - mov eax,[sa_8001sf.exe+416A648] { (896.00) }
		sa_8001sf.exe+42A8B - 89 15 98 29 58 04        - mov [sa_8001sf.exe+4182998],edx { (1280.00) }
		sa_8001sf.exe+42A91 - A3 94 29 58 04           - mov [sa_8001sf.exe+4182994],eax { (896.00) }
		*/
		util::MemoryMove(pImageLock, "\xE8\xF0\xFD\xFF\xFF\x8B\x15\x44\xA6\x56\x04\xA1\x48\xA6\x56\x04\x89\x15\x98\x29\x58\x04\xA3\x94\x29\x58\x04", 27u);
	}
	else
	{
		//27 nop
		util::MemoryMove(pImageLock, "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90", 27u);
	}

	return TRUE;
}

//橫衝直撞
BOOL GameService::WM_EnablePassWall(BOOL enable)
{
	if (!isInitialized_.load(std::memory_order_acquire))
		return FALSE;

	if (nullptr == g_hGameModule)
		return FALSE;

	DWORD pPassWall = CONVERT_GAMEVAR<DWORD>(0x42051ul);

	if (FALSE == enable)
	{
		//sa_8001sf.exe+42051 - 66 83 3C 55 749B5604 01 - cmp word ptr [edx*2+sa_8001sf.exe+4169B74],01 { 1 }
		util::MemoryMove(pPassWall, "\x66\x83\x3C\x55\x74\x9B\x56\x04\x01", 9u);
	}
	else
	{
		//sa_8001sf.exe+42051 - ret nop...
		util::MemoryMove(pPassWall, "\xC3\x90\x90\x90\x90\x90\x90\x90\x90", 9u);
	}

	return TRUE;
}

//快速走路
BOOL GameService::WM_EnableFastWalk(BOOL enable)
{
	if (!isInitialized_.load(std::memory_order_acquire))
		return FALSE;

	if (nullptr == g_hGameModule)
		return FALSE;

	DWORD pFastWalk = CONVERT_GAMEVAR<DWORD>(0x42EB8ul);

	if (FALSE == enable)
	{
		//sa_8001sf.exe+42EB8 - 75 33                 - jne sa_8001sf.exe+42EED
		util::MemoryMove(pFastWalk, "\x75\x33", 2u);
		*CONVERT_GAMEVAR<float*>(0x9C328ul) = 4.0f;

	}
	else
	{
		*CONVERT_GAMEVAR<float*>(0x9C328ul) = 32.0f;
		util::MemoryMove(pFastWalk, "\x90\x90", 2u);
	}

	return TRUE;
}

//加速
BOOL GameService::WM_SetBoostSpeed(BOOL enable, int speed)
{
	if (!isInitialized_.load(std::memory_order_acquire))
		return FALSE;

	if (nullptr == g_hGameModule)
		return FALSE;

	DWORD pBoostSpeed = CONVERT_GAMEVAR<DWORD>(0x1DEE4ul);
	int* pSpeed = CONVERT_GAMEVAR<int*>(0xAB7CCul);

	if ((FALSE == enable) || (speed <= 0))
	{
		//sa_8001sf.exe+1DEE4 - 83 F9 02              - cmp ecx,02 { 2 }
		*pSpeed = 14;

		//util::MemoryMove(pBoostSpeed, "\x83\xF9\x02", 3u);
		*reinterpret_cast<BYTE*>(pBoostSpeed + 2) = 2;

		speedBoostValue = 1UL;
	}
	else
	{
		if ((speed >= 1) && (speed <= 14))
		{
			*pSpeed = 14 - speed + 1;

			//sa_8001sf.exe+1DEE4 - 83 F9 0E              - cmp ecx,0E { 14 }
			//util::MemoryMove(pBoostSpeed, "\x83\xF9\x0E", 3u);
			*reinterpret_cast<BYTE*>(pBoostSpeed + 2) = 14;

			speedBoostValue = 1UL;
		}
		else if ((speed > 14) && (speed <= 125))
		{
			*pSpeed = 1;

			BYTE* pBoostSpeedValue = reinterpret_cast<BYTE*>(pBoostSpeed + 2UL);

			*pBoostSpeedValue = static_cast<BYTE>(speed + 2);

			speedBoostValue = static_cast<DWORD>(speed) - 13UL;
		}
	}

	return TRUE;
}

//鎖定原地
BOOL GameService::WM_EnableMoveLock(BOOL enable)
{
	if (!isInitialized_.load(std::memory_order_acquire))
		return FALSE;

	if (nullptr == g_hGameModule)
		return FALSE;

	//DWORD pMoveLock = CONVERT_GAMEVAR<DWORD>(0x42773ul);

	if (enable != IS_MOVE_LOCK)
	{
		IS_MOVE_LOCK = enable;
		DWORD* pMoveStart = CONVERT_GAMEVAR<DWORD*>(0x42795ul + 0x6ul);
		*pMoveStart = !enable ? 1UL : 0UL;
	}

	if (FALSE == enable)
	{
		DWORD* pMoveStart = CONVERT_GAMEVAR<DWORD*>(0x4181198ul);
		*pMoveStart = 0UL;
		pMoveStart = CONVERT_GAMEVAR<DWORD*>(0x41829FCul);
		*pMoveStart = 0UL;
	}

	//DWORD* pMoveLock = CONVERT_GAMEVAR<DWORD*>(0x4275Eul);
	//if (!enable)
	//{
	//	//00442773 - 77 2A                 - ja 0044279F
	//	util::MemoryMove(pMoveLock, "\x74\x3F", 2u);

	//	//sa_8001.exe+42795 - C7 05 E0295804 01000000 - mov [sa_8001.exe+41829E0],00000001 { (0),1 }
	//	//util::MemoryMove(pMoveLock, "\x01", 1u);
	//}
	//else
	//{
	//	//sa_8001sf.exe+42773 - EB 2A                 - jmp sa_8001sf.exe+4279F  //直接改跳轉會有機率完全卡住恢復後不能移動
	//	util::MemoryMove(pMoveLock, "\xEB\x3F", 2u);

	//	//fill \x90  取代的是把切換成1會觸發移動的值移除掉
	//	//util::MemoryMove(pMoveLock, "\x00", 1u);
	//}

	return TRUE;
}

//公告
BOOL GameService::WM_Announce(char* str, int color)
{
	if (!isInitialized_.load(std::memory_order_acquire))
		return FALSE;

	if (nullptr == g_hGameModule)
		return FALSE;

	if (nullptr == str)
		return FALSE;

	auto pAnnounce = CONVERT_GAMEVAR<void(_cdecl*)(char*, int)>(0x115C0ul);
	if (nullptr == pAnnounce)
		return FALSE;

	pAnnounce(str, color);

	return TRUE;
}

//移動
BOOL GameService::WM_Move(int x, int y)
{
	if (!isInitialized_.load(std::memory_order_acquire))
		return FALSE;

	/*
	sa_8001sf.exe+4277A - 8B 15 38A65604        - mov edx,[sa_8001sf.exe+416A638] { (98) }
	sa_8001sf.exe+42780 - A3 54B74B00           - mov [sa_8001sf.exe+BB754],eax { (580410503) }
	sa_8001sf.exe+42785 - A1 3CA65604           - mov eax,[sa_8001sf.exe+416A63C] { (0.01) }
	sa_8001sf.exe+4278A - 89 15 4C025604        - mov [sa_8001sf.exe+416024C],edx { (100) }
	sa_8001sf.exe+42790 - A3 50025604           - mov [sa_8001sf.exe+4160250],eax { (100) }
	sa_8001sf.exe+42795 - C7 05 E0295804 01000000 - mov [sa_8001sf.exe+41829E0],00000001 { (0),1 }

	*/
	if (nullptr == g_hGameModule)
		return FALSE;

	*CONVERT_GAMEVAR<int*>(0x416024Cul) = x;//goalX
	*CONVERT_GAMEVAR<int*>(0x4160250ul) = y;//goalY
	*CONVERT_GAMEVAR<int*>(0x41829E0ul) = 1;//moveStart

	//*CONVERT_GAMEVAR<int*>(0x4181D3C) = x;
	//*CONVERT_GAMEVAR<int*>(0x4181D40) = y;
	//*CONVERT_GAMEVAR<float*>(0x416A644) = static_cast<float>(x) * 64.0f;
	//*CONVERT_GAMEVAR<float*>(0x416A648) = static_cast<float>(y) * 64.0f;
	//*CONVERT_GAMEVAR<float*>(0x4182998) = static_cast<float>(x) * 64.0f;
	//*CONVERT_GAMEVAR<float*>(0x4182994) = static_cast<float>(y) * 64.0f;
	return TRUE;
}

//銷毀NPC對話框
BOOL GameService::WM_DistoryDialog()
{
	if (!isInitialized_.load(std::memory_order_acquire))
		return FALSE;

	if (nullptr == g_hGameModule)
		return FALSE;

	auto a = CONVERT_GAMEVAR<int*>(0xB83ECul);
	if (a != nullptr)
		*a = -1;

	auto distory = CONVERT_GAMEVAR<void(_cdecl*)(int)>(0x11C0ul);
	if (nullptr == distory)
		return FALSE;

	int* pDialog = CONVERT_GAMEVAR<int*>(0x415EF98ul);
	if (nullptr == pDialog)
		return FALSE;

	if (0 == *pDialog)
		return FALSE;

	distory(*pDialog);
	*pDialog = 0;

	return TRUE;
}

//清空聊天緩存
BOOL GameService::WM_CleanChatHistory()
{
	if (!isInitialized_.load(std::memory_order_acquire))
		return FALSE;

	if (nullptr == g_hGameModule)
		return FALSE;

	//sa_8001.exe + 117C0 - B8 884D5400 - mov eax, sa_8001.exe + 144D88 { (0) }
	//sa_8001.exe + 117C5 - C6 00 00 - mov byte ptr[eax], 00 { 0 }
	//sa_8001.exe + 117C8 - 05 0C010000 - add eax, 0000010C { 268 }
	//sa_8001.exe + 117CD - 3D 78625400 - cmp eax, sa_8001.exe + 146278 { (652401777) }
	//sa_8001.exe + 117D2 - 7C F1 - jl sa_8001.exe + 117C5
	//sa_8001.exe + 117D4 - C7 05 F8A45400 00000000 - mov[sa_8001.exe + 14A4F8], 00000000 { (0), 0 }
	//sa_8001.exe + 117DE - C3 - ret

	constexpr size_t bufsize = 268U;

	char* chatbuffer = CONVERT_GAMEVAR<char*>(0x144D88ul);
	char* pmaxchatbuffer = CONVERT_GAMEVAR<char*>(0x146278ul);
	int* pchatsize = CONVERT_GAMEVAR<int*>(0x14A4F8ul);
	if ((nullptr == chatbuffer) || (nullptr == pmaxchatbuffer) || (nullptr == pchatsize))
		return FALSE;

	const size_t count = static_cast<size_t>(pmaxchatbuffer - chatbuffer);

	for (size_t i = 0; i < count; i += bufsize)
	{
		*chatbuffer = '\0';
		chatbuffer += bufsize;
	}

	*pchatsize = 0;

	return TRUE;
}

//創建對話框
BOOL GameService::WM_CreateDialog(int type, int button, const char* data)
{
	if (!isInitialized_.load(std::memory_order_acquire))
		return FALSE;

	if (nullptr == g_hGameModule)
		return FALSE;

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

	std::cout << std::to_string(type) << " " << std::to_string(button) << std::endl;
	pCreateDialog(0, type, button, 0x10E1, 0x4D2, data);

	return TRUE;
}

BOOL GameService::WM_SetBlockPacket(BOOL enable)
{
	if (!isInitialized_.load(std::memory_order_acquire))
		return FALSE;

	IS_ENCOUNT_BLOCK_FLAG = enable;

	return TRUE;
}
#pragma endregion

#pragma region WinProc
static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	GameService& g_GameService = GameService::getInstance();

	switch (message)
	{
	case WM_NULL:
		return 1234L;
	case WM_CLOSE:
	{
		MINT::NtTerminateProcess(GetCurrentProcess(), 0UL);
		return 1L;
	}
	case WM_MOUSEMOVE:
	{
		//通知外掛更新當前鼠標坐標顯示
		PostMessageW(g_ParenthWnd, message + static_cast<UINT>(WM_USER), wParam, lParam);
		break;
	}
	case WM_KEYDOWN:
	{
		//按下CTRL+V
		if ((static_cast<char>(wParam) == 'V') && (GetKeyState(VK_CONTROL) < 0i16))
		{
			if (TRUE == OpenClipboard(hWnd))
			{
				HANDLE hClipboardData = GetClipboardData(CF_TEXT);
				if (hClipboardData != nullptr)
				{
					char* pszText = static_cast<char*>(GlobalLock(hClipboardData));
					if (pszText != nullptr)
					{
						if ((((1 == g_GameService.getWorldStatue()) && (2 == g_GameService.getGameStatue()))
							|| ((1 == g_GameService.getWorldStatue()) && (3 == g_GameService.getGameStatue()))))
						{
							constexpr size_t inputBoxBufSize = 20u;

							int index = *CONVERT_GAMEVAR<int*>(0x415EF50ul);

							if ((index == 0) || (strlen(CONVERT_GAMEVAR<char*>(0x414F278ul)) == 0))
							{
								//account
								memset(CONVERT_GAMEVAR<char*>(0x414F278ul), 0, inputBoxBufSize);
								_snprintf_s(CONVERT_GAMEVAR<char*>(0x414F278ul), inputBoxBufSize, _TRUNCATE, "%s", pszText);
							}
							else
							{
								//password
								memset(CONVERT_GAMEVAR<char*>(0x415AA58ul), 0, inputBoxBufSize);
								_snprintf_s(CONVERT_GAMEVAR<char*>(0x415AA58ul), inputBoxBufSize, _TRUNCATE, "%s", pszText);
							}
						}

						GlobalUnlock(hClipboardData);
					}
				}

				return CloseClipboard();
			}

			return SendMessageW(hWnd, WM_PASTE, NULL, NULL);
		}
		break;
	}
	case WM_KEYUP:
	{
		switch (wParam)
		{
		case VK_DELETE:		//檢查是否為delete
		{
			if (nullptr == g_ParenthWnd)
				break;

			PostMessageW(g_ParenthWnd, message + static_cast<UINT>(WM_USER + VK_DELETE), wParam, lParam);
			break;
		}
		default:
			break;
		}

		break;
	}
	case kInitialize:
	{
		std::cout << "kInitialize" << std::endl;
		struct InitialData
		{
			long long parentHWnd = 0i64;
			long long index = 0i64;
			long long port = 0i64;
			long long type = 0i64;
		};

		InitialData* pData = reinterpret_cast<InitialData*>(wParam);
		if (nullptr == pData)
			return 0L;

		std::wcout << std::hex << L"parentHWnd:0x" << pData->parentHWnd << L" port:" << std::to_wstring(pData->port) << L" type:" << std::to_wstring(pData->type) << std::endl;

		return g_GameService.initialize(pData->index,
			reinterpret_cast<HWND>(pData->parentHWnd),
			static_cast<unsigned short>(pData->type),
			static_cast<unsigned short>(pData->port));
	}
	case kUninitialize:
	{
		std::wcout << L"kUninitialize" << std::endl;

		return 	g_GameService.uninitialize();
	}
	case kGetModule:
	{
		return reinterpret_cast<int>(GetModuleHandleW(nullptr));
	}
	case kSendPacket:
	{
		//從外掛送來的封包
		if (INVALID_SOCKET == *g_GameService.g_sockfd)
			return 0L;

		if (NULL == *g_GameService.g_sockfd)
			return 0L;

		if (lParam <= 0)
			return 0L;

		DWORD numberOfBytesSent = 0UL;
		constexpr DWORD flags = 0UL;
		WSABUF buffer = { static_cast<ULONG>(lParam), reinterpret_cast<char*>(wParam) };
		WSASend(static_cast<SOCKET>(*g_GameService.g_sockfd), &buffer, 1, &numberOfBytesSent, flags, nullptr, nullptr);
		return numberOfBytesSent;
	}
	///////////////////////////////////////////////////////////
	case kEnableEffect://關閉特效
	{
		return g_GameService.WM_EnableEffect(wParam > 0 ? TRUE : FALSE);
	}
	case kEnableCharShow://隱藏人物
	{
		return g_GameService.WM_EnableCharShow(wParam > 0 ? TRUE : FALSE);
	}
	case kSetTimeLock://鎖定時間
	{
		return g_GameService.WM_SetTimeLock(wParam > 0 ? TRUE : FALSE, static_cast<unsigned int>(lParam));
	}
	case kEnableSound://關閉音效
	{
		return g_GameService.WM_EnableSound(wParam > 0 ? TRUE : FALSE);
	}
	case kEnableImageLock://鎖定畫面
	{
		return g_GameService.WM_EnableImageLock(wParam > 0 ? TRUE : FALSE);
	}
	case kEnablePassWall://橫衝直撞
	{
		return g_GameService.WM_EnablePassWall(wParam > 0 ? TRUE : FALSE);
	}
	case kEnableFastWalk://快速走路
	{
		return g_GameService.WM_EnableFastWalk(wParam > 0 ? TRUE : FALSE);
	}
	case kSetBoostSpeed://加速
	{
		return g_GameService.WM_SetBoostSpeed(wParam > 0 ? TRUE : FALSE, static_cast<int>(lParam));
	}
	case kEnableMoveLock://鎖定原地
	{
		return g_GameService.WM_EnableMoveLock(wParam > 0 ? TRUE : FALSE);
	}
	case kMuteSound://靜音
	{
		return g_GameService.WM_MuteSound(wParam > 0 ? TRUE : FALSE);
	}
	case kEnableBattleDialog://關閉戰鬥對話框
	{
		return g_GameService.WM_EnableBattleDialog(wParam > 0 ? TRUE : FALSE);
	}
	case kSetGameStatus://設置遊戲動畫狀態
	{
		return g_GameService.WM_SetGameStatus(static_cast<int>(wParam));
	}
	case kSetWorldStatus://設置世界動畫狀態
	{
		return g_GameService.WM_SetWorldStatus(static_cast<int>(wParam));
	}
	case kSetBlockPacket://戰鬥封包阻擋
	{
		return g_GameService.WM_SetBlockPacket(wParam > 0 ? TRUE : FALSE);
	}
	case kBattleTimeExtend://戰鬥時間延長
	{
		return g_GameService.WM_BattleTimeExtend(wParam > 0 ? TRUE : FALSE);
	}
	case kEnableOptimize:
	{
		return g_GameService.WM_SetOptimize(wParam > 0 ? TRUE : FALSE);
	}
	case kEnableWindowHide:
	{
		return g_GameService.WM_SetWindowHide(wParam > 0 ? TRUE : FALSE);
	}
	//action
	case kSendAnnounce://公告
	{
		return g_GameService.WM_Announce(reinterpret_cast<char*>(wParam), static_cast<int>(lParam));
	}
	case kSetMove://移動
	{
		return g_GameService.WM_Move(static_cast<int>(wParam), static_cast<int>(lParam));
	}
	case kDistoryDialog://銷毀對話框
	{
		return g_GameService.WM_DistoryDialog();
	}
	case kCleanChatHistory://清空聊天緩存
	{
		return g_GameService.WM_CleanChatHistory();
	}
	case kCreateDialog:
	{
		int type = LOWORD(wParam);
		int button = HIWORD(wParam);
		return g_GameService.WM_CreateDialog(type, button, reinterpret_cast<const char*>(lParam));
	}
	default:
		break;
	}

	return CallWindowProcW(g_OldWndProc, g_MainHwnd, message, wParam, lParam);
}
#pragma endregion

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID)
{
	if (DLL_PROCESS_ATTACH == ul_reason_for_call)
	{
		DWORD dwThreadId = GetCurrentThreadId();

		if ((g_MainThreadId > 0UL) && (g_MainThreadId != dwThreadId))
			return TRUE;

		HWND hWnd = util::GetCurrentWindowHandle();
		if (nullptr == hWnd)
			return TRUE;

		GetModuleFileNameW(nullptr, g_szGameModulePath, MAX_PATH);

		GameService& g_GameService = GameService::getInstance();
		g_GameService.g_consoleHwnd = util::createConsole();
		if (g_GameService.g_consoleHwnd != nullptr)
			ShowWindow(g_GameService.g_consoleHwnd, SW_HIDE);

		g_hGameModule = GetModuleHandleW(nullptr);

		g_MainThreadId = dwThreadId;

		g_MainThreadHandle = GetCurrentThread();

		g_hDllModule = hModule;

		g_MainHwnd = hWnd;

		g_OldWndProc = reinterpret_cast<WNDPROC>(GetWindowLongW(hWnd, GWL_WNDPROC));

		SetWindowLongW(hWnd, GWL_WNDPROC, reinterpret_cast<LONG>(WndProc));

#ifdef USE_MINIDUMP
		SetUnhandledExceptionFilter(MinidumpCallback);
#endif

		DisableThreadLibraryCalls(hModule);
	}
	else if (DLL_PROCESS_DETACH == ul_reason_for_call)
	{
		GameService& g_GameService = GameService::getInstance();
		if (g_GameService.uninitialize() == TRUE)
			SetWindowLongW(g_MainHwnd, GWL_WNDPROC, reinterpret_cast<LONG>(g_OldWndProc));
	}

	return TRUE;
}

BOOL GameService::initialize(long long index, HWND parentHwnd, unsigned short type, unsigned short port)
{
	if (isInitialized_.load(std::memory_order_acquire) == TRUE)
		return FALSE;

	isInitialized_.store(TRUE, std::memory_order_release);

	g_ParenthWnd = parentHwnd;
	index_.store(index, std::memory_order_release);

	const UINT acp = GetACP();
	constexpr UINT gb2312CodePage = 936U;
	if (acp != gb2312CodePage)
	{
		//將環境設置回簡體 8001端 99.99%都是簡體
		SetConsoleCP(gb2312CodePage);
		SetConsoleOutputCP(gb2312CodePage);
		setlocale(LC_ALL, "Chinese.GB2312");
	}

	g_sockfd = CONVERT_GAMEVAR<int*>(0x421B404ul);//套接字

	g_world_status = CONVERT_GAMEVAR<int*>(0x4230DD8ul);
	g_game_status = CONVERT_GAMEVAR<int*>(0x4230DF0ul);

#ifdef AUTIL_H
	Autil::PersonalKey = CONVERT_GAMEVAR<char*>(0x4AC0898ul);//封包解密密鑰
#endif

	//pBattleCommandReady = CONVERT_GAMEVAR<pfnBattleCommandReady>(0x6B9A0);//戰鬥面板生成call裡面的一個小function
	pBattleCommandReady = CONVERT_GAMEVAR<DWORD*>(0xA8CAul);
	pPlaySound = CONVERT_GAMEVAR<pfnPlaySound>(0x88190ul);//播放音效
	pBattleProc = CONVERT_GAMEVAR<pfnBattleProc>(0x3940ul);//戰鬥循環
	pTimeProc = CONVERT_GAMEVAR<pfnTimeProc>(0x1E6D0ul);//刷新時間循環
	pLssproto_EN_recv = CONVERT_GAMEVAR<pfnLssproto_EN_recv>(0x64E10ul);//進戰鬥封包
	pLssproto_B_recv = CONVERT_GAMEVAR<pfnLssproto_B_recv>(0x64EF0ul);//戰鬥封包
	pLssproto_WN_send = CONVERT_GAMEVAR<pfnLssproto_WN_send>(0x8FDC0ul);//對話框發送封包
	pLssproto_TK_send = CONVERT_GAMEVAR<pfnLssproto_TK_send>(0x8F7C0ul);//喊話發送封包
	pLssproto_W2_send = CONVERT_GAMEVAR<pfnLssproto_W2_send>(0x8EEA0ul);//喊話發送封包
	pCreateDialog = CONVERT_GAMEVAR<pfnCreateDialog>(0x64AC0ul);//創建對話框

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
	psocket = CONVERT_GAMEVAR<pfnsocket>(0x91764ul);//::socket;
	psend = CONVERT_GAMEVAR<pfnsend>(0x9171Cul); //::send;
	precv = CONVERT_GAMEVAR<pfnrecv>(0x91728ul);//這裡直接勾::recv會無效，遊戲通常會複寫另一個call dword ptr指向recv
	pclosesocket = CONVERT_GAMEVAR<pfnclosesocket>(0x91716ul);//::closesocket;

	//pinet_addr = CONVERT_GAMEVAR<pfninet_addr>(0x9C2C4); //::inet_addr; //
	//pntohs = ::ntohs; //CONVERT_GAMEVAR<pfnntohs>(0x9C2C8);//

	pconnect = ::connect;//CONVERT_GAMEVAR<pfnconnect>(0x9C2BC);

	/*WINAPI*/
	pSetWindowTextA = ::SetWindowTextA;//防止部分私服調用A類函數，導致其他語系系統的窗口標題亂碼
	pGetTickCount = ::GetTickCount;
	pQueryPerformanceCounter = ::QueryPerformanceCounter;
	pTimeGetTime = ::timeGetTime;
	pSleep = ::Sleep;

	//禁止遊戲內切AI SE切AI會崩潰
	DWORD paddr = CONVERT_GAMEVAR <DWORD>(0x1DF82ul);
	util::MemoryMove(paddr, "\xB8\x00\x00\x00\x00", 5u);//只要點pgup或pgdn就會切0
	paddr = CONVERT_GAMEVAR <DWORD>(0x1DFE6);
	util::MemoryMove(paddr, "\xBA\x00\x00\x00\x00", 5u);//只要點pgup或pgdn就會切0

	//paddr = CONVERT_GAMEVAR <DWORD>(0x1F2EF);
	//util::MemoryMove(paddr, "\xB9\x00\x00\x00\x00", 5);

	int* pEnableAI = CONVERT_GAMEVAR<int*>(0xD9050ul);
	*pEnableAI = 0;

	//sa_8001sf.exe+A8CA - FF 05 F0 0D 63 04        - inc [sa_8001sf.exe+4230DF0] { (2) }
	BYTE newByte[6u] = { 0x90ui8, 0xE8ui8, 0x90ui8, 0x90ui8, 0x90ui8, 0x90ui8 }; //新數據
	util::detour(::New_BattleCommandReady, reinterpret_cast<DWORD>(pBattleCommandReady), oldBattleCommandReadyByte, newByte, sizeof(oldBattleCommandReadyByte), 0UL);

#if 0
	//禁止開頭那隻寵物亂跑
	paddr = CONVERT_GAMEVAR<DWORD>(0x79A0Ful);
	//sa_8001.exe+79A0F - E8 DCFAFBFF           - call sa_8001.exe+394F0s
	util::MemoryMove(paddr, "\x90\x90\x90\x90\x90", 5u);
	//開頭那隻寵物會不會動
	paddr = CONVERT_GAMEVAR<DWORD>(0x79A14ul);
	//sa_8001.exe+79A14 - E8 5777F8FF           - call sa_8001.exe+1170
	util::MemoryMove(paddr, "\x90\x90\x90\x90\x90", 5u);

	////使遊戲開頭那隻亂竄的寵物不可見
	paddr = CONVERT_GAMEVAR<DWORD>(0x79A19ul);
	//sa_8001.exe+79A19 - E8 224A0000           - call sa_8001.exe+7E440
	util::MemoryMove(paddr, "\x90\x90\x90\x90\x90", 5u);
#endif


	//開始下勾子
	DetourRestoreAfterWith();
	DetourTransactionBegin();
	DetourUpdateThread(g_MainThreadHandle);

	DetourAttach(&(PVOID&)precv, ::New_recv);
	DetourAttach(&(PVOID&)pclosesocket, ::New_closesocket);
	DetourAttach(&(PVOID&)psend, ::New_send);

#if 0
	DetourAttach(&(PVOID&)psocket, ::New_socket);
	DetourAttach(&(PVOID&)pinet_addr, ::New_inet_addr);
	DetourAttach(&(PVOID&)pntohs, ::New_ntohs);
	DetourAttach(&(PVOID&)pconnect, ::New_connect);
#endif

	DetourAttach(&(PVOID&)pSetWindowTextA, ::New_SetWindowTextA);
	DetourAttach(&(PVOID&)pGetTickCount, ::New_GetTickCount);
	DetourAttach(&(PVOID&)pQueryPerformanceCounter, ::New_QueryPerformanceCounter);
	DetourAttach(&(PVOID&)pTimeGetTime, ::New_TimeGetTime);
	DetourAttach(&(PVOID&)pSleep, ::New_Sleep);

	DetourAttach(&(PVOID&)pPlaySound, ::New_PlaySound);
	DetourAttach(&(PVOID&)pBattleProc, ::New_BattleProc);
	DetourAttach(&(PVOID&)pTimeProc, ::New_TimeProc);
	DetourAttach(&(PVOID&)pLssproto_EN_recv, ::New_lssproto_EN_recv);
	DetourAttach(&(PVOID&)pLssproto_B_recv, ::New_lssproto_B_recv);
	DetourAttach(&(PVOID&)pLssproto_WN_send, ::New_lssproto_WN_send);
	DetourAttach(&(PVOID&)pLssproto_TK_send, ::New_lssproto_TK_send);
	DetourAttach(&(PVOID&)pLssproto_W2_send, ::New_lssproto_W2_send);
	DetourAttach(&(PVOID&)pCreateDialog, ::New_CreateDialog);

	DetourTransactionCommit();

	WM_SetOptimize(TRUE);

#ifdef USE_ASYNC_TCP
	if (nullptr == asyncClient_)
	{
		//與外掛連線
		asyncClient_.reset(new AsyncClient(parentHwnd, index));
		assert(nullptr != asyncClient_);
		if (nullptr == asyncClient_)
			return FALSE;

		asyncClient_->setCloseSocketFunction(pclosesocket);

		if (FALSE == asyncClient_->asyncConnect(type, port))
			return FALSE;

		return asyncClient_->start();
	}

	asyncClient_.reset();

	return FALSE;
#else

	//與外掛連線
	syncClient_.reset(new SyncClient(parentHwnd, index));
	assert(nullptr != syncClient_);
	if (nullptr == syncClient_)
		return FALSE;

	syncClient_->setCloseSocketFunction(pclosesocket);
	syncClient_->setRecvFunction(precv);
	if (syncClient_->Connect(type, port) == TRUE)
	{
		WM_Announce(const_cast<char*>(u8"<SASH> connected to SASH local host."), 0);
		*CONVERT_GAMEVAR<int*>(0x4200000) = 1;
		return TRUE;
	}

	return FALSE;
#endif
}

//這裡的東西其實沒有太大必要，一般外掛斷開就直接關遊戲了，如果需要設計能重連才需要
BOOL GameService::uninitialize()
{
	if (isInitialized_.load(std::memory_order_acquire) == FALSE)
		return FALSE;

	WM_Announce(const_cast<char*>(u8"<SASH> disconnected from SASH local host."), 6);
	isInitialized_.store(FALSE, std::memory_order_release);

#ifdef USE_ASYNC_TCP
	asyncClient_.reset();
#else
	syncClient_.reset();
#endif

	* CONVERT_GAMEVAR<int*>(0x4200000) = 0;

	WM_EnableEffect(TRUE);
	WM_EnableCharShow(TRUE);
	WM_SetTimeLock(FALSE, NULL);
	WM_EnableSound(TRUE);
	WM_EnableImageLock(FALSE);
	WM_EnablePassWall(FALSE);
	WM_EnableFastWalk(FALSE);
	WM_SetBoostSpeed(FALSE, NULL);
	WM_EnableMoveLock(FALSE);
	WM_MuteSound(FALSE);
	WM_BattleTimeExtend(FALSE);
	WM_EnableBattleDialog(TRUE);
	WM_SetOptimize(FALSE);
	WM_SetWindowHide(FALSE);
	WM_SetBlockPacket(FALSE);

	IS_BATTLE_PROC_FLAG = FALSE;
	IS_TIME_LOCK_FLAG = FALSE;
	IS_SOUND_MUTE_FLAG = FALSE;
	IS_ENCOUNT_BLOCK_FLAG = FALSE;
	IS_MOVE_LOCK = FALSE;
	currentMusic_ = 15;
	currentSound_ = 15;
	nowChatRowCount_ = 10;
	speedBoostValue = 1UL;
	enableSleepAdjust.store(FALSE, std::memory_order_release);

	DetourRestoreAfterWith();
	DetourTransactionBegin();
	DetourUpdateThread(g_MainThreadHandle);

	DetourDetach(&(PVOID&)psend, ::New_send);
	DetourDetach(&(PVOID&)precv, ::New_recv);
	DetourDetach(&(PVOID&)pclosesocket, ::New_closesocket);

#if 0
	DetourDetach(&(PVOID&)psocket, ::New_socket);
	DetourDetach(&(PVOID&)pinet_addr, ::New_inet_addr);
	DetourDetach(&(PVOID&)pntohs, ::New_ntohs);
	DetourDetach(&(PVOID&)pconnect, ::New_connect);
#endif

	DetourDetach(&(PVOID&)pSetWindowTextA, ::New_SetWindowTextA);
	DetourDetach(&(PVOID&)pGetTickCount, ::New_GetTickCount);
	DetourDetach(&(PVOID&)pQueryPerformanceCounter, ::New_QueryPerformanceCounter);
	DetourDetach(&(PVOID&)pTimeGetTime, ::New_TimeGetTime);
	DetourDetach(&(PVOID&)pSleep, ::New_Sleep);

	DetourDetach(&(PVOID&)pPlaySound, ::New_PlaySound);
	DetourDetach(&(PVOID&)pBattleProc, ::New_BattleProc);
	DetourDetach(&(PVOID&)pTimeProc, ::New_TimeProc);
	DetourDetach(&(PVOID&)pLssproto_EN_recv, ::New_lssproto_EN_recv);
	DetourDetach(&(PVOID&)pLssproto_B_recv, ::New_lssproto_B_recv);
	DetourDetach(&(PVOID&)pLssproto_WN_send, ::New_lssproto_WN_send);
	DetourDetach(&(PVOID&)pLssproto_TK_send, ::New_lssproto_TK_send);
	DetourDetach(&(PVOID&)pLssproto_W2_send, ::New_lssproto_W2_send);
	DetourDetach(&(PVOID&)pCreateDialog, ::New_CreateDialog);

	DetourTransactionCommit();

	util::undetour(pBattleCommandReady, oldBattleCommandReadyByte, sizeof(oldBattleCommandReadyByte));

	return TRUE;
}

BOOL GameService::sendToServer(const std::string& str)
{
#ifdef USE_ASYNC_TCP
	if (asyncClient_ != nullptr)
		return asyncClient_->queueSendData(str.c_str(), str.size());
#else
	if (syncClient_ != nullptr)
		return syncClient_->Send(str);
#endif

	return FALSE;
}

BOOL GameService::sendToServer(const char* buf, size_t len)
{
#ifdef USE_ASYNC_TCP
	if (asyncClient_ != nullptr)
		return asyncClient_->queueSendData(buf, len);
#else
	if (syncClient_ != nullptr)
		return syncClient_->Send(buf, len);
#endif

	return FALSE;
}

BOOL GameService::recvFromServer(char* buf, size_t len)
{
#ifdef USE_ASYNC_TCP
	if (asyncClient_ != nullptr)
		return asyncClient_->queueRecvData(buf, len);
#else
	if (syncClient_ != nullptr)
		return syncClient_->Recv(buf, len);
#endif

	return FALSE;
}

////////////////////////////////////////////////////

#pragma region Dump
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
#if 0
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
		struct timeval timeout = {};
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

		std::fill_n(reinterpret_cast<char*>(GurrentModule->BaseDllName.Buffer), GurrentModule->BaseDllName.Length, 0);
		std::fill_n(reinterpret_cast<char*>(GurrentModule), sizeof(PLDR_DATA_TABLE_ENTRY), 0);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return;
	}
}
#endif
#pragma endregion