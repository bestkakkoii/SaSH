#include "stdafx.h"
#include "interpreter.h"
#include "util.h"
#include "injector.h"

#include "signaldispatcher.h"

int Interpreter::reg(int currentline, const TokenMap& TK)
{
	QString text;
	if (!checkString(TK, 1, &text))
		return Parser::kArgError;

	QString typeStr;
	if (!checkString(TK, 2, &typeStr))
		return Parser::kArgError;

	util::SafeHash<QString, int>& hash = parser_->getLabels();
	if (!hash.contains(text))
		return Parser::kArgError;

	parser_->insertUserCallBack(text, typeStr);
	return Parser::kNoChange;
}

int Interpreter::sleep(int currentline, const TokenMap& TK)
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

int Interpreter::press(int currentline, const TokenMap& TK)
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
			dialog_t dialog = injector.server->currentDialog.get();
			QStringList textList = dialog.linebuttontext;
			if (!textList.isEmpty())
			{
				bool isExact = true;
				QString newText = text.toUpper();
				if (newText.startsWith(kFuzzyPrefix))
				{
					newText = newText.mid(1);
					isExact = false;
				}

				for (int i = 0; i < textList.size(); ++i)
				{
					if (!isExact && textList.at(i).toUpper().contains(newText))
					{
						injector.server->press(i + 1, dialogid, npcId);
						break;
					}
					else if (isExact && textList.at(i).toUpper() == newText)
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

int Interpreter::eo(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	injector.server->EO();

	return Parser::kNoChange;
}

int Interpreter::announce(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();


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
	if (color == -1)
		color = QRandomGenerator::global()->bounded(0, 10);
	else if (color < -1)
	{
		logExport(currentline, text, 0);
		return Parser::kNoChange;
	}

	if (injector.server.isNull())
		return Parser::kError;

	injector.server->announce(text, color);

	return Parser::kNoChange;
}

int Interpreter::input(int currentline, const TokenMap& TK)
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

int Interpreter::messagebox(int currentline, const TokenMap& TK)
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

int Interpreter::talk(int currentline, const TokenMap& TK)
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
	if (color < 0)
		color = QRandomGenerator::global()->bounded(0, 10);

	TalkMode talkmode = kTalkNormal;
	int nTalkMode = 0;
	checkInt(TK, 3, &nTalkMode);
	if (nTalkMode > 0 && nTalkMode < kTalkModeMax)
	{
		--nTalkMode;
		talkmode = static_cast<TalkMode>(nTalkMode);
	}

	injector.server->talk(text, color, talkmode);

	return Parser::kNoChange;
}

int Interpreter::talkandannounce(int currentline, const TokenMap& TK)
{
	announce(currentline, TK);
	return talk(currentline, TK);
}

int Interpreter::logout(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();
	if (!injector.server.isNull())
		injector.server->logOut();

	return Parser::kNoChange;
}

int Interpreter::logback(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	injector.server->logBack();

	return Parser::kNoChange;
}

int Interpreter::cleanchat(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();
	if (!injector.server.isNull())
		injector.server->cleanChatHistory();

	return Parser::kNoChange;
}

int Interpreter::savesetting(int currentline, const TokenMap& TK)
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

int Interpreter::loadsetting(int currentline, const TokenMap& TK)
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

int Interpreter::set(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();

	QString typeStr;
	if (!checkString(TK, 1, &typeStr))
		return Parser::kError;

	const QHash<QString, util::UserSetting> hash = {

#pragma region GB2312

		{ u8"BattleCharRoundActionRoundValue", util::kBattleCharRoundActionRoundValue },
		{ u8"BattleCharRoundActionTypeValue", util::kBattleCharRoundActionTypeValue },
		{ u8"BattleCharRoundActionEnemyValue", util::kBattleCharRoundActionEnemyValue },
		{ u8"BattleCharRoundActionLevelValue", util::kBattleCharRoundActionLevelValue },

		{ u8"战斗间隔TypeValue", util::kBattleCharCrossActionTypeValue },
		{ u8"战斗间隔RoundValue", util::kBattleCharCrossActionRoundValue },

		{ u8"BattleCharNormalActionTypeValue", util::kBattleCharNormalActionTypeValue },
		{ u8"BattleCharNormalActionEnemyValue", util::kBattleCharNormalActionEnemyValue },
		{ u8"BattleCharNormalActionLevelValue", util::kBattleCharNormalActionLevelValue },

		{ u8"BattlePetRoundActionRoundValue", util::kBattlePetRoundActionRoundValue },
		{ u8"BattlePetRoundActionTypeValue", util::kBattlePetRoundActionTypeValue },
		{ u8"BattlePetRoundActionEnemyValue", util::kBattlePetRoundActionEnemyValue },
		{ u8"BattlePetRoundActionLevelValue", util::kBattlePetRoundActionLevelValue },

		{ u8"战斗宠间隔回合TypeValue", util::kBattlePetCrossActionTypeValue },
		{ u8"战斗宠间隔回合RoundValue", util::kBattlePetCrossActionRoundValue },

		{ u8"BattlePetNormalActionTypeValue", util::kBattlePetNormalActionTypeValue },
		{ u8"BattlePetNormalActionEnemyValue", util::kBattlePetNormalActionEnemyValue },
		{ u8"BattlePetNormalActionLevelValue", util::kBattlePetNormalActionLevelValue },

		{ u8"BattleCharRoundActionTargetValue", util::kBattleCharRoundActionTargetValue },
		{ u8"BattleCharCrossActionTargetValue", util::kBattleCharCrossActionTargetValue },
		{ u8"BattleCharNormalActionTargetValue", util::kBattleCharNormalActionTargetValue },

		{ u8"BattlePetRoundActionTargetValue", util::kBattlePetRoundActionTargetValue },
		{ u8"BattlePetCrossActionTargetValue", util::kBattlePetCrossActionTargetValue },
		{ u8"BattlePetNormalActionTargetValue", util::kBattlePetNormalActionTargetValue },


		{ u8"战斗间隔回合", util::kCrossActionCharEnable },
		{ u8"战斗宠间隔回合", util::kCrossActionPetEnable },

		/*{u8"战斗道具补血战宠", util::kBattleItemHealPetValue},
		{ u8"战斗道具补血队友", util::kBattleItemHealAllieValue },
		{ u8"战斗道具补血人物", util::kBattleItemHealCharValue },*/
		{ u8"战斗道具补血", util::kBattleItemHealEnable },//{ u8"战斗道具补血", util::kBattleItemHealItemString },//{ u8"战斗道具肉优先", util::kBattleItemHealMeatPriorityEnable },{ u8"BattleItemHealTargetValue", util::kBattleItemHealTargetValue },


		{ u8"战斗精灵復活", util::kBattleMagicReviveEnable },//{ u8"BattleMagicReviveMagicValue", util::kBattleMagicReviveMagicValue },{ u8"BattleMagicReviveTargetValue", util::kBattleMagicReviveTargetValue },
		{ u8"战斗道具復活", util::kBattleItemReviveEnable },//{ u8"战斗道具復活", util::kBattleItemReviveItemString },{ u8"BattleItemReviveTargetValue", util::kBattleItemReviveTargetValue },

		/*{u8"ItemHealCharNormalValue", util::kNormalItemHealCharValue},
		{ u8"ItemHealPetNormalValue", util::kNormalItemHealPetValue },
		{ u8"ItemHealAllieNormalValue", util::kNormalItemHealAllieValue },*/
		{ u8"道具补血", util::kNormalItemHealEnable },//{ u8"平时道具补血", util::kNormalItemHealItemString },{ u8"道具肉优先", util::kNormalItemHealMeatPriorityEnable },

		{ u8"自动丢弃宠物", util::kDropPetEnable },//{ u8"自动丢弃宠物名单", util::kDropPetNameString },
		{ u8"自动丢弃宠物攻", util::kDropPetStrEnable },//{ u8"自动丢弃宠物攻数值", util::kDropPetStrValue },
		{ u8"自动丢弃宠物防", util::kDropPetDefEnable },//{ u8"自动丢弃宠物防数值", util::kDropPetDefValue },
		{ u8"自动丢弃宠物敏", util::kDropPetAgiEnable },//{ u8"自动丢弃宠物敏数值", util::kDropPetAgiValue },
		{ u8"自动丢弃宠物血", util::kDropPetHpEnable },//{ u8"自动丢弃宠物血数值", util::kDropPetHpValue },
		{ u8"自动丢弃宠物攻防敏", util::kDropPetAggregateEnable },//{ u8"自动丢弃宠物攻防敏数值", util::kDropPetAggregateValue },


		//ok
		{ u8"主机", util::kServerValue },
		{ u8"副机", util::kSubServerValue },
		{ u8"位置", util::kPositionValue },

		{ u8"加速", util::kSpeedBoostValue },
		{ u8"帐号", util::kGameAccountString },
		{ u8"密码", util::kGamePasswordString },
		{ u8"安全码", util::kGameSecurityCodeString },
		{ u8"远程白名单", util::kMailWhiteListString },

		{ u8"自走延时", util::kAutoWalkDelayValue },
		{ u8"自走步长", util::kAutoWalkDistanceValue },
		{ u8"自走方向", util::kAutoWalkDirectionValue },

		{ u8"脚本速度", util::kScriptSpeedValue },

		{ u8"自动登陆", util::kAutoLoginEnable },
		{ u8"断线重连", util::kAutoReconnectEnable },
		{ u8"隐藏人物", util::kHideCharacterEnable },
		{ u8"关闭特效", util::kCloseEffectEnable },
		{ u8"资源优化", util::kOptimizeEnable },
		{ u8"隐藏石器", util::kHideWindowEnable },
		{ u8"屏蔽声音", util::kMuteEnable },

		{ u8"自动调整内存", util::kAutoFreeMemoryEnable },
		{ u8"快速走路", util::kFastWalkEnable },
		{ u8"横冲直撞", util::kPassWallEnable },
		{ u8"锁定原地", util::kLockMoveEnable },
		{ u8"锁定画面", util::kLockImageEnable },
		{ u8"自动丢肉", util::kAutoDropMeatEnable },
		{ u8"自动叠加", util::kAutoStackEnable },
		{ u8"自动迭加", util::kAutoStackEnable },
		{ u8"自动KNPC", util::kKNPCEnable },
		{ u8"自动猜谜", util::kAutoAnswerEnable },
		{ u8"自动吃豆", util::kAutoEatBeanEnable },
		{ u8"走路遇敌", util::kAutoWalkEnable },
		{ u8"快速遇敌", util::kFastAutoWalkEnable },
		{ u8"快速战斗", util::kFastBattleEnable },
		{ u8"自动战斗", util::kAutoBattleEnable },
		{ u8"自动捉宠", util::kAutoCatchEnable },
		{ u8"自动逃跑", util::kAutoEscapeEnable },
		{ u8"战斗99秒", util::kBattleTimeExtendEnable },
		{ u8"落马逃跑", util::kFallDownEscapeEnable },
		{ u8"显示经验", util::kShowExpEnable },

		{ u8"队伍开关", util::kSwitcherTeamEnable },
		{ u8"PK开关", util::kSwitcherPKEnable },
		{ u8"交名开关", util::kSwitcherCardEnable },
		{ u8"交易开关", util::kSwitcherTradeEnable },
		{ u8"家频开关", util::kSwitcherFamilyEnable },
		{ u8"职频开关", util::kSwitcherJobEnable },
		{ u8"世界开关", util::kSwitcherWorldEnable },

		{ u8"锁定战宠", util::kLockPetEnable },//{ u8"战后自动锁定战宠编号", util::kLockPetValue },
		{ u8"锁定骑宠", util::kLockRideEnable },//{ u8"战后自动锁定骑宠编号", util::kLockRideValue },
		{ u8"锁宠排程", util::kLockPetScheduleEnable },
		{ u8"锁定时间", util::kLockTimeEnable },//{ u8"时间", util::kLockTimeValue },

		{ u8"捉宠模式", util::kBattleCatchModeValue },
		{ u8"捉宠等级", util::kBattleCatchTargetLevelEnable },//{ u8"捉宠目标等级", util::kBattleCatchTargetLevelValue },
		{ u8"捉宠血量", util::kBattleCatchTargetMaxHpEnable },//{ u8"捉宠目标最大耐久力", util::kBattleCatchTargetMaxHpValue },
		{ u8"捉宠目标", util::kBattleCatchPetNameString },
		{ u8"捉宠宠技能", util::kBattleCatchPetSkillEnable },////{ u8"捉宠战宠技能索引", util::kBattleCatchPetSkillValue },
		{ u8"捉宠道具", util::kBattleCatchPlayerItemEnable },//{ u8"捉宠使用道具直到血量低于", util::kBattleCatchTargetItemHpValue },{ u8"捉宠道具", util::kBattleCatchPlayerItemString }, 
		{ u8"捉宠精灵", util::kBattleCatchPlayerMagicEnable },//{ u8"捉宠使用精灵直到血量低于", util::kBattleCatchTargetMagicHpValue },{ u8"捉宠人物精灵索引", util::kBattleCatchPlayerMagicValue },

		{ u8"自动组队", util::kAutoJoinEnable },//{ u8"自动组队名称", util::kAutoFunNameString },	{ u8"自动移动功能编号", util::kAutoFunTypeValue },
		{ u8"自动丢弃", util::kAutoDropEnable },//{ u8"自动丢弃名单", util::kAutoDropItemString },
		{ u8"锁定攻击", util::kLockAttackEnable },//{ u8"锁定攻击名单", util::kLockAttackString },
		{ u8"锁定逃跑", util::kLockEscapeEnable },//{ u8"锁定逃跑名单", util::kLockEscapeString },

		{ u8"道具补气", util::kNormalItemHealMpEnable },//{ u8"ItemHealMpNormalValue", util::kNormalItemHealMpValue },{ u8"平时道具补气", util::kNormalItemHealMpItemString },
		{ u8"战斗道具补气", util::kBattleItemHealMpEnable },//{ u8"战斗道具补气人物", util::kBattleItemHealMpValue },{ u8"战斗道具补气 ", util::kBattleItemHealMpItemString },


		/*{u8"MagicHealCharNormalValue", util::kNormalMagicHealCharValue},
		{ u8"MagicHealPetNormalValue", util::kNormalMagicHealPetValue },
		{ u8"MagicHealAllieNormalValue", util::kNormalMagicHealAllieValue },*/
		{ u8"精灵补血", util::kNormalMagicHealEnable },//{ u8"平时精灵补血精灵索引", util::kNormalMagicHealMagicValue },
		/*{ u8"战斗精灵补血人物", util::kBattleMagicHealCharValue },
		{u8"战斗精灵补血战宠", util::kBattleMagicHealPetValue},
		{ u8"战斗精灵补血队友", util::kBattleMagicHealAllieValue },*/
		{ u8"战斗精灵补血", util::kBattleMagicHealEnable },//util::kBattleMagicHealMagicValue, util::kBattleMagicHealTargetValue,


#pragma endregion
	};

	util::UserSetting type = hash.value(typeStr, util::kSettingNotUsed);
	if (type == util::kSettingNotUsed)
		return Parser::kArgError;

	//start with 1
	switch (type)
	{
	case util::kAutoWalkDirectionValue://自走方向
	case util::kServerValue://主機
	case util::kSubServerValue://副機
	case util::kPositionValue://位置
	case util::kBattleCatchModeValue://戰鬥捕捉模式
	{
		int value = 0;
		if (!checkInt(TK, 2, &value))
			return Parser::kArgError;
		--value;
		if (value < 0)
			value = 0;

		injector.setValueHash(type, value);
		emit signalDispatcher.applyHashSettingsToUI();
		return Parser::kNoChange;
	}
	default:
		break;
	}

	//start with 0
	switch (type)
	{
	case util::kAutoWalkDelayValue://自走延時
	case util::kAutoWalkDistanceValue://自走步長
	case util::kSpeedBoostValue://加速
	case util::kScriptSpeedValue://腳本速度
	{
		int value = 0;
		if (!checkInt(TK, 2, &value))
			return Parser::kArgError;

		if (value < 0)
			value = 0;

		injector.setValueHash(type, value);
		emit signalDispatcher.applyHashSettingsToUI();
		return Parser::kNoChange;
	}
	default:
		break;
	}

	//0:close 1:open
	switch (type)
	{
	case util::kAutoLoginEnable:
	case util::kAutoReconnectEnable:

	case util::kHideCharacterEnable:
	case util::kCloseEffectEnable:
	case util::kOptimizeEnable:
	case util::kHideWindowEnable:
	case util::kMuteEnable:

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

	case util::kAutoEscapeEnable:

	case util::kBattleTimeExtendEnable:
	case util::kFallDownEscapeEnable:
	case util::kShowExpEnable:

		//switcher
	case util::kSwitcherTeamEnable:
	case util::kSwitcherPKEnable:
	case util::kSwitcherCardEnable:
	case util::kSwitcherTradeEnable:
	case util::kSwitcherFamilyEnable:
	case util::kSwitcherJobEnable:
	case util::kSwitcherWorldEnable:
	{
		int value = 0;
		if (!checkInt(TK, 2, &value))
			return Parser::kArgError;

		if (value < 0)
			value = 0;

		injector.setEnableHash(type, value > 0);
		emit signalDispatcher.applyHashSettingsToUI();
		return Parser::kNoChange;
	}
	default:
		break;
	}

	//0:close >1:open string value
	switch (type)
	{
	case util::kBattleItemHealMpEnable:
	case util::kNormalItemHealMpEnable:
	case util::kBattleCatchPlayerItemEnable:
	case util::kLockAttackEnable:
	case util::kLockEscapeEnable:
	case util::kAutoDropEnable:
	case util::kAutoJoinEnable:
	case util::kLockPetScheduleEnable:
	{
		int value = 0;
		if (!checkInt(TK, 2, &value))
			return Parser::kArgError;

		if (value < 0)
			value = 0;

		QString text;
		checkString(TK, 3, &text);

		bool ok = value > 0;
		injector.setEnableHash(type, ok);
		if (type == util::kLockPetScheduleEnable && ok)
			injector.setStringHash(util::kLockPetScheduleString, text);
		else if (type == util::kAutoJoinEnable && ok)
		{
			injector.setStringHash(util::kAutoFunNameString, text);
			injector.setValueHash(util::kAutoFunTypeValue, 0);
		}
		else if (type == util::kLockAttackEnable && ok)
			injector.setStringHash(util::kLockAttackString, text);
		else if (type == util::kLockEscapeEnable && ok)
			injector.setStringHash(util::kLockEscapeString, text);
		else if (type == util::kAutoDropEnable && ok)
			injector.setStringHash(util::kAutoDropItemString, text);
		else if (type == util::kBattleCatchPlayerItemEnable && ok)
		{
			injector.setStringHash(util::kBattleCatchPlayerItemString, text);
			injector.setValueHash(util::kBattleCatchTargetItemHpValue, value);
		}
		else if (type == util::kNormalItemHealMpEnable && ok)
		{
			injector.setStringHash(util::kNormalItemHealMpItemString, text);
			injector.setValueHash(util::kNormalItemHealMpValue, value);
		}
		else if (type == util::kBattleItemHealMpEnable && ok)
		{
			injector.setStringHash(util::kBattleItemHealMpItemString, text);
			injector.setValueHash(util::kBattleItemHealMpValue, value);
		}

		emit signalDispatcher.applyHashSettingsToUI();
		return Parser::kNoChange;
	}
	default:
		break;
	}

	//0:close >1:open int value
	switch (type)
	{
	case util::kBattleCatchPetSkillEnable:
	case util::kBattleCatchTargetMaxHpEnable:
	case util::kBattleCatchTargetLevelEnable:
	case util::kLockTimeEnable:
	case util::kLockPetEnable:
	case util::kLockRideEnable:
	{
		int value = 0;
		if (!checkInt(TK, 2, &value))
			return Parser::kArgError;
		--value;
		if (value < 0)
			value = 0;

		bool ok = value > 0;
		injector.setEnableHash(type, ok);
		if (type == util::kLockPetEnable && ok)
			injector.setValueHash(util::kLockPetValue, value);
		else if (type == util::kLockRideEnable && ok)
			injector.setValueHash(util::kLockRideValue, value);
		else if (type == util::kLockTimeEnable && ok)
			injector.setValueHash(util::kLockTimeValue, value);
		else if (type == util::kBattleCatchTargetLevelEnable && ok)
			injector.setValueHash(util::kBattleCatchTargetLevelValue, value);
		else if (type == util::kBattleCatchTargetMaxHpEnable && ok)
			injector.setValueHash(util::kBattleCatchTargetMaxHpValue, value);
		else if (type == util::kBattleCatchPetSkillEnable && ok)
			injector.setValueHash(util::kBattleCatchPetSkillValue, value);

		emit signalDispatcher.applyHashSettingsToUI();
		return Parser::kNoChange;
	}
	default:
		break;
	}

	//0:close >1:open two int value
	switch (type)
	{
	case util::kBattleCatchPlayerMagicEnable:
	{
		int value = 0;
		if (!checkInt(TK, 2, &value))
			return Parser::kArgError;

		if (value < 0)
			value = 0;

		int value2 = 0;
		if (!checkInt(TK, 3, &value2))
			return Parser::kArgError;

		--value2;
		if (value2 < 0)
			value2 = 0;

		bool ok = value > 0;
		injector.setEnableHash(type, ok);
		if (type == util::kBattleCatchPlayerMagicEnable && ok)
		{
			injector.setValueHash(util::kBattleCatchTargetMagicHpValue, value);
			injector.setValueHash(util::kBattleCatchPlayerMagicValue, value2);
		}
		emit signalDispatcher.applyHashSettingsToUI();
		return Parser::kNoChange;
	}
	default:
		break;
	}

	//4int
	switch (type)
	{
	case util::kBattleMagicHealEnable:
	case util::kNormalMagicHealEnable:
	{
		int value = 0;
		if (!checkInt(TK, 2, &value))
			return Parser::kArgError;
		--value;

		if (value < 0)
			value = 0;

		bool ok = value > 0;
		injector.setEnableHash(type, ok);

		int value2 = 0;
		checkInt(TK, 3, &value2);

		int value3 = 0;
		checkInt(TK, 4, &value3);

		int value4 = 0;
		checkInt(TK, 5, &value4);

		int value5 = util::kSelectSelf | util::kSelectPet;
		checkInt(TK, 6, &value5);

		if (type == util::kNormalMagicHealEnable && ok)
		{
			injector.setValueHash(util::kNormalMagicHealMagicValue, value);
			injector.setValueHash(util::kNormalMagicHealCharValue, value2);
			injector.setValueHash(util::kNormalMagicHealPetValue, value3);
			injector.setValueHash(util::kNormalMagicHealAllieValue, value4);
		}
		else if (type == util::kBattleMagicHealEnable && ok)
		{
			injector.setValueHash(util::kBattleMagicHealMagicValue, value);
			injector.setValueHash(util::kBattleMagicHealCharValue, value2);
			injector.setValueHash(util::kBattleMagicHealPetValue, value3);
			injector.setValueHash(util::kBattleMagicHealAllieValue, value4);
			injector.setValueHash(util::kBattleMagicHealTargetValue, value5);
		}


		emit signalDispatcher.applyHashSettingsToUI();
		return Parser::kNoChange;
	}
	default:
		break;
	}

	//string only
	switch (type)
	{
	case util::kGameAccountString:
	case util::kGamePasswordString:
	case util::kGameSecurityCodeString:
	case util::kMailWhiteListString:
	case util::kBattleCatchPetNameString:
	{
		QString text;
		checkString(TK, 2, &text);
		injector.setStringHash(type, text);
		emit signalDispatcher.applyHashSettingsToUI();
		return Parser::kNoChange;
	}
	default:
		break;
	}

	return Parser::kNoChange;
}


///////////////////////////////////////////////////////////////
int Interpreter::dlg(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	QString varName;
	checkString(TK, 1, &varName);

	QString buttonStrs;
	checkString(TK, 2, &buttonStrs);
	if (buttonStrs.isEmpty())
		return Parser::kArgError;

	QString text;
	if (!checkString(TK, 3, &text))
		return Parser::kArgError;

	int timeout = 5000;
	checkInt(TK, 4, &timeout);

	text.replace("\\n", "\n");

	buttonStrs = buttonStrs.toUpper();
	QStringList buttonStrList = buttonStrs.split(util::rexOR, Qt::SkipEmptyParts);
	QVector<int> buttonVec;
	unsigned int buttonFlag = 0;
	for (const QString str : buttonStrList)
	{
		if (!buttonMap.contains(str))
			return Parser::kArgError;
		unsigned int value = buttonMap.value(str);
		buttonFlag |= value;
	}

	injector.server->IS_WAITFOR_CUSTOM_DIALOG_FLAG = true;
	injector.server->createRemoteDialog(buttonFlag, text);
	bool bret = waitfor(timeout, [&injector]() { return !injector.server->IS_WAITFOR_CUSTOM_DIALOG_FLAG; });
	QHash<QString, BUTTON_TYPE> big5 = {
		{ "OK", BUTTON_OK},
		{ "CANCEL", BUTTON_CANCEL },
		//big5
		{ u8"確定", BUTTON_YES },
		{ u8"取消", BUTTON_NO },
		{ u8"上一頁", BUTTON_PREVIOUS },
		{ u8"下一頁", BUTTON_NEXT },
	};

	QHash<QString, BUTTON_TYPE> gb2312 = {
		{ "OK", BUTTON_OK},
		{ "CANCEL", BUTTON_CANCEL },
		//gb2312
		{ u8"确定", BUTTON_YES },
		{ u8"取消", BUTTON_NO },
		{ u8"上一页", BUTTON_PREVIOUS },
		{ u8"下一页", BUTTON_NEXT },
	};
	UINT acp = GetACP();

	customdialog_t dialog = injector.server->customDialog;
	QString type;
	if (acp == 950)
		type = big5.key(dialog.button, "");
	else
		type = gb2312.key(dialog.button, "");

	QVariant result;
	if (type.isEmpty() && dialog.row > 0)
		result = dialog.row;
	else
		result = type;

	parser_->getPVariables()->insert(varName, result);
	injector.server->IS_WAITFOR_CUSTOM_DIALOG_FLAG = false;
	return checkJump(TK, 6, bret, FailedJump);
}

int Interpreter::regex(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	QString varName;
	checkString(TK, 1, &varName);
	if (varName.isEmpty())
		return Parser::kArgError;

	QString srcText;
	checkString(TK, 2, &srcText);
	if (srcText.isEmpty())
		return Parser::kArgError;

	QString text;
	checkString(TK, 3, &text);
	if (text.isEmpty())
		return Parser::kArgError;

	int capture = 1;
	checkInt(TK, 4, &capture);

	QRegularExpression regex(text);

	QRegularExpressionMatch match = regex.match(srcText);
	if (match.hasMatch())
	{
		if (capture < 0 || capture > match.lastCapturedIndex())
			return Parser::kArgError;
		parser_->getPVariables()->insert(varName, match.captured(capture));
	}

	return Parser::kNoChange;
}

int Interpreter::find(int currentline, const TokenMap& TK)
{
	QString varName;
	checkString(TK, 1, &varName);
	if (varName.isEmpty())
		return Parser::kArgError;

	QString src;
	checkString(TK, 2, &src);
	if (src.isEmpty())
		return Parser::kArgError;

	QString text1;
	checkString(TK, 3, &text1);
	if (text1.isEmpty())
		return Parser::kArgError;

	QString text2;
	checkString(TK, 4, &text2);

	//查找 src 中 text1 到 text2 之间的文本 如果 text2 为空 则查找 text1 到行尾的文本

	int pos1 = src.indexOf(text1);
	if (pos1 < 0)
		return Parser::kNoChange;

	int pos2 = -1;
	if (text2.isEmpty())
		pos2 = src.length();
	else
		pos2 = src.indexOf(text2, pos1 + text1.length());

	if (pos2 < 0)
		return Parser::kNoChange;

	QString result = src.mid(pos1 + text1.length(), pos2 - pos1 - text1.length());

	parser_->getPVariables()->insert(varName, result);

	return Parser::kNoChange;
}

int Interpreter::half(int currentline, const TokenMap& TK)
{
	const QString FullWidth = "０１２３４５６７８９"
		"ａｂｃｄｅｆｇｈｉｊｋｌｍｎｏｐｑｒｓｔｕｖｗｘｙｚ"
		"ＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯＰＱＲＳＴＵＶＷＸＹＺ"
		"‵～！＠＃＄％︿＆＊（）＿－＝＋［｛］｝＼｜；：’＂，＜．＞／？　";
	const QString HalfWidth = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ~!@#$%^&*()_-+=[]{}\\|;:'\",<.>/? ";

	QString varName;
	checkString(TK, 1, &varName);
	if (varName.isEmpty())
		return Parser::kArgError;

	QString text;
	checkString(TK, 2, &text);
	if (text.isEmpty())
		return Parser::kArgError;

	QString result = text;
	int size = FullWidth.size();
	for (int i = 0; i < size; i++)
	{
		result.replace(FullWidth[i], HalfWidth[i]);
	}

	parser_->getPVariables()->insert(varName, result);
	return Parser::kNoChange;
}

int Interpreter::full(int currentline, const TokenMap& TK)
{
	const QString FullWidth = "０１２３４５６７８９"
		"ａｂｃｄｅｆｇｈｉｊｋｌｍｎｏｐｑｒｓｔｕｖｗｘｙｚ"
		"ＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯＰＱＲＳＴＵＶＷＸＹＺ"
		"‵～！＠＃＄％︿＆＊（）＿－＝＋［｛］｝＼｜；：’＂，＜．＞／？　";
	const QString HalfWidth = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ~!@#$%^&*()_-+=[]{}\\|;:'\",<.>/? ";

	QString varName;
	checkString(TK, 1, &varName);
	if (varName.isEmpty())
		return Parser::kArgError;

	QString text;
	checkString(TK, 2, &text);
	if (text.isEmpty())
		return Parser::kArgError;

	QString result = text;
	int size = FullWidth.size();
	for (int i = 0; i < size; i++)
	{
		result.replace(HalfWidth[i], FullWidth[i]);
	}

	parser_->getPVariables()->insert(varName, result);
	return Parser::kNoChange;
}

int Interpreter::upper(int currentline, const TokenMap& TK)
{
	QString varName;
	checkString(TK, 1, &varName);
	if (varName.isEmpty())
		return Parser::kArgError;

	QString text;
	checkString(TK, 2, &text);
	if (text.isEmpty())
		return Parser::kArgError;

	QString result = text.toUpper();

	parser_->getPVariables()->insert(varName, result);

	return Parser::kNoChange;
}

int Interpreter::lower(int currentline, const TokenMap& TK)
{
	QString varName;
	checkString(TK, 1, &varName);
	if (varName.isEmpty())
		return Parser::kArgError;

	QString text;
	checkString(TK, 2, &text);
	if (text.isEmpty())
		return Parser::kArgError;

	QString result = text.toLower();

	parser_->getPVariables()->insert(varName, result);

	return Parser::kNoChange;
}

int Interpreter::replace(int currentline, const TokenMap& TK)
{
	QString varName;
	checkString(TK, 1, &varName);
	if (varName.isEmpty())
		return Parser::kArgError;

	QString src;
	checkString(TK, 2, &src);

	QString cmpText;
	checkString(TK, 3, &cmpText);
	if (cmpText.isEmpty())
		return Parser::kArgError;

	QString replaceText;
	checkString(TK, 4, &replaceText);
	if (replaceText.isEmpty())
		return Parser::kArgError;

	bool isrex = false;
	int n = 0;
	if (!checkInt(TK, 5, &n))
	{
		isrex = true;
	}


	QString result = src;
	if (!isrex)
		result.replace(cmpText, replaceText);
	else
	{
		QRegularExpression regex(cmpText);
		result.replace(regex, replaceText);
	}

	parser_->getPVariables()->insert(varName, result);

	return Parser::kNoChange;
}

int Interpreter::toint(int currentline, const TokenMap& TK)
{
	QString varName;
	checkString(TK, 1, &varName);
	if (varName.isEmpty())
		return Parser::kArgError;

	QString text;
	if (!checkString(TK, 2, &text))
	{
		double d = 0;
		if (!checkDouble(TK, 2, &d))
		{
			int i = 0;
			if (!checkInt(TK, 2, &i))
				return Parser::kArgError;
			text = QString::number(i);
		}
		text = QString::number(d);
	}

	bool ok = false;

	int result = text.toInt(&ok);
	if (!ok)
		return Parser::kNoChange;

	parser_->getPVariables()->insert(varName, result);

	return Parser::kNoChange;
}

int Interpreter::tostr(int currentline, const TokenMap& TK)
{
	QString varName;
	checkString(TK, 1, &varName);
	if (varName.isEmpty())
		return Parser::kArgError;

	QString text;
	if (!checkString(TK, 2, &text))
	{
		double d = 0;
		if (!checkDouble(TK, 2, &d))
		{
			int i = 0;
			if (!checkInt(TK, 2, &i))
				return Parser::kArgError;
			text = QString::number(i);
		}
		text = QString::number(d);
	}

	parser_->getPVariables()->insert(varName, text);

	return Parser::kNoChange;
}

int Interpreter::todb(int currentline, const TokenMap& TK)
{
	QString varName;
	checkString(TK, 1, &varName);
	if (varName.isEmpty())
		return Parser::kArgError;

	QString text;
	if (!checkString(TK, 2, &text))
	{
		double d = 0;
		if (!checkDouble(TK, 2, &d))
		{
			int i = 0;
			if (!checkInt(TK, 2, &i))
				return Parser::kArgError;
			text = QString::number(i);
		}
		text = QString::number(d, 'f', 16);
	}

	bool ok = false;

	double result = text.toDouble(&ok);
	if (!ok)
		return Parser::kNoChange;

	parser_->getPVariables()->insert(varName, result);

	return Parser::kNoChange;
}

int Interpreter::ocr(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	int debugmode = 0;
	checkInt(TK, 1, &debugmode);

	QString ret;
	if (injector.server->postGifCodeImage(&ret))
	{
		qDebug() << ret;
		if (!ret.isEmpty())
		{
			if (debugmode == 0)
				injector.server->inputtext(ret);
		}
	}

	return Parser::kNoChange;
}
