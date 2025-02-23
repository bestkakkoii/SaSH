﻿/*
				GNU GENERAL PUBLIC LICENSE
				   Version 2, June 1991
COPYRIGHT (C) Bestkakkoii 2024 All Rights Reserved.
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
#include <QObject>
#include "net/tcpserver.h"
#include <indexer.h>
#include <model/scopedhandle.h>
#include <util.h>
#include "net/autil.h"
#include "model/logger.h"

class StringListModel;
class GameDevice : public QObject, public Indexer
{
private:
	static safe::hash<long long, GameDevice*> instances;

	explicit GameDevice(long long index);

public:
	virtual ~GameDevice();

	static GameDevice& getInstance(long long index)
	{
		if (!instances.contains(index))
		{
			GameDevice* instance = q_check_ptr(new GameDevice(index));
			sash_assume(instance != nullptr);
			if (instance != nullptr)
				instances.insert(index, instance);

		}
		return *instances.value(index);
	}

	static bool contains(long long index)
	{
		return instances.contains(index);
	}

	static bool get(long long index, GameDevice** ppinstance)
	{
		if (!instances.contains(index))
			return false;

		if (ppinstance != nullptr)
			*ppinstance = instances.value(index);
		return true;
	}

	static void reset();
	static void reset(long long index);
public:
	virtual inline void setIndex(long long index) override
	{
		if (!worker.isNull())
			worker->setIndex(index);

		setIndex(index);
	}

public:
	[[nodiscard]] inline bool __fastcall isGameInterruptionRequested() const
	{
		return isGameInterruptionRequested_.get();
	}

	void gameRequestInterruption()
	{
		isGameInterruptionRequested_.on();
	}

	void resetGameInterruptionFlag()
	{
		isGameInterruptionRequested_.off();
	}

private:
	safe::flag isGameInterruptionRequested_ = false;

public:
	enum CreateProcessResult
	{
		CreateAboveWindow8Failed = -2,
		CreateBelowWindow8Failed = -1,
		CreateFail = 0,
		CreateAboveWindow8Success,
		CreateBelowWindow8Success,
		CreateWithExistingProcess,
		CreateWithExistingProcessNoDll,
	};

	typedef struct process_information_s
	{
		long long dwProcessId = NULL;
		long long dwThreadId = NULL;
		HWND hWnd = nullptr;
	} process_information_t, * pprocess_information_t, * lpprocess_information_t;

	inline void __fastcall close() const { if (processHandle_) MINT::NtTerminateProcess(processHandle_, 0); }

	[[nodiscard]] inline HANDLE __fastcall getProcess() const { return processHandle_; }

	[[nodiscard]] inline HWND __fastcall getProcessWindow() const { return pi_.hWnd; }

	[[nodiscard]] inline long long __fastcall getProcessId() const { return pi_.dwProcessId; }

	[[nodiscard]] inline long long __fastcall getProcessModule() const { return hGameModule_; }

	[[nodiscard]] inline bool __fastcall isValid() const { return hGameModule_ != NULL && pi_.dwProcessId != NULL && pi_.hWnd != nullptr && processHandle_.isValid(); }

	CreateProcessResult __fastcall createProcess(process_information_t& pi);

	bool __fastcall remoteInitialize(GameDevice::process_information_t& pi, unsigned short port, util::LPREMOVE_THREAD_REASON pReason);

	bool __fastcall injectLibrary(process_information_t& pi, unsigned short port, util::LPREMOVE_THREAD_REASON pReason, bool bConnectOnly);

	[[nodiscard]] bool __fastcall isWindowAlive() const;

	[[nodiscard]] bool __fastcall IsUIThreadHung() const;

	long long __fastcall sendMessage(long long msg, long long wParam, long long lParam) const;

	bool __fastcall postMessage(long long msg, long long wParam, long long lParam) const;

	inline void __fastcall setValueHash(util::UserSetting setting, long long value) { userSetting_value_hash_.insert(setting, value); }

	inline void __fastcall setEnableHash(util::UserSetting setting, bool enable) { userSetting_enable_hash_.insert(setting, enable); }

	inline void __fastcall setStringHash(util::UserSetting setting, const QString& string) { userSetting_string_hash_.insert(setting, string); }

	[[nodiscard]] inline long long __fastcall getValueHash(util::UserSetting setting) const { return userSetting_value_hash_.value(setting); }

	[[nodiscard]] inline bool __fastcall getEnableHash(util::UserSetting setting) const { return userSetting_enable_hash_.value(setting); }

	[[nodiscard]] inline QString __fastcall getStringHash(util::UserSetting setting) const { return userSetting_string_hash_.value(setting); }

	[[nodiscard]] inline QHash<util::UserSetting, long long> __fastcall getValuesHash() const { return userSetting_value_hash_.toHash(); }

	[[nodiscard]] inline QHash<util::UserSetting, bool> __fastcall getEnablesHash() const { return userSetting_enable_hash_.toHash(); }

	[[nodiscard]] inline QHash<util::UserSetting, QString> __fastcall getStringsHash() const { return userSetting_string_hash_.toHash(); }

	inline void __fastcall setValuesHash(const QHash<util::UserSetting, long long>& hash) { userSetting_value_hash_ = hash; }

	inline void __fastcall setEnablesHash(const QHash<util::UserSetting, bool>& hash) { userSetting_enable_hash_ = hash; }

	inline void __fastcall setStringsHash(const QHash<util::UserSetting, QString>& hash) { userSetting_string_hash_ = hash; }

	[[nodiscard]] inline QVariant __fastcall getUserData(util::UserData type) const { return userData_hash_.value(type); }

	inline void __fastcall setUserData(util::UserData type, const QVariant& data) { userData_hash_.insert(type, QVariant::fromValue(data)); }

	void __fastcall mouseMove(long long x, long long y) const;

	void __fastcall leftClick(long long x, long long y) const;

	void __fastcall leftDoubleClick(long long x, long long y) const;

	void __fastcall rightClick(long long x, long long y) const;

	void __fastcall dragto(long long x1, long long y1, long long x2, long long y2) const;

	bool __fastcall capture(const QString& fileName) const;

	void __fastcall hide(long long mode = 0) const;

	void __fastcall show() const;

	QString __fastcall getPointFileName() const;

	QString __fastcall ecbDecrypt(long long address) const;

	QByteArray __fastcall ecbEncrypt(const QString& str) const;

	inline void __fastcall setParentWidget(HWND parentWidget) { parentWidget_ = parentWidget; }

	[[nodiscard]] inline HWND __fastcall getParentWidget() const { return parentWidget_; }

	inline bool __fastcall isScriptPaused() const { return isScriptPaused_.get(); }

	void __fastcall checkScriptPause();

	void __fastcall paused();

	void __fastcall resumed();

	inline void __fastcall stopScript() { IS_SCRIPT_INTERRUPT.on(); }

private:
	static BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam)
	{
		lpprocess_information_t data = reinterpret_cast<lpprocess_information_t>(lParam);
		DWORD dwProcessId = 0;
		do
		{
			if (nullptr == handle || NULL == lParam)
				break;

			if (IsWindowVisible(handle) == FALSE || IsWindow(handle) == FALSE || IsWindowEnabled(handle) == FALSE)
				break;

			// check is not console
			if (util::checkAND(GetWindowLongW(handle, GWL_STYLE), WS_CHILD))
				break;

			::GetWindowThreadProcessId(handle, &dwProcessId);
			if (data->dwProcessId == static_cast<long long>(dwProcessId))
			{
				data->hWnd = handle;
				return FALSE;
			}
		} while (false);
		return TRUE;
	}

	[[nodiscard]] bool __fastcall isHandleValid(long long pid);

#if 0
	DWORD WINAPI __fastcall getFunAddr(const DWORD* DllBase, const char* FunName);
#endif

	bool __fastcall isWin7OrLower() const { return isWin7OrLower_.get(); }

public:
	safe::flag IS_INJECT_OK = false;//是否注入成功

	safe::flag IS_TCP_CONNECTION_OK_TO_USE = false;

	safe::flag IS_SCRIPT_EDITOR_OPENED = false;

	QString currentGameExePath;//當前使用的遊戲進程完整路徑

	StringListModel scriptLogModel; //腳本日誌模型

	StringListModel chatLogModel; //聊天日誌模型

	safe::data<QStringList> serverNameList;

	safe::data<QStringList> subServerNameList;

	unsigned long long scriptThreadId = 0;

	static Server server;//與遊戲TCP通信專用，所有實例共用Server，但分派不同線程給Client socket
	QScopedPointer<Worker> worker;

	Autil autil;

	Logger log;

	safe::hash<QString, safe::hash<long long, break_marker_t>> break_markers;//interpreter.cpp//用於標記自訂義中斷點(紅點)
	safe::hash<QString, safe::hash<long long, break_marker_t>> forward_markers;//interpreter.cpp//用於標示當前執行中斷處(黃箭頭)
	safe::hash<QString, safe::hash<long long, break_marker_t>> error_markers;//interpreter.cpp//用於標示錯誤發生行(紅線)
	safe::hash<QString, safe::hash<long long, break_marker_t>> step_markers;//interpreter.cpp//隱式標記中斷點用於單步執行(無)

public:
	safe::flag IS_SCRIPT_FLAG = false;//主腳本是否運行 //DO NOT RESET!!!

	safe::flag IS_SCRIPT_INTERRUPT = false;//主腳本是否中斷 //DO NOT RESET!!!

	safe::flag IS_FINDINGPATH = false;//DO NOT RESET!!!

	safe::flag IS_SCRIPT_DEBUG_ENABLE = false;//DO NOT RESET!!!

	safe::integer currentServerListIndex = 0;//DO NOT RESET!!!

	safe::data<QString> currentScriptFileName;//當前運行的主腳本完整路徑 //DO NOT RESET!!!

private:
	unsigned long long hGameModule_ = 0x400000ULL;
	process_information_t pi_ = {};
	ScopedHandle processHandle_;
	HWND parentWidget_ = nullptr;//主窗口句柄
	HMODULE hookdllModule_ = nullptr;

	safe::flag isWin7OrLower_ = false;

private:
	safe::flag isScriptPaused_ = false;
	std::condition_variable scriptPausedCondition_;
	std::mutex scriptPausedMutex_;

private:
	safe::hash<util::UserData, QVariant> userData_hash_ = {
		{ util::kUserItemNames, QStringList() },

	};

	safe::hash<util::UserSetting, long long> userSetting_value_hash_ = {
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

		{ util::kBattlePetHealTargetValue, util::kSelectSelf | util::kSelectPet },
		{ util::kBattlePetPurgTargetValue, util::kSelectSelf | util::kSelectPet },
		{ util::kBattleCharPurgTargetValue, util::kSelectSelf | util::kSelectPet },

		//afk->heal combobox
		{ util::kBattleMagicHealMagicValue, 0 },
		{ util::kBattleMagicReviveMagicValue, 0 },

		{ util::kNormalMagicHealMagicValue, 0 },

		{ util::kBattlePetHealActionTypeValue, 0 },
		{ util::kBattlePetPurgActionTypeValue, 0 },
		{ util::kBattleCharPurgActionTypeValue, 0 },
		{ util::kBattlePetHealTargetValue, 0 },
		{ util::kBattlePetPurgTargetValue, 0 },
		{ util::kBattleCharPurgTargetValue, 0 },
		{ util::kBattlePetHealCharValue, 0 },
		{ util::kBattlePetHealPetValue, 0 },
		{ util::kBattlePetHealAllieValue, 0 },

		//afk->heal spinbox
		{ util::kBattleMagicHealCharValue, 50 },
		{ util::kBattleMagicHealPetValue, 50 },
		{ util::kBattleMagicHealAllieValue, 50 },
		{ util::kBattleItemHealCharValue, 50 },
		{ util::kBattleItemHealPetValue, 50 },
		{ util::kBattleItemHealAllieValue, 50 },
		{ util::kBattleItemHealMpValue, 50 },
		{ util::kBattleSkillMpValue, 30 },

		{ util::kNormalMagicHealCharValue, 50 },
		{ util::kNormalMagicHealPetValue, 50 },
		{ util::kNormalMagicHealAllieValue, 50 },
		{ util::kNormalItemHealCharValue, 50 },
		{ util::kNormalItemHealPetValue, 50 },
		{ util::kNormalItemHealAllieValue, 50 },
		{ util::kNormalItemHealMpValue, 50 },

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
		{ util::kBattleCatchCharMagicValue, 0 },
		{ util::kBattleCatchPetSkillValue, 0 },

		{ util::kBattleActionDelayValue, 0 },
		{ util::kBattleResendDelayValue, 3000 },

		{ util::kAutoResetCharObjectCountValue, -1 },

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

		{ util::kTcpDelayValue, 0 },

		{ util::kSettingMaxValue, util::kSettingMaxValue },
		{ util::kSettingMinString, util::kSettingMinString },
		{ util::kSettingMaxString, util::kSettingMaxString },

	};

	safe::hash<util::UserSetting, bool> userSetting_enable_hash_ = {
		{ util::kAutoLoginEnable, false },
		{ util::kAutoReconnectEnable, true },

		{ util::kHideCharacterEnable, false },
		{ util::kCloseEffectEnable, true },
		{ util::kAutoStartScriptEnable, false },
		{ util::kHideWindowEnable, false },
		{ util::kMuteEnable, false },
		{ util::kAutoJoinEnable, false },
		{ util::kLockTimeEnable, false },
		{ util::kAutoRestartGameEnable, false },
		{ util::kFastWalkEnable, true },
		{ util::kPassWallEnable, false },
		{ util::kLockMoveEnable, false },
		{ util::kLockImageEnable, false },
		{ util::kAutoDropMeatEnable, true },
		{ util::kAutoDropEnable, false },
		{ util::kAutoStackEnable, true },
		{ util::kKNPCEnable, true },
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
		{ util::kBattleTimeExtendEnable, true },
		{ util::kFallDownEscapeEnable, false },
		{ util::kShowExpEnable, true },
		{ util::kWindowDockEnable, false },
		{ util::kBattleAutoSwitchEnable, true },
		{ util::kBattleAutoEOEnable, true },
		{ util::kBattleLuaModeEnable, false },

		//switcher
		{ util::kSwitcherTeamEnable, false },
		{ util::kSwitcherPKEnable, false },
		{ util::kSwitcherCardEnable, false },
		{ util::kSwitcherTradeEnable, false },
		{ util::kSwitcherGroupEnable, false },
		{ util::kSwitcherFamilyEnable, false },
		{ util::kSwitcherJobEnable, false },
		{ util::kSwitcherWorldEnable, false },

		//afk->battle
		{ util::kBattleCrossActionCharEnable, false },
		{ util::kBattleCrossActionPetEnable, false },

		//afk->heal
		{ util::kBattleMagicHealEnable, false },
		{ util::kBattleItemHealEnable, false },
		{ util::kBattleItemHealMeatPriorityEnable, false },
		{ util::kBattleItemHealMpEnable, false },
		{ util::kBattleMagicReviveEnable, false },
		{ util::kBattleItemReviveEnable, false },
		{ util::kBattleSkillMpEnable, false },

		{ util::kNormalMagicHealEnable, false },
		{ util::kNormalItemHealEnable, false },
		{ util::kNormalItemHealMeatPriorityEnable, false },
		{ util::kNormalItemHealMpEnable, false },

		//afk->catch
		{ util::kBattleCatchTargetLevelEnable, false },
		{ util::kBattleCatchTargetMaxHpEnable, false },
		{ util::kBattleCatchCharMagicEnable, false },
		{ util::kBattleCatchCharItemEnable, false },
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
		{ util::kBattleNoEscapeWhileLockPetEnable, false },

		{ util::kAutoEncodeEnable, false },

		{ util::kForwardSendEnable, false},

		{ util::kScriptDebugModeEnable, false },

	};

	safe::hash<util::UserSetting, QString> userSetting_string_hash_ = {
		{ util::kAutoDropItemString, "" },
		{ util::kLockAttackString, "" },
		{ util::kLockEscapeString, "" },

		{ util::kNormalMagicHealItemString, "" },
		{ util::kBattleItemHealItemString, "" },
		{ util::kBattleItemHealMpItemString, "" },
		{ util::kBattleItemReviveItemString, "" },
		{ util::kNormalItemHealItemString, "" },
		{ util::kNormalItemHealMpItemString, "" },

		//afk->catch
		{ util::kBattleCatchPetNameString, "" },
		{ util::kBattleCatchCharItemString, "" },
		{ util::kDropPetNameString, "" },

		{ util::kAutoFunNameString, "" },

		//other->lockpet
		{ util::kLockPetScheduleString, "" },

		{ util::kGameAccountString, "" },
		{ util::kGamePasswordString, "" },
		{ util::kGameSecurityCodeString, "" },

		{ util::kMailWhiteListString , "" },

		{ util::kEOCommandString, "/EO" },

		{ util::kTitleFormatString, "[%(index)] [%(sser):%(pos)] %(name) Lv:%(lv) Hp:%(hpp)" },
		{ util::kBattleAllieFormatString, "[%(pos)]%(self)%(name) LV:%(lv)(%(hp)|%(hpp))[|%(status)]" },
		{ util::kBattleEnemyFormatString, "[%(pos)]%(mod):%(name) LV:%(lv)(%(hp)|%(hpp))[|%(status)]" },
		{ util::kBattleSelfMarkString, "🔷" },
		{ util::kBattleActMarkString, "⭕" },
		{ util::kBattleSpaceMarkString, "　" },

		{ util::kBattleActionOrderString, "1|2|3|4|5|6|7|8|9|10|11|12|13|14" },

		{ util::kAutoAbilityString, "" },

		{ util::kKNPCListString, "如果能贏過我的,auto|如果想通過,auto|吼,auto|你想找麻煩,auto|多謝～。,auto|轉以上確定要出售？,auto|再度光臨,auto|已經太多,auto|如果能赢过我的,auto|如果想通过,auto|你想找麻烦,auto|多謝～。,auto|转以上确定要出售？,auto|再度光临,auto|已经太多|auto" },

		{ util::kBattleCharLuaFilePathString , "battle_char" },
		{ util::kBattlePetLuaFilePathString , "battle_pet" },
	};
};

class KNPCDevice
{
public:
	enum Type
	{
		Button,
		RowButton,
		Inout,
	};

	struct action_t
	{
		QString cmpText;
		QVariant action;
		Type type;
	};

	KNPCDevice(const QString& tests)
	{
		//divid util::kKNPCListString if is find in sa::ButtonType button = sa::buttonMap.value(text.toUpper(), sa::kButtonNone); or pure number then type as button alse type as inout
		QStringList actionStrs = tests.split(util::rexOR, Qt::SkipEmptyParts);

		for (const QString& actionStr : actionStrs)
		{
			//spilt by ,
			QStringList pair = actionStr.split(util::rexComma, Qt::SkipEmptyParts);
			if (pair.size() != 2)
				continue;

			action_t action;
			action.cmpText = pair.front();
			sa::ButtonType button = sa::buttonMap.value(pair.back().toUpper(), sa::kButtonNone);
			if (button != sa::kButtonNone)
			{
				action.action = static_cast<long long>(button);
				action.type = Type::Button;
				actions_.append(action);
				continue;
			}

			//check is pure number
			bool isNumber = false;
			long long number = pair.back().toLongLong(&isNumber);
			if (isNumber)
			{
				if (number <= 0)
					continue;

				action.action = number - 1;
				action.type = Type::RowButton;
				actions_.append(action);
				continue;
			}

			action.action = pair.back();
			action.type = Type::Inout;
			actions_.append(action);
		}
	}

	~KNPCDevice() = default;

	//begin
	QVector<action_t>::const_iterator cbegin() const
	{
		return actions_.cbegin();
	}

	QVector<action_t>::iterator begin()
	{
		return actions_.begin();
	}

	//end
	QVector<action_t>::const_iterator cend() const
	{
		return actions_.cend();
	}

	QVector<action_t>::iterator end()
	{
		return actions_.end();
	}
private:
	QVector<action_t> actions_;
};
