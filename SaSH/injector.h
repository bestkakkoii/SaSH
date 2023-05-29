#pragma once
#include <QObject>
#include "net/tcpserver.h"
#include "util.h"

class Injector : public QObject
{
	Q_DISABLE_COPY_MOVE(Injector)
private:
	Injector() = default;
	static Injector* instance;
public:
	virtual ~Injector()
	{
		qDebug() << "Injector is destroyed";
	}

	static Injector& getInstance()
	{
		if (instance == nullptr)
		{
			instance = new Injector();
		}
		return *instance;
	}
	static void clear()
	{
		if (instance != nullptr)
		{
			instance->server.reset();
			instance->hModule_ = NULL;
			instance->hookdllModule_ = NULL;
			instance->pi_ = {  };
			instance->processHandle_.reset();
			delete instance;
			instance = nullptr;
		}
	}
public:

	enum UserMessage
	{
		kUserMessage = WM_USER + 0x1024,
		kInitialize,
		kUninitialize,
		kGetModule,
		kSendPacket,

		//
		kEnableEffect,
		kEnablePlayerShow,
		kSetTimeLock,
		kEnableSound,
		kEnableImageLock,
		kEnablePassWall,
		kEnableFastWalk,
		kSetBoostSpeed,
		kEnableMoveLock,
		kMuteSound,
		kEnableBattleDialog,
		kSetGameStatus,
		kSetBLockPacket,

		//Action
		kSendAnnounce,
		kSetMove,
	};


	typedef struct process_information_s
	{
		qint64 dwProcessId = NULL;
		DWORD dwThreadId = NULL;
		HWND hWnd = nullptr;
	} process_information_t, * pprocess_information_t, * lpprocess_information_t;

	HANDLE getProcess() const
	{
		return processHandle_;
	}

	HWND getProcessWindow() const
	{
		return pi_.hWnd;
	}

	DWORD getProcessId() const
	{
		return pi_.dwProcessId;
	}

	int getProcessModule() const
	{
		return hModule_;
	}

	bool isValid() const
	{
		return hModule_ != NULL && pi_.dwProcessId != NULL && pi_.hWnd != nullptr && processHandle_.isValid();
	}

	bool createProcess(process_information_t& pi);

	bool injectLibrary(process_information_t& pi, unsigned short port, util::LPREMOVE_THREAD_REASON pReason);

	void remoteFreeModule();

	bool isWindowAlive() const;

	int sendMessage(int msg, int wParam, int lParam) const
	{
		if (!msg) return 0;

#ifdef _WIN64
		DWORD_PTR dwResult = NULL;
		SendMessageTimeoutA(m_pi.hWnd, msg, wParam, lParam, SMTO_ABORTIFHUNG | SMTO_NOTIMEOUTIFNOTHUNG, 5000, &dwResult);
#else
		DWORD dwResult = NULL;
		SendMessageTimeoutW(pi_.hWnd, msg, wParam, lParam, SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT | SMTO_BLOCK, 5000, &dwResult);
#endif
		return static_cast<int>(dwResult);
	}

	bool postMessage(int msg, int wParam, int lParam)
	{
		if (!msg) return 0;
		BOOL ret = PostMessageW(pi_.hWnd, msg, wParam, lParam);
		return  ret == TRUE;
	}

	void setValueHash(util::UserSetting setting, int value)
	{
		userSetting_value_hash_.insert(setting, value);
	}

	void setEnableHash(util::UserSetting setting, bool enable)
	{
		userSetting_enable_hash_.insert(setting, enable);
	}

	void setStringHash(util::UserSetting setting, const QString& string)
	{
		userSetting_string_hash_.insert(setting, string);
	}

	int getValueHash(util::UserSetting setting) const
	{
		return userSetting_value_hash_.value(setting);
	}

	bool getEnableHash(util::UserSetting setting) const
	{
		return userSetting_enable_hash_.value(setting);
	}

	QString getStringHash(util::UserSetting setting) const
	{
		return userSetting_string_hash_.value(setting);
	}

	util::SafeHash<util::UserSetting, int> getValueHash() const
	{
		return userSetting_value_hash_;
	}

	util::SafeHash<util::UserSetting, bool> getEnableHash() const
	{
		return userSetting_enable_hash_;
	}

	util::SafeHash<util::UserSetting, QString> getStringHash() const
	{
		return userSetting_string_hash_;
	}

	void setValueHash(const util::SafeHash<util::UserSetting, int>& hash)
	{
		userSetting_value_hash_ = hash;
	}

	void setEnableHash(const util::SafeHash<util::UserSetting, bool>& hash)
	{
		userSetting_enable_hash_ = hash;
	}

	void setStringHash(const util::SafeHash<util::UserSetting, QString>& hash)
	{
		userSetting_string_hash_ = hash;
	}

	QVariant getUserData(util::UserData type) const
	{
		return userData_hash_.value(type);
	}

	void setUserData(util::UserData type, const QVariant& data)
	{
		userData_hash_.insert(type, QVariant::fromValue(data));
	}

private:
	static BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam)
	{
		lpprocess_information_t data = reinterpret_cast<lpprocess_information_t>(lParam);
		DWORD dwProcessId = 0;
		do
		{
			if (!handle || !lParam) break;

			::GetWindowThreadProcessId(handle, &dwProcessId);
			if (data->dwProcessId == dwProcessId && IsWindowVisible(handle))
			{
				data->hWnd = handle;
				return FALSE;
			}
		} while (false);
		return TRUE;
	}

	bool isHandleValid(qint64 pid)
	{
		if (!pid)
			pid = pi_.dwProcessId;
		if (!pid)
			return false;
		if (!processHandle_.isValid())
		{
			processHandle_.reset(pid);
			return processHandle_.isValid();
		}
		return true;
	}

public:
	QScopedPointer<Server> server;

private:
	int hModule_ = NULL;
	HMODULE hookdllModule_ = NULL;
	process_information_t pi_ = {  };
	QScopedHandle processHandle_;


	util::SafeHash<util::UserData, QVariant> userData_hash_ = {
		{ util::kUserItemNames, QStringList() },

	};

	util::SafeHash<util::UserSetting, int> userSetting_value_hash_ = {
		{ util::kSettingNotUsed, util::kSettingNotUsed },
		{ util::kSettingMinValue, util::kSettingMinValue },

		{ util::kServerValue, 0 },
		{ util::kSubServerValue, 0 },
		{ util::kPositionValue, 0 },
		{ util::kLockTimeValue, 0 },
		{ util::kSpeedBoostValue, 10 },

		//afk->walk
		{ util::kAutoWalkDelayValue, 10 },
		{ util::kAutoWalkDistanceValue, 1 },
		{ util::kAutoWalkDirectionValue, 0 },

		{ util::kSettingMaxValue, util::kSettingMaxValue },
		{ util::kSettingMinString, util::kSettingMinString },
		{ util::kSettingMaxString, util::kSettingMaxString }
	};

	util::SafeHash<util::UserSetting, bool> userSetting_enable_hash_ = {
		{ util::kAutoLoginEnable, false },
		{ util::kAutoReconnectEnable, false },
		{ util::kLogOutEnable, false },
		{ util::kLogBackEnable, false },
		{ util::kEchoEnable, false },
		{ util::kHideCharacterEnable, false },
		{ util::kCloseEffectEnable, false },
		{ util::kOptimizeEnable, false },
		{ util::kHideWindowEnable, false },
		{ util::kMuteEnable, false },
		{ util::kAutoJoinEnable, false },
		{ util::kLockTimeEnable, false },
		{ util::kAutoFreeMemoryEnable, false },
		{ util::kFastWalkEnable, false },
		{ util::kPassWallEnable, false },
		{ util::kLockMoveEnable, false },
		{ util::kLockImageEnable, false },
		{ util::kAutoDropMeatEnable, false },
		{ util::kAutoDropEnable, false },
		{ util::kAutoStackEnable, false },
		{ util::kKNPCEnable, false },
		{ util::kAutoAnswerEnable, false },
		{ util::kForceLeaveBattleEnable, false },
		{ util::kAutoWalkEnable, false },
		{ util::kFastAutoWalkEnable, false },
		{ util::kFastBattleEnable, false },
		{ util::kAutoBattleEnable, false },
		{ util::kAutoCatchEnable, false },
		{ util::kLockAttackEnable, false },
		{ util::kAutoEscapeEnable, false },
		{ util::kLockEscapeEnable, false },
		{ util::kBattleTimeExtendEnable, false },
		{ util::kFallDownEscapeEnable, false },

		//switcher
		{ util::kSwitcherTeamEnable, false },
		{ util::kSwitcherPKEnable, false },
		{ util::kSwitcherCardEnable, false },
		{ util::kSwitcherTradeEnable, false },
		{ util::kSwitcherFamilyEnable, false },
		{ util::kSwitcherJobEnable, false },
		{ util::kSwitcherWorldEnable, false },
	};

	util::SafeHash<util::UserSetting, QString> userSetting_string_hash_ = {
		{ util::kAutoDropItemString, "" },
		{ util::kLockAttackString, "" },
		{ util::kLockEscapeString, "" },

		{ util::kGameAccountString, "" },
		{ util::kGamePasswordString, "" },
		{ util::kGameSecurityCodeString, "" },

	};
};
