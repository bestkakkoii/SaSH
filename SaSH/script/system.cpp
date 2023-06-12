#include "stdafx.h"
#include "interpreter.h"
#include "util.h"
#include "injector.h"

#include "signaldispatcher.h"


int Interpreter::sleep(const TokenMap& TK)
{
	int t;
	if (!checkInt(TK, 1, &t))
		return Parser::kArgError;

	if (t >= 1000u)
	{
		int i = 0;
		for (; i < t / 1000; ++i)
		{
			QThread::msleep(1000UL);
			QThread::yieldCurrentThread();
			if (isInterruptionRequested())
				break;
		}
		QThread::msleep(static_cast<DWORD>(i) % 1000UL);
	}
	else
		QThread::msleep(static_cast<DWORD>(t));

	return Parser::kNoChange;
}

int Interpreter::press(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	QString text;
	checkString(TK, 1, &text);

	int row = 0;
	checkInt(TK, 1, &row);

	QString npcName;
	int npcId = -1;
	checkString(TK, 2, &npcName);
	mapunit_t unit;
	if (!npcName.isEmpty() && injector.server->findUnit(npcName, util::OBJ_NPC, &unit))
	{
		npcId = unit.id;
	}

	int dialogid = -1;
	checkInt(TK, 3, &dialogid);

	if (text.isEmpty() && row == 0)
		return Parser::kArgError;

	if (!text.isEmpty())
	{
		BUTTON_TYPE button = buttonMap.value(text.toUpper(), BUTTON_NOTUSED);
		if (button != BUTTON_NOTUSED)
			injector.server->press(button, dialogid, npcId);
		else
		{
			dialog_t dialog = injector.server->currentDialog;
			QStringList textList = dialog.linebuttontext;
			if (!textList.isEmpty())
			{
				for (int i = 0; i < textList.size(); ++i)
				{
					if (textList.at(i).toUpper().contains(text))
					{
						injector.server->press(i + 1, dialogid, npcId);
						break;
					}
				}
			}
		}
	}
	else if (row > 0)
		injector.server->press(row, dialogid, npcId);

	return Parser::kNoChange;
}

int Interpreter::eo(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	injector.server->EO();

	return Parser::kNoChange;
}

int Interpreter::announce(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	QString text;
	if (!checkString(TK, 1, &text))
	{
		int value = 0;
		if (!checkInt(TK, 1, &value))
			return Parser::kArgError;

		text = QString::number(value);
	}

	int color = 4;
	checkInt(TK, 2, &color);

	injector.server->announce(text, color);
	return Parser::kNoChange;
}

int Interpreter::input(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	QString text;
	if (!checkString(TK, 1, &text))
	{
		int value = 0;
		if (!checkInt(TK, 1, &value))
			return Parser::kArgError;

		text = QString::number(value);
	}

	injector.server->inputtext(text);

	return Parser::kNoChange;
}

int Interpreter::messagebox(const TokenMap& TK)
{
	QString text;
	if (!checkString(TK, 1, &text))
	{
		int value = 0;
		if (!checkInt(TK, 2, &value))
			return Parser::kArgError;

		text = QString::number(value);
	}

	int type = 0;
	checkInt(TK, 2, &type);

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	emit signalDispatcher.messageBoxShow(text, type);

	return Parser::kNoChange;
}

int Interpreter::talk(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	QString text;
	if (!checkString(TK, 1, &text))
		return Parser::kArgError;

	int color = 4;
	checkInt(TK, 2, &color);

	injector.server->talk(text, color);

	return Parser::kNoChange;
}

int Interpreter::talkandannounce(const TokenMap& TK)
{
	announce(TK);
	return talk(TK);
}

int Interpreter::logout(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();
	if (!injector.server.isNull())
		injector.server->logOut();

	return Parser::kNoChange;
}

int Interpreter::logback(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	injector.server->logBack();

	return Parser::kNoChange;
}

int Interpreter::cleanchat(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();
	if (!injector.server.isNull())
		injector.server->cleanChatHistory();

	return Parser::kNoChange;
}

int Interpreter::savesetting(const TokenMap& TK)
{
	QString fileName;
	if (!checkString(TK, 1, &fileName))
	{
		if (TK.value(1).type == TK_FUZZY)
		{
			Injector& injector = Injector::getInstance();
			if (injector.server.isNull())
				return Parser::kError;

			fileName = injector.server->pc.name;
		}
	}

	fileName.replace("\\", "/");

	fileName = QCoreApplication::applicationDirPath() + "/settings/" + fileName;
	fileName.replace("\\", "/");
	fileName.replace("//", "/");

	QFileInfo fileInfo(fileName);
	QString suffix = fileInfo.suffix();
	if (suffix.isEmpty())
		fileName += ".json";
	else if (suffix != "json")
	{
		fileName.replace(suffix, "json");
	}

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	emit signalDispatcher.saveHashSettings(fileName, true);

	return Parser::kNoChange;
}

int Interpreter::loadsetting(const TokenMap& TK)
{
	QString fileName;
	if (!checkString(TK, 1, &fileName))
	{
		if (TK.value(1).type == TK_FUZZY)
		{
			Injector& injector = Injector::getInstance();
			if (injector.server.isNull())
				return Parser::kError;

			fileName = injector.server->pc.name;
		}
	}

	fileName.replace("\\", "/");

	fileName = QCoreApplication::applicationDirPath() + "/settings/" + fileName;
	fileName.replace("\\", "/");
	fileName.replace("//", "/");

	QFileInfo fileInfo(fileName);
	QString suffix = fileInfo.suffix();
	if (suffix.isEmpty())
		fileName += ".json";
	else if (suffix != "json")
	{
		fileName.replace(suffix, "json");
	}

	if (!QFile::exists(fileName))
		return Parser::kArgError;

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	emit signalDispatcher.loadHashSettings(fileName, true);

	return Parser::kNoChange;
}

int Interpreter::set(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();

	QString typeStr;
	if (!checkString(TK, 1, &typeStr))
		return Parser::kError;

	const QHash<QString, util::UserSetting> hash = {
#pragma region BIG5
		{ u8"主機", util::kServerValue },
		{ u8"副機", util::kSubServerValue },
		{ u8"位置", util::kPositionValue },
		{ u8"時間", util::kLockTimeValue },
		{ u8"加速", util::kSpeedBoostValue },
		{ u8"BattleCharRoundActionRoundValue", util::kBattleCharRoundActionRoundValue },
		{ u8"BattleCharRoundActionTypeValue", util::kBattleCharRoundActionTypeValue },
		{ u8"BattleCharRoundActionEnemyValue", util::kBattleCharRoundActionEnemyValue },
		{ u8"BattleCharRoundActionLevelValue", util::kBattleCharRoundActionLevelValue },
		{ u8"BattleCharCrossActionTypeValue", util::kBattleCharCrossActionTypeValue },
		{ u8"BattleCharCrossActionRoundValue", util::kBattleCharCrossActionRoundValue },
		{ u8"BattleCharNormalActionTypeValue", util::kBattleCharNormalActionTypeValue },
		{ u8"BattleCharNormalActionEnemyValue", util::kBattleCharNormalActionEnemyValue },
		{ u8"BattleCharNormalActionLevelValue", util::kBattleCharNormalActionLevelValue },
		{ u8"BattlePetRoundActionRoundValue", util::kBattlePetRoundActionRoundValue },
		{ u8"BattlePetRoundActionTypeValue", util::kBattlePetRoundActionTypeValue },
		{ u8"BattlePetRoundActionEnemyValue", util::kBattlePetRoundActionEnemyValue },
		{ u8"BattlePetRoundActionLevelValue", util::kBattlePetRoundActionLevelValue },
		{ u8"BattlePetCrossActionTypeValue", util::kBattlePetCrossActionTypeValue },
		{ u8"BattlePetCrossActionRoundValue", util::kBattlePetCrossActionRoundValue },
		{ u8"BattlePetNormalActionTypeValue", util::kBattlePetNormalActionTypeValue },
		{ u8"BattlePetNormalActionEnemyValue", util::kBattlePetNormalActionEnemyValue },
		{ u8"BattlePetNormalActionLevelValue", util::kBattlePetNormalActionLevelValue },
		{ u8"BattleCharRoundActionTargetValue", util::kBattleCharRoundActionTargetValue },
		{ u8"BattleCharCrossActionTargetValue", util::kBattleCharCrossActionTargetValue },
		{ u8"BattleCharNormalActionTargetValue", util::kBattleCharNormalActionTargetValue },
		{ u8"BattlePetRoundActionTargetValue", util::kBattlePetRoundActionTargetValue },
		{ u8"BattlePetCrossActionTargetValue", util::kBattlePetCrossActionTargetValue },
		{ u8"BattlePetNormalActionTargetValue", util::kBattlePetNormalActionTargetValue },
		{ u8"BattleMagicHealTargetValue", util::kBattleMagicHealTargetValue },
		{ u8"BattleItemHealTargetValue", util::kBattleItemHealTargetValue },
		{ u8"BattleMagicReviveTargetValue", util::kBattleMagicReviveTargetValue },
		{ u8"BattleItemReviveTargetValue", util::kBattleItemReviveTargetValue },
		{ u8"BattleMagicHealMagicValue", util::kBattleMagicHealMagicValue },
		{ u8"BattleMagicReviveMagicValue", util::kBattleMagicReviveMagicValue },
		{ u8"平時精靈補血精靈索引", util::kNormalMagicHealMagicValue },
		{ u8"戰鬥精靈補血人物", util::kMagicHealCharValue },
		{ u8"戰鬥精靈補血戰寵", util::kMagicHealPetValue },
		{ u8"戰鬥精靈補血隊友", util::kMagicHealAllieValue },
		{ u8"戰鬥道具補血人物", util::kItemHealCharValue },
		{ u8"戰鬥道具補血戰寵", util::kItemHealPetValue },
		{ u8"戰鬥道具補血隊友", util::kItemHealAllieValue },
		{ u8"戰鬥道具補氣人物", util::kItemHealMpValue },
		{ u8"MagicHealCharNormalValue", util::kMagicHealCharNormalValue },
		{ u8"MagicHealPetNormalValue", util::kMagicHealPetNormalValue },
		{ u8"MagicHealAllieNormalValue", util::kMagicHealAllieNormalValue },
		{ u8"ItemHealCharNormalValue", util::kItemHealCharNormalValue },
		{ u8"ItemHealPetNormalValue", util::kItemHealPetNormalValue },
		{ u8"ItemHealAllieNormalValue", util::kItemHealAllieNormalValue },
		{ u8"ItemHealMpNormalValue", util::kItemHealMpNormalValue },
		{ u8"AutoWalkDelayValue", util::kAutoWalkDelayValue },
		{ u8"AutoWalkDistanceValue", util::kAutoWalkDistanceValue },
		{ u8"AutoWalkDirectionValue", util::kAutoWalkDirectionValue },
		{ u8"捉寵模式", util::kBattleCatchModeValue },
		{ u8"捉寵目標等級", util::kBattleCatchTargetLevelValue },
		{ u8"捉寵目標最大耐久力", util::kBattleCatchTargetMaxHpValue },
		{ u8"捉寵使用精靈直到血量低於", util::kBattleCatchTargetMagicHpValue },
		{ u8"捉寵使用道具直到血量低於", util::kBattleCatchTargetItemHpValue },
		{ u8"捉寵人物精靈索引", util::kBattleCatchPlayerMagicValue },
		{ u8"捉寵戰寵技能索引", util::kBattleCatchPetSkillValue },
		{ u8"自動丟棄寵物攻數值", util::kDropPetStrValue },
		{ u8"自動丟棄寵物防數值", util::kDropPetDefValue },
		{ u8"自動丟棄寵物敏數值", util::kDropPetAgiValue },
		{ u8"自動丟棄寵物血數值", util::kDropPetHpValue },
		{ u8"自動丟棄寵物攻防敏數值", util::kDropPetAggregateValue },
		{ u8"自動移動功能編號", util::kAutoFunTypeValue },
		{ u8"戰後自動鎖定戰寵編號", util::kLockPetValue },
		{ u8"戰後自動鎖定騎寵編號", util::kLockRideValue },

		{ u8"自動登陸", util::kAutoLoginEnable },
		{ u8"斷線重連", util::kAutoReconnectEnable },
		{ u8"隱藏人物", util::kHideCharacterEnable },
		{ u8"關閉特效", util::kCloseEffectEnable },
		{ u8"資源優化", util::kOptimizeEnable },
		{ u8"隱藏石器", util::kHideWindowEnable },
		{ u8"屏蔽聲音", util::kMuteEnable },
		{ u8"自動組隊", util::kAutoJoinEnable },
		{ u8"鎖定時間", util::kLockTimeEnable },
		{ u8"自動調整記憶體用量", util::kAutoFreeMemoryEnable },
		{ u8"快速走路", util::kFastWalkEnable },
		{ u8"橫衝直撞", util::kPassWallEnable },
		{ u8"鎖定原地", util::kLockMoveEnable },
		{ u8"鎖定畫面", util::kLockImageEnable },
		{ u8"自動丟肉", util::kAutoDropMeatEnable },
		{ u8"自動丟棄", util::kAutoDropEnable },
		{ u8"自動疊加", util::kAutoStackEnable },
		{ u8"自動KNPC", util::kKNPCEnable },
		{ u8"自動猜謎", util::kAutoAnswerEnable },
		{ u8"自動吃豆", util::kAutoEatBeanEnable },
		{ u8"走路遇敵", util::kAutoWalkEnable },
		{ u8"快速遇敵", util::kFastAutoWalkEnable },
		{ u8"快速戰鬥", util::kFastBattleEnable },
		{ u8"自動戰鬥", util::kAutoBattleEnable },
		{ u8"自動捉寵", util::kAutoCatchEnable },
		{ u8"鎖定攻擊", util::kLockAttackEnable },
		{ u8"自動逃跑", util::kAutoEscapeEnable },
		{ u8"鎖定逃跑", util::kLockEscapeEnable },
		{ u8"戰鬥99秒", util::kBattleTimeExtendEnable },
		{ u8"落馬逃跑", util::kFallDownEscapeEnable },

		{ u8"隊伍開關", util::kSwitcherTeamEnable },
		{ u8"PK開關", util::kSwitcherPKEnable },
		{ u8"交名開關", util::kSwitcherCardEnable },
		{ u8"交易開關", util::kSwitcherTradeEnable },
		{ u8"家頻開關", util::kSwitcherFamilyEnable },
		{ u8"職頻開關", util::kSwitcherJobEnable },
		{ u8"世界開關", util::kSwitcherWorldEnable },

		{ u8"CrossActionCharEnable", util::kCrossActionCharEnable },
		{ u8"CrossActionPetEnable", util::kCrossActionPetEnable },
		{ u8"戰鬥精靈補血", util::kBattleMagicHealEnable },
		{ u8"戰鬥道具補血", util::kBattleItemHealEnable },
		{ u8"戰鬥道具補血肉優先", util::kBattleItemHealMeatPriorityEnable },
		{ u8"戰鬥道具補氣", util::kBattleItemHealMpEnable },
		{ u8"戰鬥精靈復活", util::kBattleMagicReviveEnable },
		{ u8"戰鬥道具復活", util::kBattleItemReviveEnable },
		{ u8"平時精靈補血", util::kNormalMagicHealEnable },
		{ u8"平時道具補血", util::kNormalItemHealEnable },
		{ u8"平時道具補血肉優先", util::kNormalItemHealMeatPriorityEnable },
		{ u8"平時道具補氣", util::kNormalItemHealMpEnable },
		{ u8"BattleCatchTargetLevelEnable", util::kBattleCatchTargetLevelEnable },
		{ u8"BattleCatchTargetMaxHpEnable", util::kBattleCatchTargetMaxHpEnable },
		{ u8"BattleCatchPlayerMagicEnable", util::kBattleCatchPlayerMagicEnable },
		{ u8"BattleCatchPlayerItemEnable", util::kBattleCatchPlayerItemEnable },
		{ u8"BattleCatchPetSkillEnable", util::kBattleCatchPetSkillEnable },

		{ u8"自動丟棄寵物", util::kDropPetEnable },
		{ u8"自動丟棄寵物攻", util::kDropPetStrEnable },
		{ u8"自動丟棄寵物防", util::kDropPetDefEnable },
		{ u8"自動丟棄寵物敏", util::kDropPetAgiEnable },
		{ u8"自動丟棄寵物血", util::kDropPetHpEnable },
		{ u8"自動丟棄寵物攻防敏", util::kDropPetAggregateEnable },

		{ u8"戰後自動鎖定戰寵", util::kLockPetEnable },
		{ u8"戰後自動鎖定騎寵", util::kLockRideEnable },

		{ u8"自動丟棄名單", util::kAutoDropItemString },
		{ u8"鎖定攻擊名單", util::kLockAttackString },
		{ u8"鎖定逃跑名單", util::kLockEscapeString },

		{ u8"戰鬥道具補血名單", util::kBattleItemHealItemString },
		{ u8"戰鬥道具補氣名單 ", util::kBattleItemHealMpIteamString },
		{ u8"戰鬥道具復活名單", util::kBattleItemReviveItemString },
		{ u8"平時道具補血名單", util::kNormalItemHealItemString },
		{ u8"平時道具補氣名單", util::kNormalItemHealMpItemString },
		{ u8"捉寵目標名單", util::kBattleCatchPetNameString },
		{ u8"捉寵道具名單", util::kBattleCatchPlayerItemString },

		{ u8"自動丟棄寵物名單", util::kDropPetNameString },
		{ u8"自動組隊名稱", util::kAutoFunNameString },
		{ u8"帳號", util::kGameAccountString },
		{ u8"密碼", util::kGamePasswordString },
		{ u8"安全碼", util::kGameSecurityCodeString },

#pragma endregion

#pragma region GB2312
		{ u8"主机", util::kServerValue },
		{ u8"副机", util::kSubServerValue },
		{ u8"位置", util::kPositionValue },
		{ u8"时间", util::kLockTimeValue },
		{ u8"加速", util::kSpeedBoostValue },
		{ u8"BattleCharRoundActionRoundValue", util::kBattleCharRoundActionRoundValue },
		{ u8"BattleCharRoundActionTypeValue", util::kBattleCharRoundActionTypeValue },
		{ u8"BattleCharRoundActionEnemyValue", util::kBattleCharRoundActionEnemyValue },
		{ u8"BattleCharRoundActionLevelValue", util::kBattleCharRoundActionLevelValue },
		{ u8"BattleCharCrossActionTypeValue", util::kBattleCharCrossActionTypeValue },
		{ u8"BattleCharCrossActionRoundValue", util::kBattleCharCrossActionRoundValue },
		{ u8"BattleCharNormalActionTypeValue", util::kBattleCharNormalActionTypeValue },
		{ u8"BattleCharNormalActionEnemyValue", util::kBattleCharNormalActionEnemyValue },
		{ u8"BattleCharNormalActionLevelValue", util::kBattleCharNormalActionLevelValue },
		{ u8"BattlePetRoundActionRoundValue", util::kBattlePetRoundActionRoundValue },
		{ u8"BattlePetRoundActionTypeValue", util::kBattlePetRoundActionTypeValue },
		{ u8"BattlePetRoundActionEnemyValue", util::kBattlePetRoundActionEnemyValue },
		{ u8"BattlePetRoundActionLevelValue", util::kBattlePetRoundActionLevelValue },
		{ u8"BattlePetCrossActionTypeValue", util::kBattlePetCrossActionTypeValue },
		{ u8"BattlePetCrossActionRoundValue", util::kBattlePetCrossActionRoundValue },
		{ u8"BattlePetNormalActionTypeValue", util::kBattlePetNormalActionTypeValue },
		{ u8"BattlePetNormalActionEnemyValue", util::kBattlePetNormalActionEnemyValue },
		{ u8"BattlePetNormalActionLevelValue", util::kBattlePetNormalActionLevelValue },
		{ u8"BattleCharRoundActionTargetValue", util::kBattleCharRoundActionTargetValue },
		{ u8"BattleCharCrossActionTargetValue", util::kBattleCharCrossActionTargetValue },
		{ u8"BattleCharNormalActionTargetValue", util::kBattleCharNormalActionTargetValue },
		{ u8"BattlePetRoundActionTargetValue", util::kBattlePetRoundActionTargetValue },
		{ u8"BattlePetCrossActionTargetValue", util::kBattlePetCrossActionTargetValue },
		{ u8"BattlePetNormalActionTargetValue", util::kBattlePetNormalActionTargetValue },
		{ u8"BattleMagicHealTargetValue", util::kBattleMagicHealTargetValue },
		{ u8"BattleItemHealTargetValue", util::kBattleItemHealTargetValue },
		{ u8"BattleMagicReviveTargetValue", util::kBattleMagicReviveTargetValue },
		{ u8"BattleItemReviveTargetValue", util::kBattleItemReviveTargetValue },
		{ u8"BattleMagicHealMagicValue", util::kBattleMagicHealMagicValue },
		{ u8"BattleMagicReviveMagicValue", util::kBattleMagicReviveMagicValue },
		{ u8"平时精灵补血精灵索引", util::kNormalMagicHealMagicValue },
		{ u8"战斗精灵补血人物", util::kMagicHealCharValue },
		{ u8"战斗精灵补血战宠", util::kMagicHealPetValue },
		{ u8"战斗精灵补血队友", util::kMagicHealAllieValue },
		{ u8"战斗道具补血人物", util::kItemHealCharValue },
		{ u8"战斗道具补血战宠", util::kItemHealPetValue },
		{ u8"战斗道具补血队友", util::kItemHealAllieValue },
		{ u8"战斗道具补气人物", util::kItemHealMpValue },
		{ u8"MagicHealCharNormalValue", util::kMagicHealCharNormalValue },
		{ u8"MagicHealPetNormalValue", util::kMagicHealPetNormalValue },
		{ u8"MagicHealAllieNormalValue", util::kMagicHealAllieNormalValue },
		{ u8"ItemHealCharNormalValue", util::kItemHealCharNormalValue },
		{ u8"ItemHealPetNormalValue", util::kItemHealPetNormalValue },
		{ u8"ItemHealAllieNormalValue", util::kItemHealAllieNormalValue },
		{ u8"ItemHealMpNormalValue", util::kItemHealMpNormalValue },
		{ u8"AutoWalkDelayValue", util::kAutoWalkDelayValue },
		{ u8"AutoWalkDistanceValue", util::kAutoWalkDistanceValue },
		{ u8"AutoWalkDirectionValue", util::kAutoWalkDirectionValue },
		{ u8"捉宠模式", util::kBattleCatchModeValue },
		{ u8"捉宠目标等级", util::kBattleCatchTargetLevelValue },
		{ u8"捉宠目标最大耐久力", util::kBattleCatchTargetMaxHpValue },
		{ u8"捉宠使用精灵直到血量低于", util::kBattleCatchTargetMagicHpValue },
		{ u8"捉宠使用道具直到血量低于", util::kBattleCatchTargetItemHpValue },
		{ u8"捉宠人物精灵索引", util::kBattleCatchPlayerMagicValue },
		{ u8"捉宠战宠技能索引", util::kBattleCatchPetSkillValue },
		{ u8"自动丢弃宠物攻数值", util::kDropPetStrValue },
		{ u8"自动丢弃宠物防数值", util::kDropPetDefValue },
		{ u8"自动丢弃宠物敏数值", util::kDropPetAgiValue },
		{ u8"自动丢弃宠物血数值", util::kDropPetHpValue },
		{ u8"自动丢弃宠物攻防敏数值", util::kDropPetAggregateValue },
		{ u8"自动移动功能编号", util::kAutoFunTypeValue },
		{ u8"战后自动锁定战宠编号", util::kLockPetValue },
		{ u8"战后自动锁定骑宠编号", util::kLockRideValue },

		{ u8"自动登陆", util::kAutoLoginEnable },
		{ u8"断线重连", util::kAutoReconnectEnable },
		{ u8"隐藏人物", util::kHideCharacterEnable },
		{ u8"关闭特效", util::kCloseEffectEnable },
		{ u8"资源优化", util::kOptimizeEnable },
		{ u8"隐藏石器", util::kHideWindowEnable },
		{ u8"屏蔽声音", util::kMuteEnable },
		{ u8"自动组队", util::kAutoJoinEnable },
		{ u8"锁定时间", util::kLockTimeEnable },
		{ u8"自动调整记忆体用量", util::kAutoFreeMemoryEnable },
		{ u8"快速走路", util::kFastWalkEnable },
		{ u8"横冲直撞", util::kPassWallEnable },
		{ u8"锁定原地", util::kLockMoveEnable },
		{ u8"锁定画面", util::kLockImageEnable },
		{ u8"自动丢肉", util::kAutoDropMeatEnable },
		{ u8"自动丢弃", util::kAutoDropEnable },
		{ u8"自动叠加", util::kAutoStackEnable },
		{ u8"自动KNPC", util::kKNPCEnable },
		{ u8"自动猜谜", util::kAutoAnswerEnable },
		{ u8"自动吃豆", util::kAutoEatBeanEnable },
		{ u8"走路遇敌", util::kAutoWalkEnable },
		{ u8"快速遇敌", util::kFastAutoWalkEnable },
		{ u8"快速战斗", util::kFastBattleEnable },
		{ u8"自动战斗", util::kAutoBattleEnable },
		{ u8"自动捉宠", util::kAutoCatchEnable },
		{ u8"锁定攻击", util::kLockAttackEnable },
		{ u8"自动逃跑", util::kAutoEscapeEnable },
		{ u8"锁定逃跑", util::kLockEscapeEnable },
		{ u8"战斗99秒", util::kBattleTimeExtendEnable },
		{ u8"落马逃跑", util::kFallDownEscapeEnable },

		{ u8"队伍开关", util::kSwitcherTeamEnable },
		{ u8"PK开关", util::kSwitcherPKEnable },
		{ u8"交名开关", util::kSwitcherCardEnable },
		{ u8"交易开关", util::kSwitcherTradeEnable },
		{ u8"家频开关", util::kSwitcherFamilyEnable },
		{ u8"职频开关", util::kSwitcherJobEnable },
		{ u8"世界开关", util::kSwitcherWorldEnable },

		{ u8"CrossActionCharEnable", util::kCrossActionCharEnable },
		{ u8"CrossActionPetEnable", util::kCrossActionPetEnable },
		{ u8"战斗精灵补血", util::kBattleMagicHealEnable },
		{ u8"战斗道具补血", util::kBattleItemHealEnable },
		{ u8"战斗道具补血肉优先", util::kBattleItemHealMeatPriorityEnable },
		{ u8"战斗道具补气", util::kBattleItemHealMpEnable },
		{ u8"战斗精灵復活", util::kBattleMagicReviveEnable },
		{ u8"战斗道具復活", util::kBattleItemReviveEnable },
		{ u8"平时精灵补血", util::kNormalMagicHealEnable },
		{ u8"平时道具补血", util::kNormalItemHealEnable },
		{ u8"平时道具补血肉优先", util::kNormalItemHealMeatPriorityEnable },
		{ u8"平时道具补气", util::kNormalItemHealMpEnable },
		{ u8"BattleCatchTargetLevelEnable", util::kBattleCatchTargetLevelEnable },
		{ u8"BattleCatchTargetMaxHpEnable", util::kBattleCatchTargetMaxHpEnable },
		{ u8"BattleCatchPlayerMagicEnable", util::kBattleCatchPlayerMagicEnable },
		{ u8"BattleCatchPlayerItemEnable", util::kBattleCatchPlayerItemEnable },
		{ u8"BattleCatchPetSkillEnable", util::kBattleCatchPetSkillEnable },

		{ u8"自动丢弃宠物", util::kDropPetEnable },
		{ u8"自动丢弃宠物攻", util::kDropPetStrEnable },
		{ u8"自动丢弃宠物防", util::kDropPetDefEnable },
		{ u8"自动丢弃宠物敏", util::kDropPetAgiEnable },
		{ u8"自动丢弃宠物血", util::kDropPetHpEnable },
		{ u8"自动丢弃宠物攻防敏", util::kDropPetAggregateEnable },

		{ u8"战后自动锁定战宠", util::kLockPetEnable },
		{ u8"战后自动锁定骑宠", util::kLockRideEnable },

		{ u8"自动丢弃名单", util::kAutoDropItemString },
		{ u8"锁定攻击名单", util::kLockAttackString },
		{ u8"锁定逃跑名单", util::kLockEscapeString },

		{ u8"战斗道具补血名单", util::kBattleItemHealItemString },
		{ u8"战斗道具补气名单 ", util::kBattleItemHealMpIteamString },
		{ u8"战斗道具復活名单", util::kBattleItemReviveItemString },
		{ u8"平时道具补血名单", util::kNormalItemHealItemString },
		{ u8"平时道具补气名单", util::kNormalItemHealMpItemString },
		{ u8"捉宠目标名单", util::kBattleCatchPetNameString },
		{ u8"捉宠道具名单", util::kBattleCatchPlayerItemString },

		{ u8"自动丢弃宠物名单", util::kDropPetNameString },
		{ u8"自动组队名称", util::kAutoFunNameString },
		{ u8"帐号", util::kGameAccountString },
		{ u8"密码", util::kGamePasswordString },
		{ u8"安全码", util::kGameSecurityCodeString }

#pragma endregion
	};

	util::UserSetting type = hash.value(typeStr, util::kSettingNotUsed);
	if (type == util::kSettingNotUsed)
		return Parser::kArgError;

	switch (type)
	{
	case util::kSettingNotUsed:

		///////////////////

	case util::kSettingMinValue:

	case util::kServerValue:
	case util::kSubServerValue:
	case util::kPositionValue:
	case util::kLockTimeValue:

		//afk->battle button
	case util::kBattleCharRoundActionTargetValue:
	case util::kBattleCharCrossActionTargetValue:
	case util::kBattleCharNormalActionTargetValue:
	case util::kBattlePetNormalActionTargetValue:

	case util::kBattlePetRoundActionTargetValue:
	case util::kBattlePetCrossActionTargetValue:

		//afk->battle combobox
	case util::kBattleCharRoundActionRoundValue:
	case util::kBattleCharRoundActionTypeValue:
	case util::kBattleCharRoundActionEnemyValue:
	case util::kBattleCharRoundActionLevelValue:

	case util::kBattleCharCrossActionTypeValue:
	case util::kBattleCharCrossActionRoundValue:

	case util::kBattleCharNormalActionTypeValue:
	case util::kBattleCharNormalActionEnemyValue:
	case util::kBattleCharNormalActionLevelValue:

	case util::kBattlePetRoundActionRoundValue:
	case util::kBattlePetRoundActionTypeValue:
	case util::kBattlePetRoundActionEnemyValue:
	case util::kBattlePetRoundActionLevelValue:

	case util::kBattlePetCrossActionTypeValue:
	case util::kBattlePetCrossActionRoundValue:

	case util::kBattlePetNormalActionTypeValue:
	case util::kBattlePetNormalActionEnemyValue:
	case util::kBattlePetNormalActionLevelValue:

		//afk->heal button
	case util::kBattleMagicHealTargetValue:
	case util::kBattleItemHealTargetValue:
	case util::kBattleMagicReviveTargetValue:
	case util::kBattleItemReviveTargetValue:

		//afk->heal combobox
	case util::kBattleMagicHealMagicValue:
	case util::kBattleMagicReviveMagicValue:
	case util::kNormalMagicHealMagicValue:

		//afk->heal spinbox
	case util::kMagicHealCharValue:
	case util::kMagicHealPetValue:
	case util::kMagicHealAllieValue:

	case util::kItemHealCharValue:
	case util::kItemHealPetValue:
	case util::kItemHealAllieValue:

	case util::kItemHealMpValue:

	case util::kMagicHealCharNormalValue:
	case util::kMagicHealPetNormalValue:
	case util::kMagicHealAllieNormalValue:

	case util::kItemHealCharNormalValue:
	case util::kItemHealPetNormalValue:
	case util::kItemHealAllieNormalValue:

	case util::kItemHealMpNormalValue:

		//afk->walk
	case util::kAutoWalkDelayValue:
	case util::kAutoWalkDistanceValue:
	case util::kAutoWalkDirectionValue:

		//afk->catch
	case util::kBattleCatchModeValue:
	case util::kBattleCatchTargetLevelValue:
	case util::kBattleCatchTargetMaxHpValue:
	case util::kBattleCatchTargetMagicHpValue:
	case util::kBattleCatchTargetItemHpValue:
	case util::kBattleCatchPlayerMagicValue:
	case util::kBattleCatchPetSkillValue:

	case util::kDropPetStrValue:
	case util::kDropPetDefValue:
	case util::kDropPetAgiValue:
	case util::kDropPetHpValue:
	case util::kDropPetAggregateValue:

		//general
	case util::kSpeedBoostValue:


		//other->group
	case util::kAutoFunTypeValue:

		//lockpet
	case util::kLockPetValue:
	case util::kLockRideValue:

	case util::kSettingMaxValue:
	{
		int value = 0;
		if (checkInt(TK, 2, &value))
		{
			injector.setValueHash(type, value);
			emit signalDispatcher.applyHashSettingsToUI();
		}
		else
			return Parser::kArgError;
		break;
	}

	///////////////////

	case util::kSettingMinEnable:

	case util::kAutoLoginEnable:
	case util::kAutoReconnectEnable:
	case util::kLogOutEnable:
	case util::kLogBackEnable:
	case util::kEchoEnable:
	case util::kHideCharacterEnable:
	case util::kCloseEffectEnable:
	case util::kOptimizeEnable:
	case util::kHideWindowEnable:
	case util::kMuteEnable:
	case util::kAutoJoinEnable:
	case util::kLockTimeEnable:
	case util::kAutoFreeMemoryEnable:
	case util::kFastWalkEnable:
	case util::kPassWallEnable:
	case util::kLockMoveEnable:
	case util::kLockImageEnable:
	case util::kAutoDropMeatEnable:
	case util::kAutoDropEnable:
	case util::kAutoStackEnable:
	case util::kKNPCEnable:
	case util::kAutoAnswerEnable:
	case util::kAutoEatBeanEnable:
	case util::kAutoWalkEnable:
	case util::kFastAutoWalkEnable:
	case util::kFastBattleEnable:
	case util::kAutoBattleEnable:
	case util::kAutoCatchEnable:
	case util::kLockAttackEnable:
	case util::kAutoEscapeEnable:
	case util::kLockEscapeEnable:
	case util::kBattleTimeExtendEnable:
	case util::kFallDownEscapeEnable:

		//switcher
	case util::kSwitcherTeamEnable:
	case util::kSwitcherPKEnable:
	case util::kSwitcherCardEnable:
	case util::kSwitcherTradeEnable:
	case util::kSwitcherFamilyEnable:
	case util::kSwitcherJobEnable:
	case util::kSwitcherWorldEnable:

		//afk->battle
	case util::kCrossActionCharEnable:
	case util::kCrossActionPetEnable:

		//afk->heal
	case util::kBattleMagicHealEnable:
	case util::kBattleItemHealEnable:
	case util::kBattleItemHealMeatPriorityEnable:
	case util::kBattleItemHealMpEnable:
	case util::kBattleMagicReviveEnable:
	case util::kBattleItemReviveEnable:

	case util::kNormalMagicHealEnable:
	case util::kNormalItemHealEnable:
	case util::kNormalItemHealMeatPriorityEnable:
	case util::kNormalItemHealMpEnable:

		//afk->catch
	case util::kBattleCatchTargetLevelEnable:
	case util::kBattleCatchTargetMaxHpEnable:
	case util::kBattleCatchPlayerMagicEnable:
	case util::kBattleCatchPlayerItemEnable:
	case util::kBattleCatchPetSkillEnable:

	case util::kDropPetEnable:
	case util::kDropPetStrEnable:
	case util::kDropPetDefEnable:
	case util::kDropPetAgiEnable:
	case util::kDropPetHpEnable:
	case util::kDropPetAggregateEnable:

		//lockpet
	case util::kLockPetEnable:
	case util::kLockRideEnable:

	case util::kSettingMaxEnable:
	{
		int value = 0;
		if (checkInt(TK, 2, &value))
		{
			injector.setEnableHash(type, value > 0);
			emit signalDispatcher.applyHashSettingsToUI();
		}
		else
			return Parser::kArgError;
		break;
	}

	//////////////////

	case util::kSettingMinString:

		//global
	case util::kAutoDropItemString:
	case util::kLockAttackString:
	case util::kLockEscapeString:

		//afk->heal
	case util::kBattleItemHealItemString:
	case util::kBattleItemHealMpIteamString:
	case util::kBattleItemReviveItemString:

	case util::kNormalItemHealItemString:
	case util::kNormalItemHealMpItemString:

		//afk->catch
	case util::kBattleCatchPetNameString:
	case util::kBattleCatchPlayerItemString:
	case util::kDropPetNameString:

		//other->group
	case util::kAutoFunNameString:

		//other->other2
	case util::kGameAccountString:
	case util::kGamePasswordString:
	case util::kGameSecurityCodeString:

	case util::kSettingMaxString:
	{
		QString valueStr;
		if (checkString(TK, 2, &valueStr))
		{
			injector.setStringHash(type, valueStr);
			emit signalDispatcher.applyHashSettingsToUI();
		}
		else
			return Parser::kArgError;
		break;
	}

	default:
		break;
	}

	return Parser::kNoChange;
}
