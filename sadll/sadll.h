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

#ifndef SADLL_H
#define SADLL_H

#ifdef SADLL_EXPORTS
#define SADLL_API __declspec(dllexport)
#else
#define SADLL_API __declspec(dllimport)
#endif

#ifdef USE_ASYNC_TCP
class AsyncClient;
#else
class SyncClient;
#endif

class GameService
{
	DISABLE_COPY_MOVE(GameService)
private:
	GameService() = default;

public:
	virtual ~GameService() = default;
	BOOL initialize(__int64 index, HWND parentHwnd, unsigned short type, unsigned short port);
	void uninitialize();

public:
	void WM_EnableEffect(BOOL enable);
	void WM_EnableCharShow(BOOL enable);
	void WM_SetTimeLock(BOOL enable, unsigned int time);
	void WM_EnableSound(BOOL enable);
	void WM_EnableImageLock(BOOL enable);
	void WM_EnablePassWall(BOOL enable);
	void WM_EnableFastWalk(BOOL enable);
	void WM_SetBoostSpeed(BOOL enable, int speed);
	void WM_EnableMoveLock(BOOL enable);
	void WM_MuteSound(BOOL enable);
	void WM_BattleTimeExtend(BOOL enable);
	void WM_EnableBattleDialog(BOOL enable);
	void WM_SetGameStatus(int status);
	void WM_SetOptimize(BOOL enable);
	void WM_SetWindowHide(BOOL enable);
	void WM_Announce(char* str, int color);
	void WM_Move(int x, int y);
	void WM_DistoryDialog();
	void WM_CleanChatHistory();
	void WM_CreateDialog(int type, int button, const char* data);

	void WM_SetBLockPacket(BOOL enable);

public://hook
	SOCKET WSAAPI New_socket(int af, int type, int protocol);
	int WSAAPI New_closesocket(SOCKET s);
	int WSAAPI New_send(SOCKET s, const char* buf, int len, int flags);
	int WSAAPI New_recv(SOCKET s, char* buf, int len, int flags);
	int WSAAPI New_connect(SOCKET s, const struct sockaddr* name, int namelen);
	unsigned long WSAAPI New_inet_addr(const char* cp);
	u_short WSAAPI New_ntohs(u_short netshort);

	BOOL WINAPI New_SetWindowTextA(HWND hWnd, LPCSTR lpString);
	DWORD WINAPI New_GetTickCount();
	BOOL WINAPI New_QueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount);
	DWORD WINAPI New_TimeGetTime();
	void WINAPI New_Sleep(DWORD dwMilliseconds);

	void __cdecl New_PlaySound(int a, int b, int c);
	void __cdecl New_BattleProc();
	void __cdecl New_BattleCommandReady();
	void __cdecl New_TimeProc(int fd);
	void __cdecl New_lssproto_EN_recv(int fd, int result, int field);
	void __cdecl New_lssproto_B_recv(int fd, char* command);
	void __cdecl New_lssproto_WN_send(int fd, int x, int y, int dialogid, int unitid, int select, const char* data);
	void __cdecl New_lssproto_TK_send(int fd, int x, int y, const char* message, int color, int area);
	void __cdecl New_lssproto_W2_send(int fd, int x, int y, const char* dir);
	void __cdecl New_CreateDialog(int unk, int type, int button, int unitid, int dialogid, const char* data);

public:
	//setwindowtexta
	using pfnSetWindowTextA = BOOL(__stdcall*)(HWND hWnd, LPCSTR lpString);
	pfnSetWindowTextA pSetWindowTextA = nullptr;

	using pfnsocket = SOCKET(__stdcall*)(int af, int type, int protocol);
	pfnsocket psocket = nullptr;

	using pfnclosesocket = int(__stdcall*)(SOCKET s);
	pfnclosesocket pclosesocket = nullptr;

	using pfnrecv = int(__stdcall*)(SOCKET s, char* buf, int len, int flags);
	pfnrecv precv = nullptr;

	using pfnsend = int(__stdcall*)(SOCKET s, const char* buf, int len, int flags);
	pfnsend psend = nullptr;

	using pfninet_addr = unsigned long(__stdcall*)(const char* cp);
	pfninet_addr pinet_addr = nullptr;

	using pfnconnect = int(__stdcall*)(SOCKET s, const sockaddr* name, int namelen);
	pfnconnect pconnect = nullptr;

	using pfnntohs = u_short(__stdcall*)(u_short netshort);
	pfnntohs pntohs = nullptr;

	//DWORD WINAPI New_GetTickCount();
	using pfnGetTickCount = DWORD(__stdcall*)();
	pfnGetTickCount pGetTickCount = nullptr;

	using pfnTimeGetTime = DWORD(__stdcall*)();
	pfnTimeGetTime pTimeGetTime = nullptr;

	using pfnSleep = void(__stdcall*)(DWORD dwMilliseconds);
	pfnSleep pSleep = nullptr;

	using pfnQueryPerformanceCounter = BOOL(__stdcall*)(LARGE_INTEGER*);
	pfnQueryPerformanceCounter pQueryPerformanceCounter = nullptr;

	using pfnPlaySound = void(__cdecl*)(int, int, int);
	pfnPlaySound pPlaySound = nullptr;

	using pfnBattleProc = void(__cdecl*)();
	pfnBattleProc pBattleProc = nullptr;

	DWORD* pBattleCommandReady = nullptr;

	using pfnTimeProc = void(__cdecl*)(int);
	pfnTimeProc pTimeProc = nullptr;

	using pfnLssproto_EN_recv = void(__cdecl*)(int, int, int);
	pfnLssproto_EN_recv pLssproto_EN_recv = nullptr;

	using pfnLssproto_B_recv = void(__cdecl*)(int, char*);
	pfnLssproto_B_recv pLssproto_B_recv = nullptr;

	using pfnLssproto_WN_send = void(__cdecl*)(int, int, int, int, int, int, const char*);
	pfnLssproto_WN_send pLssproto_WN_send = nullptr;

	using pfnLssproto_TK_send = void(__cdecl*)(int, int, int, const char*, int, int);
	pfnLssproto_TK_send pLssproto_TK_send = nullptr;

	using pfnLssproto_W2_send = void(__cdecl*)(int, int, int, const char*);
	pfnLssproto_W2_send pLssproto_W2_send = nullptr;

	using pfnCreateDialog = void(_cdecl*)(int, int type, int button, int unitid, int dialogid, const char* data);
	pfnCreateDialog pCreateDialog = nullptr;

public://g-var
	int* g_sockfd = nullptr;
	int* g_world_status = nullptr;
	int* g_game_status = nullptr;

private:
	void sendToServer(const std::string& text);
	void sendToServer(const char* buf, size_t len);
#if 0
	void hideModule(HMODULE hLibrary);
	int connectServer(SOCKET& rsocket, const char* ip, unsigned short port);
#endif

private:
	std::atomic_int IS_BATTLE_PROC_FLAG = FALSE;
	std::atomic_int IS_TIME_LOCK_FLAG = FALSE;
	std::atomic_int IS_SOUND_MUTE_FLAG = FALSE;
	std::atomic_int IS_ENCOUNT_BLOCK_FLAG = FALSE;
	std::atomic_int IS_MOVE_LOCK = FALSE;

private:
	std::atomic_int currentMusic_ = 15;
	std::atomic_int currentSound_ = 15;
	std::atomic_int nowChatRowCount_ = 10;
	std::atomic_ulong speedBoostValue = 1UL;

	std::atomic_int isInitialized_ = FALSE;
	std::atomic_int enableSleepAdjust = FALSE;

	std::atomic_int64_t index_ = 0i64;

	BYTE oldBattleCommandReadyByte[6u] = {}; //保存舊數據用於還原

#ifdef USE_ASYNC_TCP
	std::unique_ptr<AsyncClient> asyncClient_ = nullptr;
#else
	std::unique_ptr<SyncClient> syncClient_ = nullptr;
#endif

};

#endif //SADLL_H