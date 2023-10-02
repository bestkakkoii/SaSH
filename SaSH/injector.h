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

class StringListModel;
class Injector : public QObject, public Indexer
{
private:
	static util::SafeHash<qint64, Injector*> instances;

	explicit Injector(qint64 index);

public:
	virtual ~Injector();

	static Injector& getInstance(qint64 index)
	{
		if (!instances.contains(index))
		{
			Injector* instance = new Injector(index);
			Q_ASSERT(instance != nullptr);

			if (instance != nullptr)
				instances.insert(index, instance);

		}
		return *instances.value(index);
	}

public:
	static void reset();
	static void reset(qint64 index);

	inline void setIndex(qint64 index) override
	{
		if (!server.isNull())
			server->setIndex(index);

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
		qint64 dwProcessId = NULL;
		qint64 dwThreadId = NULL;
		HWND hWnd = nullptr;
	} process_information_t, * pprocess_information_t, * lpprocess_information_t;

	inline void close() const { if (processHandle_) MINT::NtTerminateProcess(processHandle_, 0); }

	Q_REQUIRED_RESULT inline HANDLE getProcess() const { return processHandle_; }

	Q_REQUIRED_RESULT inline HWND getProcessWindow() const { return pi_.hWnd; }

	Q_REQUIRED_RESULT inline qint64 getProcessId() const { return pi_.dwProcessId; }

	Q_REQUIRED_RESULT inline qint64 getProcessModule() const { return hGameModule_; }

	Q_REQUIRED_RESULT inline bool isValid() const { return hGameModule_ != NULL && pi_.dwProcessId != NULL && pi_.hWnd != nullptr && processHandle_.isValid(); }

	CreateProcessResult createProcess(process_information_t& pi);

	bool injectLibrary(process_information_t& pi, unsigned short port, util::LPREMOVE_THREAD_REASON pReason);

	Q_REQUIRED_RESULT bool isWindowAlive() const;

	qint64 sendMessage(qint64 msg, qint64 wParam, qint64 lParam) const;

	bool postMessage(qint64 msg, qint64 wParam, qint64 lParam) const;

	inline void setValueHash(util::UserSetting setting, qint64 value) { userSetting_value_hash_.insert(setting, value); }

	inline void setEnableHash(util::UserSetting setting, bool enable) { userSetting_enable_hash_.insert(setting, enable); }

	inline void setStringHash(util::UserSetting setting, const QString& string) { userSetting_string_hash_.insert(setting, string); }

	Q_REQUIRED_RESULT inline qint64 getValueHash(util::UserSetting setting) const { return userSetting_value_hash_.value(setting); }

	Q_REQUIRED_RESULT inline bool getEnableHash(util::UserSetting setting) const { return userSetting_enable_hash_.value(setting); }

	Q_REQUIRED_RESULT inline QString getStringHash(util::UserSetting setting) const { return userSetting_string_hash_.value(setting); }

	Q_REQUIRED_RESULT inline QHash<util::UserSetting, qint64> getValuesHash() const { return userSetting_value_hash_.toHash(); }

	Q_REQUIRED_RESULT inline QHash<util::UserSetting, bool> getEnablesHash() const { return userSetting_enable_hash_.toHash(); }

	Q_REQUIRED_RESULT inline QHash<util::UserSetting, QString> getStringsHash() const { return userSetting_string_hash_.toHash(); }

	inline void setValuesHash(const QHash<util::UserSetting, qint64>& hash) { userSetting_value_hash_ = hash; }

	inline void setEnablesHash(const QHash<util::UserSetting, bool>& hash) { userSetting_enable_hash_ = hash; }

	inline void setStringsHash(const QHash<util::UserSetting, QString>& hash) { userSetting_string_hash_ = hash; }

	Q_REQUIRED_RESULT inline QVariant getUserData(util::UserData type) const { return userData_hash_.value(type); }

	inline void setUserData(util::UserData type, const QVariant& data) { userData_hash_.insert(type, QVariant::fromValue(data)); }

	void mouseMove(qint64 x, qint64 y) const;

	void leftClick(qint64 x, qint64 y) const;

	void leftDoubleClick(qint64 x, qint64 y) const;

	void rightClick(qint64 x, qint64 y) const;

	void dragto(qint64 x1, qint64 y1, qint64 x2, qint64 y2) const;

	void hide(qint64 mode = 0);

	void show();

	QString getPointFileName();

	inline void setParentWidget(HWND parentWidget) { parentWidget_ = parentWidget; }

	Q_REQUIRED_RESULT inline HWND getParentWidget() const { return parentWidget_; }

private:
	static BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam)
	{
		lpprocess_information_t data = reinterpret_cast<lpprocess_information_t>(lParam);
		DWORD dwProcessId = 0;
		do
		{
			if (!handle || !lParam) break;

			::GetWindowThreadProcessId(handle, &dwProcessId);
			if (data->dwProcessId == static_cast<qint64>(dwProcessId) && IsWindowVisible(handle))
			{
				data->hWnd = handle;
				return FALSE;
			}
		} while (false);
		return TRUE;
	}

	Q_REQUIRED_RESULT bool isHandleValid(qint64 pid);

	DWORD WINAPI getFunAddr(const DWORD* DllBase, const char* FunName);

public:
	QString currentGameExePath;//當前使用的遊戲進程完整路徑

	QSharedPointer<Server> server;//與遊戲TCP通信專用

	std::atomic_bool IS_SCRIPT_FLAG = false;//主腳本是否運行
	std::atomic_bool IS_SCRIPT_INTERRUPT = false;//主腳本是否中斷

	QString currentScriptFileName;//當前運行的主腳本完整路徑

	QStack<QString> scriptFileNameStack;//腳本堆疊

	QSharedPointer<StringListModel> scriptLogModel; //腳本日誌模型

	QSharedPointer<StringListModel> chatLogModel; //聊天日誌模型

	QMutex globalMutex; //用於保證 主線程 | 收包線程 | 腳本線程 數據同步的主要鎖

	util::SafeData<QStringList> serverNameList;

	util::SafeData<QStringList> subServerNameList;

	qint64 currentServerListIndex = 0;

	std::atomic_bool isScriptDebugModeEnable = false;

	std::atomic_bool isScriptEditorOpened = false;

	quint64 scriptThreadId = 0;

	Autil autil;

	bool IS_INJECT_OK = false;//是否注入成功
private:
	quint64 hGameModule_ = NULL;
	HMODULE hookdllModule_ = NULL;
	process_information_t pi_ = {};
	ScopedHandle processHandle_;
	HWND parentWidget_ = nullptr;//主窗口句柄


	qint64 nowChatRowCount_ = 0;

	util::SafeHash<util::UserData, QVariant> userData_hash_ = {
		{ util::kUserItemNames, QStringList() },

	};

	util::SafeHash<util::UserSetting, qint64> userSetting_value_hash_ = {
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



		{ util::kScriptDebugModeEnable, false },

	};

	util::SafeHash<util::UserSetting, QString> userSetting_string_hash_ = {
		{ util::kAutoDropItemString, "" },
		{ util::kLockAttackString, "" },
		{ util::kLockEscapeString, "" },

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

	};
};
