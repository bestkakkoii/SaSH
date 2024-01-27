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

#include <shared_mutex>
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
	BOOL __fastcall initialize(long long index, HWND parentHwnd, unsigned short type, unsigned short port);
	BOOL __fastcall uninitialize();

public:
	BOOL __fastcall WM_EnableEffect(BOOL enable);
	BOOL __fastcall WM_EnableCharShow(BOOL enable);
	BOOL __fastcall WM_SetTimeLock(BOOL enable, unsigned int time);
	BOOL __fastcall WM_EnableSound(BOOL enable);
	BOOL __fastcall WM_EnableImageLock(BOOL enable);
	BOOL __fastcall WM_EnablePassWall(BOOL enable);
	BOOL __fastcall WM_EnableFastWalk(BOOL enable);
	BOOL __fastcall WM_SetBoostSpeed(BOOL enable, int speed);
	BOOL __fastcall WM_EnableMoveLock(BOOL enable);
	BOOL __fastcall WM_MuteSound(BOOL enable);
	BOOL __fastcall WM_BattleTimeExtend(BOOL enable);
	BOOL __fastcall WM_EnableBattleDialog(BOOL enable);
	BOOL __fastcall WM_SetGameStatus(int status);
	BOOL __fastcall WM_SetWorldStatus(int status);
	BOOL __fastcall WM_SetOptimize(BOOL enable);
	BOOL __fastcall WM_SetWindowHide(BOOL enable);
	BOOL __fastcall WM_Announce(char* str, int color);
	BOOL __fastcall WM_Move(int x, int y);
	BOOL __fastcall WM_DistoryDialog();
	BOOL __fastcall WM_CleanChatHistory();
	BOOL __fastcall WM_CreateDialog(int type, int button, const char* data);

	BOOL __fastcall WM_SetBlockPacket(BOOL enable);

public://hook
	SOCKET __fastcall New_socket(int af, int type, int protocol) const;
	int __fastcall New_closesocket(SOCKET s);
	int __fastcall New_send(SOCKET s, const char* buf, int len, int flags);
	int __fastcall New_recv(SOCKET s, char* buf, int len, int flags);
	int __fastcall New_connect(SOCKET s, const struct sockaddr* name, int namelen) const;
	unsigned long __fastcall New_inet_addr(const char* cp) const;
	u_short __fastcall New_ntohs(u_short netshort) const;

	BOOL __fastcall New_SetWindowTextA(HWND hWnd, LPCSTR lpString);
	DWORD __fastcall New_GetTickCount() const;
	BOOL __fastcall New_QueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount) const;
	DWORD __fastcall New_TimeGetTime() const;
	void __fastcall New_Sleep(DWORD dwMilliseconds);

	void __cdecl New_PlaySound(int a, int b, int c) const;
	void __cdecl New_BattleProc() const;
	void __cdecl New_BattleCommandReady();
	void __cdecl New_TimeProc(int fd) const;
	void __cdecl New_lssproto_EN_recv(int fd, int result, int field) const;
	void __cdecl New_lssproto_B_recv(int fd, char* command) const;
	void __cdecl New_lssproto_WN_send(int fd, int x, int y, int dialogid, int unitid, int select, const char* data);
	void __cdecl New_lssproto_TK_send(int fd, int x, int y, const char* message, int color, int area);
	void __cdecl New_lssproto_W2_send(int fd, int x, int y, const char* dir) const;
	void __cdecl New_CreateDialog(int unk, int type, int button, int unitid, int dialogid, const char* data) const;

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

	HWND g_consoleHwnd = nullptr;

	mutable std::shared_mutex g_statusLock;

	int __fastcall getWorldStatue() const { /*lock read*/ std::shared_lock<std::shared_mutex> lock(g_statusLock); return *g_world_status; }
	int __fastcall getGameStatue() const { /*lock read*/ std::shared_lock<std::shared_mutex> lock(g_statusLock); return *g_game_status; }


private:
	BOOL __fastcall sendToServer(const std::string& text);
	BOOL __fastcall sendToServer(const char* buf, size_t len);
	BOOL __fastcall recvFromServer(char* buf, size_t len);
#if 0
	void __fastcall hideModule(HMODULE hLibrary);
	int __fastcall connectServer(SOCKET& rsocket, const char* ip, unsigned short port);
#endif

private:
	BOOL IS_BATTLE_PROC_FLAG = FALSE;
	BOOL IS_TIME_LOCK_FLAG = FALSE;
	BOOL IS_SOUND_MUTE_FLAG = FALSE;
	BOOL IS_ENCOUNT_BLOCK_FLAG = FALSE;
	BOOL IS_MOVE_LOCK = FALSE;

private:
	int currentMusic_ = 15;
	int currentSound_ = 15;
	int nowChatRowCount_ = 10;
	DWORD speedBoostValue = 1UL;

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