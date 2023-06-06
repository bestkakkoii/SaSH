#pragma once
#include <QScopedPointer>
#include <QRegularExpression>
#include <QObject>
#include <QWidget>
#include <QList>
#include <QVariant>
#include <QVariantMap>
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QTabBar>
#include <QLocale>
#include <QCollator>
#include <QHash>
#include "3rdparty/simplecrypt.h"
#include "model/treewidgetitem.h"

constexpr int SASH_VERSION_MAJOR = 1;
constexpr int SASH_VERSION_MINOR = 0;
constexpr int SASH_VERSION_PATCH = 0;

namespace mem
{
	bool read(HANDLE hProcess, DWORD desiredAccess, SIZE_T size, PVOID buffer);
	Q_REQUIRED_RESULT int readInt(HANDLE hProcess, DWORD desiredAccess, SIZE_T size);
	Q_REQUIRED_RESULT float readFloat(HANDLE hProcess, DWORD desiredAccess);
	Q_REQUIRED_RESULT qreal readDouble(HANDLE hProcess, DWORD desiredAccess);
	Q_REQUIRED_RESULT QString readString(HANDLE hProcess, DWORD desiredAccess, int size, bool enableTrim);
	bool write(HANDLE hProcess, DWORD baseAddress, PVOID buffer, SIZE_T dwSize);
	bool writeInt(HANDLE hProcess, DWORD baseAddress, int data, int mode);
	bool writeString(HANDLE hProcess, DWORD baseAddress, const QString& str);
	bool virtualFree(HANDLE hProcess, int baseAddress);
	Q_REQUIRED_RESULT int virtualAlloc(HANDLE hProcess, int size);
	Q_REQUIRED_RESULT int virtualAllocW(HANDLE hProcess, const QString& str);
	Q_REQUIRED_RESULT int virtualAllocA(HANDLE hProcess, const QString& str);
	Q_REQUIRED_RESULT DWORD getRemoteModuleHandle(DWORD dwProcessId, const QString& moduleName);
	void freeUnuseMemory(HANDLE hProcess);
}

namespace util
{
	constexpr const char* SA_NAME = u8"sa_8001.exe";
	constexpr const char* CODEPAGE_DEFAULT = "UTF-8";
	constexpr const char* SCRIPT_SUFFIX_DEFAULT = ".txt";

	typedef enum
	{
		REASON_NO_ERROR,
		//Thread
		REASON_CLOSE_BEFORE_START,
		REASON_PID_INVALID,
		REASON_FAILED_TO_INIT,
		REASON_MEMBERSHIP_INVALID,
		REASON_REQUEST_STOP,
		REASON_TARGET_WINDOW_DISAPPEAR,
		REASON_FAILED_READ_DATA,

		//TCP
		REASON_TCP_PRECONNECTION_TIMEOUT,
		REASON_TCP_CONNECTION_TIMEOUT,
		REASON_TCP_CREATE_SOCKET_FAIL,
		REASON_TCP_IPV6_CONNECT_FAIL,
		REASON_TCP_IPV4_CONNECT_FAIL,
		REASON_TCP_EVENT_SELECT_FAIL,
		REASON_TCP_KEEPALIVE_FAIL,

		//CreateProcess
		REASON_DIRECTORY_EMPTY,
		REASON_ARGUMENT_EMPTY,
		REASON_IMAGE_PATH_NOFOUND,
		REASON_EXECUTABLE_FILE_PATH_EMPTY,
		REASON_FAILED_TO_START,

		//Injection
		REASON_ENUM_WINDOWS_FAIL,
		REASON_HOOK_DLL_SHA512_FAIL,
		REASON_GET_KERNEL32_FAIL,
		REASON_GET_KERNEL32_UNDOCUMENT_API_FAIL,
		REASON_OPEN_PROCESS_FAIL,
		REASON_VIRTUAL_ALLOC_FAIL,
		REASON_INJECT_LIBRARY_FAIL,
		REASON_CLOSE_MUTEX_FAIL,
		REASON_REMOTE_LIBRARY_INIT_FAIL,
	}REMOVE_THREAD_REASON, * LPREMOVE_THREAD_REASON;

	enum UserStatus
	{
		kLabelStatusNotUsed = 0,//未知
		kLabelStatusNotOpen,//未開啟石器
		kLabelStatusOpening,//開啟石器中
		kLabelStatusOpened,//已開啟石器
		kLabelStatusLogining,//登入
		kLabelStatusSignning,//簽入中
		kLabelStatusSelectServer,//選擇伺服器
		kLabelStatusSelectSubServer,//選擇分伺服器
		kLabelStatusGettingPlayerList,//取得人物中
		kLabelStatusSelectPosition,//選擇人物中
		kLabelStatusLoginSuccess,//登入成功
		kLabelStatusInNormal,//平時
		kLabelStatusInBattle,//戰鬥中
		kLabelStatusBusy,//忙碌中
		kLabelStatusTimeout,//連線逾時
		kLabelNoUserNameOrPassword,//無帳號密碼
		kLabelStatusDisconnected,//斷線
	};

	enum UserData
	{
		kUserDataNotUsed = 0x0,
		kUserItemNames = 0x1,
		kUserEnemyNames = 0x2,
		kUserPetNames = 0x4,
	};

	enum ComboBoxUpdateType
	{
		kComboBoxCharAction,
		kComboBoxPetAction,
		kComboBoxItem,
	};

	enum SelectTarget
	{
		kSelectSelf = 0x1,            // 己 (Self)
		kSelectPet = 0x2,             // 寵 (Pet)
		kSelectAllieAny = 0x4,        // 我任 (Any ally)
		kSelectAllieAll = 0x8,        // 我全 (All allies)
		kSelectEnemyAny = 0x10,       // 敵任 (Any enemy)
		kSelectEnemyAll = 0x20,       // 敵全 (All enemies)
		kSelectEnemyFront = 0x40,     // 敵前 (Front enemy)
		kSelectEnemyBack = 0x80,      // 敵後 (Back enemy)
		kSelectLeader = 0x100,        // 隊 (Leader)
		kSelectLeaderPet = 0x200,     // 隊寵 (Leader's pet)
		kSelectTeammate1 = 0x400,     // 隊1 (Teammate 1)
		kSelectTeammate1Pet = 0x800,  // 隊1寵 (Teammate 1's pet)
		kSelectTeammate2 = 0x1000,    // 隊2 (Teammate 2)
		kSelectTeammate2Pet = 0x2000, // 隊2寵 (Teammate 2's pet)
		kSelectTeammate3 = 0x4000,    // 隊3 (Teammate 3)
		kSelectTeammate3Pet = 0x8000, // 隊3寵 (Teammate 3's pet)
		kSelectTeammate4 = 0x10000,   // 隊4 (Teammate 4)
		kSelectTeammate4Pet = 0x20000 // 隊4寵 (Teammate 4's pet)
	};

	enum UnLoginStatus
	{
		kStatusUnknown,
		kStatusInputUser,
		kStatusSelectServer,
		kStatusSelectSubServer,
		kStatusSelectCharacter,
		kStatusLogined,
		kStatusDisconnect,
		kStatusTimeout,
		kNoUserNameOrPassword,
		kStatusBusy,
	};

	enum CraftType
	{
		kCraftFood,
		kCraftItem
	};

	enum UserSetting
	{
		kSettingNotUsed = 0,

		///////////////////

		kSettingMinValue,

		kServerValue,
		kSubServerValue,
		kPositionValue,
		kLockTimeValue,

		//afk->battle button
		kBattleCharRoundActionTargetValue,
		kBattleCharCrossActionTargetValue,
		kBattleCharNormalActionTargetValue,
		kBattlePetNormalActionTargetValue,

		kBattlePetRoundActionTargetValue,
		kBattlePetCrossActionTargetValue,

		//afk->battle combobox
		kBattleCharRoundActionRoundValue,
		kBattleCharRoundActionTypeValue,
		kBattleCharRoundActionEnemyValue,
		kBattleCharRoundActionLevelValue,

		kBattleCharCrossActionTypeValue,
		kBattleCharCrossActionRoundValue,

		kBattleCharNormalActionTypeValue,
		kBattleCharNormalActionEnemyValue,
		kBattleCharNormalActionLevelValue,

		kBattlePetRoundActionRoundValue,
		kBattlePetRoundActionTypeValue,
		kBattlePetRoundActionEnemyValue,
		kBattlePetRoundActionLevelValue,

		kBattlePetCrossActionTypeValue,
		kBattlePetCrossActionRoundValue,

		kBattlePetNormalActionTypeValue,
		kBattlePetNormalActionEnemyValue,
		kBattlePetNormalActionLevelValue,

		//afk->heal button
		kBattleMagicHealTargetValue,
		kBattleItemHealTargetValue,
		kBattleMagicReviveTargetValue,
		kBattleItemReviveTargetValue,

		//afk->heal combobox
		kBattleMagicHealMagicValue,
		kBattleMagicReviveMagicValue,
		kNormalMagicHealMagicValue,

		//afk->heal spinbox
		kMagicHealCharValue,
		kMagicHealPetValue,
		kMagicHealAllieValue,

		kItemHealCharValue,
		kItemHealPetValue,
		kItemHealAllieValue,

		kItemHealMpValue,

		kMagicHealCharNormalValue,
		kMagicHealPetNormalValue,
		kMagicHealAllieNormalValue,

		kItemHealCharNormalValue,
		kItemHealPetNormalValue,
		kItemHealAllieNormalValue,

		kItemHealMpNormalValue,

		//afk->walk
		kAutoWalkDelayValue,
		kAutoWalkDistanceValue,
		kAutoWalkDirectionValue,

		//afk->catch
		kBattleCatchModeValue,
		kBattleCatchTargetLevelValue,
		kBattleCatchTargetMaxHpValue,
		kBattleCatchTargetMagicHpValue,
		kBattleCatchTargetItemHpValue,
		kBattleCatchPlayerMagicValue,
		kBattleCatchPetSkillValue,

		kDropPetStrValue,
		kDropPetDefValue,
		kDropPetAgiValue,
		kDropPetHpValue,
		kDropPetAggregateValue,

		//general
		kSpeedBoostValue,


		//other->group
		kAutoFunTypeValue,

		//lockpet
		kLockPetValue,
		kLockRideValue,

		kSettingMaxValue,

		///////////////////

		kSettingMinEnable,

		kAutoLoginEnable,
		kAutoReconnectEnable,
		kLogOutEnable,
		kLogBackEnable,
		kEchoEnable,
		kHideCharacterEnable,
		kCloseEffectEnable,
		kOptimizeEnable,
		kHideWindowEnable,
		kMuteEnable,
		kAutoJoinEnable,
		kLockTimeEnable,
		kAutoFreeMemoryEnable,
		kFastWalkEnable,
		kPassWallEnable,
		kLockMoveEnable,
		kLockImageEnable,
		kAutoDropMeatEnable,
		kAutoDropEnable,
		kAutoStackEnable,
		kKNPCEnable,
		kAutoAnswerEnable,
		kForceLeaveBattleEnable,
		kAutoWalkEnable,
		kFastAutoWalkEnable,
		kFastBattleEnable,
		kAutoBattleEnable,
		kAutoCatchEnable,
		kLockAttackEnable,
		kAutoEscapeEnable,
		kLockEscapeEnable,
		kBattleTimeExtendEnable,
		kFallDownEscapeEnable,

		//switcher
		kSwitcherTeamEnable,
		kSwitcherPKEnable,
		kSwitcherCardEnable,
		kSwitcherTradeEnable,
		kSwitcherFamilyEnable,
		kSwitcherJobEnable,
		kSwitcherWorldEnable,

		//afk->battle
		kCrossActionCharEnable,
		kCrossActionPetEnable,

		//afk->heal
		kBattleMagicHealEnable,
		kBattleItemHealEnable,
		kBattleItemHealMeatPriorityEnable,
		kBattleItemHealMpEnable,
		kBattleMagicReviveEnable,
		kBattleItemReviveEnable,

		kNormalMagicHealEnable,
		kNormalItemHealEnable,
		kNormalItemHealMeatPriorityEnable,
		kNormalItemHealMpEnable,

		//afk->catch
		kBattleCatchTargetLevelEnable,
		kBattleCatchTargetMaxHpEnable,
		kBattleCatchPlayerMagicEnable,
		kBattleCatchPlayerItemEnable,
		kBattleCatchPetSkillEnable,

		kDropPetEnable,
		kDropPetStrEnable,
		kDropPetDefEnable,
		kDropPetAgiEnable,
		kDropPetHpEnable,
		kDropPetAggregateEnable,


		//lockpet
		kLockPetEnable,
		kLockRideEnable,

		kSettingMaxEnable,

		//////////////////

		kSettingMinString,

		//global
		kAutoDropItemString,
		kLockAttackString,
		kLockEscapeString,

		//afk->heal
		kBattleItemHealItemString,
		kBattleItemHealMpIteamString,
		kBattleItemReviveItemString,

		kNormalItemHealItemString,
		kNormalItemHealMpItemString,

		//afk->catch
		kBattleCatchPetNameString,
		kBattleCatchPlayerItemString,
		kDropPetNameString,


		//other->group
		kAutoFunNameString,

		//other->other2
		kGameAccountString,
		kGamePasswordString,
		kGameSecurityCodeString,



		kSettingMaxString,
	};

	enum ObjectType
	{
		OBJ_UNKNOWN,
		OBJ_ROAD,
		OBJ_UP,
		OBJ_DOWN,
		OBJ_JUMP,
		OBJ_WARP,
		OBJ_WALL,
		OBJ_ROCK,
		OBJ_ROCKEX,
		OBJ_BOUNDARY,
		OBJ_WATER,
		OBJ_EMPTY,
		OBJ_HUMAN,
		OBJ_NPC,
		OBJ_BUILDING,
		OBJ_ITEM,
		OBJ_PET,
		OBJ_GOLD,
		OBJ_GM,
	};

	//用於將枚舉直轉換為字符串，提供給json當作key
	static const QHash<UserSetting, QString> user_setting_string_hash = {
		{ kSettingNotUsed, "SettingNotUsed" },

		{ kSettingMinValue, "SettingMinValue" },

		{ kServerValue, "ServerValue" },
		{ kSubServerValue, "SubServerValue" },
		{ kPositionValue, "PositionValue" },
		{ kLockTimeValue, "LockTimeValue" },
		{ kSpeedBoostValue, "SpeedBoostValue" },

		//afk->battle combobox
		{ kBattleCharRoundActionRoundValue, "BattleCharRoundActionRoundValue" },
		{ kBattleCharRoundActionTypeValue, "BattleCharRoundActionTypeValue" },
		{ kBattleCharRoundActionEnemyValue, "BattleCharRoundActionEnemyValue" },
		{ kBattleCharRoundActionLevelValue, "BattleCharRoundActionLevelValue" },

		{ kBattleCharCrossActionTypeValue, "BattleCharCrossActionTypeValue" },
		{ kBattleCharCrossActionRoundValue, "BattleCharCrossActionRoundValue" },

		{ kBattleCharNormalActionTypeValue, "BattleCharNormalActionTypeValue" },
		{ kBattleCharNormalActionEnemyValue, "BattleCharNormalActionEnemyValue" },
		{ kBattleCharNormalActionLevelValue, "BattleCharNormalActionLevelValue" },

		{ kBattlePetRoundActionRoundValue, "BattlePetRoundActionRoundValue" },
		{ kBattlePetRoundActionTypeValue, "BattlePetRoundActionTypeValue" },
		{ kBattlePetRoundActionEnemyValue, "BattlePetRoundActionEnemyValue" },
		{ kBattlePetRoundActionLevelValue, "BattlePetRoundActionLevelValue" },

		{ kBattlePetCrossActionTypeValue, "BattlePetCrossActionTypeValue" },
		{ kBattlePetCrossActionRoundValue, "BattlePetCrossActionRoundValue" },

		{ kBattlePetNormalActionTypeValue, "BattlePetNormalActionTypeValue" },
		{ kBattlePetNormalActionEnemyValue, "BattlePetNormalActionEnemyValue" },
		{ kBattlePetNormalActionLevelValue, "BattlePetNormalActionLevelValue" },

		//afk->battle buttom
		{ kBattleCharRoundActionTargetValue, "BattleCharRoundActionTargetValue" },
		{ kBattleCharCrossActionTargetValue, "BattleCharCrossActionTargetValue" },
		{ kBattleCharNormalActionTargetValue, "BattleCharNormalActionTargetValue" },

		{ kBattlePetRoundActionTargetValue, "BattlePetRoundActionTargetValue" },
		{ kBattlePetCrossActionTargetValue, "BattlePetCrossActionTargetValue" },
		{ kBattlePetNormalActionTargetValue, "BattlePetNormalActionTargetValue" },

		//afk->heal button
		{ kBattleMagicHealTargetValue, "BattleMagicHealTargetValue" },
		{ kBattleItemHealTargetValue, "BattleItemHealTargetValue" },
		{ kBattleMagicReviveTargetValue, "BattleMagicReviveTargetValue" },
		{ kBattleItemReviveTargetValue, "BattleItemReviveTargetValue" },

		//afk->heal combobox
		{ kBattleMagicHealMagicValue, "BattleMagicHealMagicValue" },
		{ kBattleMagicReviveMagicValue, "BattleMagicReviveMagicValue" },
		{ kNormalMagicHealMagicValue, "NormalMagicHealMagicValue" },


		//afk->heal spinbox
		{ kMagicHealCharValue, "MagicHealCharValue" },
		{ kMagicHealPetValue, "MagicHealPetValue" },
		{ kMagicHealAllieValue, "MagicHealAllieValue" },

		{ kItemHealCharValue, "ItemHealCharValue" },
		{ kItemHealPetValue, "ItemHealPetValue" },
		{ kItemHealAllieValue, "ItemHealAllieValue" },

		{ kItemHealMpValue, "ItemHealMpValue" },

		{ kMagicHealCharNormalValue, "MagicHealCharNormalValue" },
		{ kMagicHealPetNormalValue, "MagicHealPetNormalValue" },
		{ kMagicHealAllieNormalValue, "MagicHealAllieNormalValue" },

		{ kItemHealCharNormalValue, "ItemHealCharNormalValue" },
		{ kItemHealPetNormalValue, "ItemHealPetNormalValue" },
		{ kItemHealAllieNormalValue, "ItemHealAllieNormalValue" },

		{ kItemHealMpNormalValue, "ItemHealMpNormalValue" },

		//afk->walk
		{ kAutoWalkDelayValue, "AutoWalkDelayValue" },
		{ kAutoWalkDistanceValue, "AutoWalkDistanceValue" },
		{ kAutoWalkDirectionValue, "AutoWalkDirectionValue" },

		//afk->catch
		{ kBattleCatchModeValue, "BattleCatchModeValue" },
		{ kBattleCatchTargetLevelValue, "BattleCatchTargetLevelValue" },
		{ kBattleCatchTargetMaxHpValue, "BattleCatchTargetMaxHpValue" },
		{ kBattleCatchTargetMagicHpValue, "BattleCatchTargetMagicHpValue" },
		{ kBattleCatchTargetItemHpValue, "BattleCatchTargetItemHpValue" },
		{ kBattleCatchPlayerMagicValue, "BattleCatchPlayerMagicValue" },
		{ kBattleCatchPetSkillValue, "BattleCatchPetSkillValue" },

		{ kDropPetStrValue, "DropPetStrValue" },
		{ kDropPetDefValue, "DropPetDefValue" },
		{ kDropPetAgiValue, "DropPetAgiValue" },
		{ kDropPetHpValue, "DropPetHpValue" },
		{ kDropPetAggregateValue, "DropPetAggregateValue" },

		//other->group
		{ kAutoFunTypeValue, "AutoFunTypeValue" },

		//lockpet
		{ kLockPetValue, "LockPetValue" },
		{ kLockRideValue, "LockRideValue" },

		{ kSettingMaxValue, "SettingMaxValue" },

		//check
		{ kSettingMinEnable, "SettingMinEnable" },
		{ kAutoLoginEnable, "AutoLoginEnable" },
		{ kAutoReconnectEnable, "AutoReconnectEnable" },
		{ kLogOutEnable, "LogOutEnable" },
		{ kLogBackEnable, "LogBackEnable" },
		{ kEchoEnable, "EchoEnable" },
		{ kHideCharacterEnable, "HideCharacterEnable" },
		{ kCloseEffectEnable, "CloseEffectEnable" },
		{ kOptimizeEnable, "OptimizeEnable" },
		{ kHideWindowEnable, "HideWindowEnable" },
		{ kMuteEnable, "MuteEnable" },
		{ kAutoJoinEnable, "AutoJoinEnable" },
		{ kLockTimeEnable, "LockTimeEnable" },
		{ kAutoFreeMemoryEnable, "AutoFreeMemoryEnable" },
		{ kFastWalkEnable, "FastWalkEnable" },
		{ kPassWallEnable, "PassWallEnable" },
		{ kLockMoveEnable, "LockMoveEnable" },
		{ kLockImageEnable, "LockImageEnable" },
		{ kAutoDropMeatEnable, "AutoDropMeatEnable" },
		{ kAutoDropEnable, "AutoDropEnable" },
		{ kAutoStackEnable, "AutoStackEnable" },
		{ kKNPCEnable, "KNPCEnable" },
		{ kAutoAnswerEnable, "AutoAnswerEnable" },
		{ kForceLeaveBattleEnable, "ForceLeaveBattleEnable" },
		{ kAutoWalkEnable, "AutoWalkEnable" },
		{ kFastAutoWalkEnable, "FastAutoWalkEnable" },
		{ kFastBattleEnable, "FastBattleEnable" },
		{ kAutoBattleEnable, "AutoBattleEnable" },
		{ kAutoCatchEnable, "AutoCatchEnable" },
		{ kLockAttackEnable, "LockAttackEnable" },
		{ kAutoEscapeEnable, "AutoEscapeEnable" },
		{ kLockEscapeEnable, "LockEscapeEnable" },
		{ kBattleTimeExtendEnable, "BattleTimeExtendEnable" },
		{ kFallDownEscapeEnable, "FallDownEscapeEnable" },
		{ kSettingMaxEnable, "SettingMaxEnable" },

		//switcher
		{ kSwitcherTeamEnable, "SwitcherTeamEnable" },
		{ kSwitcherPKEnable, "SwitcherPKEnable" },
		{ kSwitcherCardEnable, "SwitcherCardEnable" },
		{ kSwitcherTradeEnable, "SwitcherTradeEnable" },
		{ kSwitcherFamilyEnable, "SwitcherFamilyEnable" },
		{ kSwitcherJobEnable, "SwitcherJobEnable" },
		{ kSwitcherWorldEnable, "SwitcherWorldEnable" },
		//afk->battle
		{ kCrossActionCharEnable, "CrossActionCharEnable" },
		{ kCrossActionPetEnable, "CrossActionPetEnable" },

		//afk->heal
		{ kBattleMagicHealEnable, "BattleMagicHealEnable" },
		{ kBattleItemHealEnable, "BattleItemHealEnable" },
		{ kBattleItemHealMeatPriorityEnable, "BattleItemHealMeatPriorityEnable" },
		{ kBattleItemHealMpEnable, "BattleItemHealMpEnable" },
		{ kBattleMagicReviveEnable, "BattleMagicReviveEnable" },
		{ kBattleItemReviveEnable, "BattleItemReviveEnable" },

		{ kNormalMagicHealEnable, "NormalMagicHealEnable" },
		{ kNormalItemHealEnable, "NormalItemHealEnable" },
		{ kNormalItemHealMeatPriorityEnable, "NormalItemHealMeatPriorityEnable" },
		{ kNormalItemHealMpEnable, "NormalItemHealMpEnable" },

		//afk->catch
		{ kBattleCatchTargetLevelEnable, "BattleCatchTargetLevelEnable" },
		{ kBattleCatchTargetMaxHpEnable, "BattleCatchTargetMaxHpEnable" },
		{ kBattleCatchPlayerMagicEnable, "BattleCatchPlayerMagicEnable" },
		{ kBattleCatchPlayerItemEnable, "BattleCatchPlayerItemEnable" },
		{ kBattleCatchPetSkillEnable, "BattleCatchPetSkillEnable" },

		{ kDropPetEnable, "DropPetEnable" },
		{ kDropPetStrEnable, "DropPetStrEnable" },
		{ kDropPetDefEnable, "DropPetDefEnable" },
		{ kDropPetAgiEnable, "DropPetAgiEnable" },
		{ kDropPetHpEnable, "DropPetHpEnable" },
		{ kDropPetAggregateEnable, "DropPetAggregateEnable" },

		//lockpet
		{ kLockPetEnable, "LockPetEnable" },
		{ kLockRideEnable, "LockRideEnable" },


		//string
		{ kSettingMinString, "SettingMinString" },

		{ kAutoDropItemString, "AutoDropItemString" },
		{ kLockAttackString, "LockAttackString" },
		{ kLockEscapeString, "LockEscapeString" },

		//afk->heal
		{ kBattleItemHealItemString, "BattleItemHealItemString" },
		{ kBattleItemHealMpIteamString, "BattleItemHealMpIteamString" },
		{ kBattleItemReviveItemString, "BattleItemReviveItemString" },
		{ kNormalItemHealItemString, "NormalItemHealItemString" },
		{ kNormalItemHealMpItemString, "NormalItemHealMpItemString" },

		//afk->catch
		{ kBattleCatchPetNameString, "BattleCatchPetNameString" },
		{ kBattleCatchPlayerItemString, "BattleCatchPlayerItemString" },
		{ kDropPetNameString, "DropPetNameString" },


		//other->group
		{ kAutoFunNameString, "AutoFunNameString" },

		{ kGameAccountString, "GameAccountString" },
		{ kGamePasswordString, "GamePasswordString" },
		{ kGameSecurityCodeString, "GameSecurityCodeString" },

		{ kSettingMaxString, "SettingMaxString" }
	};

	//8方位坐標補正
	const static QVector<QPoint> fix_point = {
		{0, -1},  //北0
		{1, -1},  //東北1
		{1, 0},	  //東2
		{1, 1},	  //東南3
		{0, 1},	  //南4
		{-1, 1},  //西南0
		{-1, 0},  //西1
		{-1, -1}, //西北2
	};

	static inline const int __vectorcall percent(int value, int total)
	{
		double d = std::floor(static_cast<double>(value) * 100.0 / static_cast<double>(total));
		if ((value > 0) && (d < 1.0))
			return 1;
		else
			return static_cast<int>(d);
	}

	static Q_REQUIRED_RESULT QString toUnicode(const char* str, bool ext = true)
	{
		static QTextCodec* codec = QTextCodec::codecForName("GB2312");

		QString qstr = codec->toUnicode(str);
		std::wstring wstr = qstr.toStdWString();
		UINT ACP = ::GetACP();
		if (950 == ACP && ext)
		{
			// 繁體系統地圖名要轉繁體否則遊戲視窗標題會亂碼
			int size = lstrlenW(wstr.c_str());
			QScopedArrayPointer <wchar_t> wbuf(new wchar_t[size + 1]());
			LCMapStringEx(LOCALE_NAME_SYSTEM_DEFAULT, LCMAP_TRADITIONAL_CHINESE, wstr.c_str(), size, wbuf.data(), size, NULL, NULL, NULL);
			qstr = QString::fromWCharArray(wbuf.data());
		}

		return qstr;
	}

	static Q_REQUIRED_RESULT std::string fromUnicode(const QString& str)
	{
		static QTextCodec* codec = QTextCodec::codecForName("GB2312");
		QString qstr = str;
		std::wstring wstr = qstr.toStdWString();
		UINT ACP = ::GetACP();
		if (950 == ACP)
		{
			// 繁體系統要轉回簡體體否則遊戲視窗會亂碼
			int size = lstrlenW(wstr.c_str());
			QScopedArrayPointer <wchar_t> wbuf(new wchar_t[size + 1]());
			LCMapStringEx(LOCALE_NAME_SYSTEM_DEFAULT, LCMAP_SIMPLIFIED_CHINESE, wstr.c_str(), size, wbuf.data(), size, NULL, NULL, NULL);
			wstr = wbuf.data();
			qstr = QString::fromWCharArray(wstr.c_str());
		}

		QByteArray bytes = codec->fromUnicode(qstr);

		std::string s = bytes.data();

		return s;
	}

	static void setTab(QTabWidget* pTab)
	{
		QString styleSheet = R"(
			QTabWidget{
				background-color:rgb(249, 249, 249);
				background-clip:rgb(31, 31, 31);
				background-origin:rgb(31, 31, 31);
			}

			QTabWidget::pane{
				top:0px;
				border-top:2px solid #7160E8;
				border-bottom:1px solid rgb(61,61,61);
				border-right:1px solid rgb(61,61,61);
				border-left:1px solid rgb(61,61,61);
			}

			QTabWidget::tab-bar
			{
				alignment: left;
				top:0px;
				left:5px;
				border:none;
			}

			QTabBar::tab:first{
				/*color:rgb(178,178,178);*/
				background:rgb(249, 249, 249);

				padding-left:0px;
				padding-right:0px;
				/*width:100px;*/
				height:24px;
				margin-left:0px;
				margin-right:0px;

				border-top:2px solid rgb(31, 31, 31);
			}

			QTabBar::tab:middle{
				/*color:rgb(178,178,178);*/
				background:rgb(249, 249, 249);
				padding-left:0px;
				padding-right:0px;
				/*width:100px;*/
				height:24px;
				margin-left:0px;
				margin-right:0px;

				border-top:2px solid rgb(31, 31, 31);

			}

			QTabBar::tab:last{
				/*color:rgb(178,178,178);*/
				background:rgb(249, 249, 249);

				padding-left:0px;
				padding-right:0px;
				/*width:100px;*/
				height:24px;
				margin-left:0px;
				margin-right:0px;

				border-top:2px solid rgb(31, 31, 31);
			}

			QTabBar::tab:selected{
				background:rgb(50, 130, 246);
				border-top:2px solid rgb(50, 130, 246);

			}

			QTabBar::tab:hover{
				background:rgb(50, 130, 246);
				border-top:2px solid rgb(50, 130, 246);
				padding-left:0px;
				padding-right:0px;
				/*width:100px;*/
				height:24px;
				margin-left:0px;
				margin-right:0px;
			}
		)";
		pTab->setStyleSheet(styleSheet);

		QTabBar* pTabBar = pTab->tabBar();

		pTabBar->setDocumentMode(true);
		pTabBar->setExpanding(true);
	}

	static bool createFileDialog(const QString& name, QString* retstring, QWidget* parent)
	{
		QFileDialog dialog(parent);
		dialog.setModal(true);
		dialog.setFileMode(QFileDialog::ExistingFile);
		dialog.setViewMode(QFileDialog::Detail);
		dialog.setOption(QFileDialog::ReadOnly, true);
		dialog.setOption(QFileDialog::DontResolveSymlinks, true);
		dialog.setOption(QFileDialog::DontConfirmOverwrite, true);
		dialog.setAcceptMode(QFileDialog::AcceptOpen);


		//check suffix
		if (!name.isEmpty())
		{
			QStringList filters;
			filters << name;
			dialog.setNameFilters(filters);
		}

		//directory
		//自身目錄往上一層
		QString directory = QCoreApplication::applicationDirPath();
		directory = QDir::toNativeSeparators(directory);
		directory = QDir::cleanPath(directory + QDir::separator() + "..");
		dialog.setDirectory(directory);

		if (dialog.exec() == QDialog::Accepted)
		{
			QStringList fileNames = dialog.selectedFiles();
			if (fileNames.size() > 0)
			{
				QString fileName = fileNames.at(0);
				if (retstring)
					*retstring = fileName;

				return true;
			}
		}
		return false;
	}

	static bool customStringCompare(const QString& str1, const QString& str2)
	{
		//中文locale
		QLocale locale;
		QCollator collator(locale);

		// 按照數字進行排序
		if (str1.contains(QRegularExpression("\\d")) && str2.contains(QRegularExpression("\\d")))
		{
			return str1.toInt() < str2.toInt();
		}

		// 按照字母進行排序
		if (str1.contains(QRegularExpression("[A-Za-z]")) && str2.contains(QRegularExpression("[A-Za-z]")))
		{
			return str1.localeAwareCompare(str2) < 0;
		}

		// 按照中文進行排序
		return collator.compare(str1, str2) < 0;
	}

	QFileInfoList loadAllFileLists(TreeWidgetItem* root, const QString& path, QStringList* list = nullptr);

	bool enumAllFiles(const QString dir, const QString suffix, QVector<QPair<QString, QString>>* result);

	template<typename T>
	QList<T*> findWidgets(QWidget* widget)
	{
		QList<T*> widgets;

		if (widget)
		{
			// 檢查 widget 是否為指定類型的控件，如果是，則添加到結果列表中
			T* typedWidget = qobject_cast<T*>(widget);
			if (typedWidget)
			{
				widgets.append(typedWidget);
			}

			// 遍歷 widget 的子控件並遞歸查找
			QList<QWidget*> childWidgets = widget->findChildren<QWidget*>();
			for (QWidget* childWidget : childWidgets)
			{
				// 遞歸調用 findWidgets 函數並將結果合並到 widgets 列表中
				QList<T*> childResult = findWidgets<T>(childWidget);
				widgets += childResult;
			}
		}

		return widgets;
	}

	static QString formatMilliseconds(qint64 milliseconds)
	{
		qint64 totalSeconds = milliseconds / 1000ll;
		qint64 days = totalSeconds / (24ll * 60ll * 60ll);
		qint64 hours = (totalSeconds % (24ll * 60ll * 60ll)) / (60ll * 60ll);
		qint64 minutes = (totalSeconds % (60ll * 60ll)) / 60ll;
		qint64 seconds = totalSeconds % 60ll;
		qint64 remainingMilliseconds = milliseconds % 1000ll;

		return QString(QObject::tr("%1 day %2 hour %3 min %4 sec %5 msec"))
			.arg(days).arg(hours).arg(minutes).arg(seconds).arg(remainingMilliseconds);
	}

	//基於Qt QHash 的線程安全Hash容器
	template <typename K, typename V>
	class SafeHash
	{
	public:
		SafeHash()
		{
		}

		inline SafeHash(std::initializer_list<std::pair<K, V> > list)
		{
			QWriteLocker locker(&lock);
			hash = list;
		}

		//copy
		SafeHash(const QHash<K, V>& other)
		{
			QWriteLocker locker(&lock);
			hash = other;
		}

		//copy assign
		SafeHash(const SafeHash<K, V>& other)
		{
			QWriteLocker locker(&lock);
			hash = other.hash;
		}

		//move
		SafeHash(QHash<K, V>&& other)
		{
			QWriteLocker locker(&lock);
			hash = std::move(other);
		}

		//move assign
		SafeHash(SafeHash<K, V>&& other) noexcept
		{
			QWriteLocker locker(&lock);
			hash = std::move(other.hash);
		}

		SafeHash& operator=(const SafeHash& other)
		{
			QWriteLocker locker(&lock);
			if (this != &other)
			{
				hash = other.hash;
			}
			return *this;
		}

		//operator=
		SafeHash& operator=(const QHash <K, V>& other)
		{
			QWriteLocker locker(&lock);
			if (this != &other)
			{
				hash = other;
			}
			return *this;
		}

		inline void insert(const K& key, const V& value)
		{
			QWriteLocker locker(&lock);
			hash.insert(key, value);
		}
		inline void remove(const K& key)
		{
			QWriteLocker locker(&lock);
			hash.remove(key);
		}
		inline bool contains(const K& key) const
		{
			QReadLocker locker(&lock);
			return hash.contains(key);
		}
		inline V value(const K& key) const
		{
			QReadLocker locker(&lock);
			return hash.value(key);
		}
		inline V value(const K& key, const V& defaultValue) const
		{
			QReadLocker locker(&lock);
			return hash.value(key, defaultValue);
		}

		inline int size() const
		{
			QReadLocker locker(&lock);
			return hash.size();
		}
		inline QList <K> keys() const
		{
			QReadLocker locker(&lock);
			return hash.keys();
		}

		inline QList <V> values() const
		{
			QReadLocker locker(&lock);
			return hash.values();
		}
		//take
		inline V take(const K& key)
		{
			QWriteLocker locker(&lock);
			return hash.take(key);
		}

		inline void clear()
		{
			QWriteLocker locker(&lock);
			hash.clear();
		}

		//=
		inline V& operator[](const K& key)
		{
			QWriteLocker locker(&lock);
			return hash[key];
		}


		inline K key(const V& value) const
		{
			QReadLocker locker(&lock);
			return hash.key(value);
		}

		inline K key(const V& value, const K& defaultValue) const
		{
			QReadLocker locker(&lock);
			return hash.key(value, defaultValue);
		}

		inline typename QHash<K, V>::iterator begin()
		{
			QReadLocker locker(&lock);
			return hash.begin();
		}

		inline typename QHash<K, V>::const_iterator begin() const
		{
			QReadLocker locker(&lock);
			return hash.begin();
		}

		inline typename QHash<K, V>::iterator end()
		{
			QReadLocker locker(&lock);
			return hash.end();
		}

		inline typename QHash<K, V>::const_iterator end() const
		{
			QReadLocker locker(&lock);
			return hash.end();
		}

	private:
		QHash<K, V> hash;
		mutable QReadWriteLock lock;
	};

	template <typename V>
	class SafeQueue
	{
	public:
		explicit SafeQueue(int maxSize = 20)
			: maxSize_(maxSize)
		{
		}
		virtual ~SafeQueue() = default;

		void clear()
		{
			QWriteLocker locker(&lock_);
			queue_.clear();
		}

		QVector<V> values() const
		{
			QReadLocker locker(&lock_);
			return queue_.toVector();

		}

		int size() const
		{
			QReadLocker locker(&lock_);
			return queue_.size();
		}

		bool isEmpty() const
		{
			QReadLocker locker(&lock_);
			return queue_.isEmpty();
		}

		void enqueue(const V& value)
		{
			QWriteLocker locker(&lock_);
			if (queue_.size() >= maxSize_)
			{
				queue_.dequeue();
			}
			queue_.enqueue(value);
		}

		bool dequeue(V* pvalue)
		{
			QWriteLocker locker(&lock_);
			if (queue_.isEmpty())
			{
				return false;
			}
			if (pvalue)
				*pvalue = queue_.dequeue();
			return true;
		}

		V front() const
		{
			QReadLocker locker(&lock_);
			return queue_.head();
		}

		void setMaxSize(int maxSize)
		{
			QWriteLocker locker(&lock_);
			queue_.setMaxSize(maxSize);
		}

	private:
		QQueue<V> queue_;
		int maxSize_;
		mutable QReadWriteLock lock_;
	};;

	struct MapData
	{
		int floor;
		QString name;
		int x;
		int y;
	};

	static inline QString getPointFileName()
	{
		UINT acp = ::GetACP();
		if (acp == 950)
		{
			return (QCoreApplication::applicationDirPath() + QString(u8"/map/point_zh_TW.json"));
		}
		else if (acp == 936)
		{
			return (QCoreApplication::applicationDirPath() + QString(u8"/map/point_zh_CN.json"));
		}
		else
		{
			return (QCoreApplication::applicationDirPath() + QString(u8"/map/point.json"));
		}
	}

	//Json配置讀寫
	class Config
	{
	public:
		Config(const QString& fileName);
		~Config();

		bool open(const QString& fileName);
		void sync();

		void removeSec(const QString sec);

		void write(const QString& key, const QVariant& value);
		void write(const QString& sec, const QString& key, const QVariant& value);
		void write(const QString& sec, const QString& key, const QString& sub, const QVariant& value);

		void WriteHash(const QString& sec, const QString& key, QMap<QString, QPair<bool, QString>>& hash);
		QMap<QString, QPair<bool, QString>> EnumString(const QString& sec, const QString& key) const;

		QString readString(const QString& sec, const QString& key, const QString& sub) const;
		QString readString(const QString& sec, const QString& key) const;
		QString readString(const QString& key) const;
		bool readBool(const QString& key) const;
		bool readBool(const QString& sec, const QString& key) const;
		bool readBool(const QString& sec, const QString& key, const QString& sub) const;
		int readInt(const QString& key) const;
		int readInt(const QString& sec, const QString& key) const;
		int readInt(const QString& sec, const QString& key, const QString& sub) const;
		qreal readDouble(const QString& sec, const QString& key, qreal retnot) const;
		int readInt(const QString& sec, const QString& key, int retnot) const;

		void writeIntArray(const QString& sec, const QString& key, const QList<int>& values);
		void writeIntArray(const QString& sec, const QString& key, const QString& sub, const QList<int>& values);
		QList<int> readIntArray(const QString& sec, const QString& key, const QString& sub) const;

		void util::Config::writeMapData(const QString& sec, const util::MapData& data);

		QList<util::MapData> util::Config::readMapData(const QString& key) const;

	private:
		QLockFile lock_;

		QJsonDocument document_;

		QString fileName_ = "\0";

		QVariantMap cache_ = {};

		bool hasChanged_ = false;
	};

	//簡易字符串加解密 主要用於將一些二進制數據轉換為可視字符串方便保存json
	class Crypt
	{
	public:
		Crypt()
			:crypto_(new SimpleCrypt(0x7f5f6c3ee5bai64))
		{

		}

		virtual ~Crypt() = default;

		bool isBase64(const QString data) const
		{
			//正則式 + 長度判斷 + 字符集判斷 + 字符串判斷 是否為 Base64
			QRegularExpression rx("^[A-Za-z0-9+/]+={0,2}$");
			return rx.match(data).hasMatch() && data.length() % 4 == 0 && data.contains(QRegularExpression("[^A-Za-z0-9+/=]")) == false;
		}

		/* 加密數據 */
		QString encryptToString(const QString& plaintext)
		{
			QString  ciphertext = crypto_->encryptToString(plaintext);
			//QString str = QEncode(ciphertext);
			return ciphertext;

		}

		QString encryptFromByteArray(QByteArray plaintext)
		{
			QString  ciphertext = crypto_->encryptToString(plaintext);
			//QString str = QEncode(ciphertext);
			return ciphertext;
		}

		/* 解密數據 */
		QString decryptToString(const QString& plaintext)
		{
			//QString str = QDecode(plaintext);
			QString ciphertext = crypto_->decryptToString(plaintext);
			return ciphertext;
		}

		QString decryptFromByteArray(QByteArray plaintext)
		{
			//QString str = QDecode(plaintext);
			QString ciphertext = crypto_->decryptToString(plaintext);
			return ciphertext;
		}

		QByteArray decryptToByteArray(const QString& plaintext)
		{
			//QString str = QDecode(plaintext);
			QByteArray ciphertext = crypto_->decryptToByteArray(plaintext);
			return ciphertext;
		}

	private:
		QScopedPointer<SimpleCrypt> crypto_;

	};

	//介面排版或配置管理 主用於保存窗口狀態
	class FormSettingManager
	{
	public:
		explicit FormSettingManager(QWidget* widget) { widget_ = widget; }
		explicit FormSettingManager(QMainWindow* widget) { mainwindow_ = widget; }
		void loadSettings();
		void saveSettings();

	private:
		QWidget* widget_ = nullptr;
		QMainWindow* mainwindow_ = nullptr;
	};

	//遠程虛擬內存申請、釋放、或申請+寫入字符串(ANSI | UNICODE)
	class VirtualMemory
	{
	public:
		enum VirtualEncode
		{
			kAnsi,
			kUnicode,
		};
		VirtualMemory() = default;

		explicit VirtualMemory(HANDLE h, int size, bool autoclear)
			: autoclear(autoclear)
			, hProcess(h)
		{

			lpAddress = mem::virtualAlloc(h, size);
		}

		explicit VirtualMemory(HANDLE h, const QString& str, VirtualEncode use_unicode, bool autoclear)
			: autoclear(autoclear)
		{

			lpAddress = (use_unicode) == VirtualMemory::kUnicode ? (mem::virtualAllocW(h, str)) : (mem::virtualAllocA(h, str));
			hProcess = h;
		}

		operator int() const
		{
			return this->lpAddress;
		}

		VirtualMemory& operator=(int other)
		{
			this->lpAddress = other;
			return *this;
		}

		VirtualMemory* operator&()
		{
			return this;
		}

		// copy constructor
		VirtualMemory(const VirtualMemory& other)
			: autoclear(other.autoclear)
			, lpAddress(other.lpAddress)
		{
		}
		//copy assignment
		VirtualMemory& operator=(const VirtualMemory& other)
		{
			this->lpAddress = other.lpAddress;
			this->autoclear = other.autoclear;
			this->hProcess = other.hProcess;
			return *this;
		}
		// move constructor
		VirtualMemory(VirtualMemory&& other) noexcept
			: autoclear(other.autoclear)
			, lpAddress(other.lpAddress)
			, hProcess(other.hProcess)

		{
			other.lpAddress = 0;
		}
		// move assignment
		VirtualMemory& operator=(VirtualMemory&& other) noexcept
		{
			this->lpAddress = other.lpAddress;
			this->autoclear = other.autoclear;
			this->hProcess = other.hProcess;
			other.lpAddress = 0;
			return *this;
		}

		friend constexpr inline bool operator==(const VirtualMemory& p1, const VirtualMemory& p2)
		{
			return p1.lpAddress == p2.lpAddress;
		}
		friend constexpr inline bool operator!=(const VirtualMemory& p1, const VirtualMemory& p2)
		{
			return p1.lpAddress != p2.lpAddress;
		}

		virtual ~VirtualMemory()
		{
			do
			{
				if ((autoclear) && (this->lpAddress) && (hProcess))
				{

					mem::writeInt(hProcess, this->lpAddress, 0, 2);
					mem::virtualFree(hProcess, this->lpAddress);
					this->lpAddress = NULL;
				}
			} while (false);
		}

		Q_REQUIRED_RESULT inline bool __vectorcall isNull() const
		{
			return !lpAddress;
		}
		inline void __vectorcall clear()
		{

			if ((this->lpAddress))
			{
				mem::virtualFree(hProcess, (this->lpAddress));
				this->lpAddress = NULL;
			}
		}

		Q_REQUIRED_RESULT inline bool __vectorcall isValid()const
		{
			return (this->lpAddress) != NULL;
		}

	private:
		bool autoclear = false;
		int lpAddress = NULL;
		HANDLE hProcess = NULL;
	};

	//智能文件句柄類
	class QScopedFile : public QFile
	{
	public:
		QScopedFile(const QString& filename, OpenMode ioFlags)
			: QFile(filename)
		{
			QFile::open(ioFlags);
		}

		~QScopedFile()
		{
			if (m_maps.size())
			{
				for (uchar* map : m_maps)
				{
					QFile::unmap(map);
				}
				m_maps.clear();
			}
			if (QFile::isOpen())
				QFile::close();
		}

		template <typename T>
		bool __fastcall  mmap(T*& p, int offset, int size, QFile::MemoryMapFlags flags = QFileDevice::MapPrivateOption)//QFile::NoOptions
		{
			uchar* uc = QFile::map(offset, size, flags);
			if (uc)
			{
				m_maps.insert(uc);
				p = reinterpret_cast<T*>(uc);
			}
			return uc != nullptr;
		}

		QFile& __fastcall  file() { return *this; }

	private:
		QSet<uchar*> m_maps;
	};

	//用於掛機訊息紀錄
	typedef struct tagAfkRecorder
	{
		int levelrecord = 0;
		int leveldifference = 0;
		int exprecord = 0;
		int expdifference = 0;
		int goldearn = 0;
		int deadthcount = 0;
	}AfkRecorder;
#pragma warning(push)
#pragma warning(disable:304)
	Q_DECLARE_METATYPE(VirtualMemory);
	Q_DECLARE_TYPEINFO(VirtualMemory, Q_MOVABLE_TYPE);
#pragma warning(pop)

	static const QRegularExpression rexOR(R"(\s*\|\s*)");
	static const QRegularExpression rexComma(R"(\s*,\s*)");
}

