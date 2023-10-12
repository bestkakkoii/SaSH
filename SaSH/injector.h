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

import Scoped;
import Logger;
#pragma once
#include <QObject>
#include "net/tcpserver.h"
#include <indexer.h>
#include "net/autil.h"

class StringListModel;
class Injector : public Indexer
{
private:
	explicit Injector(__int64 index);

public:
	virtual ~Injector();

	static Injector& getInstance(__int64 index);

	static bool get(__int64 index, Injector** ppinstance);

public:
	static void reset();
	static void reset(__int64 index);

	virtual void setIndex(__int64 index) override;

public:
	enum CreateProcessResult
	{
		CreateAboveWindow8Failed = -2,
		CreateBelowWindow8Failed = -1,
		CreateFail = 0,
		CreateAboveWindow8Success,
		CreateBelowWindow8Success,
	};

	typedef struct process_information_s
	{
		__int64 dwProcessId = NULL;
		__int64 dwThreadId = NULL;
		HWND hWnd = nullptr;
	} process_information_t, * pprocess_information_t, * lpprocess_information_t;

	void close() const;

	[[nodiscard]] HANDLE getProcess() const;

	[[nodiscard]] bool isValid() const;

	[[nodiscard]] inline HWND getProcessWindow() const { return pi_.hWnd; }

	[[nodiscard]] inline __int64 getProcessId() const { return pi_.dwProcessId; }

	[[nodiscard]] inline __int64 getProcessModule() const { return hGameModule_; }

	CreateProcessResult createProcess(process_information_t& pi);

	bool injectLibrary(process_information_t& pi, unsigned short port, LPREMOVE_THREAD_REASON pReason);

	[[nodiscard]] bool isWindowAlive() const;

	__int64 sendMessage(__int64 msg, __int64 wParam, __int64 lParam) const;

	bool postMessage(__int64 msg, __int64 wParam, __int64 lParam) const;

	inline void setValueHash(UserSetting setting, __int64 value) { userSetting_value_hash_.insert(setting, value); }

	inline void setEnableHash(UserSetting setting, bool enable) { userSetting_enable_hash_.insert(setting, enable); }

	inline void setStringHash(UserSetting setting, const QString& string) { userSetting_string_hash_.insert(setting, string); }

	[[nodiscard]] inline __int64 getValueHash(UserSetting setting) const { return userSetting_value_hash_.value(setting); }

	[[nodiscard]] inline bool getEnableHash(UserSetting setting) const { return userSetting_enable_hash_.value(setting); }

	[[nodiscard]] inline QString getStringHash(UserSetting setting) const { return userSetting_string_hash_.value(setting); }

	[[nodiscard]] inline QHash<UserSetting, __int64> getValuesHash() const { return userSetting_value_hash_.toHash(); }

	[[nodiscard]] inline QHash<UserSetting, bool> getEnablesHash() const { return userSetting_enable_hash_.toHash(); }

	[[nodiscard]] inline QHash<UserSetting, QString> getStringsHash() const { return userSetting_string_hash_.toHash(); }

	inline void setValuesHash(const QHash<UserSetting, __int64>& hash) { userSetting_value_hash_ = hash; }

	inline void setEnablesHash(const QHash<UserSetting, bool>& hash) { userSetting_enable_hash_ = hash; }

	inline void setStringsHash(const QHash<UserSetting, QString>& hash) { userSetting_string_hash_ = hash; }

	[[nodiscard]] inline QVariant getUserData(UserData type) const { return userData_hash_.value(type); }

	inline void setUserData(UserData type, const QVariant& data) { userData_hash_.insert(type, QVariant::fromValue(data)); }

	void mouseMove(__int64 x, __int64 y) const;

	void leftClick(__int64 x, __int64 y) const;

	void leftDoubleClick(__int64 x, __int64 y) const;

	void rightClick(__int64 x, __int64 y) const;

	void dragto(__int64 x1, __int64 y1, __int64 x2, __int64 y2) const;

	void hide(__int64 mode = 0);

	void show();

	QString getPointFileName();

	inline void setParentWidget(HWND parentWidget) { parentWidget_ = parentWidget; }

	[[nodiscard]] inline HWND getParentWidget() const { return parentWidget_; }

private:
	static BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam)
	{
		lpprocess_information_t data = reinterpret_cast<lpprocess_information_t>(lParam);
		DWORD dwProcessId = 0;
		do
		{
			if (!handle || !lParam) break;

			::GetWindowThreadProcessId(handle, &dwProcessId);
			if (data->dwProcessId == static_cast<__int64>(dwProcessId) && IsWindowVisible(handle))
			{
				data->hWnd = handle;
				return FALSE;
			}
		} while (false);
		return TRUE;
	}

	[[nodiscard]] bool isHandleValid(__int64 pid);

#if 0
	DWORD WINAPI getFunAddr(const DWORD* DllBase, const char* FunName);
#endif

public:
	QString currentGameExePath;//當前使用的遊戲進程完整路徑

	QSharedPointer<Server> server;//與遊戲TCP通信專用

	std::atomic_bool IS_SCRIPT_FLAG = false;//主腳本是否運行
	std::atomic_bool IS_SCRIPT_INTERRUPT = false;//主腳本是否中斷

	QString currentScriptFileName;//當前運行的主腳本完整路徑

	QSharedPointer<StringListModel> scriptLogModel; //腳本日誌模型

	QSharedPointer<StringListModel> chatLogModel; //聊天日誌模型

	SafeData<QStringList> serverNameList;

	SafeData<QStringList> subServerNameList;

	__int64 currentServerListIndex = 0;

	std::atomic_bool isScriptDebugModeEnable = false;

	std::atomic_bool isScriptEditorOpened = false;

	quint64 scriptThreadId = 0;

	Autil autil;

	std::unique_ptr<Logger> log = nullptr;

	SafeHash<QString, SafeHash<__int64, break_marker_t>> break_markers;//interpreter.cpp//用於標記自訂義中斷點(紅點)
	SafeHash<QString, SafeHash<__int64, break_marker_t>> forward_markers;//interpreter.cpp//用於標示當前執行中斷處(黃箭頭)
	SafeHash<QString, SafeHash<__int64, break_marker_t>> error_markers;//interpreter.cpp//用於標示錯誤發生行(紅線)
	SafeHash<QString, SafeHash<__int64, break_marker_t>> step_markers;//interpreter.cpp//隱式標記中斷點用於單步執行(無)

	bool IS_INJECT_OK = false;//是否注入成功
private:
	quint64 hGameModule_ = NULL;
	HMODULE hookdllModule_ = NULL;
	process_information_t pi_ = {};
	ScopedHandle processHandle_;
	HWND parentWidget_ = nullptr;//主窗口句柄


	__int64 nowChatRowCount_ = 0;

	SafeHash<UserData, QVariant> userData_hash_ = {
		{ kUserItemNames, QStringList() },

	};

	SafeHash<UserSetting, __int64> userSetting_value_hash_ = {
		{ kSettingNotUsed, kSettingNotUsed },
		{ kSettingMinValue, kSettingMinValue },

		{ kServerValue, 0 },
		{ kSubServerValue, 0 },
		{ kPositionValue, 0 },
		{ kLockTimeValue, 0 },
		{ kSpeedBoostValue, 10 },

		//afk->battle button
		{ kBattleCharRoundActionTargetValue, kSelectEnemyAny },
		{ kBattleCharCrossActionTargetValue, kSelectEnemyAny },
		{ kBattleCharNormalActionTargetValue, kSelectEnemyAny },
		{ kBattlePetNormalActionTargetValue, kSelectEnemyAny },

		{ kBattlePetRoundActionTargetValue, kSelectEnemyAny },
		{ kBattlePetCrossActionTargetValue, kSelectEnemyAny },

		//afk->battle combobox
		{ kBattleCharRoundActionRoundValue, 0 },
		{ kBattleCharRoundActionTypeValue, 0 },
		{ kBattleCharRoundActionEnemyValue, 0 },
		{ kBattleCharRoundActionLevelValue, 0 },

		{ kBattleCharCrossActionTypeValue, 0 },
		{ kBattleCharCrossActionRoundValue, 0 },

		{ kBattleCharNormalActionTypeValue, 0 },
		{ kBattleCharNormalActionEnemyValue, 0 },
		{ kBattleCharNormalActionLevelValue, 0 },

		{ kBattlePetRoundActionRoundValue, 0 },
		{ kBattlePetRoundActionTypeValue, 0 },
		{ kBattlePetRoundActionEnemyValue, 0 },
		{ kBattlePetRoundActionLevelValue, 0 },

		{ kBattlePetCrossActionTypeValue, 0 },
		{ kBattlePetCrossActionRoundValue, 0 },

		{ kBattlePetNormalActionTypeValue, 0 },
		{ kBattlePetNormalActionEnemyValue, 0 },
		{ kBattlePetNormalActionLevelValue, 0 },

		//afk->heal button
		{ kBattleMagicHealTargetValue, kSelectSelf | kSelectPet },
		{ kBattleItemHealTargetValue, kSelectSelf | kSelectPet },
		{ kBattleMagicReviveTargetValue, kSelectSelf | kSelectPet },
		{ kBattleItemReviveTargetValue, kSelectSelf | kSelectPet },

		{ kBattlePetHealTargetValue, kSelectSelf | kSelectPet },
		{ kBattlePetPurgTargetValue, kSelectSelf | kSelectPet },
		{ kBattleCharPurgTargetValue, kSelectSelf | kSelectPet },

		//afk->heal combobox
		{ kBattleMagicHealMagicValue, 0 },
		{ kBattleMagicReviveMagicValue, 0 },

		{ kNormalMagicHealMagicValue, 0 },

		{ kBattlePetHealActionTypeValue, 0 },
		{ kBattlePetPurgActionTypeValue, 0 },
		{ kBattleCharPurgActionTypeValue, 0 },
		{ kBattlePetHealTargetValue, 0 },
		{ kBattlePetPurgTargetValue, 0 },
		{ kBattleCharPurgTargetValue, 0 },
		{ kBattlePetHealCharValue, 0 },
		{ kBattlePetHealPetValue, 0 },
		{ kBattlePetHealAllieValue, 0 },

		//afk->heal spinbox
		{ kBattleMagicHealCharValue, 50 },
		{ kBattleMagicHealPetValue, 50 },
		{ kBattleMagicHealAllieValue, 50 },
		{ kBattleItemHealCharValue, 50 },
		{ kBattleItemHealPetValue, 50 },
		{ kBattleItemHealAllieValue, 50 },
		{ kBattleItemHealMpValue, 50 },
		{ kBattleSkillMpValue, 30 },

		{ kNormalMagicHealCharValue, 50 },
		{ kNormalMagicHealPetValue, 50 },
		{ kNormalMagicHealAllieValue, 50 },
		{ kNormalItemHealCharValue, 50 },
		{ kNormalItemHealPetValue, 50 },
		{ kNormalItemHealAllieValue, 50 },
		{ kNormalItemHealMpValue, 50 },

		//afk->walk
		{ kAutoWalkDelayValue, 10 },
		{ kAutoWalkDistanceValue, 2 },
		{ kAutoWalkDirectionValue, 0 },

		//afk->catch
		{ kBattleCatchModeValue, 0 },
		{ kBattleCatchTargetLevelValue, 1 },
		{ kBattleCatchTargetMaxHpValue, 10 },
		{ kBattleCatchTargetMagicHpValue, 10 },
		{ kBattleCatchTargetItemHpValue, 10 },
		{ kBattleCatchCharMagicValue, 0 },
		{ kBattleCatchPetSkillValue, 0 },

		{ kBattleActionDelayValue, 0 },
		{ kBattleResendDelayValue, 3000},

		{ kDropPetStrValue, 10 },
		{ kDropPetDefValue, 10 },
		{ kDropPetAgiValue, 10 },
		{ kDropPetHpValue, 50 },
		{ kDropPetAggregateValue, 10 },

		//other->group
		{ kAutoFunTypeValue, 0 },

		//lockpet
		{ kLockPetValue, 0 },
		{ kLockRideValue, 0 },


		//script
		{kScriptSpeedValue, 0},

		{ kSettingMaxValue, kSettingMaxValue },
		{ kSettingMinString, kSettingMinString },
		{ kSettingMaxString, kSettingMaxString },

	};

	SafeHash<UserSetting, bool> userSetting_enable_hash_ = {
		{ kAutoLoginEnable, false },
		{ kAutoReconnectEnable, true },

		{ kLogOutEnable, false },
		{ kLogBackEnable, false },
		{ kEchoEnable, false },

		{ kHideCharacterEnable, false },
		{ kCloseEffectEnable, true },
		{ kOptimizeEnable, true },
		{ kHideWindowEnable, false },
		{ kMuteEnable, false },
		{ kAutoJoinEnable, false },
		{ kLockTimeEnable, false },
		{ kAutoFreeMemoryEnable, true },
		{ kFastWalkEnable, true },
		{ kPassWallEnable, false },
		{ kLockMoveEnable, false },
		{ kLockImageEnable, false },
		{ kAutoDropMeatEnable, true },
		{ kAutoDropEnable, false },
		{ kAutoStackEnable, true },
		{ kKNPCEnable, true },
		{ kAutoAnswerEnable, false },
		{ kAutoEatBeanEnable, false },
		{ kAutoWalkEnable, false },
		{ kFastAutoWalkEnable, false },
		{ kFastBattleEnable, true },
		{ kAutoBattleEnable, false },
		{ kAutoCatchEnable, false },
		{ kLockAttackEnable, false },
		{ kAutoEscapeEnable, false },
		{ kLockEscapeEnable, false },
		{ kBattleTimeExtendEnable, true },
		{ kFallDownEscapeEnable, false },
		{ kShowExpEnable, true },
		{ kWindowDockEnable, false },
		{ kBattleAutoSwitchEnable, true },
		{ kBattleAutoEOEnable, true },

		//switcher
		{ kSwitcherTeamEnable, false },
		{ kSwitcherPKEnable, false },
		{ kSwitcherCardEnable, false },
		{ kSwitcherTradeEnable, false },
		{ kSwitcherGroupEnable, false },
		{ kSwitcherFamilyEnable, false },
		{ kSwitcherJobEnable, false },
		{ kSwitcherWorldEnable, false },

		//afk->battle
		{ kBattleCrossActionCharEnable, false },
		{ kBattleCrossActionPetEnable, false },

		//afk->heal
		{ kBattleMagicHealEnable, false },
		{ kBattleItemHealEnable, false },
		{ kBattleItemHealMeatPriorityEnable, false },
		{ kBattleItemHealMpEnable, false },
		{ kBattleMagicReviveEnable, false },
		{ kBattleItemReviveEnable, false },
		{ kBattleSkillMpEnable, false },

		{ kNormalMagicHealEnable, false },
		{ kNormalItemHealEnable, false },
		{ kNormalItemHealMeatPriorityEnable, false },
		{ kNormalItemHealMpEnable, false },

		//afk->catch
		{ kBattleCatchTargetLevelEnable, false },
		{ kBattleCatchTargetMaxHpEnable, false },
		{ kBattleCatchCharMagicEnable, false },
		{ kBattleCatchCharItemEnable, false },
		{ kBattleCatchPetSkillEnable, false },

		{ kDropPetEnable, false },
		{ kDropPetStrEnable, false },
		{ kDropPetDefEnable, false },
		{ kDropPetAgiEnable, false },
		{ kDropPetHpEnable, false },
		{ kDropPetAggregateEnable, false },

		//lockpet
		{ kLockPetEnable, false },
		{ kLockRideEnable, false },
		{ kLockPetScheduleEnable, false },
		{ kBattleNoEscapeWhileLockPetEnable, false },



		{ kScriptDebugModeEnable, false },

	};

	SafeHash<UserSetting, QString> userSetting_string_hash_ = {
		{ kAutoDropItemString, "" },
		{ kLockAttackString, "" },
		{ kLockEscapeString, "" },

		{ kBattleItemHealItemString, "" },
		{ kBattleItemHealMpItemString, "" },
		{ kBattleItemReviveItemString, "" },
		{ kNormalItemHealItemString, "" },
		{ kNormalItemHealMpItemString, "" },

		//afk->catch
		{ kBattleCatchPetNameString, "" },
		{ kBattleCatchCharItemString, "" },
		{ kDropPetNameString, "" },

		{ kAutoFunNameString, "" },

		//other->lockpet
		{ kLockPetScheduleString, "" },

		{ kGameAccountString, "" },
		{ kGamePasswordString, "" },
		{ kGameSecurityCodeString, "" },

		{ kMailWhiteListString , "" },

		{ kEOCommandString, "/EO" },

	};
};
