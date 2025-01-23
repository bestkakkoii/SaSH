#include "stdafx.h"
#include "sahook/game/include/gameservice_p.h"

#pragma region Detours

#pragma comment(lib, "Detours/detours.lib")

namespace game
{
	namespace detours
	{
		//new socket
		static SOCKET __stdcall New_socket(int af, int type, int protocol)
		{
			return game::Service::instance().New_socket(af, type, protocol);
		}

		//new connect
		static int __stdcall New_connect(SOCKET s, const struct sockaddr* name, int namelen)
		{
			return game::Service::instance().New_connect(s, name, namelen);
		}

		//new send
		static int __stdcall New_send(SOCKET s, const char* buf, int len, int flags)
		{
			return game::Service::instance().New_send(s, buf, len, flags);
		}

		//new recv
		static int __stdcall New_recv(SOCKET s, char* buf, int len, int flags)
		{
			return game::Service::instance().New_recv(s, buf, len, flags);
		}

		//new closesocket
		static int __stdcall New_closesocket(SOCKET s)
		{
			return game::Service::instance().New_closesocket(s);
		}

		//new SetWindowTextA
		static BOOL __stdcall New_SetWindowTextA(HWND hWnd, LPCSTR lpString)
		{
			return game::Service::instance().New_SetWindowTextA(hWnd, lpString);
		}

		static void __stdcall New_Sleep(DWORD dwMilliseconds)
		{
			return game::Service::instance().New_Sleep(dwMilliseconds);
		}

		/////// game client ///////

		//PlaySound
		static void __cdecl New_PlaySound(int a, int b, int c)
		{
			return game::Service::instance().New_PlaySound(a, b, c);
		}

		//BattleProc
		static void __cdecl New_BattleProc()
		{
			return game::Service::instance().New_BattleProc();
		}

		//BattleCommandReady
		static void __cdecl New_BattleCommandReady()
		{
			return  game::Service::instance().New_BattleCommandReady();
		}

		//TimeProc
		static void __cdecl New_TimeProc(int fd)
		{
			return game::Service::instance().New_TimeProc(fd);
		}

		//lssproto_EN_recv
		static void __cdecl New_lssproto_EN_recv(int fd, int result, int field)
		{
			return game::Service::instance().New_lssproto_EN_recv(fd, result, field);
		}

		//lssproto_B_recv
		static void __cdecl New_lssproto_B_recv(int fd, char* command)
		{
			return game::Service::instance().New_lssproto_B_recv(fd, command);
		}

		//lssproto_WN_send
		static void __cdecl New_lssproto_WN_send(int fd, int x, int y, int dialogid, int unitid, int select, char* data)
		{
			return game::Service::instance().New_lssproto_WN_send(fd, x, y, dialogid, unitid, select, data);
		}

		static void __cdecl New_lssproto_TK_send(int fd, int x, int y, const char* message, int color, int area)
		{
			return game::Service::instance().New_lssproto_TK_send(fd, x, y, message, color, area);
		}

		static void __cdecl New_lssproto_W2_send(int fd, int x, int y, const char* dir)
		{
			return game::Service::instance().New_lssproto_W2_send(fd, x, y, dir);
		}

		static void __cdecl New_CreateDialog(int unk, int type, int button, int unitid, int dialogid, const char* data)
		{
			return game::Service::instance().New_CreateDialog(unk, type, button, unitid, dialogid, data);
		}

		// detours
		static void transact()
		{
			DetourRestoreAfterWith();
			DetourTransactionBegin();
			DetourUpdateThread(GetCurrentThread());
		}

		static void commit()
		{
			DetourTransactionCommit();
		}

		template<typename T, typename U>
		void attach(T target, U detour)
		{
			DetourAttach(&(PVOID&)target, (PVOID)detour);
		}

		template<typename T, typename U>
		void detach(T target, U detour)
		{
			DetourDetach(&(PVOID&)target, (PVOID)detour);
		}
	}
}

BOOL game::Service::initialize(int index, HWND parentWId)
{
	Q_D(Service);

	if (d->isInitialized_.load(std::memory_order_acquire) == TRUE)
	{
		return FALSE;
	}

	d->isInitialized_.store(TRUE, std::memory_order_release);

	d->g_ParenthWnd = parentWId;
	d->index_.store(index, std::memory_order_release);

	const UINT acp = GetACP();
	constexpr UINT gb2312CodePage = 936U;
	if (acp != gb2312CodePage)
	{
		//將環境設置回簡體 8001端 99.99%都是簡體
		SetConsoleCP(gb2312CodePage);
		SetConsoleOutputCP(gb2312CodePage);
		setlocale(LC_ALL, "Chinese.GB2312");
	}

	d->g_sockfd = d->converTo<int*>(0x421B404ul);//套接字

	d->g_world_status = d->converTo<int*>(0x4230DD8ul);
	d->g_game_status = d->converTo<int*>(0x4230DF0ul);

	d->pBattleCommandReady = d->converTo<DWORD*>(0xA8CAul);
	d->pPlaySound = d->converTo<Implement::pfnPlaySound>(0x88190ul);//播放音效
	d->pBattleProc = d->converTo<Implement::pfnBattleProc>(0x3940ul);//戰鬥循環
	d->pTimeProc = d->converTo<Implement::pfnTimeProc>(0x1E6D0ul);//刷新時間循環
	d->pLssproto_EN_recv = d->converTo<Implement::pfnLssproto_EN_recv>(0x64E10ul);//進戰鬥封包
	d->pLssproto_B_recv = d->converTo<Implement::pfnLssproto_B_recv>(0x64EF0ul);//戰鬥封包
	d->pLssproto_WN_send = d->converTo<Implement::pfnLssproto_WN_send>(0x8FDC0ul);//對話框發送封包
	d->pLssproto_TK_send = d->converTo<Implement::pfnLssproto_TK_send>(0x8F7C0ul);//喊話發送封包
	d->pLssproto_W2_send = d->converTo<Implement::pfnLssproto_W2_send>(0x8EEA0ul);//喊話發送封包
	d->pCreateDialog = d->converTo<Implement::pfnCreateDialog>(0x64AC0ul);//創建對話框
	d->pecb_crypt = d->converTo<Implement::pfnecb_crypt>(0x8BB90ul);//ECB加解密

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
	d->psocket = d->converTo<Implement::pfnsocket>(0x91764ul);//::socket;
	d->psend = d->converTo<Implement::pfnsend>(0x9171Cul); //::send;
	d->precv = d->converTo<Implement::pfnrecv>(0x91728ul);//這裡直接勾::recv會無效，遊戲通常會複寫另一個call dword ptr指向recv
	d->pclosesocket = d->converTo<Implement::pfnclosesocket>(0x91716ul);//::closesocket;
	d->pconnect = ::connect;//d->converTo<pfnconnect>(0x9C2BC);

	/*WINAPI*/
	d->pSetWindowTextA = ::SetWindowTextA;//防止部分私服調用A類函數，導致其他語系系統的窗口標題亂碼
	d->pSleep = ::Sleep;

	//禁止遊戲內切AI SE切AI會崩潰
	DWORD paddr = d->converTo <DWORD>(0x1DF82ul);
	tool::memoryMove(paddr, "\xB8\x00\x00\x00\x00", 5u);//只要點pgup或pgdn就會切0
	paddr = d->converTo <DWORD>(0x1DFE6);
	tool::memoryMove(paddr, "\xBA\x00\x00\x00\x00", 5u);//只要點pgup或pgdn就會切0

	//paddr = d->converTo <DWORD>(0x1F2EF);
	//tool::memoryMove(paddr, "\xB9\x00\x00\x00\x00", 5);

	int* pEnableAI = d->converTo<int*>(0xD9050ul);
	*pEnableAI = 0;

	//sa_8001sf.exe+A8CA - FF 05 F0 0D 63 04        - inc [sa_8001sf.exe+4230DF0] { (2) }
	BYTE newByte[6u] = { 0x90ui8, 0xE8ui8, 0x90ui8, 0x90ui8, 0x90ui8, 0x90ui8 }; //新數據
	tool::detour(detours::New_BattleCommandReady, reinterpret_cast<DWORD>(d->pBattleCommandReady), d->oldBattleCommandReadyByte, newByte, sizeof(d->oldBattleCommandReadyByte), 0UL);

	detours();

	WM_SetOptimize(TRUE);

	return TRUE;

}

/*
	This stuff is actually not necessary.
	Generally, the game will be turned off directly when it disconnected.
	If you need to design to reconnect, then you will need it.
*/
BOOL game::Service::uninitialize()
{
	Q_D(Service);

	if (d->isInitialized_.load(std::memory_order_acquire) == FALSE)
	{
		return FALSE;
	}

	WM_Announce(const_cast<char*>("<SASH> disconnected from SASH local host."), 6);
	d->isInitialized_.store(FALSE, std::memory_order_release);


	*d->converTo<int*>(0x4200000) = 0;

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

	d->IS_BATTLE_PROC_FLAG = FALSE;
	d->IS_TIME_LOCK_FLAG = FALSE;
	d->IS_SOUND_MUTE_FLAG = FALSE;
	d->IS_ENCOUNT_BLOCK_FLAG = FALSE;
	d->IS_MOVE_LOCK = FALSE;
	d->currentMusic_ = 15;
	d->currentSound_ = 15;
	d->nowChatRowCount_ = 10;
	d->speedBoostValue = 1UL;
	d->enableSleepAdjust.store(FALSE, std::memory_order_release);

	undetours();

	tool::undetour(d->pBattleCommandReady, d->oldBattleCommandReadyByte, sizeof(d->oldBattleCommandReadyByte));

	return TRUE;
}


void game::Service::detours()
{
	Q_D(Service);

	detours::transact();

	DetourAttach(&(PVOID&)d->precv, detours::New_recv);
	DetourAttach(&(PVOID&)d->pclosesocket, detours::New_closesocket);
	DetourAttach(&(PVOID&)d->psend, detours::New_send);

	DetourAttach(&(PVOID&)d->psocket, detours::New_socket);
	DetourAttach(&(PVOID&)d->pconnect, detours::New_connect);


	DetourAttach(&(PVOID&)d->pSetWindowTextA, detours::New_SetWindowTextA);
	DetourAttach(&(PVOID&)d->pSleep, detours::New_Sleep);

	DetourAttach(&(PVOID&)d->pPlaySound, detours::New_PlaySound);
	DetourAttach(&(PVOID&)d->pBattleProc, detours::New_BattleProc);
	DetourAttach(&(PVOID&)d->pTimeProc, detours::New_TimeProc);
	DetourAttach(&(PVOID&)d->pLssproto_EN_recv, detours::New_lssproto_EN_recv);
	DetourAttach(&(PVOID&)d->pLssproto_B_recv, detours::New_lssproto_B_recv);
	DetourAttach(&(PVOID&)d->pLssproto_WN_send, detours::New_lssproto_WN_send);
	DetourAttach(&(PVOID&)d->pLssproto_TK_send, detours::New_lssproto_TK_send);
	DetourAttach(&(PVOID&)d->pLssproto_W2_send, detours::New_lssproto_W2_send);
	DetourAttach(&(PVOID&)d->pCreateDialog, detours::New_CreateDialog);

	detours::commit();
}

void game::Service::undetours()
{
	Q_D(Service);
	detours::transact();

	DetourDetach(&(PVOID&)d->precv, detours::New_recv);
	DetourDetach(&(PVOID&)d->pclosesocket, detours::New_closesocket);
	DetourDetach(&(PVOID&)d->psend, detours::New_send);

#if 0
	DetourDetach(&(PVOID&)d->psocket, detours::New_socket);
	DetourDetach(&(PVOID&)d->pinet_addr, detours::New_inet_addr);
	DetourDetach(&(PVOID&)d->pntohs, detours::New_ntohs);
	DetourDetach(&(PVOID&)d->pconnect, detours::New_connect);
#endif

	DetourDetach(&(PVOID&)d->pSetWindowTextA, detours::New_SetWindowTextA);
	DetourDetach(&(PVOID&)d->pSleep, detours::New_Sleep);

	DetourDetach(&(PVOID&)d->pPlaySound, detours::New_PlaySound);
	DetourDetach(&(PVOID&)d->pBattleProc, detours::New_BattleProc);
	DetourDetach(&(PVOID&)d->pTimeProc, detours::New_TimeProc);
	DetourDetach(&(PVOID&)d->pLssproto_EN_recv, detours::New_lssproto_EN_recv);
	DetourDetach(&(PVOID&)d->pLssproto_B_recv, detours::New_lssproto_B_recv);
	DetourDetach(&(PVOID&)d->pLssproto_WN_send, detours::New_lssproto_WN_send);
	DetourDetach(&(PVOID&)d->pLssproto_TK_send, detours::New_lssproto_TK_send);
	DetourDetach(&(PVOID&)d->pLssproto_W2_send, detours::New_lssproto_W2_send);
	DetourDetach(&(PVOID&)d->pCreateDialog, detours::New_CreateDialog);

	detours::commit();
}
#pragma endregion

#pragma region Overrides
//hooks
SOCKET game::Service::New_socket(int af, int type, int protocol) const
{
	Q_D(const Service);
	SOCKET fd = d->psocket(af, type, protocol);

	return fd;
}

int game::Service::New_send(SOCKET s, const char* buf, int len, int flags)
{
	Q_D(Service);
	int snedlen = d->psend(s, buf, len, flags);

	if (d->IS_FORWARD_SEND == TRUE && snedlen > 0)
	{
		tool::JsonReply reply("notify");
		reply.insertParameter("index", d->index_.load(std::memory_order_acquire));
		reply.setCommand("send");
		reply.insertParameter("message", QByteArray(buf, len).toHex());

		emit sendWebSocketBinaryMessage(reply.toUtf8());
	}

	return snedlen;
}

int game::Service::New_connect(SOCKET s, const struct sockaddr* name, int namelen) const
{
	Q_D(const Service);

	return d->pconnect(s, name, namelen);
}

int game::Service::New_recv(SOCKET s, char* buf, int len, int flags)
{
	Q_D(Service);
	int recvlen = d->precv(s, buf, len, flags);

	if ((recvlen > 0) && (recvlen <= len))
	{
		tool::JsonReply reply("notify", QByteArray(buf, static_cast<size_t>(recvlen)).toHex());
		reply.insertParameter("index", d->index_.load(std::memory_order_acquire));
		reply.setCommand("recv");
		emit sendWebSocketBinaryMessage(reply.toUtf8());
		//qDebug() << "recv:" << QByteArray(buf, static_cast<size_t>(recvlen)).toHex();
	}

	return recvlen;
}

int game::Service::New_closesocket(SOCKET s)
{
	Q_D(Service);
	if (d->g_sockfd != nullptr)
	{
		SOCKET cmps = static_cast<SOCKET>(*d->g_sockfd);
		if ((s != INVALID_SOCKET) && (s > 0u)
			&& (cmps != INVALID_SOCKET) && (cmps > 0u)
			&& (s == cmps))
		{
			tool::JsonReply reply("notify");
			reply.insertParameter("index", d->index_.load(std::memory_order_acquire));
			reply.setCommand("disconnect");

			emit sendWebSocketBinaryMessage(reply.toUtf8());
		}
	}

	int ret = d->pclosesocket(s);
	return ret;
}

//防止其他私服使用A類函數導致標題亂碼
BOOL game::Service::New_SetWindowTextA(HWND, LPCSTR)
{
	return TRUE;
}

void game::Service::New_Sleep(DWORD dwMilliseconds)
{
	Q_D(Service);
	if ((TRUE == d->enableSleepAdjust.load(std::memory_order_acquire)) && (0UL == dwMilliseconds))
	{
		dwMilliseconds = 1UL;
	}

	d->pSleep(dwMilliseconds);
}

//////////////////////////////////////////////////////////////////////

//音效
void game::Service::New_PlaySound(int a, int b, int c) const
{
	Q_D(const Service);
	if (FALSE == d->IS_SOUND_MUTE_FLAG)
	{
		d->pPlaySound(a, b, c);
	}
}

//戰鬥循環
void game::Service::New_BattleProc() const
{
	Q_D(const Service);
	if (TRUE == d->IS_BATTLE_PROC_FLAG)
	{
		d->pBattleProc();
	}
}

//每回合開始顯示戰鬥面板 (這裡只是切換面板時會順便調用到的函數)
void game::Service::New_BattleCommandReady()
{
	Q_D(Service);
	tool::JsonReply reply("notify");
	reply.insertParameter("index", d->index_.load(std::memory_order_acquire));
	reply.setCommand("battleCommandReady");

	emit sendWebSocketBinaryMessage(reply.toUtf8());

	int* p = d->converTo<int*>(0x4230DF0ul);
	++(*p);
}


//遊戲時間刷新循環 (早上,下午....)
void game::Service::New_TimeProc(int fd) const
{
	Q_D(const Service);
	if (FALSE == d->IS_TIME_LOCK_FLAG)
	{
		d->pTimeProc(fd);
	}
}

//EN封包攔截，戰鬥進場
void game::Service::New_lssproto_EN_recv(int fd, int result, int field) const
{
	Q_D(const Service);
	if (FALSE == d->IS_ENCOUNT_BLOCK_FLAG)
	{
		d->pLssproto_EN_recv(fd, result, field);
	}
}

//B封包攔截，戰鬥封包
void game::Service::New_lssproto_B_recv(int fd, char* command) const
{
	Q_D(const Service);
	if (FALSE == d->IS_ENCOUNT_BLOCK_FLAG)
	{
		d->pLssproto_B_recv(fd, command);
	}
}

//WN對話框發包攔截
void game::Service::New_lssproto_WN_send(int fd, int x, int y, int dialogid, int unitid, int select, const char* data)
{
	Q_D(Service);

	*d->converTo<int*>(0x4200004) = 0; // 自定義打開對話框標誌

	if ((1234 == unitid) && (4321 == dialogid))
	{
		tool::JsonReply reply("notify");
		reply.insertParameter("index", d->index_.load(std::memory_order_acquire));
		reply.setCommand("createDialog");
		reply.insertParameter("x", x);
		reply.insertParameter("y", y);

		emit sendWebSocketBinaryMessage(reply.toUtf8());

		return;
	}

	d->pLssproto_WN_send(fd, x, y, dialogid, unitid, select, data);
}

//TK對話框收包攔截
void game::Service::New_lssproto_TK_send(int fd, int x, int y, const char* message, int color, int area)
{
	Q_D(Service);
	const QByteArray msg = message;

	//查找是否包含'//'
	if (msg.size() > 0 && msg.contains("//"))
	{
		tool::JsonReply reply("notify");
		reply.insertParameter("index", d->index_.load(std::memory_order_acquire));
		reply.setCommand("chat");
		reply.insertParameter("message", msg.mid(2u));
		reply.insertParameter("color", color);
		reply.insertParameter("area", area);
		reply.insertParameter("x", x);
		reply.insertParameter("y", y);

		emit sendWebSocketBinaryMessage(reply.toUtf8());

		return;
	}

	d->pLssproto_TK_send(fd, x, y, message, color, area);
}

// W2移動發包攔截
void game::Service::New_lssproto_W2_send(int fd, int x, int y, const char* message) const
{
	Q_D(const Service);
	d->pLssproto_W2_send(fd, x, y, message);
}

void game::Service::New_CreateDialog(int unk, int type, int button, int unitid, int dialogid, const char* data)
{
	Q_D(Service);

	d->pCreateDialog(unk, type, button, unitid, dialogid, data);

	*(d->converTo<int*>(0x4200004)) = 1;  // 自定義打開對話框標誌

}
#pragma endregion
