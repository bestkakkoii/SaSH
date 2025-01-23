#ifndef GAMEIMPLEMENT_H_
#define GAMEIMPLEMENT_H_

#include <windows.h>

namespace game
{
	class Implement
	{
	public:
		explicit Implement(HMODULE processBase);
		virtual ~Implement() = default;

	public:

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

		using pfnconnect = int(__stdcall*)(SOCKET s, const sockaddr* name, int namelen);
		pfnconnect pconnect = nullptr;

		using pfnSleep = void(__stdcall*)(DWORD dwMilliseconds);
		pfnSleep pSleep = nullptr;

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

		using pfnecb_crypt = int(__cdecl*)(const char* key, char* buf, unsigned int len, unsigned int mode);
		pfnecb_crypt pecb_crypt = nullptr;

	protected:
		DWORD processBase_ = 0;
	};

}

#endif // GAMEIMPLEMENT_H_