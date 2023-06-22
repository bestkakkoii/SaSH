#pragma once
#include <QObject>
#include "net/tcpserver.h"
#include <util.h>

class StringListModel;
class Injector : public QObject
{
	Q_DISABLE_COPY_MOVE(Injector)
private:
	Injector();

	static Injector* instance;
public:
	virtual ~Injector();

	static Injector& getInstance()
	{
		if (nullptr == instance)
		{
			instance = new Injector();
		}
		return *instance;
	}

	static void clear();

public:

	enum UserMessage
	{
		kUserMessage = WM_USER + 0x1024,
		kConnectionOK,
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
		kBattleTimeExtend,
		kEnableOptimize,

		//Action
		kSendAnnounce,
		kSetMove,
		kDistoryDialog,
		kCleanChatHistory,
	};

	typedef struct process_information_s
	{
		qint64 dwProcessId = NULL;
		DWORD dwThreadId = NULL;
		HWND hWnd = nullptr;
	} process_information_t, * pprocess_information_t, * lpprocess_information_t;

	inline void close() const { if (processHandle_) MINT::NtTerminateProcess(processHandle_, 0); }

	Q_REQUIRED_RESULT inline HANDLE getProcess() const { return processHandle_; }

	Q_REQUIRED_RESULT inline HWND getProcessWindow() const { return pi_.hWnd; }

	Q_REQUIRED_RESULT inline DWORD getProcessId() const { return pi_.dwProcessId; }

	Q_REQUIRED_RESULT inline int getProcessModule() const { return hModule_; }

	Q_REQUIRED_RESULT inline bool isValid() const { return hModule_ != NULL && pi_.dwProcessId != NULL && pi_.hWnd != nullptr && processHandle_.isValid(); }

	bool createProcess(process_information_t& pi);

	bool injectLibrary(process_information_t& pi, unsigned short port, util::LPREMOVE_THREAD_REASON pReason);

	void remoteFreeModule();

	Q_REQUIRED_RESULT bool isWindowAlive() const;

	int sendMessage(int msg, int wParam, int lParam) const;

	bool postMessage(int msg, int wParam, int lParam) const;

	inline void setValueHash(util::UserSetting setting, int value) { userSetting_value_hash_.insert(setting, value); }

	inline void setEnableHash(util::UserSetting setting, bool enable) { userSetting_enable_hash_.insert(setting, enable); }

	inline void setStringHash(util::UserSetting setting, const QString& string) { userSetting_string_hash_.insert(setting, string); }

	Q_REQUIRED_RESULT inline int getValueHash(util::UserSetting setting) const { return userSetting_value_hash_.value(setting); }

	Q_REQUIRED_RESULT inline bool getEnableHash(util::UserSetting setting) const { return userSetting_enable_hash_.value(setting); }

	Q_REQUIRED_RESULT inline QString getStringHash(util::UserSetting setting) const { return userSetting_string_hash_.value(setting); }

	Q_REQUIRED_RESULT inline util::SafeHash<util::UserSetting, int> getValueHash() const { return userSetting_value_hash_; }

	Q_REQUIRED_RESULT inline util::SafeHash<util::UserSetting, bool> getEnableHash() const { return userSetting_enable_hash_; }

	Q_REQUIRED_RESULT inline util::SafeHash<util::UserSetting, QString> getStringHash() const { return userSetting_string_hash_; }

	inline void setValueHash(const util::SafeHash<util::UserSetting, int>& hash) { userSetting_value_hash_ = hash; }

	inline void setEnableHash(const util::SafeHash<util::UserSetting, bool>& hash) { userSetting_enable_hash_ = hash; }

	inline void setStringHash(const util::SafeHash<util::UserSetting, QString>& hash) { userSetting_string_hash_ = hash; }

	Q_REQUIRED_RESULT inline QVariant getUserData(util::UserData type) const { return userData_hash_.value(type); }

	inline void setUserData(util::UserData type, const QVariant& data) { userData_hash_.insert(type, QVariant::fromValue(data)); }

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

	Q_REQUIRED_RESULT bool isHandleValid(qint64 pid);

public:
	QScopedPointer<Server> server;

	bool IS_SCRIPT_FLAG = false;

	QScopedPointer<StringListModel> scriptLogModel;

	QScopedPointer<StringListModel> chatLogModel;

private:
	int hModule_ = NULL;
	HMODULE hookdllModule_ = NULL;
	process_information_t pi_ = {};
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

		//afk->battle button
		{ util::kBattleCharRoundActionTargetValue, util::kSelectEnemyAny },
		{ util::kBattleCharCrossActionTargetValue, util::kSelectEnemyAny },
		{ util::kBattleCharNormalActionTargetValue, util::kSelectEnemyAny },
		{ util::kBattlePetNormalActionTargetValue, util::kSelectEnemyAny },

		{ util::kBattlePetRoundActionTargetValue, util::kSelectEnemyAny },
		{ util::kBattlePetCrossActionTargetValue, util::kSelectEnemyAny },

		//afk->battle combobox
		{ util::kBattleCharRoundActionRoundValue, 0 },
		{ util::kBattleCharRoundActionTypeValue, 0 },
		{ util::kBattleCharRoundActionEnemyValue, 0 },
		{ util::kBattleCharRoundActionLevelValue, 0 },

		{ util::kBattleCharCrossActionTypeValue, 0 },
		{ util::kBattleCharCrossActionRoundValue, 0 },

		{ util::kBattleCharNormalActionTypeValue, 0 },
		{ util::kBattleCharNormalActionEnemyValue, 0 },
		{ util::kBattleCharNormalActionLevelValue, 0 },

		{ util::kBattlePetRoundActionRoundValue, 0 },
		{ util::kBattlePetRoundActionTypeValue, 0 },
		{ util::kBattlePetRoundActionEnemyValue, 0 },
		{ util::kBattlePetRoundActionLevelValue, 0 },

		{ util::kBattlePetCrossActionTypeValue, 0 },
		{ util::kBattlePetCrossActionRoundValue, 0 },

		{ util::kBattlePetNormalActionTypeValue, 0 },
		{ util::kBattlePetNormalActionEnemyValue, 0 },
		{ util::kBattlePetNormalActionLevelValue, 0 },

		//afk->heal button
		{ util::kBattleMagicHealTargetValue, util::kSelectSelf | util::kSelectPet },
		{ util::kBattleItemHealTargetValue, util::kSelectSelf | util::kSelectPet },
		{ util::kBattleMagicReviveTargetValue, util::kSelectSelf | util::kSelectPet },
		{ util::kBattleItemReviveTargetValue, util::kSelectSelf | util::kSelectPet },

		//afk->heal combobox
		{ util::kBattleMagicHealMagicValue, 0 },
		{ util::kBattleMagicReviveMagicValue, 0 },

		{ util::kNormalMagicHealMagicValue, 0 },

		//afk->heal spinbox
		{ util::kMagicHealCharValue, 50 },
		{ util::kMagicHealPetValue, 50 },
		{ util::kMagicHealAllieValue, 50 },
		{ util::kItemHealCharValue, 50 },
		{ util::kItemHealPetValue, 50 },
		{ util::kItemHealAllieValue, 50 },
		{ util::kItemHealMpValue, 50 },

		{ util::kMagicHealCharNormalValue, 50 },
		{ util::kMagicHealPetNormalValue, 50 },
		{ util::kMagicHealAllieNormalValue, 50 },
		{ util::kItemHealCharNormalValue, 50 },
		{ util::kItemHealPetNormalValue, 50 },
		{ util::kItemHealAllieNormalValue, 50 },
		{ util::kItemHealMpNormalValue, 50 },

		//afk->walk
		{ util::kAutoWalkDelayValue, 10 },
		{ util::kAutoWalkDistanceValue, 2 },
		{ util::kAutoWalkDirectionValue, 0 },

		//afk->catch
		{ util::kBattleCatchModeValue, 0 },
		{ util::kBattleCatchTargetLevelValue, 1 },
		{ util::kBattleCatchTargetMaxHpValue, 10 },
		{ util::kBattleCatchTargetMagicHpValue, 10 },
		{ util::kBattleCatchTargetItemHpValue, 10 },
		{ util::kBattleCatchPlayerMagicValue, 0 },
		{ util::kBattleCatchPetSkillValue, 0 },

		{ util::kDropPetStrValue, 10 },
		{ util::kDropPetDefValue, 10 },
		{ util::kDropPetAgiValue, 10 },
		{ util::kDropPetHpValue, 50 },
		{ util::kDropPetAggregateValue, 10 },

		//other->group
		{ util::kAutoFunTypeValue, 0 },

		//lockpet
		{ util::kLockPetValue, 0 },
		{ util::kLockRideValue, 0 },


		//script
		{util::kScriptSpeedValue, 0},

		{ util::kSettingMaxValue, util::kSettingMaxValue },
		{ util::kSettingMinString, util::kSettingMinString },
		{ util::kSettingMaxString, util::kSettingMaxString },

	};

	util::SafeHash<util::UserSetting, bool> userSetting_enable_hash_ = {
		{ util::kAutoLoginEnable, false },
		{ util::kAutoReconnectEnable, true },
		{ util::kLogOutEnable, false },
		{ util::kLogBackEnable, false },
		{ util::kEchoEnable, false },
		{ util::kHideCharacterEnable, false },
		{ util::kCloseEffectEnable, false },
		{ util::kOptimizeEnable, true },
		{ util::kHideWindowEnable, false },
		{ util::kMuteEnable, false },
		{ util::kAutoJoinEnable, false },
		{ util::kLockTimeEnable, false },
		{ util::kAutoFreeMemoryEnable, true },
		{ util::kFastWalkEnable, true },
		{ util::kPassWallEnable, false },
		{ util::kLockMoveEnable, false },
		{ util::kLockImageEnable, false },
		{ util::kAutoDropMeatEnable, true },
		{ util::kAutoDropEnable, false },
		{ util::kAutoStackEnable, true },
		{ util::kKNPCEnable, false },
		{ util::kAutoAnswerEnable, false },
		{ util::kAutoEatBeanEnable, false },
		{ util::kAutoWalkEnable, false },
		{ util::kFastAutoWalkEnable, false },
		{ util::kFastBattleEnable, true },
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

		//afk->battle
		{ util::kCrossActionCharEnable, false },
		{ util::kCrossActionPetEnable, false },

		//afk->heal
		{ util::kBattleMagicHealEnable, false },
		{ util::kBattleItemHealEnable, false },
		{ util::kBattleItemHealMeatPriorityEnable, false },
		{ util::kBattleItemHealMpEnable, false },
		{ util::kBattleMagicReviveEnable, false },
		{ util::kBattleItemReviveEnable, false },

		{ util::kNormalMagicHealEnable, false },
		{ util::kNormalItemHealEnable, false },
		{ util::kNormalItemHealMeatPriorityEnable, false },
		{ util::kNormalItemHealMpEnable, false },

		//afk->catch
		{ util::kBattleCatchTargetLevelEnable, false },
		{ util::kBattleCatchTargetMaxHpEnable, false },
		{ util::kBattleCatchPlayerMagicEnable, false },
		{ util::kBattleCatchPlayerItemEnable, false },
		{ util::kBattleCatchPetSkillEnable, false },

		{ util::kDropPetEnable, false },
		{ util::kDropPetStrEnable, false },
		{ util::kDropPetDefEnable, false },
		{ util::kDropPetAgiEnable, false },
		{ util::kDropPetHpEnable, false },
		{ util::kDropPetAggregateEnable, false },

		//lockpet
		{ util::kLockPetEnable, false },
		{ util::kLockRideEnable, false },
		{ util::kLockPetScheduleEnable, false },
		{ util::kLockRideScheduleEnable, false },
		//other->group

	};

	util::SafeHash<util::UserSetting, QString> userSetting_string_hash_ = {
		{ util::kAutoDropItemString, "" },
		{ util::kLockAttackString, "" },
		{ util::kLockEscapeString, "" },

		{ util::kBattleItemHealItemString, "" },
		{ util::kBattleItemHealMpIteamString, "" },
		{ util::kBattleItemReviveItemString, "" },
		{ util::kNormalItemHealItemString, "" },
		{ util::kNormalItemHealMpItemString, "" },

		//afk->catch
		{ util::kBattleCatchPetNameString, "" },
		{ util::kBattleCatchPlayerItemString, "" },
		{ util::kDropPetNameString, "" },

		{ util::kAutoFunNameString, "" },

		//other->lockpet
		{ util::kLockPetScheduleString, "" },
		{ util::kLockRideScheduleString, "" },

		{ util::kGameAccountString, "" },
		{ util::kGamePasswordString, "" },
		{ util::kGameSecurityCodeString, "" },

		{ util::kMailWhiteListString , "" },

	};
};
