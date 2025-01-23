#ifndef GAMESERVICE_P_H_
#define GAMESERVICE_P_H_
#include "windows.h"
#include <QHash>
#include <QString>
#include <QStringList>

#include "sahook/game/include/gameservice.h"

#include "sahook/game/include/gameImplement.h"

namespace game
{
	class ServicePrivate : public Implement
	{
	public:
		explicit ServicePrivate(Service* q);
		virtual ~ServicePrivate();

	private:
		template <typename Ret, typename Offset>
		Ret converTo(Offset offset) const
		{
			Ret result = Ret();
			global::converTo(processBase_, offset, &result);

			return result;
		}


		bool compareLocation(qint64 x, qint64 y, qint64 direction) const;

	public:
		//Protocol protocol;

	private:
		qint64 xCache_ = 0;
		qint64 yCache_ = 0;
		qint64 directionCache_ = 0;

	private:
		int* g_sockfd = nullptr;
		int* g_world_status = nullptr;
		int* g_game_status = nullptr;

		HWND g_consoleHwnd = nullptr;

		WNDPROC g_OldWndProc = nullptr;
		HMODULE g_hGameModule = nullptr;
		HMODULE g_hDllModule = nullptr;
		WCHAR g_szGameModulePath[MAX_PATH] = {};
		DWORD g_MainThreadId = 0;
		HANDLE g_MainThreadHandle = nullptr;

		HWND g_ParenthWnd = nullptr;
		HWND g_MainHwnd = nullptr;

		mutable std::shared_mutex g_statusLock;

	private:
		BOOL IS_BATTLE_PROC_FLAG = FALSE;
		BOOL IS_TIME_LOCK_FLAG = FALSE;
		BOOL IS_SOUND_MUTE_FLAG = FALSE;
		BOOL IS_ENCOUNT_BLOCK_FLAG = FALSE;
		BOOL IS_MOVE_LOCK = FALSE;
		BOOL IS_FORWARD_SEND = FALSE;

	private:
		int currentMusic_ = 15;
		int currentSound_ = 15;
		int nowChatRowCount_ = 10;
		DWORD speedBoostValue = 1UL;

		std::atomic_int isInitialized_ = FALSE;
		std::atomic_int enableSleepAdjust = FALSE;

		std::atomic_int64_t index_ = 0i64;

		BYTE oldBattleCommandReadyByte[6u] = {}; //保存舊數據用於還原

	private:


	private:
		Q_DECLARE_PUBLIC(Service);
		Q_DISABLE_COPY_MOVE(ServicePrivate);

		Service* q_ptr = nullptr;
	};
}

#endif // GAMESERVICE_P_H_