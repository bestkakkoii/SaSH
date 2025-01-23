#include "stdafx.h"
#include "sahook/game/include/gameservice_p.h"

#pragma comment(lib, "ws2_32.lib")

LRESULT game::Service::callOrigicalWndProc(HWND hWnd, UINT message, LPARAM wParam, LPARAM lParam)
{
	Q_D(Service);

	return CallWindowProcW(d->g_OldWndProc, hWnd, message, wParam, lParam);
}

LRESULT CALLBACK game::Service::WndProc(HWND hWnd, UINT message, LPARAM wParam, LPARAM lParam)
{
	Service& gameService = Service::instance();

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
		gameService.postMessage(static_cast<UINT>(message + WM_USER), wParam, lParam, gameService.d_ptr->g_ParenthWnd);

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
						if ((((1 == gameService.getWorldStatue()) && (2 == gameService.getGameStatue()))
							|| ((1 == gameService.getWorldStatue()) && (3 == gameService.getGameStatue()))))
						{
							constexpr size_t inputBoxBufSize = 20u;

							int index = *gameService.d_ptr->converTo<int*>(0x415EF50ul);

							if ((index == 0) || (strlen(gameService.d_ptr->converTo<char*>(0x414F278ul)) == 0))
							{
								//account
								memset(gameService.d_ptr->converTo<char*>(0x414F278ul), 0, inputBoxBufSize);
								_snprintf_s(gameService.d_ptr->converTo<char*>(0x414F278ul), inputBoxBufSize, _TRUNCATE, "%s", pszText);
							}
							else
							{
								//password
								memset(gameService.d_ptr->converTo<char*>(0x415AA58ul), 0, inputBoxBufSize);
								_snprintf_s(gameService.d_ptr->converTo<char*>(0x415AA58ul), inputBoxBufSize, _TRUNCATE, "%s", pszText);
							}
						}

						GlobalUnlock(hClipboardData);
					}
				}

				return CloseClipboard();
			}

			return gameService.sendMessage(WM_PASTE, NULL, NULL);
		}
		break;
	}
	case WM_KEYUP:
	{
		switch (wParam)
		{
		case VK_DELETE:		//檢查是否為delete
		{
			if (nullptr == gameService.d_ptr->g_ParenthWnd)
				break;

			gameService.postMessage(message + static_cast<UINT>(WM_USER + VK_DELETE), wParam, lParam, gameService.d_ptr->g_ParenthWnd);
			break;
		}
		default:
			break;
		}

		break;
	}
	case kInitialize:
	{
		qDebug() << "kInitialize";

		return gameService.initialize(wParam, reinterpret_cast<HWND>(lParam));
	}
	case kUninitialize:
	{
		return 	gameService.uninitialize();
	}
	case kGetModule:
	{
		return reinterpret_cast<int>(GetModuleHandleW(nullptr));
	}
	case kSendPacket:
	{
		//從外掛送來的封包
		if (INVALID_SOCKET == *gameService.d_ptr->g_sockfd)
		{
			return 0L;
		}

		if (NULL == *gameService.d_ptr->g_sockfd)
		{
			return 0L;
		}

		if (lParam <= 0)
		{
			return 0L;
		}

		DWORD numberOfBytesSent = 0UL;
		constexpr DWORD flags = 0UL;
		WSABUF buffer = { static_cast<ULONG>(lParam), reinterpret_cast<char*>(wParam) };
		WSASend(static_cast<SOCKET>(*gameService.d_ptr->g_sockfd), &buffer, 1, &numberOfBytesSent, flags, nullptr, nullptr);
		return numberOfBytesSent;
	}
	///////////////////////////////////////////////////////////
	case kEnableForwardSend: //允許轉發 send封包
	{
		BOOL enable = wParam > 0 ? TRUE : FALSE;
		gameService.WM_EnableForwardSend(enable);
		return enable;
	}
	case kEnableEffect://關閉特效
	{
		return gameService.WM_EnableEffect(wParam > 0 ? TRUE : FALSE);
	}
	case kEnableCharShow://隱藏人物
	{
		return gameService.WM_EnableCharShow(wParam > 0 ? TRUE : FALSE);
	}
	case kSetTimeLock://鎖定時間
	{
		return gameService.WM_SetTimeLock(wParam > 0 ? TRUE : FALSE, static_cast<unsigned int>(lParam));
	}
	case kEnableSound://關閉音效
	{
		return gameService.WM_EnableSound(wParam > 0 ? TRUE : FALSE);
	}
	case kEnableImageLock://鎖定畫面
	{
		return gameService.WM_EnableImageLock(wParam > 0 ? TRUE : FALSE);
	}
	case kEnablePassWall://橫衝直撞
	{
		return gameService.WM_EnablePassWall(wParam > 0 ? TRUE : FALSE);
	}
	case kEnableFastWalk://快速走路
	{
		return gameService.WM_EnableFastWalk(wParam > 0 ? TRUE : FALSE);
	}
	case kSetBoostSpeed://加速
	{
		return gameService.WM_SetBoostSpeed(wParam > 0 ? TRUE : FALSE, static_cast<int>(lParam));
	}
	case kEnableMoveLock://鎖定原地
	{
		return gameService.WM_EnableMoveLock(wParam > 0 ? TRUE : FALSE);
	}
	case kMuteSound://靜音
	{
		return gameService.WM_MuteSound(wParam > 0 ? TRUE : FALSE);
	}
	case kEnableBattleDialog://關閉戰鬥對話框
	{
		return gameService.WM_EnableBattleDialog(wParam > 0 ? TRUE : FALSE);
	}
	case kSetGameStatus://設置遊戲動畫狀態
	{
		return gameService.WM_SetGameStatus(static_cast<int>(wParam));
	}
	case kSetWorldStatus://設置世界動畫狀態
	{
		return gameService.WM_SetWorldStatus(static_cast<int>(wParam));
	}
	case kSetBlockPacket://戰鬥封包阻擋
	{
		return gameService.WM_SetBlockPacket(wParam > 0 ? TRUE : FALSE);
	}
	case kBattleTimeExtend://戰鬥時間延長
	{
		return gameService.WM_BattleTimeExtend(wParam > 0 ? TRUE : FALSE);
	}
	case kEnableOptimize:
	{
		return gameService.WM_SetOptimize(wParam > 0 ? TRUE : FALSE);
	}
	case kEnableWindowHide:
	{
		return gameService.WM_SetWindowHide(wParam > 0 ? TRUE : FALSE);
	}
	//action
	case kSendAnnounce://公告
	{
		return gameService.WM_Announce(reinterpret_cast<char*>(wParam), static_cast<int>(lParam));
	}
	case kSetMove://移動
	{
		return gameService.WM_Move(static_cast<int>(wParam), static_cast<int>(lParam));
	}
	case kDistoryDialog://銷毀對話框
	{
		return gameService.WM_DistoryDialog();
	}
	case kCleanChatHistory://清空聊天緩存
	{
		return gameService.WM_CleanChatHistory();
	}
	case kCreateDialog://創建對話框
	{
		int type = LOWORD(wParam);
		int button = HIWORD(wParam);
		return gameService.WM_CreateDialog(type, button, reinterpret_cast<const char*>(lParam));
	}
	case kECBCrypt://ECB解密
	{
		const char* pSrc = reinterpret_cast<const char*>(wParam);
		char* pDst = reinterpret_cast<char*>(lParam);
		if (nullptr == pSrc || nullptr == pDst)
			return FALSE;

		memset(pDst, 0, 32);
		memcpy(pDst, pSrc, 32);
		gameService.d_ptr->pecb_crypt("f;encor1c", pDst, 32, 1);//mode = DES_DECRYPT(1)
		return TRUE;
	}
	case kECBEncrypt://ECB加密
	{
		const char* pSrc = reinterpret_cast<const char*>(wParam);
		char* pDst = reinterpret_cast<char*>(lParam);
		if (nullptr == pSrc || nullptr == pDst)
			return FALSE;

		memset(pDst, 0, 32);
		memcpy(pDst, pSrc, 32);
		gameService.d_ptr->pecb_crypt("f;encor1c", pDst, 32, 0);//mode = DES_ENCRYPT(0)
		return TRUE;
	}
	default:
		break;
	}


	return gameService.callOrigicalWndProc(hWnd, message, wParam, lParam);
}

#pragma region WM
//設置遊戲畫面狀態值
BOOL game::Service::WM_SetGameStatus(int status)
{
	Q_D(Service);
	std::unique_lock<std::shared_mutex> lock(d->g_statusLock);

	if (nullptr == d->g_game_status)
	{
		return FALSE;
	}

	int G = *d->g_game_status;

	if (G != status)
	{
		*d->g_game_status = status;
		return TRUE;
	}

	return FALSE;
}

//設置遊戲世界狀態值
BOOL game::Service::WM_SetWorldStatus(int status)
{
	Q_D(Service);
	std::unique_lock<std::shared_mutex> lock(d->g_statusLock);

	if (nullptr == d->g_world_status)
	{
		return FALSE;
	}

	int W = *d->g_world_status;

	if (W != status)
	{
		*d->g_world_status = status;
		return TRUE;
	}

	return FALSE;
}

//資源優化
BOOL game::Service::WM_SetOptimize(BOOL enable)
{
	Q_D(Service);

	//DWORD optimizeAddr = d->converTo<DWORD>(0x129E7);
	if (FALSE == enable)
	{
		/*
		//sa_8001.exe+129E9 - A1 0CA95400           - mov eax,[sa_8001.exe+14A90C] { (05205438) }
		//sa_8001.exe+129EE - 6A 00                 - push 00 { 0 }
		tool::memoryMove(optimizeAddr, "\xA1\x0C\xA9\x54\x00\x6A\x00", 7);

		//sa_8001.exe+129E7 - 75 37                 - jne sa_8001sf.exe+12A20
		tool::memoryMove(optimizeAddr, "\x75\x37", 2);
		*/

		*d->converTo<int*>(0xAB7C8ul) = 14;
	}
	else
	{
		/*
		//sa_8001.exe+129E9 - EB 10                 - jmp sa_8001.exe+129FB
		tool::memoryMove(optimizeAddr, "\xEB\x10\x90\x90\x90\x90\x90", 7);

		//sa_8001.exe+129E7 - EB 12                 - jmp sa_8001sf.exe+129FB
		tool::memoryMove(optimizeAddr, "\xEB\x12", 2);
		*/

		*d->converTo<int*>(0xAB7C8ul) = 0;
	}

	return TRUE;
}

//隱藏窗口
BOOL game::Service::WM_SetWindowHide(BOOL enable)
{
	Q_D(Service);

	//聊天紀錄顯示行數數量設為0
	d->nowChatRowCount_ = *d->converTo<int*>(0xA2674ul);

	if (d->nowChatRowCount_ < 20);
	{
		*d->converTo<int*>(0xA2674ul) = 20;
	}

	if (FALSE == enable)
	{
		//sa_8001sf.exe+129E7 - 75 37                 - jne sa_8001sf.exe+12A20 資源優化關閉
		tool::memoryMove(d->converTo<DWORD>(0x129E7ul), "\xEB\x12", 2u);

		//sa_8001sf.exe+1DFFD - 74 31                 - je sa_8001sf.exe+1E030
		tool::memoryMove(d->converTo<DWORD>(0x1DFFDul), "\x74\x31", 2u);

		//sa_8001sf.exe+1DEE4 - 83 F9 0E              - cmp ecx,0E { 14 } 加速相關
		//tool::memoryMove(d->converTo<DWORD>(0x1DEE4ul), "\x83\xF9\x0E", 3u);
		*reinterpret_cast<BYTE*>(d->converTo<DWORD>(0x1DEE4ul) + 2) = 14;
	}
	else
	{
		*d->converTo<int*>(0xA2674ul) = 0;

		//sa_8001sf.exe+129E7 - EB 37                 - jmp sa_8001sf.exe+12A20 資源優化開啟
		tool::memoryMove(d->converTo<DWORD>(0x129E7ul), "\xEB\x37", 2u);

		//sa_8001sf.exe+1DFFD - EB 31                 - jmp sa_8001sf.exe+1E030 強制跳過整個背景繪製
		tool::memoryMove(d->converTo<DWORD>(0x1DFFDul), "\xEB\x31", 2u);

		//sa_8001sf.exe+1DEE4 - 83 F9 0E              - cmp ecx,05 { 14 } 加速相關
		//tool::memoryMove(d->converTo<DWORD>(0x1DEE4ul), "\x83\xF9\x7F", 3u);
		*reinterpret_cast<BYTE*>(d->converTo<DWORD>(0x1DEE4ul) + 2) = 127;
	}

	d->enableSleepAdjust.store(enable, std::memory_order_release);

	return TRUE;
}

//靜音
BOOL game::Service::WM_MuteSound(BOOL enable)
{
	Q_D(Service);

	d->IS_SOUND_MUTE_FLAG = enable;

	return TRUE;
}

//戰鬥時間延長(99秒)
BOOL game::Service::WM_BattleTimeExtend(BOOL enable)
{
	Q_D(Service);

	DWORD* timerAddr = d->converTo<DWORD*>(0x9854ul);

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
BOOL game::Service::WM_EnableBattleDialog(BOOL enable)
{
	Q_D(Service);

	//DWORD addr = d->converTo<DWORD>(0x98C9ul);
	//DWORD addr2 = d->converTo<DWORD>(0x9D07ul);
	//DWORD addrSoundCharOpen = d->converTo<DWORD>(0x98DAul);
	//DWORD addrSoundCharOpen2 = d->converTo<DWORD>(0x9D13ul);
	//DWORD addrSoundCharClose = d->converTo<DWORD>(0x667Dul);
	//DWORD addrSoundCharClose2 = d->converTo<DWORD>(0x93C9ul);
	//int* timerAddr = d->converTo<int*>(0xE21E0ul);
	//DWORD timerAddr2 = d->converTo<DWORD>(0x9841ul);
	//DWORD timerAddr3 = d->converTo<DWORD>(0x9DB6ul);
	DWORD battleLoopAddr = d->converTo<DWORD>(0xA914ul);
	if (TRUE == enable)
	{
		////sa_8001sf.exe+98C9 - A3 E4214E00           - mov [sa_8001sf.exe+E21E4],eax { (0) }
		//tool::memoryMove(addr, "\xA3\xE4\x21\x4E\x00", 5);
		////sa_8001sf.exe+9D07 - 89 15 E4214E00        - mov [sa_8001sf.exe+E21E4],edx { (0) }
		//tool::memoryMove(addr2, "\x89\x15\xE4\x21\x4E\x00", 6);
		////sa_8001sf.exe+98DA - E8 B1E80700           - call sa_8001sf.exe+88190
		//tool::memoryMove(addrSoundCharOpen, "\xE8\xB1\xE8\x07\x00", 5);
		////sa_8001sf.exe+9D13 - E8 78E40700           - call sa_8001sf.exe+88190
		//tool::memoryMove(addrSoundCharOpen2, "\xE8\x78\xE4\x07\x00", 5);
		////sa_8001sf.exe+667D - E8 0E1B0800           - call sa_8001sf.exe+88190
		//tool::memoryMove(addrSoundCharClose, "\xE8\x0E\x1B\x08\x00", 5);
		////sa_8001sf.exe+93C9 - E8 C2ED0700           - call sa_8001sf.exe+88190
		//tool::memoryMove(addrSoundCharClose2, "\xE8\xC2\xED\x07\x00", 5);
		////sa_8001sf.exe+954F - A1 E0214E00           - mov eax,[sa_8001sf.exe+E21E0] { (0) }
		//*timerAddr = 1;
		////sa_8001sf.exe+9841 - 89 1D E0214E00        - mov [sa_8001sf.exe+E21E0],ebx { (1) }
		//tool::memoryMove(timerAddr2, "\x89\x1D\xE0\x21\x4E\x00", 6);
		////sa_8001sf.exe+9DB6 - 89 2D E0214E00        - mov [sa_8001sf.exe+E21E0],ebp { (0) }
		//tool::memoryMove(timerAddr3, "\x89\x2D\xE0\x21\x4E\x00", 6);
		////////////////////////////////////////////////////////////////////////////////////////////

		//sa_8001sf.exe+A914 - 75 0C                 - jne sa_8001sf.exe+A922 //原始
		//sa_8001sf.exe+A914 - EB 0C                 - jmp sa_8001sf.exe+A922 //最重要的
		tool::memoryMove(battleLoopAddr, "\xEB\x0C", 2u);
	}
	else
	{
		//tool::memoryMove(addr, "\x90\x90\x90\x90\x90", 5u);
		//tool::memoryMove(addr2, "\x90\x90\x90\x90\x90\x90", 6u);
		//tool::memoryMove(addrSoundCharOpen, "\x90\x90\x90\x90\x90", 5u);
		//tool::memoryMove(addrSoundCharOpen2, "\x90\x90\x90\x90\x90", 5u);
		//tool::memoryMove(addrSoundCharClose, "\x90\x90\x90\x90\x90", 5u);
		//tool::memoryMove(addrSoundCharClose2, "\x90\x90\x90\x90\x90", 5u);
		//*timerAddr = 0;
		//tool::memoryMove(timerAddr2, "\x90\x90\x90\x90\x90\x90", 6u);
		//tool::memoryMove(timerAddr3, "\x90\x90\x90\x90\x90\x90", 6u);
		////////////////////////////////////////////////////////////////////////////////////////////

		tool::memoryMove(battleLoopAddr, "\x90\x90", 2u);
	}

	d->IS_BATTLE_PROC_FLAG = enable;

	return TRUE;
}

//允許轉發 send 封包
BOOL game::Service::WM_EnableForwardSend(BOOL enable)
{
	Q_D(Service);

	d->IS_FORWARD_SEND = enable;

	return TRUE;
}

//顯示特效
BOOL game::Service::WM_EnableEffect(BOOL enable)
{
	Q_D(Service);

	DWORD effectAddr = d->converTo<DWORD>(0x434DDul);
	//DWORD effectAddr2 = d->converTo<DWORD>(0x482F0ul);
	DWORD effectAddr3 = d->converTo<DWORD>(0x48DE6ul);
	DWORD effectAddr4 = d->converTo<DWORD>(0x49029ul);
	DWORD effectAddr5 = d->converTo<DWORD>(0x7BDF2ul);
	DWORD effectAddr6 = d->converTo<DWORD>(0x60013);
	//DWORD effectAddr7 = d->converTo<DWORD>(0x21163);

	if (TRUE == enable)
	{
		//sa_8001sf.exe+434DD - C7 05 F0 0D 63 04 C8 00 00 00 - mov [sa_8001sf.exe+4230DF0],000000C8 { (3),200 }
		tool::memoryMove(effectAddr, "\xC7\x05\xF0\x0D\x63\x04\xC8\x00\x00\x00", 10u);

		//sa_8001.exe.text+5F013 - C7 05 F00D6304 C8000000 - mov [sa_8001.exe+4230DF0],000000C8 { (3),200 }
		tool::memoryMove(effectAddr6, "\xC7\x05\xF0\x0D\x63\x04\xC8\x00\x00\x00", 10u);

		//sa_8001.exe.text+20163 - C7 05 F00D6304 C8000000 - mov [sa_8001.exe+4230DF0],000000C8 { (3),200 }
		//tool::memoryMove(effectAddr7, "\xC7\x05\xF0\x0D\x63\x04\xC8\x00\x00\x00", 10u);

		///*
		//	sa_8001sf.exe+482F0 - 0F8C FA020000         - jl sa_8001sf.exe+485F0
		//	sa_8001sf.exe+482F6 - 8B 76 28              - mov esi,[esi+28]
		//	sa_8001sf.exe+482F9 - 83 FE FF              - cmp esi,-01 { 255 }
		//	sa_8001sf.exe+482FC - 75 12                 - jne sa_8001sf.exe+48310

		//*/
		//tool::memoryMove(effectAddr2, "\x0F\x8C\xFA\x02\x00\x00\x8B\x76\x28\x83\xFE\xFF\x75\x12", 14);
		// 
		// 
		//sa_8001sf.exe+48DE6 - 7C 42                 - jl sa_8001sf.exe+48E2A
		tool::memoryMove(effectAddr3, "\x7C\x42", 2u);
		//sa_8001sf.exe+49029 - 8B 4D 34              - mov ecx,[ebp+34]
		//sa_8001sf.exe + 4902C - 3B C1 - cmp eax, ecx
		tool::memoryMove(effectAddr4, "\x8B\x4D\x34\x3B\xC1", 5u);
		//sa_8001sf.exe+7BDF2 - 0F87 20010000         - ja sa_8001sf.exe+7BF18
		tool::memoryMove(effectAddr5, "\x0F\x87\x20\x01\x00\x00", 6u);
	}
	else
	{
		//sa_8001sf.exe+434DD - EB 08                 - jmp sa_8001sf.exe+434E7
		tool::memoryMove(effectAddr, "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90", 10u);

		//sa_8001.exe.text+5F013 - C7 05 F00D6304 C8000000 - mov [sa_8001.exe+4230DF0],000000C8 { (3),200 }
		tool::memoryMove(effectAddr6, "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90", 10u);

		//sa_8001.exe.text+20163 - C7 05 F00D6304 C8000000 - mov [sa_8001.exe+4230DF0],000000C8 { (3),200 }
		//tool::memoryMove(effectAddr7, "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90", 10u);

		///*
		//sa_8001sf.exe+482F0 - EB 04                 - jmp sa_8001sf.exe+482F6
		//sa_8001sf.exe+482F2 - FA                    - cli
		//sa_8001sf.exe+482F3 - 02 00                 - add al,[eax]
		//sa_8001sf.exe+482F5 - 00 8B 762883FE        - add [ebx-sa_8001sf.exe+13CD78A],cl
		//sa_8001sf.exe+482FB - FF 75 12              - push [ebp+12]

		//*/
		//tool::memoryMove(effectAddr2, "\xEB\x04\xFA\x02\x00\x00\x00\x8B\x76\x28\x83\xFE\xFF\x75\x12", 14);

		//sa_8001sf.exe+48DE6 - EB 42                 - jmp sa_8001sf.exe+48E2A
		tool::memoryMove(effectAddr3, "\xEB\x42", 2u);
		//sa_8001sf.exe+49029 - 3D 00000000           - cmp eax,00000000 { 0 }
		tool::memoryMove(effectAddr4, "\x3D\x00\x00\x00\x00", 5u);
		/*
		sa_8001sf.exe+7BDF2 - 90                    - nop
		sa_8001sf.exe+7BDF3 - E9 20010000           - jmp sa_8001sf.exe+7BF18
		*/
		tool::memoryMove(effectAddr5, "\x90\xE9\x20\x01\x00\x00", 6u);


	}

	return TRUE;
}

//顯示人物
BOOL game::Service::WM_EnableCharShow(BOOL enable)
{
	Q_D(Service);

	DWORD playerShowAddr = d->converTo<DWORD>(0xEA30ul);
	DWORD playerShowAddr2 = d->converTo<DWORD>(0xEFD0ul);
	DWORD playerShowAddr3 = d->converTo<DWORD>(0xF180ul);

	if (TRUE == enable)
	{
		//sa_8001sf.exe+EA30 - 51                    - push ecx
		tool::memoryMove(playerShowAddr, "\x51", 1u);
		//sa_8001sf.exe+EFD0 - 8B 44 24 04           - mov eax,[esp+04]
		tool::memoryMove(playerShowAddr2, "\x8B\x44\x24\x04", 4u);
		//sa_8001sf.exe+F180 - 8B 44 24 04           - mov eax,[esp+04]
		tool::memoryMove(playerShowAddr3, "\x8B\x44\x24\x04", 4u);
	}
	else
	{
		//sa_8001sf.exe+EA30 - C3                    - ret 
		tool::memoryMove(playerShowAddr, "\xC3", 1u);
		//sa_8001sf.exe+EFD0 - C3                    - ret 
		tool::memoryMove(playerShowAddr2, "\xC3\x90\x90\x90", 4u);
		//sa_8001sf.exe+F180 - C3                    - ret
		tool::memoryMove(playerShowAddr3, "\xC3\x90\x90\x90", 4u);
	}

	return TRUE;
}

//鎖定時間
BOOL game::Service::WM_SetTimeLock(BOOL enable, unsigned int time)
{
	Q_D(Service);

	//DWORD timerefreshAddr = d->converTo<DWORD>(0x1E6D0);
	//sa_8001sf.exe+1E6D0 - 56                    - push esi
		//tool::memoryMove(timerefreshAddr, "\x56", 1);

	if (TRUE == enable)
	{
		if (time <= 4u)
		{
			int* pcurrentTime = d->converTo<int*>(0x14CD00ul);
			int* pa = d->converTo<int*>(0x14CD68ul);
			int* pb = d->converTo<int*>(0x14D170ul);
			int* pc = d->converTo<int*>(0x1AB740ul);
			int* pd = d->converTo<int*>(0x1AB74Cul);

			constexpr int set[5 * 5] = {
				832, 3, 0, 0,    0, //下午
				64,  0, 1, 256,  1, //黃昏
				320, 1, 2, 512,  2, //午夜
				576, 2, 3, 768,  3, //早晨
				832, 3, 0, 1024, 0  //中午
			};

			//sa_8001sf.exe+1E6D0 - C3                    - ret 
			//tool::memoryMove(timerefreshAddr, "\xC3", 1);//把刷新時間的循環ret掉
			d->IS_TIME_LOCK_FLAG = TRUE;

			*pcurrentTime = set[time * 5u + 0u];
			*pa = set[time * 5u + 1u];
			*pb = set[time * 5u + 2u];
			*pc = set[time * 5u + 3u];
			*pd = set[time * 5u + 4u];
		}
	}

	d->IS_TIME_LOCK_FLAG = enable;

	return TRUE;
}

//打開聲音
BOOL game::Service::WM_EnableSound(BOOL enable)
{
	Q_D(Service);

	int* pBackgroundMusic = d->converTo<int*>(0xD36E0ul);
	int* pSoundEffect = d->converTo<int*>(0xD36DCul);
	auto pSetSound = d->converTo<void(__cdecl*)()>(0x880F0ul);

	if (TRUE == enable)
	{
		*pBackgroundMusic = d->currentMusic_;
		*pSoundEffect = d->currentSound_;
		pSetSound();
	}
	else
	{
		d->currentMusic_ = *pBackgroundMusic;
		d->currentSound_ = *pSoundEffect;
		*pBackgroundMusic = 0;
		*pSoundEffect = 0;
		pSetSound();
	}

	return TRUE;
}

//鎖定畫面
BOOL game::Service::WM_EnableImageLock(BOOL enable)
{
	Q_D(Service);

	DWORD pImageLock = d->converTo<DWORD>(0x42A7Bul);

	if (FALSE == enable)
	{
		/*
		sa_8001sf.exe+42A7B - E8 F0 FD FF FF           - call sa_8001sf.exe+42870
		sa_8001sf.exe+42A80 - 8B 15 44 A6 56 04        - mov edx,[sa_8001sf.exe+416A644] { (1280.00) }
		sa_8001sf.exe+42A86 - A1 48 A6 56 04           - mov eax,[sa_8001sf.exe+416A648] { (896.00) }
		sa_8001sf.exe+42A8B - 89 15 98 29 58 04        - mov [sa_8001sf.exe+4182998],edx { (1280.00) }
		sa_8001sf.exe+42A91 - A3 94 29 58 04           - mov [sa_8001sf.exe+4182994],eax { (896.00) }
		*/
		tool::memoryMove(pImageLock, "\xE8\xF0\xFD\xFF\xFF\x8B\x15\x44\xA6\x56\x04\xA1\x48\xA6\x56\x04\x89\x15\x98\x29\x58\x04\xA3\x94\x29\x58\x04", 27u);
	}
	else
	{
		//27 nop
		tool::memoryMove(pImageLock, "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90", 27u);
	}

	return TRUE;
}

//橫衝直撞
BOOL game::Service::WM_EnablePassWall(BOOL enable)
{
	Q_D(Service);

	DWORD pPassWall = d->converTo<DWORD>(0x42051ul);

	if (FALSE == enable)
	{
		//sa_8001sf.exe+42051 - 66 83 3C 55 749B5604 01 - cmp word ptr [edx*2+sa_8001sf.exe+4169B74],01 { 1 }
		tool::memoryMove(pPassWall, "\x66\x83\x3C\x55\x74\x9B\x56\x04\x01", 9u);
	}
	else
	{
		//sa_8001sf.exe+42051 - ret nop...
		tool::memoryMove(pPassWall, "\xC3\x90\x90\x90\x90\x90\x90\x90\x90", 9u);
	}

	return TRUE;
}

//快速走路
BOOL game::Service::WM_EnableFastWalk(BOOL enable)
{
	Q_D(Service);

	DWORD pFastWalk = d->converTo<DWORD>(0x42EB8ul);

	if (FALSE == enable)
	{
		//sa_8001sf.exe+42EB8 - 75 33                 - jne sa_8001sf.exe+42EED
		tool::memoryMove(pFastWalk, "\x75\x33", 2u);
		*d->converTo<float*>(0x9C328ul) = 4.0f;

	}
	else
	{
		*d->converTo<float*>(0x9C328ul) = 32.0f;
		tool::memoryMove(pFastWalk, "\x90\x90", 2u);
	}

	return TRUE;
}

//加速
BOOL game::Service::WM_SetBoostSpeed(BOOL enable, int speed)
{
	Q_D(Service);

	DWORD pBoostSpeed = d->converTo<DWORD>(0x1DEE4ul);
	int* pSpeed = d->converTo<int*>(0xAB7CCul);

	if ((FALSE == enable) || (speed <= 0))
	{
		//sa_8001sf.exe+1DEE4 - 83 F9 02              - cmp ecx,02 { 2 }
		*pSpeed = 14;

		//tool::memoryMove(pBoostSpeed, "\x83\xF9\x02", 3u);
		*reinterpret_cast<BYTE*>(pBoostSpeed + 2) = 2;

		d->speedBoostValue = 1UL;
	}
	else
	{
		if ((speed >= 1) && (speed <= 14))
		{
			*pSpeed = 14 - speed + 1;

			//sa_8001sf.exe+1DEE4 - 83 F9 0E              - cmp ecx,0E { 14 }
			//tool::memoryMove(pBoostSpeed, "\x83\xF9\x0E", 3u);
			*reinterpret_cast<BYTE*>(pBoostSpeed + 2) = 14;

			d->speedBoostValue = 1UL;
		}
		else if ((speed > 14) && (speed <= 125))
		{
			*pSpeed = 1;

			BYTE* pBoostSpeedValue = reinterpret_cast<BYTE*>(pBoostSpeed + 2UL);

			*pBoostSpeedValue = static_cast<BYTE>(speed + 2);

			d->speedBoostValue = static_cast<DWORD>(speed) - 13UL;
		}
	}

	return TRUE;
}

//lock character movement
BOOL game::Service::WM_EnableMoveLock(BOOL enable)
{
	Q_D(Service);

	//DWORD pMoveLock = d->converTo<DWORD>(0x42773ul);

	if (enable != d->IS_MOVE_LOCK)
	{
		d->IS_MOVE_LOCK = enable;
		DWORD* pMoveStart = d->converTo<DWORD*>(0x42795ul + 0x6ul);
		*pMoveStart = !enable ? 1UL : 0UL;
	}

	if (FALSE == enable)
	{
		DWORD* pMoveStart = d->converTo<DWORD*>(0x4181198ul);
		*pMoveStart = 0UL;
		pMoveStart = d->converTo<DWORD*>(0x41829FCul);
		*pMoveStart = 0UL;
	}

	//DWORD* pMoveLock = d->converTo<DWORD*>(0x4275Eul);
	//if (!enable)
	//{
	//	//00442773 - 77 2A                 - ja 0044279F
	//	tool::memoryMove(pMoveLock, "\x74\x3F", 2u);

	//	//sa_8001.exe+42795 - C7 05 E0295804 01000000 - mov [sa_8001.exe+41829E0],00000001 { (0),1 }
	//	//tool::memoryMove(pMoveLock, "\x01", 1u);
	//}
	//else
	//{
	//	//sa_8001sf.exe+42773 - EB 2A                 - jmp sa_8001sf.exe+4279F  //直接改跳轉會有機率完全卡住恢復後不能移動
	//	tool::memoryMove(pMoveLock, "\xEB\x3F", 2u);

	//	//fill \x90  取代的是把切換成1會觸發移動的值移除掉
	//	//tool::memoryMove(pMoveLock, "\x00", 1u);
	//}

	return TRUE;
}

//output annouce
BOOL game::Service::WM_Announce(char* str, int color)
{
	Q_D(Service);

	if (nullptr == str)
	{
		return FALSE;
	}

	auto pAnnounce = d->converTo<void(_cdecl*)(char*, int)>(0x115C0ul);
	if (nullptr == pAnnounce)
	{
		return FALSE;
	}

	pAnnounce(str, color);

	return TRUE;
}

//Move character 
BOOL game::Service::WM_Move(int x, int y)
{
	Q_D(Service);

	/*
	sa_8001sf.exe+4277A - 8B 15 38A65604        - mov edx,[sa_8001sf.exe+416A638] { (98) }
	sa_8001sf.exe+42780 - A3 54B74B00           - mov [sa_8001sf.exe+BB754],eax { (580410503) }
	sa_8001sf.exe+42785 - A1 3CA65604           - mov eax,[sa_8001sf.exe+416A63C] { (0.01) }
	sa_8001sf.exe+4278A - 89 15 4C025604        - mov [sa_8001sf.exe+416024C],edx { (100) }
	sa_8001sf.exe+42790 - A3 50025604           - mov [sa_8001sf.exe+4160250],eax { (100) }
	sa_8001sf.exe+42795 - C7 05 E0295804 01000000 - mov [sa_8001sf.exe+41829E0],00000001 { (0),1 }

	*/

	qDebug() << __FUNCTION__;
	*d->converTo<int*>(0x416024Cul) = x;//goalX
	*d->converTo<int*>(0x4160250ul) = y;//goalY
	*d->converTo<int*>(0x41829E0ul) = 1;//moveStart

	//*d->converTo<int*>(0x4181D3C) = x;
	//*d->converTo<int*>(0x4181D40) = y;
	//*d->converTo<float*>(0x416A644) = static_cast<float>(x) * 64.0f;
	//*d->converTo<float*>(0x416A648) = static_cast<float>(y) * 64.0f;
	//*d->converTo<float*>(0x4182998) = static_cast<float>(x) * 64.0f;
	//*d->converTo<float*>(0x4182994) = static_cast<float>(y) * 64.0f;
	return TRUE;
}

//Force to distory dialog
BOOL game::Service::WM_DistoryDialog()
{
	Q_D(Service);

	auto a = d->converTo<int*>(0xB83ECul);
	if (a != nullptr)
		*a = -1;

	auto distory = d->converTo<void(_cdecl*)(int)>(0x11C0ul);
	if (nullptr == distory)
	{
		return FALSE;
	}

	int* pDialog = d->converTo<int*>(0x415EF98ul);
	if (nullptr == pDialog)
	{
		return FALSE;
	}

	if (0 == *pDialog)
	{
		return FALSE;
	}

	distory(*pDialog);
	*pDialog = 0;

	return TRUE;
}

//Clear chat history
BOOL game::Service::WM_CleanChatHistory()
{
	Q_D(Service);

	//sa_8001.exe + 117C0 - B8 884D5400 - mov eax, sa_8001.exe + 144D88 { (0) }
	//sa_8001.exe + 117C5 - C6 00 00 - mov byte ptr[eax], 00 { 0 }
	//sa_8001.exe + 117C8 - 05 0C010000 - add eax, 0000010C { 268 }
	//sa_8001.exe + 117CD - 3D 78625400 - cmp eax, sa_8001.exe + 146278 { (652401777) }
	//sa_8001.exe + 117D2 - 7C F1 - jl sa_8001.exe + 117C5
	//sa_8001.exe + 117D4 - C7 05 F8A45400 00000000 - mov[sa_8001.exe + 14A4F8], 00000000 { (0), 0 }
	//sa_8001.exe + 117DE - C3 - ret

	constexpr size_t bufsize = 268U;

	char* chatbuffer = d->converTo<char*>(0x144D88ul);
	char* pmaxchatbuffer = d->converTo<char*>(0x146278ul);
	int* pchatsize = d->converTo<int*>(0x14A4F8ul);
	if ((nullptr == chatbuffer) || (nullptr == pmaxchatbuffer) || (nullptr == pchatsize))
	{
		return FALSE;
	}

	const size_t count = static_cast<size_t>(pmaxchatbuffer - chatbuffer);

	for (size_t i = 0; i < count; i += bufsize)
	{
		*chatbuffer = '\0';
		chatbuffer += bufsize;
	}

	*pchatsize = 0;

	return TRUE;
}

//Create a custom dialog
BOOL game::Service::WM_CreateDialog(int type, int button, const char* data)
{
	Q_D(Service);

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

	qDebug() << "createDialog:" << "type:" << type << " button:" << button;
	d->pCreateDialog(0, type, button, 0x10E1, 0x4D2, data);

	return TRUE;
}

//Enable/Disable the tcp packet block
BOOL game::Service::WM_SetBlockPacket(BOOL enable)
{
	Q_D(Service);

	d->IS_ENCOUNT_BLOCK_FLAG = enable;

	return TRUE;
}
#pragma endregion