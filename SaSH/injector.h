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
#include <QObject>
#include "net/tcpserver.h"
#include <indexer.h>
#include <model/scopedhandle.h>
#include <util.h>
#include "net/autil.h"
#include "model/logger.h"

class StringListModel;
class Injector : public QObject, public Indexer
{
private:
	static util::SafeHash<long long, Injector*> instances;

	explicit Injector(long long index);

public:
	virtual ~Injector();

	static Injector& getInstance(long long index)
	{
		if (!instances.contains(index))
		{
			Injector* instance = q_check_ptr(new Injector(index));
			Q_ASSERT(instance != nullptr);

			if (instance != nullptr)
				instances.insert(index, instance);

		}
		return *instances.value(index);
	}

	static bool get(long long index, Injector** ppinstance)
	{
		if (!instances.contains(index))
			return false;

		if (ppinstance != nullptr)
			*ppinstance = instances.value(index);
		return true;
	}

public:
	static void reset();
	static void reset(long long index);

	virtual inline void setIndex(long long index) override
	{
		if (!worker.isNull())
			worker->setIndex(index);

		Indexer::setIndex(index);
	}

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

	bool __fastcall injectLibrary(process_information_t& pi, unsigned short port, util::LPREMOVE_THREAD_REASON pReason);

	[[nodiscard]] bool __fastcall isWindowAlive() const;

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

	void __fastcall hide(long long mode = 0);

	void __fastcall show();

	QString __fastcall getPointFileName();

	inline void __fastcall setParentWidget(HWND parentWidget) { parentWidget_ = parentWidget; }

	[[nodiscard]] inline HWND __fastcall getParentWidget() const { return parentWidget_; }

private:
	static BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam)
	{
		lpprocess_information_t data = reinterpret_cast<lpprocess_information_t>(lParam);
		DWORD dwProcessId = 0;
		do
		{
			if (!handle || !lParam)
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

public:
	QString currentGameExePath;//當前使用的遊戲進程完整路徑

	static Server server;//與遊戲TCP通信專用
	QSharedPointer<Worker> worker;

	std::atomic_bool IS_SCRIPT_FLAG = false;//主腳本是否運行
	std::atomic_bool IS_SCRIPT_INTERRUPT = false;//主腳本是否中斷
	std::atomic_bool IS_FINDINGPATH = false;

	QString currentScriptFileName;//當前運行的主腳本完整路徑

	StringListModel scriptLogModel; //腳本日誌模型

	StringListModel chatLogModel; //聊天日誌模型

	util::SafeData<QStringList> serverNameList;

	util::SafeData<QStringList> subServerNameList;

	long long currentServerListIndex = 0;

	std::atomic_bool isScriptDebugModeEnable = false;

	std::atomic_bool isScriptEditorOpened = false;

	unsigned long long scriptThreadId = 0;

	Autil autil;

	Logger log;

	util::SafeHash<QString, util::SafeHash<long long, break_marker_t>> break_markers;//interpreter.cpp//用於標記自訂義中斷點(紅點)
	util::SafeHash<QString, util::SafeHash<long long, break_marker_t>> forward_markers;//interpreter.cpp//用於標示當前執行中斷處(黃箭頭)
	util::SafeHash<QString, util::SafeHash<long long, break_marker_t>> error_markers;//interpreter.cpp//用於標示錯誤發生行(紅線)
	util::SafeHash<QString, util::SafeHash<long long, break_marker_t>> step_markers;//interpreter.cpp//隱式標記中斷點用於單步執行(無)

	bool IS_INJECT_OK = false;//是否注入成功
	std::atomic_bool IS_TCP_CONNECTION_OK_TO_USE = false;
private:
	unsigned long long hGameModule_ = NULL;
	HMODULE hookdllModule_ = NULL;
	process_information_t pi_ = {};
	ScopedHandle processHandle_;
	HWND parentWidget_ = nullptr;//主窗口句柄


	long long nowChatRowCount_ = 0;

	util::SafeHash<util::UserData, QVariant> userData_hash_ = {
		{ util::kUserItemNames, QStringList() },

	};

	util::SafeHash<util::UserSetting, long long> userSetting_value_hash_ = {
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
		{ util::kBattleResendDelayValue, 3000},

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

		{ util::kTcpDelayValue, 10 },

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
		{ util::kCloseEffectEnable, true },
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

		{ util::kScriptDebugModeEnable, false },

	};

	util::SafeHash<util::UserSetting, QString> userSetting_string_hash_ = {
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
		{ util::kBattleAllieFormatString, "[%(pos)]%(self)%(name) LV:%(lv)(%(hp)|%(hpp))[%(status)]" },
		{ util::kBattleEnemyFormatString, "[%(pos)]%(mod):%(name) LV:%(lv)(%(hp)|%(hpp))[%(status)]" },
		{ util::kBattleSelfMarkString, "★" },
		{ util::kBattleActMarkString, "＊" },
		{ util::kBattleSpaceMarkString, "　" },

		{ util::kBattleActionOrderString, "1|2|3|4|5|6|7|8|9|10|11|12|13|14" },

	};
};
