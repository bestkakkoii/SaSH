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

//這裡還沒想好format格式怎麼設計，暫時先放著
qint64 CLuaSystem::announce(sol::object maybe_defaulted, sol::object ocolor, sol::this_state s)
{
	sol::state_view lua(s);
	luadebug::tryPopCustomErrorMsg(s, luadebug::ERROR_PARAM_SIZE_RANGE, 1, 3);
	QString msg("\0");
	QString raw("\0");
	if (maybe_defaulted == sol::lua_nil)
	{
		raw = "nil";
		msg = "(nil)";
	}
	else if (maybe_defaulted.is<bool>())
	{
		bool value = maybe_defaulted.as<bool>();
		raw = util::toQString(value);
		msg = raw;
	}
	else if (maybe_defaulted.is<qlonglong>())
	{
		const qlonglong value = maybe_defaulted.as<qlonglong>();
		raw = util::toQString(value);
		msg = raw;
	}
	else if (maybe_defaulted.is<qreal>())
	{
		const qreal value = maybe_defaulted.as<qreal>();
		raw = util::toQString(value);
		msg = raw;
	}
	else if (maybe_defaulted.is<std::string>())
	{
		raw = util::toQString(maybe_defaulted);
		msg = raw;
	}
	else if (maybe_defaulted.is<const char*>())
	{
		raw = (maybe_defaulted.as<const char*>());
		msg = raw;
	}
	else if (maybe_defaulted.is<sol::table>())
	{
		//print table
		raw = luadebug::getTableVars(s.L, 1, 50);
		msg = raw;

	}
	else
	{
		QString pointerStr = util::toQString(reinterpret_cast<qint64>(maybe_defaulted.pointer()), 16);
		//print type name
		switch (maybe_defaulted.get_type())
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
	QStringList l;
	if (msg.contains("\r\n"))
	{
		l = msg.split("\r\n");
		luadebug::logExport(s, l, color);
		split = true;
	}
	else if (msg.contains("\n"))
	{
		l = msg.split("\n");
		luadebug::logExport(s, l, color);
		split = true;
	}
	else
	{
		luadebug::logExport(s, msg, color);
	}

	bool announceOk = color != -2 && !injector.server.isNull();

	if (split)
	{
		for (const QString& it : l)
		{
			if (announceOk)
				injector.server->announce(it, color);
		}
	}
	else
	{
		if (announceOk)
			injector.server->announce(msg, color);
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
				if (!isExact && textList.at(i).toUpper().contains(newText))
				{
					row = i;
					break;
				}
				else if (isExact && textList.at(i).toUpper() == newText)
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

qint64 CLuaSystem::set(std::string enumStr, sol::object p1, sol::object p2, sol::object p3, sol::object p4, sol::object p5, sol::object p6, sol::object p7, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(lua["_INDEX"].get<qint64>());

	QString typeStr = util::toQString(enumStr);
	if (typeStr.isEmpty())
		return FALSE;

	const QHash<QString, util::UserSetting> hash = {
			{ u8"debug", util::kScriptDebugModeEnable },
#pragma region zh_TW
			/*{u8"戰鬥道具補血戰寵", util::kBattleItemHealPetValue},
			{ u8"戰鬥道具補血隊友", util::kBattleItemHealAllieValue },
			{ u8"戰鬥道具補血人物", util::kBattleItemHealCharValue },*/
			{ u8"戰鬥道具補血", util::kBattleItemHealEnable },//{ u8"戰鬥道具補血", util::kBattleItemHealItemString },//{ u8"戰鬥道具肉優先", util::kBattleItemHealMeatPriorityEnable },{ u8"BattleItemHealTargetValue", util::kBattleItemHealTargetValue },

			{ u8"戰鬥精靈復活", util::kBattleMagicReviveEnable },//{ u8"BattleMagicReviveMagicValue", util::kBattleMagicReviveMagicValue },{ u8"BattleMagicReviveTargetValue", util::kBattleMagicReviveTargetValue },
			{ u8"戰鬥道具復活", util::kBattleItemReviveEnable },//{ u8"戰鬥道具復活", util::kBattleItemReviveItemString },{ u8"BattleItemReviveTargetValue", util::kBattleItemReviveTargetValue },

			/*{u8"ItemHealCharNormalValue", util::kNormalItemHealCharValue},
			{ u8"ItemHealPetNormalValue", util::kNormalItemHealPetValue },
			{ u8"ItemHealAllieNormalValue", util::kNormalItemHealAllieValue },*/
			{ u8"道具補血", util::kNormalItemHealEnable },//{ u8"平時道具補血", util::kNormalItemHealItemString },{ u8"道具肉優先", util::kNormalItemHealMeatPriorityEnable },

			{ u8"自動丟棄寵物", util::kDropPetEnable },//{ u8"自動丟棄寵物名單", util::kDropPetNameString },
			{ u8"自動丟棄寵物攻", util::kDropPetStrEnable },//{ u8"自動丟棄寵物攻數值", util::kDropPetStrValue },
			{ u8"自動丟棄寵物防", util::kDropPetDefEnable },//{ u8"自動丟棄寵物防數值", util::kDropPetDefValue },
			{ u8"自動丟棄寵物敏", util::kDropPetAgiEnable },//{ u8"自動丟棄寵物敏數值", util::kDropPetAgiValue },
			{ u8"自動丟棄寵物血", util::kDropPetHpEnable },//{ u8"自動丟棄寵物血數值", util::kDropPetHpValue },
			{ u8"自動丟棄寵物攻防敏", util::kDropPetAggregateEnable },//{ u8"自動丟棄寵物攻防敏數值", util::kDropPetAggregateValue },


			//ok
			{ u8"主機", util::kServerValue },
			{ u8"副機", util::kSubServerValue },
			{ u8"位置", util::kPositionValue },

			{ u8"加速", util::kSpeedBoostValue },
			{ u8"帳號", util::kGameAccountString },
			{ u8"密碼", util::kGamePasswordString },
			{ u8"安全碼", util::kGameSecurityCodeString },
			{ u8"遠程白名單", util::kMailWhiteListString },

			{ u8"自走延時", util::kAutoWalkDelayValue },
			{ u8"自走步長", util::kAutoWalkDistanceValue },
			{ u8"自走方向", util::kAutoWalkDirectionValue },

			{ u8"腳本速度", util::kScriptSpeedValue },

			{ u8"自動登陸", util::kAutoLoginEnable },
			{ u8"斷線重連", util::kAutoReconnectEnable },
			{ u8"隱藏人物", util::kHideCharacterEnable },
			{ u8"關閉特效", util::kCloseEffectEnable },
			{ u8"資源優化", util::kOptimizeEnable },
			{ u8"隱藏石器", util::kHideWindowEnable },
			{ u8"屏蔽聲音", util::kMuteEnable },

			{ u8"自動調整內存", util::kAutoFreeMemoryEnable },
			{ u8"快速走路", util::kFastWalkEnable },
			{ u8"橫沖直撞", util::kPassWallEnable },
			{ u8"鎖定原地", util::kLockMoveEnable },
			{ u8"鎖定畫面", util::kLockImageEnable },
			{ u8"自動丟肉", util::kAutoDropMeatEnable },
			{ u8"自動疊加", util::kAutoStackEnable },
			{ u8"自動疊加", util::kAutoStackEnable },
			{ u8"自動KNPC", util::kKNPCEnable },
			{ u8"自動猜謎", util::kAutoAnswerEnable },
			{ u8"自動吃豆", util::kAutoEatBeanEnable },
			{ u8"走路遇敵", util::kAutoWalkEnable },
			{ u8"快速遇敵", util::kFastAutoWalkEnable },
			{ u8"快速戰鬥", util::kFastBattleEnable },
			{ u8"自動戰鬥", util::kAutoBattleEnable },
			{ u8"自動捉寵", util::kAutoCatchEnable },
			{ u8"自動逃跑", util::kAutoEscapeEnable },
			{ u8"戰鬥99秒", util::kBattleTimeExtendEnable },
			{ u8"落馬逃跑", util::kFallDownEscapeEnable },
			{ u8"顯示經驗", util::kShowExpEnable },
			{ u8"窗口吸附", util::kWindowDockEnable },
			{ u8"自動換寵", util::kBattleAutoSwitchEnable },
			{ u8"自動EO", util::kBattleAutoEOEnable },

			{ u8"隊伍開關", util::kSwitcherTeamEnable },
			{ u8"PK開關", util::kSwitcherPKEnable },
			{ u8"交名開關", util::kSwitcherCardEnable },
			{ u8"交易開關", util::kSwitcherTradeEnable },
			{ u8"組頻開關", util::kSwitcherGroupEnable },
			{ u8"家頻開關", util::kSwitcherFamilyEnable },
			{ u8"職頻開關", util::kSwitcherJobEnable },
			{ u8"世界開關", util::kSwitcherWorldEnable },

			{ u8"鎖定戰寵", util::kLockPetEnable },//{ u8"戰後自動鎖定戰寵編號", util::kLockPetValue },
			{ u8"鎖定騎寵", util::kLockRideEnable },//{ u8"戰後自動鎖定騎寵編號", util::kLockRideValue },
			{ u8"鎖寵排程", util::kLockPetScheduleEnable },
			{ u8"鎖定時間", util::kLockTimeEnable },//{ u8"時間", util::kLockTimeValue },

			{ u8"捉寵模式", util::kBattleCatchModeValue },
			{ u8"捉寵等級", util::kBattleCatchTargetLevelEnable },//{ u8"捉寵目標等級", util::kBattleCatchTargetLevelValue },
			{ u8"捉寵血量", util::kBattleCatchTargetMaxHpEnable },//{ u8"捉寵目標最大耐久力", util::kBattleCatchTargetMaxHpValue },
			{ u8"捉寵目標", util::kBattleCatchPetNameString },
			{ u8"捉寵寵技能", util::kBattleCatchPetSkillEnable },////{ u8"捉寵戰寵技能索引", util::kBattleCatchPetSkillValue },
			{ u8"捉寵道具", util::kBattleCatchCharItemEnable },//{ u8"捉寵使用道具直到血量低於", util::kBattleCatchTargetItemHpValue },{ u8"捉寵道具", util::kBattleCatchCharItemString },
			{ u8"捉寵精靈", util::kBattleCatchCharMagicEnable },//{ u8"捉寵使用精靈直到血量低於", util::kBattleCatchTargetMagicHpValue },{ u8"捉寵人物精靈索引", util::kBattleCatchCharMagicValue },

			{ u8"自動組隊", util::kAutoJoinEnable },//{ u8"自動組隊名稱", util::kAutoFunNameString },	{ u8"自動移動功能編號", util::kAutoFunTypeValue },
			{ u8"自動丟棄", util::kAutoDropEnable },//{ u8"自動丟棄名單", util::kAutoDropItemString },
			{ u8"鎖定攻擊", util::kLockAttackEnable },//{ u8"鎖定攻擊名單", util::kLockAttackString },
			{ u8"鎖定逃跑", util::kLockEscapeEnable },//{ u8"鎖定逃跑名單", util::kLockEscapeString },
			{ u8"非鎖不逃", util::kBattleNoEscapeWhileLockPetEnable },

			{ u8"道具補氣", util::kNormalItemHealMpEnable },//{ u8"ItemHealMpNormalValue", util::kNormalItemHealMpValue },{ u8"平時道具補氣", util::kNormalItemHealMpItemString },
			{ u8"戰鬥道具補氣", util::kBattleItemHealMpEnable },//{ u8"戰鬥道具補氣人物", util::kBattleItemHealMpValue },{ u8"戰鬥道具補氣 ", util::kBattleItemHealMpItemString },
			{ u8"戰鬥嗜血補氣", util::kBattleSkillMpEnable },//{ u8"戰鬥嗜血補氣技能", util::kBattleSkillMpSkillValue },{ u8"戰鬥嗜血補氣百分比", util::kBattleSkillMpValue },

			/*{u8"MagicHealCharNormalValue", util::kNormalMagicHealCharValue},
			{ u8"MagicHealPetNormalValue", util::kNormalMagicHealPetValue },
			{ u8"MagicHealAllieNormalValue", util::kNormalMagicHealAllieValue },*/
			{ u8"精靈補血", util::kNormalMagicHealEnable },//{ u8"平時精靈補血精靈索引", util::kNormalMagicHealMagicValue },
			/*{ u8"戰鬥精靈補血人物", util::kBattleMagicHealCharValue },
			{u8"戰鬥精靈補血戰寵", util::kBattleMagicHealPetValue},
			{ u8"戰鬥精靈補血隊友", util::kBattleMagicHealAllieValue },*/
			{ u8"戰鬥精靈補血", util::kBattleMagicHealEnable },//util::kBattleMagicHealMagicValue, util::kBattleMagicHealTargetValue,

			{ u8"戰鬥指定回合", util::kBattleCharRoundActionRoundValue },
			{ u8"戰鬥間隔回合", util::kBattleCrossActionCharEnable },
			{ u8"戰鬥一般", util::kBattleCharNormalActionTypeValue },

			{ u8"戰鬥寵指定回合RoundValue", util::kBattlePetRoundActionRoundValue },
			{ u8"戰鬥寵間隔回合", util::kBattleCrossActionPetEnable },
			{ u8"戰鬥寵一般", util::kBattlePetNormalActionTypeValue },

			{ u8"攻擊延時", util::kBattleActionDelayValue },
#pragma endregion

#pragma region zh_CN

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
			{ u8"EO命令", util::kEOCommandString },
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
			{ u8"窗口吸附", util::kWindowDockEnable },
			{ u8"自动换宠", util::kBattleAutoSwitchEnable },
			{ u8"自动EO", util::kBattleAutoEOEnable },

			{ u8"队伍开关", util::kSwitcherTeamEnable },
			{ u8"PK开关", util::kSwitcherPKEnable },
			{ u8"交名开关", util::kSwitcherCardEnable },
			{ u8"交易开关", util::kSwitcherTradeEnable },
			{ u8"组频开关", util::kSwitcherGroupEnable },
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
			{ u8"捉宠道具", util::kBattleCatchCharItemEnable },//{ u8"捉宠使用道具直到血量低于", util::kBattleCatchTargetItemHpValue },{ u8"捉宠道具", util::kBattleCatchCharItemString },
			{ u8"捉宠精灵", util::kBattleCatchCharMagicEnable },//{ u8"捉宠使用精灵直到血量低于", util::kBattleCatchTargetMagicHpValue },{ u8"捉宠人物精灵索引", util::kBattleCatchCharMagicValue },

			{ u8"自动组队", util::kAutoJoinEnable },//{ u8"自动组队名称", util::kAutoFunNameString },	{ u8"自动移动功能编号", util::kAutoFunTypeValue },
			{ u8"自动丢弃", util::kAutoDropEnable },//{ u8"自动丢弃名单", util::kAutoDropItemString },
			{ u8"锁定攻击", util::kLockAttackEnable },//{ u8"锁定攻击名单", util::kLockAttackString },
			{ u8"锁定逃跑", util::kLockEscapeEnable },//{ u8"锁定逃跑名单", util::kLockEscapeString },
			{ u8"非锁不逃", util::kBattleNoEscapeWhileLockPetEnable },

			{ u8"道具补气", util::kNormalItemHealMpEnable },//{ u8"ItemHealMpNormalValue", util::kNormalItemHealMpValue },{ u8"平时道具补气", util::kNormalItemHealMpItemString },
			{ u8"战斗道具补气", util::kBattleItemHealMpEnable },//{ u8"战斗道具补气人物", util::kBattleItemHealMpValue },{ u8"战斗道具补气 ", util::kBattleItemHealMpItemString },
			{ u8"战斗嗜血补气", util::kBattleSkillMpEnable },//{ u8"战斗嗜血补气技能", util::kBattleSkillMpSkillValue },{ u8"战斗嗜血补气百分比", util::kBattleSkillMpValue },

			/*{u8"MagicHealCharNormalValue", util::kNormalMagicHealCharValue},
			{ u8"MagicHealPetNormalValue", util::kNormalMagicHealPetValue },
			{ u8"MagicHealAllieNormalValue", util::kNormalMagicHealAllieValue },*/
			{ u8"精灵补血", util::kNormalMagicHealEnable },//{ u8"平时精灵补血精灵索引", util::kNormalMagicHealMagicValue },
			/*{ u8"战斗精灵补血人物", util::kBattleMagicHealCharValue },
			{u8"战斗精灵补血战宠", util::kBattleMagicHealPetValue},
			{ u8"战斗精灵补血队友", util::kBattleMagicHealAllieValue },*/
			{ u8"战斗精灵补血", util::kBattleMagicHealEnable },//util::kBattleMagicHealMagicValue, util::kBattleMagicHealTargetValue,

			{ u8"战斗指定回合", util::kBattleCharRoundActionRoundValue },
			{ u8"战斗间隔回合", util::kBattleCrossActionCharEnable },
			{ u8"战斗一般", util::kBattleCharNormalActionTypeValue },

			{ u8"战斗宠指定回合RoundValue", util::kBattlePetRoundActionRoundValue },
			{ u8"战斗宠间隔回合", util::kBattleCrossActionPetEnable },
			{ u8"战斗宠一般", util::kBattlePetNormalActionTypeValue },

			{ u8"攻击延时", util::kBattleActionDelayValue },


			{ u8"战斗回合目标", util::kBattleCharRoundActionTargetValue },
			{ u8"战斗间隔目标",  util::kBattleCharCrossActionTargetValue },
			{ u8"战斗一般目标", util::kBattleCharNormalActionTargetValue },
			{ u8"战斗宠回合目标", util::kBattlePetRoundActionTargetValue },
			{ u8"战斗宠间隔目标", util::kBattlePetCrossActionTargetValue },
			{ u8"战斗宠一般目标",  util::kBattlePetNormalActionTargetValue },
			{ u8"战斗精灵补血目标", util::kBattleMagicHealTargetValue },
			{ u8"战斗道具补血目标", util::kBattleItemHealTargetValue },
			{ u8"战斗精灵復活目标", util::kBattleMagicReviveTargetValue },
			{ u8"战斗道具復活目标", util::kBattleItemReviveTargetValue },
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
				injector.server->doBattleWork(true);//async
		}
		else if (type == util::kAutoBattleEnable && ok)
		{
			injector.setEnableHash(util::kFastBattleEnable, !ok);
			if (ok && !injector.server.isNull())
				injector.server->doBattleWork(true);//async
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