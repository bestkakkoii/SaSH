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

#include "stdafx.h"
#include "clua.h"
#include "injector.h"
#include "signaldispatcher.h"

qint64 CLuaSystem::sleep(qint64 t, sol::this_state s)
{
	if (t <= 0)
		return FALSE;

	if (t >= 1000)
	{
		qint64 i = 0;
		qint64 size = t / 1000;
		for (; i < size; ++i)
		{
			QThread::msleep(1000UL);
			luadebug::checkStopAndPause(s);
		}

		if (i % 1000 > 0)
			QThread::msleep(static_cast<DWORD>(i) % 1000UL);
		else
			return FALSE;
	}
	else if (t > 0)
		QThread::msleep(static_cast<DWORD>(t));
	else
		return FALSE;

	return TRUE;
}

qint64 CLuaSystem::logout(sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (!injector.server.isNull())
	{
		injector.server->logOut();
		return TRUE;
	}

	return FALSE;
}

qint64 CLuaSystem::logback(sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (!injector.server.isNull())
	{
		injector.server->logBack();
		return TRUE;
	}

	return FALSE;
}

qint64 CLuaSystem::eo(sol::this_state s)
{
	sol::state_view lua(s);
	qint64 currentIndex = lua["_INDEX"].get<qint64>();
	Injector& injector = Injector::getInstance(currentIndex);
	if (injector.server.isNull())
		return 0;

	luadebug::checkBattleThenWait(s);

	QElapsedTimer timer; timer.start();
	injector.server->EO();

	bool bret = luadebug::waitfor(s, 5000, [currentIndex]() { return !Injector::getInstance(currentIndex).server->isEOTTLSend.load(std::memory_order_acquire); });

	qint64 result = bret ? injector.server->lastEOTime.load(std::memory_order_acquire) : 0;

	return result;
}

qint64 CLuaSystem::openlog(std::string sfilename, sol::object oformat, sol::object obuffersize, sol::this_state s)
{
	sol::state_view lua(s);
	qint64 currentIndex = lua["_INDEX"].get<qint64>();
	Injector& injector = Injector::getInstance(currentIndex);

	QString filename = util::toQString(sfilename);
	if (filename.isEmpty())
		return false;

	QString format = "%(time) %(message)";
	if (oformat.is<std::string>())
		format = util::toQString(oformat);

	qint64 buffersize = 1024;
	if (obuffersize.is<qint64>())
		buffersize = obuffersize.as<qint64>();

	bool bret = injector.log.initialize(filename, buffersize, format);
	return bret;
}

//這裡還沒想好format格式怎麼設計，暫時先放著
qint64 CLuaSystem::print(sol::object ocontent, sol::object ocolor, sol::this_state s)
{
	sol::state_view lua(s);
	luadebug::tryPopCustomErrorMsg(s, luadebug::ERROR_PARAM_SIZE_RANGE, 1, 3);
	QString msg("\0");
	QString raw("\0");
	if (ocontent == sol::lua_nil)
	{
		raw = "nil";
		msg = "nil";
	}
	else if (ocontent.is<bool>())
	{
		bool value = ocontent.as<bool>();
		raw = util::toQString(value);
		msg = raw;
	}
	else if (ocontent.is<qlonglong>())
	{
		const qlonglong value = ocontent.as<qlonglong>();
		raw = util::toQString(value);
		msg = raw;
	}
	else if (ocontent.is<qreal>())
	{
		const qreal value = ocontent.as<qreal>();
		raw = util::toQString(value);
		msg = raw;
	}
	else if (ocontent.is<std::string>())
	{
		raw = util::toQString(ocontent);
		msg = raw;
	}
	else if (ocontent.is<const char*>())
	{
		raw = (ocontent.as<const char*>());
		msg = raw;
	}
	else if (ocontent.is<sol::table>())
	{
		//print table
		raw = luadebug::getTableVars(s.L, 1, 50);
		msg = raw;

	}
	else
	{
		QString pointerStr = util::toQString(reinterpret_cast<qint64>(ocontent.pointer()), 16);
		//print type name
		switch (ocontent.get_type())
		{
		case sol::type::function: msg = "(function) 0x" + pointerStr; break;
		case sol::type::userdata: msg = "(userdata) 0x" + pointerStr; break;
		case sol::type::thread: msg = "(thread) 0x" + pointerStr; break;
		case sol::type::lightuserdata: msg = "(lightuserdata) 0x" + pointerStr; break;
		case sol::type::none: msg = "(none) 0x" + pointerStr; break;
		case sol::type::poly: msg = "(poly) 0x" + pointerStr; break;
		default: msg = "print error: unknown type that is unable to show"; break;
		}
	}

	int color = 4;
	if (ocolor.is<qint64>())
	{
		color = ocolor.as<qint64>();
		if (color != -2 && (color < 0 || color > 10))
			luadebug::tryPopCustomErrorMsg(s, luadebug::ERROR_PARAM_TYPE, false, 2, QObject::tr("invalid value of 'color'"));
	}
	else if (ocolor == sol::lua_nil)
	{
		color = QRandomGenerator64().bounded(0, 10);
	}

	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	bool split = false;
	bool doNotAnnounce = color == -2 || injector.server.isNull();
	QStringList l;
	if (msg.contains("\r\n"))
	{
		l = msg.split("\r\n");
		luadebug::logExport(s, l, color, doNotAnnounce);
		split = true;
	}
	else if (msg.contains("\n"))
	{
		l = msg.split("\n");
		luadebug::logExport(s, l, color, doNotAnnounce);
		split = true;
	}
	else
	{
		luadebug::logExport(s, msg, color, doNotAnnounce);
	}

	return FALSE;
}

qint64 CLuaSystem::messagebox(sol::object ostr, sol::object otype, sol::this_state s)
{
	sol::state_view lua(s);
	qint64 currentIndex = lua["_INDEX"].get<qint64>();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);

	QString text;
	if (ostr.is<qint64>())
		text = util::toQString(ostr.as<qint64>());
	else if (ostr.is<double>())
		text = util::toQString(ostr.as<double>());
	else if (ostr.is<std::string>())
		text = util::toQString(ostr);
	else
		luadebug::tryPopCustomErrorMsg(s, luadebug::ERROR_PARAM_TYPE, false, 1, QObject::tr("invalid value type"));

	qint64 type = 0;
	if (ostr.is<qint64>())
		type = ostr.as<qint64>();
	else if (ostr != sol::lua_nil)
		luadebug::tryPopCustomErrorMsg(s, luadebug::ERROR_PARAM_TYPE, false, 2, QObject::tr("invalid value of 'type'"));

	qint64 nret = QMessageBox::StandardButton::NoButton;
	emit signalDispatcher.messageBoxShow(text, type, "", &nret);
	if (nret != QMessageBox::StandardButton::NoButton)
	{
		return nret;
	}
	return 0;
}

qint64 CLuaSystem::talk(sol::object ostr, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (injector.server.isNull())
		return FALSE;

	QString text;
	if (ostr.is<qint64>())
		text = util::toQString(ostr.as<qint64>());
	else if (ostr.is<double>())
		text = util::toQString(ostr.as<double>());
	else if (ostr.is<std::string>())
		text = util::toQString(ostr);
	else if (ostr.is<sol::table>())
	{
		text = luadebug::getTableVars(s.L, 1, 10);
	}
	else
		luadebug::tryPopCustomErrorMsg(s, luadebug::ERROR_PARAM_TYPE, false, 1, QObject::tr("invalid value type"));


	injector.server->talk(text);

	return TRUE;
}

qint64 CLuaSystem::cleanchat(sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (injector.server.isNull())
		return FALSE;

	injector.server->cleanChatHistory();

	return TRUE;
}

qint64 CLuaSystem::menu(qint64 index, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->saMenu(index);

	return TRUE;
}

qint64 CLuaSystem::menu(qint64 type, qint64 index, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	if (type == 0)
		injector.server->saMenu(index);
	else
		injector.server->shopOk(index);

	return TRUE;
}

qint64 CLuaSystem::savesetting(const std::string& sfileName, sol::this_state s)
{
	QString fileName = util::toQString(sfileName);
	fileName.replace("\\", "/");
	fileName = util::applicationDirPath() + "/settings/" + fileName;
	fileName.replace("\\", "/");
	fileName.replace("//", "/");

	QFileInfo fileInfo(fileName);
	QString suffix = fileInfo.suffix();
	if (suffix.isEmpty())
		fileName += ".json";
	else if (suffix != "json")
		fileName.replace(suffix, "json");

	if (!QFile::exists(fileName))
		return FALSE;

	sol::state_view lua(s);
	qint64 currentIndex = lua["_INDEX"].get<qint64>();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	emit signalDispatcher.saveHashSettings(fileName, true);

	return TRUE;
}

qint64 CLuaSystem::loadsetting(const std::string& sfileName, sol::this_state s)
{
	QString fileName = util::toQString(sfileName);
	fileName.replace("\\", "/");
	fileName = util::applicationDirPath() + "/settings/" + fileName;
	fileName.replace("\\", "/");
	fileName.replace("//", "/");

	QFileInfo fileInfo(fileName);
	QString suffix = fileInfo.suffix();
	if (suffix.isEmpty())
		fileName += ".json";
	else if (suffix != "json")
		fileName.replace(suffix, "json");

	if (!QFile::exists(fileName))
		return FALSE;

	sol::state_view lua(s);
	qint64 currentIndex = lua["_INDEX"].get<qint64>();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	emit signalDispatcher.loadHashSettings(fileName, true);

	return TRUE;
}

qint64 CLuaSystem::press(std::string sbuttonStr, qint64 unitid, qint64 dialogid, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	QString text = util::toQString(sbuttonStr);
	qint64 row = -1;
	BUTTON_TYPE button = buttonMap.value(text.toUpper(), BUTTON_NOTUSED);
	if (button == BUTTON_NOTUSED)
	{
		qint64 row = -1;
		dialog_t dialog = injector.server->currentDialog;
		QStringList textList = dialog.linebuttontext;
		if (!textList.isEmpty())
		{
			bool isExact = true;
			QString newText = text.toUpper();
			if (newText.startsWith(""))
			{
				newText = newText.mid(1);
				isExact = false;
			}

			for (qint64 i = 0; i < textList.size(); ++i)
			{
				if (!isExact && textList.value(i).toUpper().contains(newText))
				{
					row = i;
					break;
				}
				else if (isExact && textList.value(i).toUpper() == newText)
				{
					row = i;
					break;
				}
			}
		}
	}

	if (button != BUTTON_NOTUSED)
		injector.server->press(button, dialogid, unitid);
	else if (row != -1)
		injector.server->press(row, dialogid, unitid);
	else
		return FALSE;

	return TRUE;
}

qint64 CLuaSystem::press(qint64 row, qint64 unitid, qint64 dialogid, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->press(row, dialogid, unitid);

	return TRUE;
}

qint64 CLuaSystem::input(const std::string& str, qint64 unitid, qint64 dialogid, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	QString text = util::toQString(str);

	injector.server->inputtext(text, dialogid, unitid);

	return TRUE;
}

qint64 CLuaSystem::leftclick(qint64 x, qint64 y, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (injector.server.isNull())
		return FALSE;

	injector.leftClick(x, y);
	return TRUE;
}

qint64 CLuaSystem::rightclick(qint64 x, qint64 y, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (injector.server.isNull())
		return FALSE;

	injector.rightClick(x, y);
	return TRUE;
}

qint64 CLuaSystem::leftdoubleclick(qint64 x, qint64 y, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (injector.server.isNull())
		return FALSE;

	injector.leftDoubleClick(x, y);
	return TRUE;
}

qint64 CLuaSystem::mousedragto(qint64 x1, qint64 y1, qint64 x2, qint64 y2, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (injector.server.isNull())
		return FALSE;

	injector.dragto(x1, y1, x2, y2);
	return TRUE;
}

qint64 CLuaSystem::set(std::string enumStr,
	sol::object p1, sol::object p2, sol::object p3, sol::object p4, sol::object p5, sol::object p6, sol::object p7, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(lua["_INDEX"].get<qint64>());

	QString typeStr = util::toQString(enumStr);
	if (typeStr.isEmpty())
		return FALSE;

	const QHash<QString, util::UserSetting> hash = {
			{ "debug", util::kScriptDebugModeEnable },
#pragma region zh_TW
			/*{"戰鬥道具補血戰寵", util::kBattleItemHealPetValue},
			{ "戰鬥道具補血隊友", util::kBattleItemHealAllieValue },
			{ "戰鬥道具補血人物", util::kBattleItemHealCharValue },*/
			{ "戰鬥道具補血", util::kBattleItemHealEnable },//{ "戰鬥道具補血", util::kBattleItemHealItemString },//{ "戰鬥道具肉優先", util::kBattleItemHealMeatPriorityEnable },{ "BattleItemHealTargetValue", util::kBattleItemHealTargetValue },

			{ "戰鬥精靈復活", util::kBattleMagicReviveEnable },//{ "BattleMagicReviveMagicValue", util::kBattleMagicReviveMagicValue },{ "BattleMagicReviveTargetValue", util::kBattleMagicReviveTargetValue },
			{ "戰鬥道具復活", util::kBattleItemReviveEnable },//{ "戰鬥道具復活", util::kBattleItemReviveItemString },{ "BattleItemReviveTargetValue", util::kBattleItemReviveTargetValue },

			/*{"ItemHealCharNormalValue", util::kNormalItemHealCharValue},
			{ "ItemHealPetNormalValue", util::kNormalItemHealPetValue },
			{ "ItemHealAllieNormalValue", util::kNormalItemHealAllieValue },*/
			{ "道具補血", util::kNormalItemHealEnable },//{ "平時道具補血", util::kNormalItemHealItemString },{ "道具肉優先", util::kNormalItemHealMeatPriorityEnable },

			{ "自動丟棄寵物", util::kDropPetEnable },//{ "自動丟棄寵物名單", util::kDropPetNameString },
			{ "自動丟棄寵物攻", util::kDropPetStrEnable },//{ "自動丟棄寵物攻數值", util::kDropPetStrValue },
			{ "自動丟棄寵物防", util::kDropPetDefEnable },//{ "自動丟棄寵物防數值", util::kDropPetDefValue },
			{ "自動丟棄寵物敏", util::kDropPetAgiEnable },//{ "自動丟棄寵物敏數值", util::kDropPetAgiValue },
			{ "自動丟棄寵物血", util::kDropPetHpEnable },//{ "自動丟棄寵物血數值", util::kDropPetHpValue },
			{ "自動丟棄寵物攻防敏", util::kDropPetAggregateEnable },//{ "自動丟棄寵物攻防敏數值", util::kDropPetAggregateValue },

			{ "自動加點", util::kAutoAbilityEnable },

			//ok
			{ "主機", util::kServerValue },
			{ "副機", util::kSubServerValue },
			{ "位置", util::kPositionValue },

			{ "加速", util::kSpeedBoostValue },
			{ "帳號", util::kGameAccountString },
			{ "賬號", util::kGameAccountString },
			{ "密碼", util::kGamePasswordString },
			{ "安全碼", util::kGameSecurityCodeString },
			{ "遠程白名單", util::kMailWhiteListString },

			{ "自走延時", util::kAutoWalkDelayValue },
			{ "自走步長", util::kAutoWalkDistanceValue },
			{ "自走方向", util::kAutoWalkDirectionValue },

			{ "腳本速度", util::kScriptSpeedValue },

			{ "自動登陸", util::kAutoLoginEnable },
			{ "斷線重連", util::kAutoReconnectEnable },
			{ "隱藏人物", util::kHideCharacterEnable },
			{ "關閉特效", util::kCloseEffectEnable },
			{ "資源優化", util::kOptimizeEnable },
			{ "隱藏石器", util::kHideWindowEnable },
			{ "屏蔽聲音", util::kMuteEnable },

			{ "自動調整內存", util::kAutoFreeMemoryEnable },
			{ "快速走路", util::kFastWalkEnable },
			{ "橫沖直撞", util::kPassWallEnable },
			{ "鎖定原地", util::kLockMoveEnable },
			{ "鎖定畫面", util::kLockImageEnable },
			{ "自動丟肉", util::kAutoDropMeatEnable },
			{ "自動疊加", util::kAutoStackEnable },
			{ "自動疊加", util::kAutoStackEnable },
			{ "自動KNPC", util::kKNPCEnable },
			{ "自動猜謎", util::kAutoAnswerEnable },
			{ "自動吃豆", util::kAutoEatBeanEnable },
			{ "走路遇敵", util::kAutoWalkEnable },
			{ "快速遇敵", util::kFastAutoWalkEnable },
			{ "快速戰鬥", util::kFastBattleEnable },
			{ "自動戰鬥", util::kAutoBattleEnable },
			{ "自動捉寵", util::kAutoCatchEnable },
			{ "自動逃跑", util::kAutoEscapeEnable },
			{ "戰鬥99秒", util::kBattleTimeExtendEnable },
			{ "落馬逃跑", util::kFallDownEscapeEnable },
			{ "顯示經驗", util::kShowExpEnable },
			{ "窗口吸附", util::kWindowDockEnable },
			{ "自動換寵", util::kBattleAutoSwitchEnable },
			{ "自動EO", util::kBattleAutoEOEnable },

			{ "隊伍開關", util::kSwitcherTeamEnable },
			{ "PK開關", util::kSwitcherPKEnable },
			{ "交名開關", util::kSwitcherCardEnable },
			{ "交易開關", util::kSwitcherTradeEnable },
			{ "組頻開關", util::kSwitcherGroupEnable },
			{ "家頻開關", util::kSwitcherFamilyEnable },
			{ "職頻開關", util::kSwitcherJobEnable },
			{ "世界開關", util::kSwitcherWorldEnable },

			{ "鎖定戰寵", util::kLockPetEnable },//{ "戰後自動鎖定戰寵編號", util::kLockPetValue },
			{ "鎖定騎寵", util::kLockRideEnable },//{ "戰後自動鎖定騎寵編號", util::kLockRideValue },
			{ "鎖寵排程", util::kLockPetScheduleEnable },
			{ "鎖定時間", util::kLockTimeEnable },//{ "時間", util::kLockTimeValue },

			{ "捉寵模式", util::kBattleCatchModeValue },
			{ "捉寵等級", util::kBattleCatchTargetLevelEnable },//{ "捉寵目標等級", util::kBattleCatchTargetLevelValue },
			{ "捉寵血量", util::kBattleCatchTargetMaxHpEnable },//{ "捉寵目標最大耐久力", util::kBattleCatchTargetMaxHpValue },
			{ "捉寵目標", util::kBattleCatchPetNameString },
			{ "捉寵寵技能", util::kBattleCatchPetSkillEnable },////{ "捉寵戰寵技能索引", util::kBattleCatchPetSkillValue },
			{ "捉寵道具", util::kBattleCatchCharItemEnable },//{ "捉寵使用道具直到血量低於", util::kBattleCatchTargetItemHpValue },{ "捉寵道具", util::kBattleCatchCharItemString },
			{ "捉寵精靈", util::kBattleCatchCharMagicEnable },//{ "捉寵使用精靈直到血量低於", util::kBattleCatchTargetMagicHpValue },{ "捉寵人物精靈索引", util::kBattleCatchCharMagicValue },

			{ "自動組隊", util::kAutoJoinEnable },//{ "自動組隊名稱", util::kAutoFunNameString },	{ "自動移動功能編號", util::kAutoFunTypeValue },
			{ "自動丟棄", util::kAutoDropEnable },//{ "自動丟棄名單", util::kAutoDropItemString },
			{ "鎖定攻擊", util::kLockAttackEnable },//{ "鎖定攻擊名單", util::kLockAttackString },
			{ "鎖定逃跑", util::kLockEscapeEnable },//{ "鎖定逃跑名單", util::kLockEscapeString },
			{ "非鎖不逃", util::kBattleNoEscapeWhileLockPetEnable },

			{ "道具補氣", util::kNormalItemHealMpEnable },//{ "ItemHealMpNormalValue", util::kNormalItemHealMpValue },{ "平時道具補氣", util::kNormalItemHealMpItemString },
			{ "戰鬥道具補氣", util::kBattleItemHealMpEnable },//{ "戰鬥道具補氣人物", util::kBattleItemHealMpValue },{ "戰鬥道具補氣 ", util::kBattleItemHealMpItemString },
			{ "戰鬥嗜血補氣", util::kBattleSkillMpEnable },//{ "戰鬥嗜血補氣技能", util::kBattleSkillMpSkillValue },{ "戰鬥嗜血補氣百分比", util::kBattleSkillMpValue },

			/*{"MagicHealCharNormalValue", util::kNormalMagicHealCharValue},
			{ "MagicHealPetNormalValue", util::kNormalMagicHealPetValue },
			{ "MagicHealAllieNormalValue", util::kNormalMagicHealAllieValue },*/
			{ "精靈補血", util::kNormalMagicHealEnable },//{ "平時精靈補血精靈索引", util::kNormalMagicHealMagicValue },
			/*{ "戰鬥精靈補血人物", util::kBattleMagicHealCharValue },
			{"戰鬥精靈補血戰寵", util::kBattleMagicHealPetValue},
			{ "戰鬥精靈補血隊友", util::kBattleMagicHealAllieValue },*/
			{ "戰鬥精靈補血", util::kBattleMagicHealEnable },//util::kBattleMagicHealMagicValue, util::kBattleMagicHealTargetValue,

			{ "戰鬥指定回合", util::kBattleCharRoundActionRoundValue },
			{ "戰鬥間隔回合", util::kBattleCrossActionCharEnable },
			{ "戰鬥一般", util::kBattleCharNormalActionTypeValue },

			{ "戰鬥寵指定回合RoundValue", util::kBattlePetRoundActionRoundValue },
			{ "戰鬥寵間隔回合", util::kBattleCrossActionPetEnable },
			{ "戰鬥寵一般", util::kBattlePetNormalActionTypeValue },

			{ "攻擊延時", util::kBattleActionDelayValue },
			{ "動作重發", util::kBattleResendDelayValue },
#pragma endregion

#pragma region zh_CN

			/*{"战斗道具补血战宠", util::kBattleItemHealPetValue},
			{ "战斗道具补血队友", util::kBattleItemHealAllieValue },
			{ "战斗道具补血人物", util::kBattleItemHealCharValue },*/
			{ "战斗道具补血", util::kBattleItemHealEnable },//{ "战斗道具补血", util::kBattleItemHealItemString },//{ "战斗道具肉优先", util::kBattleItemHealMeatPriorityEnable },{ "BattleItemHealTargetValue", util::kBattleItemHealTargetValue },

			{ "战斗精灵復活", util::kBattleMagicReviveEnable },//{ "BattleMagicReviveMagicValue", util::kBattleMagicReviveMagicValue },{ "BattleMagicReviveTargetValue", util::kBattleMagicReviveTargetValue },
			{ "战斗道具復活", util::kBattleItemReviveEnable },//{ "战斗道具復活", util::kBattleItemReviveItemString },{ "BattleItemReviveTargetValue", util::kBattleItemReviveTargetValue },

			/*{"ItemHealCharNormalValue", util::kNormalItemHealCharValue},
			{ "ItemHealPetNormalValue", util::kNormalItemHealPetValue },
			{ "ItemHealAllieNormalValue", util::kNormalItemHealAllieValue },*/
			{ "道具补血", util::kNormalItemHealEnable },//{ "平时道具补血", util::kNormalItemHealItemString },{ "道具肉优先", util::kNormalItemHealMeatPriorityEnable },

			{ "自动丢弃宠物", util::kDropPetEnable },//{ "自动丢弃宠物名单", util::kDropPetNameString },
			{ "自动丢弃宠物攻", util::kDropPetStrEnable },//{ "自动丢弃宠物攻数值", util::kDropPetStrValue },
			{ "自动丢弃宠物防", util::kDropPetDefEnable },//{ "自动丢弃宠物防数值", util::kDropPetDefValue },
			{ "自动丢弃宠物敏", util::kDropPetAgiEnable },//{ "自动丢弃宠物敏数值", util::kDropPetAgiValue },
			{ "自动丢弃宠物血", util::kDropPetHpEnable },//{ "自动丢弃宠物血数值", util::kDropPetHpValue },
			{ "自动丢弃宠物攻防敏", util::kDropPetAggregateEnable },//{ "自动丢弃宠物攻防敏数值", util::kDropPetAggregateValue },

			{ "自动加点", util::kAutoAbilityEnable },


			//ok
			{ "EO命令", util::kEOCommandString },
			{ "主机", util::kServerValue },
			{ "副机", util::kSubServerValue },
			{ "位置", util::kPositionValue },

			{ "加速", util::kSpeedBoostValue },
			{ "帐号", util::kGameAccountString },
			{ "账号", util::kGameAccountString },
			{ "密码", util::kGamePasswordString },
			{ "安全码", util::kGameSecurityCodeString },
			{ "远程白名单", util::kMailWhiteListString },

			{ "自走延时", util::kAutoWalkDelayValue },
			{ "自走步长", util::kAutoWalkDistanceValue },
			{ "自走方向", util::kAutoWalkDirectionValue },

			{ "脚本速度", util::kScriptSpeedValue },

			{ "自动登陆", util::kAutoLoginEnable },
			{ "断线重连", util::kAutoReconnectEnable },
			{ "隐藏人物", util::kHideCharacterEnable },
			{ "关闭特效", util::kCloseEffectEnable },
			{ "资源优化", util::kOptimizeEnable },
			{ "隐藏石器", util::kHideWindowEnable },
			{ "屏蔽声音", util::kMuteEnable },

			{ "自动调整内存", util::kAutoFreeMemoryEnable },
			{ "快速走路", util::kFastWalkEnable },
			{ "横冲直撞", util::kPassWallEnable },
			{ "锁定原地", util::kLockMoveEnable },
			{ "锁定画面", util::kLockImageEnable },
			{ "自动丢肉", util::kAutoDropMeatEnable },
			{ "自动叠加", util::kAutoStackEnable },
			{ "自动迭加", util::kAutoStackEnable },
			{ "自动KNPC", util::kKNPCEnable },
			{ "自动猜谜", util::kAutoAnswerEnable },
			{ "自动吃豆", util::kAutoEatBeanEnable },
			{ "走路遇敌", util::kAutoWalkEnable },
			{ "快速遇敌", util::kFastAutoWalkEnable },
			{ "快速战斗", util::kFastBattleEnable },
			{ "自动战斗", util::kAutoBattleEnable },
			{ "自动捉宠", util::kAutoCatchEnable },
			{ "自动逃跑", util::kAutoEscapeEnable },
			{ "战斗99秒", util::kBattleTimeExtendEnable },
			{ "落马逃跑", util::kFallDownEscapeEnable },
			{ "显示经验", util::kShowExpEnable },
			{ "窗口吸附", util::kWindowDockEnable },
			{ "自动换宠", util::kBattleAutoSwitchEnable },
			{ "自动EO", util::kBattleAutoEOEnable },

			{ "队伍开关", util::kSwitcherTeamEnable },
			{ "PK开关", util::kSwitcherPKEnable },
			{ "交名开关", util::kSwitcherCardEnable },
			{ "交易开关", util::kSwitcherTradeEnable },
			{ "组频开关", util::kSwitcherGroupEnable },
			{ "家频开关", util::kSwitcherFamilyEnable },
			{ "职频开关", util::kSwitcherJobEnable },
			{ "世界开关", util::kSwitcherWorldEnable },

			{ "锁定战宠", util::kLockPetEnable },//{ "战后自动锁定战宠编号", util::kLockPetValue },
			{ "锁定骑宠", util::kLockRideEnable },//{ "战后自动锁定骑宠编号", util::kLockRideValue },
			{ "锁宠排程", util::kLockPetScheduleEnable },
			{ "锁定时间", util::kLockTimeEnable },//{ "时间", util::kLockTimeValue },

			{ "捉宠模式", util::kBattleCatchModeValue },
			{ "捉宠等级", util::kBattleCatchTargetLevelEnable },//{ "捉宠目标等级", util::kBattleCatchTargetLevelValue },
			{ "捉宠血量", util::kBattleCatchTargetMaxHpEnable },//{ "捉宠目标最大耐久力", util::kBattleCatchTargetMaxHpValue },
			{ "捉宠目标", util::kBattleCatchPetNameString },
			{ "捉宠宠技能", util::kBattleCatchPetSkillEnable },////{ "捉宠战宠技能索引", util::kBattleCatchPetSkillValue },
			{ "捉宠道具", util::kBattleCatchCharItemEnable },//{ "捉宠使用道具直到血量低于", util::kBattleCatchTargetItemHpValue },{ "捉宠道具", util::kBattleCatchCharItemString },
			{ "捉宠精灵", util::kBattleCatchCharMagicEnable },//{ "捉宠使用精灵直到血量低于", util::kBattleCatchTargetMagicHpValue },{ "捉宠人物精灵索引", util::kBattleCatchCharMagicValue },

			{ "自动组队", util::kAutoJoinEnable },//{ "自动组队名称", util::kAutoFunNameString },	{ "自动移动功能编号", util::kAutoFunTypeValue },
			{ "自动丢弃", util::kAutoDropEnable },//{ "自动丢弃名单", util::kAutoDropItemString },
			{ "锁定攻击", util::kLockAttackEnable },//{ "锁定攻击名单", util::kLockAttackString },
			{ "锁定逃跑", util::kLockEscapeEnable },//{ "锁定逃跑名单", util::kLockEscapeString },
			{ "非锁不逃", util::kBattleNoEscapeWhileLockPetEnable },

			{ "道具补气", util::kNormalItemHealMpEnable },//{ "ItemHealMpNormalValue", util::kNormalItemHealMpValue },{ "平时道具补气", util::kNormalItemHealMpItemString },
			{ "战斗道具补气", util::kBattleItemHealMpEnable },//{ "战斗道具补气人物", util::kBattleItemHealMpValue },{ "战斗道具补气 ", util::kBattleItemHealMpItemString },
			{ "战斗嗜血补气", util::kBattleSkillMpEnable },//{ "战斗嗜血补气技能", util::kBattleSkillMpSkillValue },{ "战斗嗜血补气百分比", util::kBattleSkillMpValue },

			/*{"MagicHealCharNormalValue", util::kNormalMagicHealCharValue},
			{ "MagicHealPetNormalValue", util::kNormalMagicHealPetValue },
			{ "MagicHealAllieNormalValue", util::kNormalMagicHealAllieValue },*/
			{ "精灵补血", util::kNormalMagicHealEnable },//{ "平时精灵补血精灵索引", util::kNormalMagicHealMagicValue },
			/*{ "战斗精灵补血人物", util::kBattleMagicHealCharValue },
			{"战斗精灵补血战宠", util::kBattleMagicHealPetValue},
			{ "战斗精灵补血队友", util::kBattleMagicHealAllieValue },*/
			{ "战斗精灵补血", util::kBattleMagicHealEnable },//util::kBattleMagicHealMagicValue, util::kBattleMagicHealTargetValue,

			{ "战斗指定回合", util::kBattleCharRoundActionRoundValue },
			{ "战斗间隔回合", util::kBattleCrossActionCharEnable },
			{ "战斗一般", util::kBattleCharNormalActionTypeValue },

			{ "战斗宠指定回合RoundValue", util::kBattlePetRoundActionRoundValue },
			{ "战斗宠间隔回合", util::kBattleCrossActionPetEnable },
			{ "战斗宠一般", util::kBattlePetNormalActionTypeValue },

			{ "攻击延时", util::kBattleActionDelayValue },
			{ "动作重发", util::kBattleResendDelayValue },


			{ "战斗回合目标", util::kBattleCharRoundActionTargetValue },
			{ "战斗间隔目标",  util::kBattleCharCrossActionTargetValue },
			{ "战斗一般目标", util::kBattleCharNormalActionTargetValue },
			{ "战斗宠回合目标", util::kBattlePetRoundActionTargetValue },
			{ "战斗宠间隔目标", util::kBattlePetCrossActionTargetValue },
			{ "战斗宠一般目标",  util::kBattlePetNormalActionTargetValue },
			{ "战斗精灵补血目标", util::kBattleMagicHealTargetValue },
			{ "战斗道具补血目标", util::kBattleItemHealTargetValue },
			{ "战斗精灵復活目标", util::kBattleMagicReviveTargetValue },
			{ "战斗道具復活目标", util::kBattleItemReviveTargetValue },
	#pragma endregion
	};


	util::UserSetting type = hash.value(typeStr, util::kSettingNotUsed);
	if (type == util::kSettingNotUsed)
		return FALSE;

	if (type == util::kScriptDebugModeEnable)
	{
		qint64 value1 = 0;
		if (p1.is<qint64>())
			value1 = p1.as<qint64>();
		else if (p1.is<bool>())
			value1 = p1.as<bool>() ? 1 : 0;

		injector.setEnableHash(util::kScriptDebugModeEnable, value1 > 0);
		emit signalDispatcher.applyHashSettingsToUI();
		return TRUE;
	}

	if (type == util::kBattleCharNormalActionTypeValue)
	{
		qint64 value1 = 0;
		if (p1.is<qint64>())
			value1 = p1.as<qint64>();
		else if (p1.is<bool>())
			value1 = p1.as<bool>() ? 1 : 0;

		--value1;
		if (value1 < 0)
			value1 = 0;

		qint64 value2 = 0;
		if (p2.is<qint64>())
			value2 = p2.as<qint64>();
		else if (p2.is<bool>())
			value2 = p2.as<bool>() ? 1 : 0;
		--value2;
		if (value2 < 0)
			value2 = 0;

		qint64 value3 = 0;
		if (p3.is<qint64>())
			value3 = p3.as<qint64>();
		else if (p2.is<bool>())
			value3 = p3.as<bool>() ? 1 : 0;
		--value3;
		if (value3 < 0)
			value3 = 0;

		injector.setValueHash(util::kBattleCharNormalActionTypeValue, value1);
		injector.setValueHash(util::kBattleCharNormalActionEnemyValue, value2);
		injector.setValueHash(util::kBattleCharNormalActionLevelValue, value3);
		//injector.setValueHash(util::kBattleCharNormalActionTargetValue, value4);

		emit signalDispatcher.applyHashSettingsToUI();
		return TRUE;
	}
	else if (type == util::kBattlePetNormalActionTypeValue)
	{
		qint64 value1 = 0;
		if (p1.is<qint64>())
			value1 = p1.as<qint64>();
		else if (p1.is<bool>())
			value1 = p1.as<bool>() ? 1 : 0;

		--value1;
		if (value1 < 0)
			value1 = 0;

		qint64 value2 = 0;
		if (p2.is<qint64>())
			value2 = p2.as<qint64>();
		else if (p2.is<bool>())
			value2 = p2.as<bool>() ? 1 : 0;
		--value2;
		if (value2 < 0)
			value2 = 0;

		qint64 value3 = 0;
		if (p3.is<qint64>())
			value3 = p3.as<qint64>();
		else if (p3.is<bool>())
			value3 = p3.as<bool>() ? 1 : 0;
		--value3;
		if (value3 < 0)
			value3 = 0;

		injector.setValueHash(util::kBattlePetNormalActionTypeValue, value1);
		injector.setValueHash(util::kBattlePetNormalActionEnemyValue, value2);
		injector.setValueHash(util::kBattlePetNormalActionLevelValue, value3);
		//injector.setValueHash(util::kBattlePetNormalActionTargetValue, value4);

		emit signalDispatcher.applyHashSettingsToUI();
		return TRUE;
	}

	//start with 1
	switch (type)
	{
	case util::kAutoWalkDirectionValue://自走方向
	case util::kServerValue://主機
	case util::kSubServerValue://副機
	case util::kPositionValue://位置
	case util::kBattleCatchModeValue://戰鬥捕捉模式
	{
		if (!p1.is<qint64>() && !p1.is<bool>())
			return FALSE;

		qint64 value1 = 0;
		if (p1.is<qint64>())
			value1 = p1.as<qint64>();
		else if (p1.is<bool>())
			value1 = p1.as<bool>() ? 1 : 0;

		--value1;
		if (value1 < 0)
			value1 = 0;

		injector.setValueHash(type, value1);
		emit signalDispatcher.applyHashSettingsToUI();
		return TRUE;
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
	case util::kBattleActionDelayValue://攻擊延時
	case util::kBattleResendDelayValue://動作重發

	case util::kBattleCharRoundActionTargetValue:
	case util::kBattleCharCrossActionTargetValue:
	case util::kBattleCharNormalActionTargetValue:
	case util::kBattlePetRoundActionTargetValue:
	case util::kBattlePetCrossActionTargetValue:
	case util::kBattlePetNormalActionTargetValue:
	case util::kBattleMagicHealTargetValue:
	case util::kBattleItemHealTargetValue:
	case util::kBattleMagicReviveTargetValue:
	case util::kBattleItemReviveTargetValue:
	{
		if (!p1.is<qint64>() && !p1.is<bool>())
			return FALSE;

		qint64 value1 = 0;
		if (p1.is<qint64>())
			value1 = p1.as<qint64>();
		else if (p1.is<bool>())
			value1 = p1.as<bool>() ? 1 : 0;

		if (value1 < 0)
			value1 = 0;

		injector.setValueHash(type, value1);
		emit signalDispatcher.applyHashSettingsToUI();
		return TRUE;
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
	case util::kBattleNoEscapeWhileLockPetEnable:

	case util::kBattleTimeExtendEnable:
	case util::kFallDownEscapeEnable:
	case util::kShowExpEnable:
	case util::kWindowDockEnable:
	case util::kBattleAutoSwitchEnable:
	case util::kBattleAutoEOEnable:

		//switcher
	case util::kSwitcherTeamEnable:
	case util::kSwitcherPKEnable:
	case util::kSwitcherCardEnable:
	case util::kSwitcherTradeEnable:
	case util::kSwitcherGroupEnable:
	case util::kSwitcherFamilyEnable:
	case util::kSwitcherJobEnable:
	case util::kSwitcherWorldEnable:
	{
		if (!p1.is<qint64>() && !p1.is<bool>())
			return FALSE;

		qint64 value1 = 0;
		if (p1.is<qint64>())
			value1 = p1.as<qint64>();
		else if (p1.is<bool>())
			value1 = p1.as<bool>() ? 1 : 0;

		bool ok = value1 > 0;
		if (value1 < 0)
			value1 = 0;

		injector.setEnableHash(type, ok);
		if (type == util::kFastBattleEnable && ok)
		{
			injector.setEnableHash(util::kAutoBattleEnable, !ok);
			if (ok && !injector.server.isNull())
				injector.server->doBattleWork(false);
		}
		else if (type == util::kAutoBattleEnable && ok)
		{
			injector.setEnableHash(util::kFastBattleEnable, !ok);
			if (ok && !injector.server.isNull())
				injector.server->doBattleWork(false);
		}
		else if (type == util::kAutoWalkEnable && ok)
			injector.setEnableHash(util::kFastAutoWalkEnable, !ok);
		else if (type == util::kFastAutoWalkEnable && ok)
			injector.setEnableHash(util::kAutoWalkEnable, !ok);

		emit signalDispatcher.applyHashSettingsToUI();
		return TRUE;
	}
	default:
		break;
	}

	//0:close >1:open string value
	switch (type)
	{
	case util::kAutoAbilityEnable:
	case util::kBattleItemHealMpEnable:
	case util::kNormalItemHealMpEnable:
	case util::kBattleCatchCharItemEnable:
	case util::kLockAttackEnable:
	case util::kLockEscapeEnable:
	case util::kAutoDropEnable:
	case util::kAutoJoinEnable:
	case util::kLockPetScheduleEnable:
	{
		if (!p1.is<qint64>() && !p1.is<bool>())
			return FALSE;

		qint64 value1 = 0;
		if (p1.is<qint64>())
			value1 = p1.as<qint64>();
		else if (p1.is<bool>())
			value1 = p1.as<bool>() ? 1 : 0;

		bool ok = value1 > 0;
		if (value1 < 0)
			value1 = 0;

		QString text = util::toQString(p2);

		injector.setEnableHash(type, ok);
		if (type == util::kLockPetScheduleEnable && ok)
		{
			injector.setEnableHash(util::kLockPetEnable, !ok);
			injector.setEnableHash(util::kLockRideEnable, !ok);
			injector.setStringHash(util::kLockPetScheduleString, text);
		}
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
		else if (type == util::kBattleCatchCharItemEnable && ok)
		{
			injector.setStringHash(util::kBattleCatchCharItemString, text);
			injector.setValueHash(util::kBattleCatchTargetItemHpValue, value1 + 1);
		}
		else if (type == util::kNormalItemHealMpEnable && ok)
		{
			injector.setStringHash(util::kNormalItemHealMpItemString, text);
			injector.setValueHash(util::kNormalItemHealMpValue, value1 + 1);
		}
		else if (type == util::kBattleItemHealMpEnable && ok)
		{
			injector.setStringHash(util::kBattleItemHealMpItemString, text);
			injector.setValueHash(util::kBattleItemHealMpValue, value1 + 1);
		}
		else if (type == util::kAutoAbilityEnable)
		{
			injector.setStringHash(util::kAutoAbilityString, text);
		}

		emit signalDispatcher.applyHashSettingsToUI();
		return TRUE;
	}
	default:
		break;
	}

	//0:close >1:open qint64 value
	switch (type)
	{
	case util::kBattleSkillMpEnable:
	case util::kBattleCatchPetSkillEnable:
	case util::kBattleCatchTargetMaxHpEnable:
	case util::kBattleCatchTargetLevelEnable:
	case util::kLockTimeEnable:
	case util::kLockPetEnable:
	case util::kLockRideEnable:
	{
		if (!p1.is<qint64>() && !p1.is<bool>())
			return FALSE;

		qint64 value1 = 0;
		if (p1.is<qint64>())
			value1 = p1.as<qint64>();
		else if (p1.is<bool>())
			value1 = p1.as<bool>() ? 1 : 0;

		bool ok = value1 > 0;
		--value1;
		if (value1 < 0)
			value1 = 0;

		injector.setEnableHash(type, ok);
		if (type == util::kLockPetEnable && ok)
		{
			injector.setEnableHash(util::kLockPetScheduleEnable, !ok);
			injector.setValueHash(util::kLockPetValue, value1);
		}
		else if (type == util::kLockRideEnable && ok)
		{
			injector.setEnableHash(util::kLockPetScheduleEnable, !ok);
			injector.setValueHash(util::kLockRideValue, value1);
		}
		else if (type == util::kLockTimeEnable && ok)
			injector.setValueHash(util::kLockTimeValue, value1 + 1);
		else if (type == util::kBattleCatchTargetLevelEnable && ok)
			injector.setValueHash(util::kBattleCatchTargetLevelValue, value1 + 1);
		else if (type == util::kBattleCatchTargetMaxHpEnable && ok)
			injector.setValueHash(util::kBattleCatchTargetMaxHpValue, value1 + 1);
		else if (type == util::kBattleCatchPetSkillEnable && ok)
			injector.setValueHash(util::kBattleCatchPetSkillValue, value1 + 1);
		else if (type == util::kBattleSkillMpEnable)
			injector.setValueHash(util::kBattleSkillMpValue, value1 + 1);

		emit signalDispatcher.applyHashSettingsToUI();
		return TRUE;
	}
	default:
		break;
	}

	//0:close >1:open two qint64 value
	switch (type)
	{
	case util::kBattleCatchCharMagicEnable:
	{
		if (!p1.is<qint64>() && !p1.is<bool>())
			return FALSE;

		qint64 value1 = 0;
		if (p1.is<qint64>())
			value1 = p1.as<qint64>();
		else if (p1.is<bool>())
			value1 = p1.as<bool>() ? 1 : 0;

		bool ok = value1 > 0;
		if (value1 < 0)
			value1 = 0;

		qint64 value2 = 0;
		if (p2.is<qint64>())
			value2 = p2.as<qint64>();

		--value2;
		if (value2 < 0)
			value2 = 0;


		injector.setEnableHash(type, ok);
		if (type == util::kBattleCatchCharMagicEnable && ok)
		{
			injector.setValueHash(util::kBattleCatchTargetMagicHpValue, value1);
			injector.setValueHash(util::kBattleCatchCharMagicValue, value2);
		}

		emit signalDispatcher.applyHashSettingsToUI();
		return TRUE;
	}
	default:
		break;
	}

	//4int
	switch (type)
	{
	case util::kBattlePetRoundActionRoundValue:
	case util::kBattleCharRoundActionRoundValue:
	{
		qint64 value1 = p1.as<qint64>();
		bool ok = value1 > 0;
		if (value1 < 0)
			value1 = 0;

		qint64 value2 = 0;
		if (p2.is<qint64>())
			value2 = p2.as<qint64>();
		else if (p2.is<bool>())
			value2 = p2.as<bool>() ? 1 : 0;

		--value2;
		if (value2 < 0)
			value2 = 0;

		qint64 value3 = 0;
		if (p3.is<qint64>())
			value3 = p3.as<qint64>();
		else if (p2.is<bool>())
			value3 = p3.as<bool>() ? 1 : 0;
		--value3;
		if (value3 < 0)
			value3 = 0;

		qint64 value4 = 0;
		if (p4.is<qint64>())
			value4 = p4.as<qint64>();
		else if (p4.is<bool>())
			value4 = p4.as<bool>() ? 1 : 0;
		--value4;
		if (value4 < 0)
			value4 = 0;

		qint64 value5 = 0;
		if (p5.is<qint64>())
			value5 = p5.as<qint64>();
		else if (p5.is<bool>())
			value5 = p5.as<bool>() ? 1 : 0;
		if (value5 < 0)
			value5 = util::kSelectEnemyAny;

		injector.setValueHash(type, value1);
		if (type == util::kBattleCharRoundActionRoundValue && ok)
		{
			injector.setValueHash(util::kBattleCharRoundActionTypeValue, value2);
			injector.setValueHash(util::kBattleCharRoundActionEnemyValue, value3);
			injector.setValueHash(util::kBattleCharRoundActionLevelValue, value4);
			//injector.setValueHash(util::kBattleCharRoundActionTargetValue, value5);
		}
		else if (type == util::kBattlePetRoundActionRoundValue && ok)
		{
			injector.setValueHash(util::kBattlePetRoundActionTypeValue, value2);
			injector.setValueHash(util::kBattlePetRoundActionEnemyValue, value3);
			injector.setValueHash(util::kBattlePetRoundActionLevelValue, value4);
			//injector.setValueHash(util::kBattlePetRoundActionTargetValue, value5);
		}

		emit signalDispatcher.applyHashSettingsToUI();
		return TRUE;
	}
	default:
		break;
	}

	//1bool 4int
	switch (type)
	{
	case util::kBattleCrossActionPetEnable:
	case util::kBattleCrossActionCharEnable:
	case util::kBattleMagicHealEnable:
	case util::kNormalMagicHealEnable:
	{
		if (!p1.is<qint64>() && !p1.is<bool>())
			return FALSE;

		qint64 value1 = 0;
		if (p1.is<qint64>())
			value1 = p1.as<qint64>();
		else if (p1.is<bool>())
			value1 = p1.as<bool>() ? 1 : 0;

		bool ok = value1 > 0;
		--value1;

		if (value1 < 0)
			value1 = 0;
		injector.setEnableHash(type, ok);

		qint64 value2 = 0;
		if (p2.is<qint64>())
			value2 = p2.as<qint64>();
		else if (p2.is<bool>())
			value2 = p2.as<bool>() ? 1 : 0;
		--value2;
		if (value2 < 0)
			value2 = 0;

		qint64 value3 = 0;
		if (p3.is<qint64>())
			value3 = p3.as<qint64>();
		else if (p3.is<bool>())
			value3 = p3.as<bool>() ? 1 : 0;
		--value3;
		if (value3 < 0)
			value3 = 0;

		qint64 value4 = 0;
		if (p4.is<qint64>())
			value4 = p4.as<qint64>();
		else if (p4.is<bool>())
			value4 = p4.as<bool>() ? 1 : 0;
		--value4;
		if (value4 < 0)
			value4 = 0;

		qint64 value5 = util::kSelectSelf | util::kSelectPet;
		if (p5.is<qint64>())
			value5 = p5.as<qint64>();

		if (type == util::kNormalMagicHealEnable && ok)
		{
			injector.setValueHash(util::kNormalMagicHealMagicValue, value1);
			injector.setValueHash(util::kNormalMagicHealCharValue, value2 + 1);
			injector.setValueHash(util::kNormalMagicHealPetValue, value3 + 1);
			injector.setValueHash(util::kNormalMagicHealAllieValue, value4 + 1);
		}
		else if (type == util::kBattleMagicHealEnable && ok)
		{
			injector.setValueHash(util::kBattleMagicHealMagicValue, value1);
			injector.setValueHash(util::kBattleMagicHealCharValue, value2 + 1);
			injector.setValueHash(util::kBattleMagicHealPetValue, value3 + 1);
			injector.setValueHash(util::kBattleMagicHealAllieValue, value4 + 1);
			//injector.setValueHash(util::kBattleMagicHealTargetValue, value5);
		}
		else if (type == util::kBattleCrossActionCharEnable && ok)
		{
			injector.setValueHash(util::kBattleCharCrossActionTypeValue, value1);
			injector.setValueHash(util::kBattleCharCrossActionRoundValue, value2);
			//injector.setValueHash(util::kBattleCharCrossActionTargetValue, value3);
		}
		else if (type == util::kBattleCrossActionPetEnable && ok)
		{
			injector.setValueHash(util::kBattlePetCrossActionTypeValue, value1);
			injector.setValueHash(util::kBattlePetCrossActionRoundValue, value2);
			//injector.setValueHash(util::kBattlePetCrossActionTargetValue, value3);
		}

		emit signalDispatcher.applyHashSettingsToUI();
		return TRUE;
	}
	default:
		break;
	}

	//string only
	switch (type)
	{
	case util::kEOCommandString:
	case util::kGameAccountString:
	case util::kGamePasswordString:
	case util::kGameSecurityCodeString:
	case util::kMailWhiteListString:
	case util::kBattleCatchPetNameString:
	{
		QString text;
		if (!p1.is<std::string>())
		{
			if (!p1.is<qint64>())
				return FALSE;

			text = util::toQString(p1.as<qint64>());
		}
		else
			text = util::toQString(p1);

		injector.setStringHash(type, text);
		emit signalDispatcher.applyHashSettingsToUI();
		return TRUE;
	}
	default:
		break;
	}

	return FALSE;
}