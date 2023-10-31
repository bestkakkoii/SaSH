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

#pragma once
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
#include <type_traits>
#include "model/treewidget.h"
#include "model/treewidgetitem.h"
#include "model/safe.h"

constexpr long long SASH_VERSION_MAJOR = 1;
constexpr long long SASH_VERSION_MINOR = 0;
constexpr long long SASH_VERSION_PATCH = 0;
constexpr long long SASH_MAX_THREAD = 65535;
constexpr long long SASH_MAX_SHARED_MEM = 655350;
constexpr const char* SASH_INJECT_DLLNAME = "sadll";
constexpr const char* SASH_SUPPORT_GAMENAME = "sa_8001.exe";

typedef struct break_marker_s
{
	long long line = 0;
	long long count = 0;
	long long maker = 0;
	QString content = "\0";
} break_marker_t;

namespace mem
{
	bool __fastcall read(HANDLE hProcess, unsigned long long desiredAccess, unsigned long long size, PVOID buffer);
	template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T> && !std::is_pointer_v<T>>>
	[[nodiscard]] T __fastcall read(HANDLE hProcess, unsigned long long desiredAccess);

	template char __fastcall read<char>(HANDLE hProcess, unsigned long long desiredAccess);
	template short __fastcall read<short>(HANDLE hProcess, unsigned long long desiredAccess);
	template int __fastcall read<int>(HANDLE hProcess, unsigned long long desiredAccess);
	template float __fastcall read<float>(HANDLE hProcess, unsigned long long desiredAccess);
	template long __fastcall read<long>(HANDLE hProcess, unsigned long long desiredAccess);
	template long long __fastcall read<long long>(HANDLE hProcess, unsigned long long desiredAccess);
	template unsigned char __fastcall read<unsigned char>(HANDLE hProcess, unsigned long long desiredAccess);
	template unsigned short __fastcall read<unsigned short>(HANDLE hProcess, unsigned long long desiredAccess);
	template unsigned int __fastcall read<unsigned int>(HANDLE hProcess, unsigned long long desiredAccess);
	template unsigned long __fastcall read<unsigned long>(HANDLE hProcess, unsigned long long desiredAccess);
	template unsigned long long __fastcall read<unsigned long long>(HANDLE hProcess, unsigned long long desiredAccess);

	template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T> && !std::is_pointer_v<T>>>
	bool __fastcall write(HANDLE hProcess, unsigned long long baseAddress, T data);
	template bool __fastcall write<char>(HANDLE hProcess, unsigned long long baseAddress, char data);
	template bool __fastcall write<short>(HANDLE hProcess, unsigned long long baseAddress, short data);
	template bool __fastcall write<int>(HANDLE hProcess, unsigned long long baseAddress, int data);
	template bool __fastcall write<float>(HANDLE hProcess, unsigned long long baseAddress, float data);
	template bool __fastcall write<long>(HANDLE hProcess, unsigned long long baseAddress, long data);
	template bool __fastcall write<long long>(HANDLE hProcess, unsigned long long baseAddress, long long data);
	template bool __fastcall write<unsigned char>(HANDLE hProcess, unsigned long long baseAddress, unsigned char data);
	template bool __fastcall write<unsigned short>(HANDLE hProcess, unsigned long long baseAddress, unsigned short data);
	template bool __fastcall write<unsigned int>(HANDLE hProcess, unsigned long long baseAddress, unsigned int data);
	template bool __fastcall write<unsigned long>(HANDLE hProcess, unsigned long long baseAddress, unsigned long data);
	template bool __fastcall write<unsigned long long>(HANDLE hProcess, unsigned long long baseAddress, unsigned long long data);

	[[nodiscard]] float __fastcall readFloat(HANDLE hProcess, unsigned long long desiredAccess);
	[[nodiscard]] qreal __fastcall readDouble(HANDLE hProcess, unsigned long long desiredAccess);
	[[nodiscard]] QString __fastcall readString(HANDLE hProcess, unsigned long long desiredAccess, unsigned long long size, bool enableTrim = true, bool keepOriginal = false);
	bool __fastcall write(HANDLE hProcess, unsigned long long baseAddress, PVOID buffer, unsigned long long dwSize);
	bool __fastcall writeString(HANDLE hProcess, unsigned long long baseAddress, const QString& str);
	bool __fastcall virtualFree(HANDLE hProcess, unsigned long long baseAddress);
	[[nodiscard]] unsigned long long __fastcall virtualAlloc(HANDLE hProcess, unsigned long long size);
	[[nodiscard]] unsigned long long __fastcall virtualAllocW(HANDLE hProcess, const QString& str);
	[[nodiscard]] unsigned long long __fastcall virtualAllocA(HANDLE hProcess, const QString& str);
#ifndef _WIN64
	[[nodiscard]] DWORD __fastcall getRemoteModuleHandle(DWORD dwProcessId, const QString& moduleName);
#endif
	void __fastcall freeUnuseMemory(HANDLE hProcess);

	typedef struct _IAT_EAT_INFO
	{
		char ModuleName[256] = {};
		char FuncName[64] = {};
		ULONG64 Address = 0;
		ULONG64 RecordAddr = 0;
		ULONG64 ModBase = 0;//just for export table
	} IAT_EAT_INFO, * PIAT_EAT_INFO;

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0) && defined(_M_IX86)
	inline __declspec(naked) DWORD* getKernel32()
	{
		__asm
		{
			mov eax, fs: [0x30]
			mov eax, [eax + 0xC]
			mov eax, [eax + 0x1C]
			mov eax, [eax]
			mov eax, [eax]
			mov eax, [eax + 8]
			ret
		}
	}
#else
	inline DWORD* __fastcall getKernel32()
	{
		return (DWORD*)GetModuleHandleW(L"kernel32.dll");
	}
#endif

#if 0
#ifdef _WIN64
	DWORD __fastcall getFunAddr(const DWORD* DllBase, const char* FunName);
#endif
#endif

	HMODULE __fastcall getRemoteModuleHandleByProcessHandleW(HANDLE hProcess, const QString& szModuleName);
	long __fastcall getProcessExportTable32(HANDLE hProcess, const QString& ModuleName, IAT_EAT_INFO tbinfo[], int tb_info_max);
	ULONG64 __fastcall getProcAddressIn32BitProcess(HANDLE hProcess, const QString& ModuleName, const QString& FuncName);
#ifndef _WIN64
	bool __fastcall injectByWin7(long long index, DWORD dwProcessId, HANDLE hProcess, QString dllPath, HMODULE* phDllModule, unsigned long long* phGameModule, HWND hWnd = nullptr);
#endif
	bool __fastcall injectBy64(long long index, DWORD dwProcessId, HANDLE hProcess, QString dllPath, HMODULE* phDllModule, unsigned long long* phGameModule, HWND hWnd = nullptr);//兼容64位注入32位
#if 0
	bool __fastcall inject(long long index, HANDLE hProcess, QString dllPath, HMODULE* phDllModule, unsigned long long* phGameModule);//32注入32
#endif
	bool __fastcall enumProcess(QVector<long long>* pprocesses, const QString& moduleName);

	bool __fastcall isProcessExist(long long pid);
}

namespace util
{
	constexpr const char* DEFAULT_CODEPAGE = "UTF-8";
	constexpr const char* DEFAULT_GAME_CODEPAGE = "GBK";//gb18030/gb2312
	constexpr const char* SCRIPT_DEFAULT_SUFFIX = ".txt";
	constexpr const char* SCRIPT_LUA_SUFFIX_DEFAULT = ".lua";
	constexpr const char* SCRIPT_PRIVATE_SUFFIX_DEFAULT = ".sac";
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

	enum UnLoginStatus
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

		kTcpDelayValue,

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

		kAutoEncodeEnable,

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

		kNormalMagicHealItemString,

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

		kTitleFormatString,
		kBattleAllieFormatString,
		kBattleEnemyFormatString,
		kBattleSelfMarkString,
		kBattleActMarkString,
		kBattleSpaceMarkString,

		kBattleActionOrderString,

		kBattleLogicsString,

		kAutoAbilityString,

		kSettingMaxString,

		kScriptDebugModeEnable,
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
		{ kTcpDelayValue, "TcpDelayValue" },

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

		{ kAutoEncodeEnable, "AutoEncodeEnable" },

		//string
		{ kSettingMinString, "SettingMinString" },

		{ kAutoDropItemString, "AutoDropItemString" },
		{ kLockAttackString, "LockAttackString" },
		{ kLockEscapeString, "LockEscapeString" },

		//afk->heal
		{ kNormalMagicHealItemString, "NormalMagicHealItemString" },
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

		{ kTitleFormatString , "kTitleFormatString" },
		{ kBattleAllieFormatString, "BattleAllieFormatString" },
		{ kBattleEnemyFormatString, "BattleEnemyFormatString" },
		{ kBattleSelfMarkString, "BattleSelfMarkString" },
		{ kBattleActMarkString, "BattleActMarkString" },
		{ kBattleSpaceMarkString, "BattleSpaceMarkString" },

		{ kBattleActionOrderString, "BattleActionOrderString" },

		{ kBattleLogicsString , "BattleLogicsString" },

		{ kAutoAbilityString , "AutoAbilityString" },

		{ kSettingMaxString, "SettingMaxString" },

		{ kScriptDebugModeEnable, "ScriptDebugModeEnable" },
	};

	//8方位坐標補正
	static const QVector<QPoint> fix_point = {
		{0, -1},  //北0
		{1, -1},  //東北1
		{1, 0},	  //東2
		{1, 1},	  //東南3
		{0, 1},	  //南4
		{-1, 1},  //西南0
		{-1, 0},  //西1
		{-1, -1}, //西北2
	};

	[[nodiscard]] QString __fastcall applicationFilePath();

	[[nodiscard]] QString __fastcall applicationDirPath();

	[[nodiscard]] QString __fastcall applicationName();

	[[nodiscard]] long long __fastcall percent(long long value, long long total);

	template<typename T>
	inline [[nodiscard]] QString __fastcall toQString(T d, long long base = 10)
	{
		if constexpr (std::is_same_v<T, double>)
		{
			return QString::number(d, 'f', 5);
		}
		else if constexpr (std::is_same_v<T, float>)
		{
			return QString::number(d, 'f', 5);
		}
		else if constexpr (std::is_same_v<T, bool>)
		{
			return (d) ? QString("true") : QString("false");
		}
		else if constexpr (std::is_same_v<T, wchar_t*>)
		{
			return QString::fromWCharArray(d);
		}
		else if constexpr (std::is_same_v<T, const wchar_t*>)
		{
			return QString::fromWCharArray(d);
		}
		else if constexpr (std::is_same_v<T, char*>)
		{
			return QString::fromUtf8(d);
		}
		else if constexpr (std::is_same_v<T, const char*>)
		{
			return QString::fromUtf8(d);
		}
		else if constexpr (std::is_same_v<T, std::string>)
		{
			return QString::fromUtf8(d.c_str());
		}
		else if constexpr (std::is_same_v<T, long long>)
		{
			return QString::number(d, base);
		}
		else if constexpr (std::is_same_v<T, unsigned long long>)
		{
			return QString::number(d, base);
		}
		else if constexpr (std::is_same_v<T, int>)
		{
			return QString::number(d, base);
		}
		else if constexpr (std::is_same_v < T, QByteArray>)
		{
			return QString::fromUtf8(d);
		}
		else if constexpr (std::is_same_v < T, sol::object>)
		{
			if (d.is<std::string>())
				return QString::fromUtf8(d.as<std::string>().c_str());
			else
				return "";
		}
		else if constexpr (std::is_integral_v<T>)
		{
			return QString::number(d, base);
		}
		else
		{
			qDebug() << "toQString: unknown type" << typeid(T).name();
			MessageBoxA(NULL, typeid(T).name(), "toQString: unknown type", MB_OK | MB_ICONERROR);
			__assume(false);
		}
	}

	inline [[nodiscard]] QString __fastcall toUnicode(const char* str, bool trim = true, bool ext = true)
	{
		QTextCodec* codec = QTextCodec::codecForName(util::DEFAULT_GAME_CODEPAGE);//QTextCodec::codecForMib(2025);//取GB2312解碼器
		QString qstr(codec->toUnicode(str));//先以GB2312解碼轉成UNICODE
		UINT ACP = ::GetACP();
		if (950 == ACP && ext)
		{
			//std::wstring wstr = qstr.toStdWString();
			//long long size = lstrlenW(wstr.c_str());
			//QScopedArrayPointer <wchar_t> wbuf(q_check_ptr(new wchar_t[size + 1]()));
			////繁體字碼表映射
			//LCMapStringEx(LOCALE_NAME_SYSTEM_DEFAULT, LCMAP_TRADITIONAL_CHINESE, wstr.c_str(), size, wbuf.get(), size, NULL, NULL, NULL);
			//qstr = util::toQString(wbuf.get());
		}
		if (!trim)
			return qstr;
		else
			return qstr.simplified();
	}

	inline [[nodiscard]] std::string __fastcall fromUnicode(const QString& str, bool keepOriginalData = false)
	{
		QString qstr(str);
		std::wstring wstr(qstr.toStdWString());
		UINT ACP = ::GetACP();
		if (950 == ACP && !keepOriginalData)
		{
			// 繁體系統要轉回簡體體否則遊戲視窗會亂碼
			long long size = lstrlenW(wstr.c_str());
			//QScopedArrayPointer <wchar_t> wbuf(q_check_ptr(new wchar_t[size + 1]()));
			wchar_t wbuf[8192] = {};
			LCMapStringEx(LOCALE_NAME_SYSTEM_DEFAULT, LCMAP_SIMPLIFIED_CHINESE, wstr.c_str(), size, wbuf, size, NULL, NULL, NULL);
			qstr = util::toQString(wbuf);
		}

		QTextCodec* codec = QTextCodec::codecForName(util::DEFAULT_GAME_CODEPAGE);//QTextCodec::codecForMib(2025);//QTextCodec::codecForName("gb2312");
		QByteArray bytes(codec->fromUnicode(qstr));
		std::string s(bytes.constData());

		return s;
	}

	inline [[nodiscard]] std::string __fastcall toConstData(const QString& str)
	{
		const QByteArray ba(str.toUtf8());
		return ba.constData();
	}

	template<typename T>
	QList<T*> __fastcall findWidgets(QWidget* widget)
	{
		QList<T*> widgets;

		if (widget)
		{
			// 檢查 widget 是否為指定類型的控件，如果是，則添加到結果列表中
			T* typedWidget = qobject_cast<T*>(widget);
			if (typedWidget != nullptr)
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

	inline void __fastcall setTableWidget(QTableWidget* pTable)
	{
		if (pTable == nullptr)
			return;

		QString styleSheet = R"(
QWidget {
    color: black;
    background: white;
}

QTableWidget {
    color: black;
    background-color: white;
    border: 1px solid gray;
    alternate-background-color: white;
    gridline-color: gray;
	font-size:12px;
	outline:0px; /*虛線框*/
}

QTableWidget QTableCornerButton::section {
    background-color: white;
    border: 1px solid gray;
}

QHeaderView::section {
    color: black;
    background-color: white;
	border: 1px solid gray;
}

QHeaderView::section:horizontal
{
    color: black;
    background-color: white;
}

QHeaderView::section:vertical
{
    color: black;
    background-color: white;
}
/*
QTableWidget::item {
    color: black;
    background-color: white;
	min-height: 11px;
    font-size:12px;
}

QTableWidget::item:selected {
    color: white;
    background-color:black;
}

QTableWidget::item:hover {
    color: white;
    background-color:black;
}
*/
QScrollBar:vertical {
	min-height: 30px;  
    background-color: white; 
}

QScrollBar::handle:vertical {
    background-color: white;
  	border: 3px solid  white;
	min-height:30px;
}

QScrollBar::handle:hover:vertical,
QScrollBar::handle:pressed:vertical {
    background-color: #3487FF;
}

QScrollBar:horizontal {
    background-color: white; 
}

QScrollBar::handle:horizontal {
    background-color: #3282F6;
  	border: 3px solid white;
	min-width:50px;
}

QScrollBar::handle:hover:horizontal,
QScrollBar::handle:pressed:horizontal {
    background-color: #3487FF;
}
)";

		pTable->setStyleSheet(styleSheet);
		pTable->setAttribute(Qt::WA_StyledBackground);
	}

	inline void __fastcall setComboBox(QComboBox* pCombo)
	{
		if (pCombo == nullptr)
			return;

		QString styleSheet = R"(
QComboBox {
	color:black;
	background-color: white;
    border: 1px solid gray;
	border-radius: 3px;
}

QComboBox:hover {
    border: 1px solid #4096FF;
}

QComboBox QAbstractItemView { 
	color:black;
	background-color: white;
	border: 1px solid gray;
	border-radius: 3px;
    min-width: 200px;
}

QComboBox::drop-down {
	color:black;
	background-color: white;
    width: 15px;
	/* 
	subcontrol-origin: padding;
    subcontrol-position: top right;
    border-left-width: 1px;
    border-left-color: darkgray;
    border-left-style: solid;
    border-top-right-radius: 2px;
    border-bottom-right-radius: 2px;
	*/
}

QComboBox::down-arrow {
    image: url(:/image/icon_downarrow.svg);
	width:12px;
	height:12px;
}


QComboBox::down-arrow:on { /* shift the arrow when popup is open */
    top: 1px;
    left: 1px;
}

QListView{
	color:black;
	background-color: white;
	border: 2px solid gray;
	border-radius: 10px;
	outline:0px;
}

QListView:item{
	color:black;
	background-color: white;
	border: 3px solid white;
	border-radius: 10px;
}

QListView:item:hover{
	color:black;
	border: 3px solid white;
	background-color: #E6F4FF;
}

QScrollBar:vertical {
	min-height: 30px;  
    background-color: white;
}

QScrollBar::handle:vertical {
    background-color: #3282F6;
  	border: 3px solid white;
	min-height:30px;
}

QScrollBar::handle:hover:vertical,
QScrollBar::handle:pressed:vertical {
    background-color: #3487FF;
}
		)";

		pCombo->setStyleSheet(styleSheet);
		pCombo->setAttribute(Qt::WA_StyledBackground);
		pCombo->setView(q_check_ptr(q_check_ptr(new QListView())));
		pCombo->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	}

	inline void __fastcall setLabel(QLabel* pLabel)
	{
		if (pLabel == nullptr)
			return;

		QString styleSheet = R"(
	QLabel {
		background-color: white;
		border: 0px solid gray;
		color: black;
	}
		)";
		pLabel->setStyleSheet(styleSheet);
		pLabel->setAttribute(Qt::WA_StyledBackground);
		pLabel->setFixedHeight(19);

	}

	inline void __fastcall setPushButton(QPushButton* pButton)
	{
		if (pButton == nullptr)
			return;

		QString styleSheet = R"(
		QPushButton {
			background-color: white;
			border: 1px solid gray;
			border-radius: 3px;
			padding: 2px;
			color: black;
		}
		
		QPushButton:hover {
			background-color: white;
			border: 1px solid #4096FF;
			color:#4096FF;
		}
		
		QPushButton:pressed, QPushButton:checked {
			background-color: white;
			border: 1px solid #0958D9;
			color:#0958D9;
		}

		QPushButton:disabled {
			background-color: #F0F4F8;
			color:gray;
		}
)";

		pButton->setStyleSheet(styleSheet);
		pButton->setAttribute(Qt::WA_StyledBackground);
		pButton->setFixedHeight(19);
	}

	inline void __fastcall setSpinBox(QSpinBox* pCheck)
	{
		if (pCheck == nullptr)
			return;

		QString styleSheet = R"(
QSpinBox {
	color:black;
	background-color: white;
    border: 1px solid gray;
	border-radius: 3px;
}

QSpinBox:focus { 
	border: 2px solid #3282F6;
	background-color:white;
}

QSpinBox:hover{
	border: 2px solid #3282F6;
}

QSpinBox::down-button, QSpinBox::up-button {
    width: 13px;
    border: 0px solid gray;
}

QSpinBox::up-button:hover {
    image: url(:/image/icon_uparrow_blue.svg);
}

QSpinBox::up-button:pressed {
    image: url(:/image/icon_uparrow_blue.svg);
}

QSpinBox::up-arrow {
    image: url(:/image/icon_uparrow.svg);
    width: 9px;
    height: 9px;
}

QSpinBox::down-button:hover {
    image: url(:/image/icon_downarrow_blue.svg);
}

QSpinBox::down-button:pressed {
    image: url(:/image/icon_downarrow_blue.svg);
}

QSpinBox::down-arrow {
    image: url(:/image/icon_downarrow.svg);
    width: 9px;
    height: 9px;
}
		)";

		pCheck->setStyleSheet(styleSheet);
		pCheck->setAttribute(Qt::WA_StyledBackground);
		pCheck->setFixedHeight(19);
	}

	inline void __fastcall setCheckBox(QCheckBox* pCheck)
	{
		if (pCheck == nullptr)
			return;

		QString styleSheet = R"(
QCheckBox {
	color:black;
	background-color: white;
    spacing: 1px;
}

QCheckBox:disabled {
	color:gray;
}

QCheckBox::indicator {
    width: 16px;
    height: 16px;
}

QCheckBox::indicator:unchecked {
	image: url(:/image/icon_uncheck.svg);
}

QCheckBox::indicator:unchecked:hover {
    width: 17px;
    height: 17px;
	image: url(:/image/icon_uncheck_hover.svg);
}

QCheckBox::indicator:unchecked:pressed {
	image: url(:/image/icon_uncheck.svg);
}

QCheckBox::indicator:checked {
	image: url(:/image/icon_check.svg);
}

QCheckBox::indicator:checked:hover {
    width: 17px;
    height: 17px;
	image: url(:/image/icon_check.svg);
}

QCheckBox::indicator:checked:pressed {
	image: url(:/image/icon_check.svg);
}

QCheckBox::indicator:indeterminate:pressed {
	image: url(:/image/icon_uncheck.svg);
}

		)";

		pCheck->setStyleSheet(styleSheet);
		pCheck->setAttribute(Qt::WA_StyledBackground);
		pCheck->setFixedHeight(19);
	}

	inline void __fastcall setLineEdit(QLineEdit* pEdit)
	{
		if (pEdit == nullptr)
			return;

		QString styleSheet = R"(
QLineEdit {
    border: 1px solid gray;
	border-radius: 3px;
    padding: 0 4px;
    background-color: white;
    selection-background-color: #3A79B8;
	height: 20px;
}

QLineEdit:focus { 
	border: 2px solid #3282F6;
	background-color:white;
}

QLineEdit:hover{
	border: 2px solid #3282F6;
	background-color:white;
}
		)";

		pEdit->setStyleSheet(styleSheet);
		pEdit->setAttribute(Qt::WA_StyledBackground);
		pEdit->setFixedHeight(19);
	}

	inline void __fastcall setTab(QTabWidget* pTab)
	{
		if (pTab == nullptr)
			return;

		QString styleSheet = R"(
			QWidget {
				background-color: white;
				color: black;
			}

			QGroupBox {
				background-color: white;
				color: black;
			}

			QTabWidget{
				color: black;
				background-color: white;
				border-top:0px solid gray;

				background-clip:gray;
				background-origin:gray;
			}

			QTabWidget::pane{
				color: black;
				background-color: white;
				border-top:0px solid gray;

				top:0px;
				border-bottom:1px solid gray;
				border-right:1px solid gray;
				border-left:1px solid gray;
			}

			QTabWidget::tab-bar
			{
				alignment: left;
				top:0px;
				left:5px;
				border:none;
			}

			QTabBar::tab:first{
				color: black;
				background-color: white;
				border-top:0px solid gray;

				padding-left:0px;
				padding-right:0px;
				height:18px;
				margin-left:0px;
				margin-right:0px;
			}

			QTabBar::tab:middle{
				color: black;
				background-color: white;
				border-top:0px solid gray;

				padding-left:0px;
				padding-right:0px;
				height:18px;
				margin-left:0px;
				margin-right:0px;
			}

			QTabBar::tab:last{
				color: black;
				background-color: white;
				border-top:0px solid gray;

				padding-left:0px;
				padding-right:0px;
				height:18px;
				margin-left:0px;
				margin-right:0px;
			}

			QTabBar::tab:selected{
				color: #1677FF;
				background-color: white;
				border-bottom:2px solid #1677FF;

			}

			QTabBar::tab:hover{
				color:#62A9FF;
				background-color: white;
				border-top:0px solid #3282F6;
			}

		)";

		pTab->setStyleSheet(styleSheet);
		pTab->setAttribute(Qt::WA_StyledBackground);
		QTabBar* pTabBar = pTab->tabBar();

		pTabBar->setDocumentMode(true);
		pTabBar->setExpanding(true);
	}

	inline void __fastcall setWidget(QMainWindow* pWidget)
	{
		if (pWidget == nullptr)
			return;

		pWidget->setAttribute(Qt::WA_StyledBackground);

		QString style = R"(
QMenuBar {
	background-color: white;
	color: black;
}

QMenuBar::item:selected {
	color:white;
    background: #3282F6;
}

QMenuBar::item:pressed {
	color:white;
    background: #3487FF;
}

QGroupBox {
	background-color: white;
	color: black;
};
)";

		pWidget->setStyleSheet(style);
	}

	inline void __fastcall setWidget(QWidget* pWidget)
	{
		if (pWidget == nullptr)
			return;

		pWidget->setAttribute(Qt::WA_StyledBackground);

		QString style = R"(
QWidget {
	background-color: white;
	color: black;
}

QGroupBox {
	background-color: white;
	color: black;
}
)";

		pWidget->setStyleSheet(style);
	}

	inline void __fastcall setWidget(QDialog* pWidget)
	{
		if (pWidget == nullptr)
			return;

		pWidget->setAttribute(Qt::WA_StyledBackground);

		QString style = R"(
QDialog {
	background-color: white;
	color: black;
}

QWidget {
	background-color: white;
	color: black;
}

QGroupBox {
	background-color: white;
	color: black;
}
)";

		pWidget->setStyleSheet(style);
	}


	QFileInfoList __fastcall loadAllFileLists(
		TreeWidgetItem* root,
		const QString& path,
		QStringList* list = nullptr,
		const QString& fileIcon = ":/image/icon_txt.svg",
		const QString& folderIcon = ":/image/icon_directory.svg");

	QFileInfoList __fastcall loadAllFileLists(
		TreeWidgetItem* root,
		const QString& path,
		const QString& suffix,
		const QString& icon, QStringList* list = nullptr,
		const QString& folderIcon = ":/image/icon_directory.svg");

	void __fastcall searchFiles(const QString& dir, const QString& fileNamePart, const QString& suffixWithDot, QStringList* result, bool withcontent);

	bool __fastcall enumAllFiles(const QString dir, const QString suffix, QVector<QPair<QString, QString>>* result);

	QString __fastcall findFileFromName(const QString& fileName, const QString& dirpath = util::applicationDirPath());

	QString __fastcall formatMilliseconds(long long milliseconds, bool noSpace = false);

	QString __fastcall formatSeconds(long long seconds);

	bool __fastcall writeFireWallOverXP(const LPCTSTR& ruleName, const LPCTSTR& appPath, bool NoopIfExist);

	bool __fastcall monitorThreadResourceUsage(FILETIME& preidleTime, FILETIME& prekernelTime, FILETIME& preuserTime, double* pCpuUsage, double* pMemUsage, double* pMaxMemUsage);

	QFont __fastcall getFont();

	void __fastcall asyncRunBat(const QString& path, QString data);

	bool __fastcall readFileFilter(const QString& fileName, QString& content, bool* pisPrivate);

	bool __fastcall readFile(const QString& fileName, QString* pcontent, bool* isPrivate = nullptr);

	bool __fastcall writeFile(const QString& fileName, const QString& content);

	void __fastcall sortWindows(const QVector<HWND>& windowList, bool alignLeft);

	bool __fastcall fileDialogShow(const QString& name, long long acceptType, QString* retstring, QWidget* pparent = nullptr);

	inline QCollator __fastcall getCollator()
	{
		static const QLocale locale(QLocale::Chinese);
		static QCollator collator(locale);
		collator.setCaseSensitivity(Qt::CaseSensitive);
		collator.setNumericMode(true);
		collator.setIgnorePunctuation(false);

		return collator;
	}

	// 將二進制數據轉換為16進制字符串
	QString __fastcall byteArrayToHexString(const QByteArray& data);

	// 將16進制字符串轉換為二進制數據
	QByteArray __fastcall hexStringToByteArray(const QString& hexString);

#pragma region swap_row
	inline void __fastcall SwapRow(QTableWidget* p, QListWidget* p2, long long selectRow, long long targetRow)
	{

		if (p)
		{
			QStringList selectRowLine, targetRowLine;
			long long count = p->columnCount();
			for (long long i = 0; i < count; ++i)
			{
				selectRowLine.append(p->item(selectRow, i)->text());
				targetRowLine.append(p->item(targetRow, i)->text());
				if (!p->item(selectRow, i))
					p->setItem(selectRow, i, q_check_ptr(new QTableWidgetItem(targetRowLine.value(i))));
				else
					p->item(selectRow, i)->setText(targetRowLine.value(i));

				if (!p->item(targetRow, i))
					p->setItem(targetRow, i, q_check_ptr(new QTableWidgetItem(selectRowLine.value(i))));
				else
					p->item(targetRow, i)->setText(selectRowLine.value(i));
			}
		}
		else if (p2)
		{
			if (p2->count() == 0) return;
			if (selectRow == targetRow || targetRow < 0 || selectRow < 0) return;
			QString selectRowStr = p2->item(selectRow)->text();
			QString targetRowStr = p2->item(targetRow)->text();
			if (selectRow > targetRow)
			{
				p2->takeItem(selectRow);
				p2->takeItem(targetRow);
				p2->insertItem(targetRow, selectRowStr);
				p2->insertItem(selectRow, targetRowStr);
			}
			else
			{
				p2->takeItem(targetRow);
				p2->takeItem(selectRow);
				p2->insertItem(selectRow, targetRowStr);
				p2->insertItem(targetRow, selectRowStr);
			}
		}
	}

	inline void __fastcall SwapRowUp(QTableWidget* p)
	{
		if (p->rowCount() <= 0)
			return; //至少有一行
		const QList<QTableWidgetItem*> list = p->selectedItems();
		if (list.size() <= 0)
			return; //有選中
		long long t = list.value(0)->row();
		if (t - 1 < 0)
			return; //不是第一行

		long long selectRow = t;	 //當前行
		long long targetRow = t - 1; //目標行

		SwapRow(p, nullptr, selectRow, targetRow);

		QModelIndex cur = p->model()->index(targetRow, 0);
		p->setCurrentIndex(cur);
	}

	inline void __fastcall SwapRowDown(QTableWidget* p)
	{
		if (p->rowCount() <= 0)
			return; //至少有一行
		const QList<QTableWidgetItem*> list = p->selectedItems();
		if (list.size() <= 0)
			return; //有選中
		long long t = list.value(0)->row();
		if (t + 1 > static_cast<long long>(p->rowCount()) - 1)
			return; //不是最後一行

		long long selectRow = t;	 //當前行
		long long targetRow = t + 1; //目標行

		SwapRow(p, nullptr, selectRow, targetRow);

		QModelIndex cur = p->model()->index(targetRow, 0);
		p->setCurrentIndex(cur);
	}

	inline void __fastcall SwapRowUp(QListWidget* p)
	{
		if (p->count() <= 0)
			return;
		long long t = p->currentIndex().row(); // ui->tableWidget->rowCount();
		if (t < 0)
			return;
		if (t - 1 < 0)
			return;

		long long selectRow = t;
		long long targetRow = t - 1;

		SwapRow(nullptr, p, selectRow, targetRow);

		QModelIndex cur = p->model()->index(targetRow, 0);
		p->setCurrentIndex(cur);
	}

	inline void __fastcall SwapRowDown(QListWidget* p)
	{
		if (p->count() <= 0)
			return;
		long long t = p->currentIndex().row();
		if (t < 0)
			return;
		if (t + 1 > static_cast<long long>(p->count()) - 1)
			return;

		long long selectRow = t;
		long long targetRow = t + 1;

		SwapRow(nullptr, p, selectRow, targetRow);

		QModelIndex cur = p->model()->index(targetRow, 0);
		p->setCurrentIndex(cur);
	}
#pragma endregion
	class ScopedFileLocker
	{
	public:
		explicit ScopedFileLocker(const QString& fileName)
			:lock_(fileName)
		{
			isFileLocked = lock_.tryLock();
		}

		virtual ~ScopedFileLocker()
		{
			if (isFileLocked)
			{
				lock_.unlock();
			}
		}

		bool isLocked() const
		{
			return isFileLocked;
		}

	private:
		QLockFile lock_;
		bool isFileLocked = false;
	};

	class ScopedLocker
	{
	public:
		explicit ScopedLocker(QMutex* lock)
			:lock_(*lock)
		{
			isLocked_ = lock_.tryLock();
		}

		virtual ~ScopedLocker()
		{
			if (isLocked_)
			{
				lock_.unlock();
			}
		}

		bool isLocked() const
		{
			return isLocked_;
		}



	private:
		QMutex& lock_;
		bool isLocked_ = false;

	};

	//介面排版或配置管理 主用於保存窗口狀態
	class FormSettingManager
	{
	public:
		explicit FormSettingManager(QWidget* widget) { widget_ = widget; }
		explicit FormSettingManager(QMainWindow* widget) { mainwindow_ = widget; }

		void __fastcall loadSettings();
		void __fastcall saveSettings();

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

		VirtualMemory(HANDLE h, long long size, bool autoclear)
			: autoclear(autoclear)
			, hProcess(h)
		{

			lpAddress = mem::virtualAlloc(h, size);
		}

		VirtualMemory(HANDLE h, const QString& str, VirtualEncode use_unicode, bool autoclear)
			: autoclear(autoclear)
		{

			lpAddress = (use_unicode) == VirtualMemory::kUnicode ? (mem::virtualAllocW(h, str)) : (mem::virtualAllocA(h, str));
			hProcess = h;
		}

		inline operator unsigned long long() const
		{
			return this->lpAddress;
		}

		VirtualMemory& operator=(long long other)
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

					mem::write<unsigned char>(hProcess, this->lpAddress, 0);
					mem::virtualFree(hProcess, this->lpAddress);
					this->lpAddress = NULL;
				}
			} while (false);
		}

		[[nodiscard]] inline bool isNull() const
		{
			return !lpAddress;
		}

		[[nodiscard]] inline bool isData(BYTE* data, long long size) const
		{
			QScopedArrayPointer <BYTE> _data(data);
			mem::read(hProcess, lpAddress, size, _data.data());
			return memcmp(data, _data.data(), size) == 0;
		}

		inline void clear()
		{

			if ((this->lpAddress))
			{
				mem::virtualFree(hProcess, (this->lpAddress));
				this->lpAddress = NULL;
			}
		}

		[[nodiscard]] inline bool isValid()const
		{
			return (this->lpAddress) != NULL;
		}

	private:
		bool autoclear = false;
		unsigned long long lpAddress = NULL;
		HANDLE hProcess = NULL;
	};

	class TextStream : public QTextStream
	{
	public:
		explicit TextStream(FILE* file)
			: QTextStream(file)
		{
			init();
		}

		explicit TextStream(QIODevice* device)
			: QTextStream(device)
		{
			init();
		}

	private:
		void init()
		{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
			setCodec(util::DEFAULT_CODEPAGE);
#else
			setEncoding(QStringConverter::Utf8);
#endif
			setGenerateByteOrderMark(true);

			setAutoDetectUnicode(true);
		}
	};

	//智能文件句柄類
	class ScopedFile : public QFile
	{
	public:
		ScopedFile(const QString& filename, OpenMode ioFlags)
			: QFile(filename)
		{
			if (QFile::isOpen())
			{
				QFile::flush();
				QFile::close();
			}
			QFile::open(ioFlags);
		}

		explicit ScopedFile(const QString& filename)
			: QFile(filename)
		{}

		ScopedFile() = default;

		bool openWriteNew()
		{
			if (QFile::isOpen())
			{
				QFile::flush();
				QFile::close();
			}

			return QFile::open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Unbuffered);
		}

		bool openWrite()
		{
			if (QFile::isOpen())
			{
				QFile::flush();
				QFile::close();
			}

			return QFile::open(QIODevice::WriteOnly | QIODevice::Unbuffered);
		}

		bool openRead()
		{
			if (QFile::isOpen())
			{
				QFile::flush();
				QFile::close();
			}

			return QFile::open(QIODevice::ReadOnly | QIODevice::Unbuffered);
		}

		bool openReadWrite()
		{
			if (QFile::isOpen())
			{
				QFile::flush();
				QFile::close();
			}

			return QFile::open(QIODevice::ReadWrite | QIODevice::Unbuffered);
		}

		bool openReadWriteNew()
		{
			if (QFile::isOpen())
			{
				QFile::flush();
				QFile::close();
			}

			return QFile::open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Unbuffered);
		}

		bool openWriteAppend()
		{
			if (QFile::isOpen())
			{
				QFile::flush();
				QFile::close();
			}

			return QFile::open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Unbuffered);
		}

		virtual ~ScopedFile()
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
			{
				QFile::flush();
				QFile::close();
			}
		}

		template <typename T>
		bool mmap(T*& p, long long offset, long long size, QFile::MemoryMapFlags flags = QFileDevice::MapPrivateOption)//QFile::NoOptions
		{
			uchar* uc = QFile::map(offset, size, flags);
			if (uc)
			{
				m_maps.insert(uc);
				p = reinterpret_cast<T*>(uc);
			}
			return uc != nullptr;
		}

		QFile& file() { return *this; }

	private:
		QSet<uchar*> m_maps;
	};

	//Json配置讀寫
	class Config
	{
	public:
		typedef struct tagMapData
		{
			long long floor = 0;
			long long x = 0;
			long long y = 0;
			QString name = "";
		}MapData;

		Config(const QString& callBy);
		explicit Config(const QString& fileName, const QString& callBy);
		explicit Config(const QByteArray& fileName, const QString& callBy);
		virtual ~Config();

		bool __fastcall open();
		void __fastcall sync();

		inline bool __fastcall isValid() const { return isVaild; }

		void __fastcall removeSec(const QString sec);

		void __fastcall write(const QString& key, const QVariant& value);
		void __fastcall write(const QString& sec, const QString& key, const QVariant& value);
		void __fastcall write(const QString& sec, const QString& key, const QString& sub, const QVariant& value);

		void __fastcall WriteHash(const QString& sec, const QString& key, QMap<QString, QPair<bool, QString>>& hash);
		QMap<QString, QPair<bool, QString>> __fastcall EnumString(const QString& sec, const QString& key) const;

		template <typename T>
		T __fastcall read(const QString& sec, const QString& key, const QString& sub) const
		{
			if (!cache_.contains(sec))
			{
				return T();
			}

			QJsonObject json = cache_.value(sec).toJsonObject();
			if (!json.contains(key))
			{
				return T();
			}

			QJsonObject subjson = json.value(key).toObject();
			if (!subjson.contains(sub))
			{
				return T();
			}
			return subjson[sub].toVariant().value<T>();
		}

		template <typename T>
		T __fastcall read(const QString& sec, const QString& key) const
		{
			//read json value from cache_
			if (cache_.contains(sec))
			{
				QJsonObject json = cache_.value(sec).toJsonObject();
				if (json.contains(key))
				{
					return json.value(key).toVariant().value<T>();
				}
			}
			return T();
		}

		template <typename T>
		T __fastcall read(const QString& key) const
		{
			if (cache_.contains(key))
			{
				return cache_.value(key).value<T>();
			}

			return T();
		}

		template <typename T>
		QList<T> __fastcall readArray(const QString& sec, const QString& key, const QString& sub) const
		{
			QList<T> result;
			QVariant variant;

			if (cache_.contains(sec))
			{
				QJsonObject json = cache_.value(sec).toJsonObject();

				if (json.contains(key))
				{
					QJsonObject subJson = json.value(key).toObject();

					if (subJson.contains(sub))
					{
						QJsonArray jsonArray = subJson.value(sub).toArray();

						for (const QJsonValue& value : jsonArray)
						{
							variant = value.toVariant();
							result.append(variant.value<T>());
						}
					}
				}
			}

			return result;
		}

		template <typename T>
		void __fastcall writeArray(const QString& sec, const QString& key, const QList<T>& values)
		{
			if (!cache_.contains(sec))
			{
				cache_.insert(sec, QJsonObject());
			}

			QVariantList variantList;
			for (const T& value : values)
			{
				variantList.append(value);
			}

			QJsonObject json = cache_.value(sec).toJsonObject();
			json.insert(key, QJsonArray::fromVariantList(variantList));
			cache_.insert(sec, json);

			if (!hasChanged_)
				hasChanged_ = true;
		}

		template <typename T>
		void __fastcall writeArray(const QString& sec, const QString& key, const QString& sub, const QList<T>& values)
		{
			QJsonObject json;

			if (cache_.contains(sec))
			{
				json = cache_.value(sec).toJsonObject();
			}

			QJsonArray jsonArray;

			for (const T& value : values)
			{
				jsonArray.append(value);
			}

			QJsonObject subJson;

			if (json.contains(key))
			{
				subJson = json.value(key).toObject();
			}

			subJson.insert(sub, jsonArray);
			json.insert(key, subJson);
			cache_.insert(sec, json);

			if (!hasChanged_)
				hasChanged_ = true;
		}

		void __fastcall writeMapData(const QString& sec, const MapData& data);

		QList<MapData> __fastcall readMapData(const QString& key) const;

	private:
		QString callby;

		QJsonDocument document_;

		ScopedFile file_;

		QString fileName_ = "\0";

		inline static safe::Hash<QString, QVariantMap> cacheHash_ = {};

		QVariantMap cache_ = {};

		bool isVaild = false;

		bool hasChanged_ = false;
	};

	//用於掛機訊息紀錄
	typedef struct tagAfkRecorder
	{
		long long levelrecord = 0;
		long long leveldifference = 0;
		long long exprecord = 0;
		long long expdifference = 0;
		long long goldearn = 0;
		long long deadthcount = 0;
		long long reprecord = 0;
		long long repearn = 0;
		bool deadthcountflag = false;
	}AfkRecorder;
	//#pragma warning(push)
	//#pragma warning(disable:304)
	//	Q_DECLARE_METATYPE(VirtualMemory);
	//	Q_DECLARE_TYPEINFO(VirtualMemory, Q_MOVABLE_TYPE);
	//#pragma warning(pop)

	static const QRegularExpression rexSpace(R"([ ]+)");
	static const QRegularExpression rexOR(R"(\s*\|\s*)");
	static const QRegularExpression rexComma(R"(\s*,\s*)");
	static const QRegularExpression rexSemicolon(R"(\s*;\s*)");
	static const QRegularExpression rexColon(R"(\s*:\s*)");
	static const QRegularExpression rexDec(R"(\s*-\s*)");
	//(\('[\w\p{Han}]+'\))
	static const QRegularExpression rexQuoteWithParentheses(R"(\('?([\w\W\S\s\p{Han}]+)'?\))");
	//\[(\d+)\]
	static const QRegularExpression rexSquareBrackets(R"(\[(\d+)\])");
	//([\w\p{Han}]+)\[(\d+)\]

}