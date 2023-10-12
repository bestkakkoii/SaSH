module;

#include <QRegularExpression>
#include <QVector>
#include <QHash>
#include <QPoint>

export module Global;

export constexpr __int64 SASH_VERSION_MAJOR = 1;
export constexpr __int64 SASH_VERSION_MINOR = 0;
export constexpr __int64 SASH_VERSION_PATCH = 0;
export constexpr __int64 SASH_MAX_THREAD = 65535;
export constexpr const char* SASH_INJECT_DLLNAME = "sadll";
export constexpr const char* SASH_SUPPORT_GAMENAME = "sa_8001.exe";
export constexpr const char* SASH_DEFAULT_CODEPAGE = "UTF-8";
export constexpr const char* SASH_DEFAULT_GAME_CODEPAGE = "GBK";//gb18030/gb2312
export constexpr const char* SASH_SCRIPT_DEFAULT_SUFFIX = ".txt";
export constexpr const char* SASH_LUA_DEFAULT_SUFFIX = ".lua";
export constexpr const char* SASH_PRIVATE_SCRIPT_DEFAULT_SUFFIX = ".sac";

export const QRegularExpression rexSpace(R"([ ]+)");
export const QRegularExpression rexOR(R"(\s*\|\s*)");
export const QRegularExpression rexComma(R"(\s*,\s*)");
export const QRegularExpression rexSemicolon(R"(\s*;\s*)");
export const QRegularExpression rexColon(R"(\s*:\s*)");
export const QRegularExpression rexDec(R"(\s*-\s*)");
//(\('[\w\p{Han}]+'\))
export const QRegularExpression rexQuoteWithParentheses(R"(\('?([\w\W\S\s\p{Han}]+)'?\))");
//\[(\d+)\]
export const QRegularExpression rexSquareBrackets(R"(\[(\d+)\])");
//([\w\p{Han}]+)\[(\d+)\]
export typedef struct break_marker_s
{
	__int64 line = 0;
	__int64 count = 0;
	__int64 maker = 0;
	QString content = "\0";
} break_marker_t;

export typedef struct tagMapData
{
	__int64 floor = 0;
	__int64 x = 0;
	__int64 y = 0;
	QString name = "";
}MapData;


//用於掛機訊息紀錄
export typedef struct tagAfkRecorder
{
	__int64 levelrecord = 0;
	__int64 leveldifference = 0;
	__int64 exprecord = 0;
	__int64 expdifference = 0;
	__int64 goldearn = 0;
	__int64 deadthcount = 0;
	__int64 reprecord = 0;
	__int64 repearn = 0;
	bool deadthcountflag = false;
}AfkRecorder;

export typedef enum tagREMOVE_THREAD_REASON
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

export enum UserStatus
{
	kLabelStatusNotUsed = 0,//未知
	kLabelStatusNotOpen,//未開啟石器
	kLabelStatusOpening,//開啟石器中
	kLabelStatusOpened,//已開啟石器
	kLabelStatusLogining,//登入
	kLabelStatusSignning,//簽入中
	kLabelStatusSelectServer,//選擇伺服器
	kLabelStatusSelectSubServer,//選擇分伺服器
	kLabelStatusGettingCharList,//取得人物中
	kLabelStatusSelectPosition,//選擇人物中
	kLabelStatusLoginSuccess,//登入成功
	kLabelStatusInNormal,//平時
	kLabelStatusInBattle,//戰鬥中
	kLabelStatusBusy,//忙碌中
	kLabelStatusTimeout,//連線逾時
	kLabelStatusLoginFailed,//登入失敗
	kLabelNoUserNameOrPassword,//無賬號密碼
	kLabelStatusDisconnected,//斷線
	kLabelStatusConnecting,//連線中
	kLabelStatusNoUsernameAndPassword,//無賬號密碼
	kLabelStatusNoUsername,//無賬號
	kLabelStatusNoPassword,//無密碼
};

export enum UserData
{
	kUserDataNotUsed = 0x0,
	kUserItemNames = 0x1,
	kUserEnemyNames = 0x2,
	kUserPetNames = 0x4,
};

export enum ComboBoxUpdateType
{
	kComboBoxCharAction,
	kComboBoxPetAction,
	kComboBoxItem,
};

export enum SelectTarget
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
	kSelectTeammate4Pet = 0x20000,// 隊4寵 (Teammate 4's pet)
	kSelectEnemy1 = 0x40000,      // 敵1 (Enemy 1)
	kSelectEnemy2 = 0x80000,      // 敵1寵 (Enemy2)
	kSelectEnemy3 = 0x100000,     // 敵3 (Enemy 3)
	kSelectEnemy4 = 0x200000,     // 敵4 (Enemy 4)
	kSelectEnemy5 = 0x400000,     // 敵5 (Enemy 5)
	kSelectEnemy6 = 0x800000,     // 敵6 (Enemy 6)
	kSelectEnemy7 = 0x1000000,    // 敵7 (Enemy 7)
	kSelectEnemy8 = 0x2000000,    // 敵8 (Enemy 8)
	kSelectEnemy9 = 0x4000000,    // 敵9 (Enemy 9)
	kSelectEnemy10 = 0x8000000,   // 敵10 (Enemy 10)
};

export enum UnLoginStatus
{
	kStatusUnknown,
	kStatusDisappear,
	kStatusInputUser,
	kStatusSelectServer,
	kStatusSelectSubServer,
	kStatusSelectCharacter,
	kStatusLogined,
	kStatusDisconnect,
	kStatusConnecting,
	kStatusTimeout,
	kNoUserNameOrPassword,
	kStatusBusy,
	kStatusLoginFailed,
};

export enum CraftType
{
	kCraftFood,
	kCraftItem
};

export enum UserSetting
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

	kBattlePetHealActionTypeValue,
	kBattlePetPurgActionTypeValue,
	kBattleCharPurgActionTypeValue,

	//afk->heal button
	kBattleMagicHealTargetValue,
	kBattleItemHealTargetValue,
	kBattleMagicReviveTargetValue,
	kBattleItemReviveTargetValue,
	kBattlePetHealTargetValue,
	kBattlePetPurgTargetValue,
	kBattleCharPurgTargetValue,

	//afk->heal combobox
	kBattleMagicHealMagicValue,
	kBattleMagicReviveMagicValue,
	kNormalMagicHealMagicValue,

	//afk->heal spinbox
	kBattleMagicHealCharValue,
	kBattleMagicHealPetValue,
	kBattleMagicHealAllieValue,

	kBattleItemHealCharValue,
	kBattleItemHealPetValue,
	kBattleItemHealAllieValue,

	kBattleItemHealMpValue,
	kBattleSkillMpValue,

	kNormalMagicHealCharValue,
	kNormalMagicHealPetValue,
	kNormalMagicHealAllieValue,

	kNormalItemHealCharValue,
	kNormalItemHealPetValue,
	kNormalItemHealAllieValue,

	kNormalItemHealMpValue,

	kBattlePetHealCharValue,
	kBattlePetHealPetValue,
	kBattlePetHealAllieValue,
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
	kBattleCatchCharMagicValue,
	kBattleCatchPetSkillValue,


	//afk->battle delay
	kBattleActionDelayValue,
	kBattleResendDelayValue,


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

	//script
	kScriptSpeedValue,

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
	kAutoEatBeanEnable,
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
	kShowExpEnable,
	kWindowDockEnable,
	kBattleAutoSwitchEnable,
	kBattleAutoEOEnable,

	//switcher
	kSwitcherTeamEnable,
	kSwitcherPKEnable,
	kSwitcherCardEnable,
	kSwitcherTradeEnable,
	kSwitcherGroupEnable,
	kSwitcherFamilyEnable,
	kSwitcherJobEnable,
	kSwitcherWorldEnable,

	//afk->battle
	kBattleCrossActionCharEnable,
	kBattleCrossActionPetEnable,

	//afk->heal
	kBattleMagicHealEnable,
	kBattleItemHealEnable,
	kBattleItemHealMeatPriorityEnable,
	kBattleItemHealMpEnable,
	kBattleMagicReviveEnable,
	kBattleItemReviveEnable,
	kBattleSkillMpEnable,
	kBattlePetHealEnable,
	kBattlePetPurgEnable,
	kBattleCharPurgEnable,

	kNormalMagicHealEnable,
	kNormalItemHealEnable,
	kNormalItemHealMeatPriorityEnable,
	kNormalItemHealMpEnable,

	//afk->catch
	kBattleCatchTargetLevelEnable,
	kBattleCatchTargetMaxHpEnable,
	kBattleCatchCharMagicEnable,
	kBattleCatchCharItemEnable,
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

	kLockPetScheduleEnable,

	kBattleNoEscapeWhileLockPetEnable,

	kAutoAbilityEnable,

	kSettingMaxEnable,

	//////////////////

	kSettingMinString,

	//global
	kAutoDropItemString,
	kLockAttackString,
	kLockEscapeString,

	//afk->heal
	kBattleItemHealItemString,
	kBattleItemHealMpItemString,
	kBattleItemReviveItemString,

	kNormalItemHealItemString,
	kNormalItemHealMpItemString,

	//afk->catch
	kBattleCatchPetNameString,
	kBattleCatchCharItemString,
	kDropPetNameString,


	//other->group
	kAutoFunNameString,

	//other->lockpet
	kLockPetScheduleString,

	//other->other2
	kGameAccountString,
	kGamePasswordString,
	kGameSecurityCodeString,

	kMailWhiteListString,

	kEOCommandString,

	kBattleLogicsString,

	kAutoAbilityString,

	kSettingMaxString,

	kScriptDebugModeEnable,
};

export enum ObjectType
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
	OBJ_MAX,
};

//用於將枚舉直轉換為字符串，提供給json當作key
export const QHash<UserSetting, QString> user_setting_string_hash = {
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

	{ kBattlePetHealTargetValue, "BattlePetHealTargetValue" },
	{ kBattlePetPurgTargetValue, "BattlePetPurgTargetValue" },
	{ kBattleCharPurgTargetValue, "BattleCharPurgTargetValue" },

	{ kBattlePetHealCharValue, "BattlePetHealCharValue" },
	{ kBattlePetHealPetValue, "BattlePetHealPetValue" },
	{ kBattlePetHealAllieValue, "BattlePetHealAllieValue" },

	{ kBattlePetHealActionTypeValue, "BattlePetHealActionTypeValue" },
	{ kBattlePetPurgActionTypeValue, "BattlePetPurgActionTypeValue" },
	{ kBattleCharPurgActionTypeValue, "BattleCharPurgActionTypeValue" },

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
	{ kBattleSkillMpValue, "BattleSkillMpValue" },

	//afk->heal spinbox
	{ kBattleMagicHealCharValue, "MagicHealCharValue" },
	{ kBattleMagicHealPetValue, "MagicHealPetValue" },
	{ kBattleMagicHealAllieValue, "MagicHealAllieValue" },

	{ kBattleItemHealCharValue, "ItemHealCharValue" },
	{ kBattleItemHealPetValue, "ItemHealPetValue" },
	{ kBattleItemHealAllieValue, "ItemHealAllieValue" },

	{ kBattleItemHealMpValue, "ItemHealMpValue" },

	{ kNormalMagicHealCharValue, "MagicHealCharNormalValue" },
	{ kNormalMagicHealPetValue, "MagicHealPetNormalValue" },
	{ kNormalMagicHealAllieValue, "MagicHealAllieNormalValue" },

	{ kNormalItemHealCharValue, "ItemHealCharNormalValue" },
	{ kNormalItemHealPetValue, "ItemHealPetNormalValue" },
	{ kNormalItemHealAllieValue, "ItemHealAllieNormalValue" },

	{ kNormalItemHealMpValue, "ItemHealMpNormalValue" },

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
	{ kBattleCatchCharMagicValue, "BattleCatchCharMagicValue" },
	{ kBattleCatchPetSkillValue, "BattleCatchPetSkillValue" },

	//afk->battle delay
	{ kBattleActionDelayValue, "BattleActionDelayValue" },
	{ kBattleResendDelayValue, "BattleResendDelayValue" },

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
	{ kLockPetScheduleEnable, "LockPetScheduleEnable" },

	//script
	{ kScriptSpeedValue, "ScriptSpeedValue" },

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
	{ kAutoEatBeanEnable, "AutoEatBeanEnable" },
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
	{ kShowExpEnable, "ShowExpEnable" },
	{ kWindowDockEnable, "WindowDockEnable" },
	{ kBattleAutoSwitchEnable, "BattleAutoSwitchEnable" },
	{ kBattleAutoEOEnable ,"BattleAutoEOEnable" },

	{ kSettingMaxEnable, "SettingMaxEnable" },

	//switcher
	{ kSwitcherTeamEnable, "SwitcherTeamEnable" },
	{ kSwitcherPKEnable, "SwitcherPKEnable" },
	{ kSwitcherCardEnable, "SwitcherCardEnable" },
	{ kSwitcherTradeEnable, "SwitcherTradeEnable" },
	{ kSwitcherGroupEnable, "SwitcherGroupEnable" },
	{ kSwitcherFamilyEnable, "SwitcherFamilyEnable" },
	{ kSwitcherJobEnable, "SwitcherJobEnable" },
	{ kSwitcherWorldEnable, "SwitcherWorldEnable" },
	//afk->battle
	{ kBattleCrossActionCharEnable, "CrossActionCharEnable" },
	{ kBattleCrossActionPetEnable, "CrossActionPetEnable" },

	//afk->heal
	{ kBattleMagicHealEnable, "BattleMagicHealEnable" },
	{ kBattleItemHealEnable, "BattleItemHealEnable" },
	{ kBattleItemHealMeatPriorityEnable, "BattleItemHealMeatPriorityEnable" },
	{ kBattleItemHealMpEnable, "BattleItemHealMpEnable" },
	{ kBattleMagicReviveEnable, "BattleMagicReviveEnable" },
	{ kBattleItemReviveEnable, "BattleItemReviveEnable" },
	{ kBattleSkillMpEnable, "BattleSkillMpEnable" },

	{ kNormalMagicHealEnable, "NormalMagicHealEnable" },
	{ kNormalItemHealEnable, "NormalItemHealEnable" },
	{ kNormalItemHealMeatPriorityEnable, "NormalItemHealMeatPriorityEnable" },
	{ kNormalItemHealMpEnable, "NormalItemHealMpEnable" },

	//afk->catch
	{ kBattleCatchTargetLevelEnable, "BattleCatchTargetLevelEnable" },
	{ kBattleCatchTargetMaxHpEnable, "BattleCatchTargetMaxHpEnable" },
	{ kBattleCatchCharMagicEnable, "BattleCatchCharMagicEnable" },
	{ kBattleCatchCharItemEnable, "BattleCatchCharItemEnable" },
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
	{ kBattleNoEscapeWhileLockPetEnable , "BattleNoEscapeWhileLockPetEnable" },

	{ kAutoAbilityEnable, "AutoAbilityEnable" },

	//string
	{ kSettingMinString, "SettingMinString" },

	{ kAutoDropItemString, "AutoDropItemString" },
	{ kLockAttackString, "LockAttackString" },
	{ kLockEscapeString, "LockEscapeString" },

	//afk->heal
	{ kBattleItemHealItemString, "BattleItemHealItemString" },
	{ kBattleItemHealMpItemString, "BattleItemHealMpIteamString" },
	{ kBattleItemReviveItemString, "BattleItemReviveItemString" },
	{ kNormalItemHealItemString, "NormalItemHealItemString" },
	{ kNormalItemHealMpItemString, "NormalItemHealMpItemString" },

	//afk->catch
	{ kBattleCatchPetNameString, "BattleCatchPetNameString" },
	{ kBattleCatchCharItemString, "BattleCatchCharItemString" },
	{ kDropPetNameString, "DropPetNameString" },


	//other->group
	{ kAutoFunNameString, "AutoFunNameString" },

	//other->lockpet
	{ kLockPetScheduleString, "LockPetScheduleString" },

	{ kGameAccountString, "GameAccountString" },
	{ kGamePasswordString, "GamePasswordString" },
	{ kGameSecurityCodeString, "GameSecurityCodeString" },

	{ kMailWhiteListString , "MailWhiteListString" },

	{ kEOCommandString , "EOCommandString" },

	{ kBattleLogicsString , "BattleLogicsString" },

	{ kAutoAbilityString , "AutoAbilityString" },

	{ kSettingMaxString, "SettingMaxString" },

	{ kScriptDebugModeEnable, "ScriptDebugModeEnable" },
};

//8方位坐標補正
export const QVector<QPoint> fix_point = {
	{0, -1},  //北0
	{1, -1},  //東北1
	{1, 0},	  //東2
	{1, 1},	  //東南3
	{0, 1},	  //南4
	{-1, 1},  //西南0
	{-1, 0},  //西1
	{-1, -1}, //西北2
};