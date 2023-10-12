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
import Utility;
import String;
#include "stdafx.h"
#include "clua.h"
#include "injector.h"
#include "signaldispatcher.h"

__int64 CLuaSystem::sleep(__int64 t, sol::this_state s)
{
	if (t < 0)
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("sleep time must above 0"));
		return FALSE;
	}

	if (t >= 1000)
	{
		__int64 i = 0;
		__int64 size = t / 1000;
		for (; i < size; ++i)
		{
			QThread::msleep(1000UL);
			luadebug::checkStopAndPause(s);
		}

		if (i % 1000 > 0)
			QThread::msleep(static_cast<DWORD>(i) % 1000UL);
	}
	else if (t > 0)
		QThread::msleep(static_cast<DWORD>(t));

	return TRUE;
}

__int64 CLuaSystem::logout(sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<__int64>());
	if (!injector.server.isNull())
	{
		injector.server->logOut();
		return TRUE;
	}

	return FALSE;
}

__int64 CLuaSystem::logback(sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<__int64>());
	if (!injector.server.isNull())
	{
		injector.server->logBack();
		return TRUE;
	}

	return FALSE;
}

__int64 CLuaSystem::eo(sol::this_state s)
{
	sol::state_view lua(s);
	__int64 currentIndex = lua["_INDEX"].get<__int64>();
	Injector& injector = Injector::getInstance(currentIndex);
	if (injector.server.isNull())
		return 0;

	luadebug::checkBattleThenWait(s);

	QElapsedTimer timer; timer.start();
	injector.server->EO();

	bool bret = luadebug::waitfor(s, 5000, [currentIndex]() { return !Injector::getInstance(currentIndex).server->isEOTTLSend.load(std::memory_order_acquire); });

	__int64 result = bret ? injector.server->lastEOTime.load(std::memory_order_acquire) : 0;

	return result;
}

__int64 CLuaSystem::openlog(std::string sfilename, sol::object oformat, sol::object obuffersize, sol::this_state s)
{
	sol::state_view lua(s);
	__int64 currentIndex = lua["_INDEX"].get<__int64>();
	Injector& injector = Injector::getInstance(currentIndex);

	QString filename = toQString(sfilename);
	if (filename.isEmpty())
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("log file name cannot be empty"));
		return false;
	}

	QString format = "[%(date) %(time)] | [@%(line)] | %(message)";
	if (oformat.is<std::string>() && !oformat.as<std::string>().empty())
		format = toQString(oformat);

	__int64 buffersize = 1024;
	if (obuffersize.is<__int64>())
		buffersize = obuffersize.as<__int64>();

	bool bret = injector.log->initialize(filename, buffersize, format);
	return bret;
}

//這裡還沒想好format格式怎麼設計，暫時先放著
__int64 CLuaSystem::print(sol::object ocontent, sol::object ocolor, sol::this_state s)
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
		raw = toQString(value);
		msg = raw;
	}
	else if (ocontent.is<qlonglong>())
	{
		const qlonglong value = ocontent.as<qlonglong>();
		raw = toQString(value);
		msg = raw;
	}
	else if (ocontent.is<qreal>())
	{
		const qreal value = ocontent.as<qreal>();
		raw = toQString(value);
		msg = raw;
	}
	else if (ocontent.is<std::string>())
	{
		raw = toQString(ocontent);
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
		__int64 depth = 1024;
		raw = luadebug::getTableVars(s.L, 1, depth);
		msg = raw;

	}
	else
	{
		QString pointerStr = toQString(reinterpret_cast<__int64>(ocontent.pointer()), 16);
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
	if (ocolor.is<__int64>())
	{
		color = ocolor.as<__int64>();
		if (color != -2 && (color < 0 || color > 10))
			luadebug::tryPopCustomErrorMsg(s, luadebug::ERROR_PARAM_TYPE, false, 2, QObject::tr("invalid value of 'color'"));
	}
	else if (ocolor == sol::lua_nil)
	{
		color = QRandomGenerator64().bounded(0, 10);
	}

	Injector& injector = Injector::getInstance(lua["_INDEX"].get<__int64>());
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

__int64 CLuaSystem::messagebox(sol::object ostr, sol::object otype, sol::this_state s)
{
	sol::state_view lua(s);
	__int64 currentIndex = lua["_INDEX"].get<__int64>();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);

	QString text;
	if (ostr.is<__int64>())
		text = toQString(ostr.as<__int64>());
	else if (ostr.is<double>())
		text = toQString(ostr.as<double>());
	else if (ostr.is<std::string>())
		text = toQString(ostr);
	else
		luadebug::tryPopCustomErrorMsg(s, luadebug::ERROR_PARAM_TYPE, false, 1, QObject::tr("invalid value type"));

	__int64 type = 0;
	if (ostr.is<__int64>())
		type = ostr.as<__int64>();
	else if (ostr != sol::lua_nil)
		luadebug::tryPopCustomErrorMsg(s, luadebug::ERROR_PARAM_TYPE, false, 2, QObject::tr("invalid value of 'type'"));

	__int64 nret = QMessageBox::StandardButton::NoButton;
	emit signalDispatcher.messageBoxShow(text, type, "", &nret);
	if (nret != QMessageBox::StandardButton::NoButton)
	{
		return nret;
	}
	return 0;
}

__int64 CLuaSystem::talk(sol::object ostr, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<__int64>());
	if (injector.server.isNull())
		return FALSE;

	QString text;
	if (ostr.is<__int64>())
		text = toQString(ostr.as<__int64>());
	else if (ostr.is<double>())
		text = toQString(ostr.as<double>());
	else if (ostr.is<std::string>())
		text = toQString(ostr);
	else if (ostr.is<sol::table>())
	{
		__int64 depth = 1024;
		text = luadebug::getTableVars(s.L, 1, depth);
	}
	else
		luadebug::tryPopCustomErrorMsg(s, luadebug::ERROR_PARAM_TYPE, false, 1, QObject::tr("invalid value type"));


	injector.server->talk(text);

	return TRUE;
}

__int64 CLuaSystem::cleanchat(sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<__int64>());
	if (injector.server.isNull())
		return FALSE;

	injector.server->cleanChatHistory();

	return TRUE;
}

__int64 CLuaSystem::savesetting(const std::string& sfileName, sol::this_state s)
{
	QString fileName = toQString(sfileName);
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

	sol::state_view lua(s);
	__int64 currentIndex = lua["_INDEX"].get<__int64>();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	emit signalDispatcher.saveHashSettings(fileName, true);

	return TRUE;
}

__int64 CLuaSystem::loadsetting(const std::string& sfileName, sol::this_state s)
{
	QString fileName = toQString(sfileName);
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
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("file '%1' does not exist").arg(fileName));
		return FALSE;
	}

	sol::state_view lua(s);
	__int64 currentIndex = lua["_INDEX"].get<__int64>();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	emit signalDispatcher.loadHashSettings(fileName, true);

	return TRUE;
}

__int64 CLuaSystem::press(std::string sbuttonStr, __int64 unitid, __int64 dialogid, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<__int64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	QString text = toQString(sbuttonStr);
	__int64 row = -1;
	BUTTON_TYPE button = buttonMap.value(text.toUpper(), BUTTON_NOTUSED);
	if (button == BUTTON_NOTUSED)
	{
		__int64 row = -1;
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

			for (__int64 i = 0; i < textList.size(); ++i)
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

__int64 CLuaSystem::press(__int64 row, __int64 unitid, __int64 dialogid, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<__int64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->press(row, dialogid, unitid);

	return TRUE;
}

__int64 CLuaSystem::input(const std::string& str, __int64 unitid, __int64 dialogid, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<__int64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	QString text = toQString(str);

	injector.server->inputtext(text, dialogid, unitid);

	return TRUE;
}

__int64 CLuaSystem::leftclick(__int64 x, __int64 y, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<__int64>());
	if (injector.server.isNull())
		return FALSE;

	injector.leftClick(x, y);
	return TRUE;
}

__int64 CLuaSystem::rightclick(__int64 x, __int64 y, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<__int64>());
	if (injector.server.isNull())
		return FALSE;

	injector.rightClick(x, y);
	return TRUE;
}

__int64 CLuaSystem::leftdoubleclick(__int64 x, __int64 y, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<__int64>());
	if (injector.server.isNull())
		return FALSE;

	injector.leftDoubleClick(x, y);
	return TRUE;
}

__int64 CLuaSystem::mousedragto(__int64 x1, __int64 y1, __int64 x2, __int64 y2, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<__int64>());
	if (injector.server.isNull())
		return FALSE;

	injector.dragto(x1, y1, x2, y2);
	return TRUE;
}

__int64 CLuaSystem::menu(__int64 index, sol::object otype, sol::this_state s)
{
	sol::state_view lua(s);
	__int64 currentIndex = lua["_INDEX"].get<__int64>();
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	--index;
	if (index < 0)
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("index must above 0"));
		return FALSE;
	}

	__int64 type = 1;
	if (otype.is<__int64>())
		type = otype.as<__int64>();

	--type;
	if (type < 0 || type > 1)
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("type must be 1 or 2"));
		return FALSE;
	}

	if (type == 0)
	{
		injector.server->saMenu(index);
	}
	else
	{
		injector.server->shopOk(index);
	}

	return TRUE;
}

__int64 CLuaSystem::createch(sol::object odataplacenum
	, std::string scharname
	, __int64 imgno
	, __int64 faceimgno
	, __int64 vit
	, __int64 str
	, __int64 tgh
	, __int64 dex
	, __int64 earth
	, __int64 water
	, __int64 fire
	, __int64 wind
	, sol::object ohometown, sol::object oforcecover, sol::this_state s)
{
	sol::state_view lua(s);
	__int64 currentIndex = lua["_INDEX"].get<__int64>();
	Injector& injector = Injector::getInstance(currentIndex);
	if (injector.server.isNull())
		return FALSE;

	__int64 dataplacenum = 0;
	if (odataplacenum.is<std::string>())
	{
		QString dataplace = toQString(odataplacenum.as<std::string>());
		if (dataplace == "左")
			dataplacenum = 0;
		else if (dataplace == "右")
			dataplacenum = 1;
		else
		{
			luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("Invalid value of 'dataplacenum'"));
			return FALSE;
		}
	}
	else if (odataplacenum.is<__int64>())
	{
		dataplacenum = odataplacenum.as<__int64>();
		if (dataplacenum != 1 && dataplacenum != 2)
		{
			luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("Invalid value of 'dataplacenum'"));
			return FALSE;
		}
		--dataplacenum;
	}

	QString charname = toQString(scharname);
	if (charname.isEmpty())
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("character name cannot be empty"));
		return FALSE;
	}

	if (imgno <= 0)
		imgno = 100050;

	if (faceimgno <= 0)
		faceimgno = 30250;

	if (vit < 0)
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("vit must above 0"));
		return FALSE;
	}

	if (str < 0)
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("str must above 0"));
		return FALSE;
	}

	if (tgh < 0)
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("tgh must above 0"));
		return FALSE;
	}

	if (dex < 0)
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("dex must above 0"));
		return FALSE;
	}

	if (vit + str + tgh + dex != 20)
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("vit + str + tgh + dex must equal to 20"));
		return FALSE;
	}

	if (earth < 0)
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("earth must above 0"));
		return FALSE;
	}

	if (water < 0)
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("water must above 0"));
		return FALSE;
	}

	if (fire < 0)
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("fire must above 0"));
		return FALSE;
	}

	if (wind < 0)
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("wind must above 0"));
		return FALSE;
	}

	if (earth + water + fire + wind != 10)
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("earth + water + fire + wind must equal to 10"));
		return FALSE;
	}

	__int64 hometown = 1;
	if (ohometown.is<std::string>())
	{
		static const QHash<QString, __int64> hash = {
			{ "薩姆吉爾",0 }, { "瑪麗娜絲", 1 }, { "加加", 2 }, { "卡魯它那", 3 },
			{ "萨姆吉尔",0 }, { "玛丽娜丝", 1 }, { "加加", 2 }, { "卡鲁它那", 3 },

			{ "薩姆吉爾村",0 }, { "瑪麗娜絲村", 1 }, { "加加村", 2 }, { "卡魯它那村", 3 },
			{ "萨姆吉尔村",0 }, { "玛丽娜丝村", 1 }, { "加加村", 2 }, { "卡鲁它那村", 3 },
		};

		QString hometownstr = toQString(ohometown.as<std::string>());
		if (hometownstr.isEmpty())
		{
			luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("hometown cannot be empty"));
			return FALSE;
		}

		hometown = hash.value(hometownstr, -1);
		if (hometown == -1)
			hometown = 1;
	}
	else if (ohometown.is<__int64>())
	{
		hometown = ohometown.as<__int64>();
		if (hometown <= 0 || hometown > 4)
			hometown = 1;
		else
			--hometown;
	}

	bool forcecover = false;
	if (oforcecover.is<bool>())
		forcecover = oforcecover.as<bool>();
	else if (oforcecover.is<__int64>())
		forcecover = oforcecover.as<__int64>() > 0;

	injector.server->createCharacter(static_cast<__int64>(dataplacenum)
		, charname
		, static_cast<__int64>(imgno)
		, static_cast<__int64>(faceimgno)
		, static_cast<__int64>(vit)
		, static_cast<__int64>(str)
		, static_cast<__int64>(tgh)
		, static_cast<__int64>(dex)
		, static_cast<__int64>(earth)
		, static_cast<__int64>(water)
		, static_cast<__int64>(fire)
		, static_cast<__int64>(wind)
		, static_cast<__int64>(hometown)
		, forcecover);

	return TRUE;
}

__int64 CLuaSystem::delch(__int64 index, std::string spsw, sol::object option, sol::this_state s)
{
	sol::state_view lua(s);
	__int64 currentIndex = lua["_INDEX"].get<__int64>();
	Injector& injector = Injector::getInstance(currentIndex);
	if (injector.server.isNull())
		return FALSE;

	--index;
	if (index < 0 || index > MAX_CHARACTER)
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("index must between 1 and %1").arg(MAX_CHARACTER));
		return FALSE;
	}

	QString password = toQString(spsw);
	if (password.isEmpty())
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("password cannot be empty"));
		return FALSE;
	}

	bool backtofirst = false;
	if (option.is<bool>())
		backtofirst = option.as<bool>();
	else if (option.is<__int64>())
		backtofirst = option.as<__int64>() > 0;

	injector.server->deleteCharacter(index, password, backtofirst);

	return TRUE;
}


__int64 CLuaSystem::set(std::string enumStr,
	sol::object p1, sol::object p2, sol::object p3, sol::object p4, sol::object p5, sol::object p6, sol::object p7, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<__int64>());
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(lua["_INDEX"].get<__int64>());

	QString typeStr = toQString(enumStr);
	if (typeStr.isEmpty())
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("type cannot be empty"));
		return FALSE;
	}

	const QHash<QString, UserSetting> hash = {
			{ "debug", kScriptDebugModeEnable },
#pragma region zh_TW
			/*{"戰鬥道具補血戰寵", kBattleItemHealPetValue},
			{ "戰鬥道具補血隊友", kBattleItemHealAllieValue },
			{ "戰鬥道具補血人物", kBattleItemHealCharValue },*/
			{ "戰鬥道具補血", kBattleItemHealEnable },//{ "戰鬥道具補血", kBattleItemHealItemString },//{ "戰鬥道具肉優先", kBattleItemHealMeatPriorityEnable },{ "BattleItemHealTargetValue", kBattleItemHealTargetValue },

			{ "戰鬥精靈復活", kBattleMagicReviveEnable },//{ "BattleMagicReviveMagicValue", kBattleMagicReviveMagicValue },{ "BattleMagicReviveTargetValue", kBattleMagicReviveTargetValue },
			{ "戰鬥道具復活", kBattleItemReviveEnable },//{ "戰鬥道具復活", kBattleItemReviveItemString },{ "BattleItemReviveTargetValue", kBattleItemReviveTargetValue },

			/*{"ItemHealCharNormalValue", kNormalItemHealCharValue},
			{ "ItemHealPetNormalValue", kNormalItemHealPetValue },
			{ "ItemHealAllieNormalValue", kNormalItemHealAllieValue },*/
			{ "道具補血", kNormalItemHealEnable },//{ "平時道具補血", kNormalItemHealItemString },{ "道具肉優先", kNormalItemHealMeatPriorityEnable },

			{ "自動丟棄寵物", kDropPetEnable },//{ "自動丟棄寵物名單", kDropPetNameString },
			{ "自動丟棄寵物攻", kDropPetStrEnable },//{ "自動丟棄寵物攻數值", kDropPetStrValue },
			{ "自動丟棄寵物防", kDropPetDefEnable },//{ "自動丟棄寵物防數值", kDropPetDefValue },
			{ "自動丟棄寵物敏", kDropPetAgiEnable },//{ "自動丟棄寵物敏數值", kDropPetAgiValue },
			{ "自動丟棄寵物血", kDropPetHpEnable },//{ "自動丟棄寵物血數值", kDropPetHpValue },
			{ "自動丟棄寵物攻防敏", kDropPetAggregateEnable },//{ "自動丟棄寵物攻防敏數值", kDropPetAggregateValue },

			{ "自動加點", kAutoAbilityEnable },

			//ok
			{ "主機", kServerValue },
			{ "副機", kSubServerValue },
			{ "位置", kPositionValue },

			{ "加速", kSpeedBoostValue },
			{ "帳號", kGameAccountString },
			{ "賬號", kGameAccountString },
			{ "密碼", kGamePasswordString },
			{ "安全碼", kGameSecurityCodeString },
			{ "遠程白名單", kMailWhiteListString },

			{ "自走延時", kAutoWalkDelayValue },
			{ "自走步長", kAutoWalkDistanceValue },
			{ "自走方向", kAutoWalkDirectionValue },

			{ "腳本速度", kScriptSpeedValue },

			{ "自動登陸", kAutoLoginEnable },
			{ "斷線重連", kAutoReconnectEnable },
			{ "隱藏人物", kHideCharacterEnable },
			{ "關閉特效", kCloseEffectEnable },
			{ "資源優化", kOptimizeEnable },
			{ "隱藏石器", kHideWindowEnable },
			{ "屏蔽聲音", kMuteEnable },

			{ "自動調整內存", kAutoFreeMemoryEnable },
			{ "快速走路", kFastWalkEnable },
			{ "橫沖直撞", kPassWallEnable },
			{ "鎖定原地", kLockMoveEnable },
			{ "鎖定畫面", kLockImageEnable },
			{ "自動丟肉", kAutoDropMeatEnable },
			{ "自動疊加", kAutoStackEnable },
			{ "自動疊加", kAutoStackEnable },
			{ "自動KNPC", kKNPCEnable },
			{ "自動猜謎", kAutoAnswerEnable },
			{ "自動吃豆", kAutoEatBeanEnable },
			{ "走路遇敵", kAutoWalkEnable },
			{ "快速遇敵", kFastAutoWalkEnable },
			{ "快速戰鬥", kFastBattleEnable },
			{ "自動戰鬥", kAutoBattleEnable },
			{ "自動捉寵", kAutoCatchEnable },
			{ "自動逃跑", kAutoEscapeEnable },
			{ "戰鬥99秒", kBattleTimeExtendEnable },
			{ "落馬逃跑", kFallDownEscapeEnable },
			{ "顯示經驗", kShowExpEnable },
			{ "窗口吸附", kWindowDockEnable },
			{ "自動換寵", kBattleAutoSwitchEnable },
			{ "自動EO", kBattleAutoEOEnable },

			{ "隊伍開關", kSwitcherTeamEnable },
			{ "PK開關", kSwitcherPKEnable },
			{ "交名開關", kSwitcherCardEnable },
			{ "交易開關", kSwitcherTradeEnable },
			{ "組頻開關", kSwitcherGroupEnable },
			{ "家頻開關", kSwitcherFamilyEnable },
			{ "職頻開關", kSwitcherJobEnable },
			{ "世界開關", kSwitcherWorldEnable },

			{ "鎖定戰寵", kLockPetEnable },//{ "戰後自動鎖定戰寵編號", kLockPetValue },
			{ "鎖定騎寵", kLockRideEnable },//{ "戰後自動鎖定騎寵編號", kLockRideValue },
			{ "鎖寵排程", kLockPetScheduleEnable },
			{ "鎖定時間", kLockTimeEnable },//{ "時間", kLockTimeValue },

			{ "捉寵模式", kBattleCatchModeValue },
			{ "捉寵等級", kBattleCatchTargetLevelEnable },//{ "捉寵目標等級", kBattleCatchTargetLevelValue },
			{ "捉寵血量", kBattleCatchTargetMaxHpEnable },//{ "捉寵目標最大耐久力", kBattleCatchTargetMaxHpValue },
			{ "捉寵目標", kBattleCatchPetNameString },
			{ "捉寵寵技能", kBattleCatchPetSkillEnable },////{ "捉寵戰寵技能索引", kBattleCatchPetSkillValue },
			{ "捉寵道具", kBattleCatchCharItemEnable },//{ "捉寵使用道具直到血量低於", kBattleCatchTargetItemHpValue },{ "捉寵道具", kBattleCatchCharItemString },
			{ "捉寵精靈", kBattleCatchCharMagicEnable },//{ "捉寵使用精靈直到血量低於", kBattleCatchTargetMagicHpValue },{ "捉寵人物精靈索引", kBattleCatchCharMagicValue },

			{ "自動組隊", kAutoJoinEnable },//{ "自動組隊名稱", kAutoFunNameString },	{ "自動移動功能編號", kAutoFunTypeValue },
			{ "自動丟棄", kAutoDropEnable },//{ "自動丟棄名單", kAutoDropItemString },
			{ "鎖定攻擊", kLockAttackEnable },//{ "鎖定攻擊名單", kLockAttackString },
			{ "鎖定逃跑", kLockEscapeEnable },//{ "鎖定逃跑名單", kLockEscapeString },
			{ "非鎖不逃", kBattleNoEscapeWhileLockPetEnable },

			{ "道具補氣", kNormalItemHealMpEnable },//{ "ItemHealMpNormalValue", kNormalItemHealMpValue },{ "平時道具補氣", kNormalItemHealMpItemString },
			{ "戰鬥道具補氣", kBattleItemHealMpEnable },//{ "戰鬥道具補氣人物", kBattleItemHealMpValue },{ "戰鬥道具補氣 ", kBattleItemHealMpItemString },
			{ "戰鬥嗜血補氣", kBattleSkillMpEnable },//{ "戰鬥嗜血補氣技能", kBattleSkillMpSkillValue },{ "戰鬥嗜血補氣百分比", kBattleSkillMpValue },

			/*{"MagicHealCharNormalValue", kNormalMagicHealCharValue},
			{ "MagicHealPetNormalValue", kNormalMagicHealPetValue },
			{ "MagicHealAllieNormalValue", kNormalMagicHealAllieValue },*/
			{ "精靈補血", kNormalMagicHealEnable },//{ "平時精靈補血精靈索引", kNormalMagicHealMagicValue },
			/*{ "戰鬥精靈補血人物", kBattleMagicHealCharValue },
			{"戰鬥精靈補血戰寵", kBattleMagicHealPetValue},
			{ "戰鬥精靈補血隊友", kBattleMagicHealAllieValue },*/
			{ "戰鬥精靈補血", kBattleMagicHealEnable },//kBattleMagicHealMagicValue, kBattleMagicHealTargetValue,

			{ "戰鬥指定回合", kBattleCharRoundActionRoundValue },
			{ "戰鬥間隔回合", kBattleCrossActionCharEnable },
			{ "戰鬥一般", kBattleCharNormalActionTypeValue },

			{ "戰鬥寵指定回合RoundValue", kBattlePetRoundActionRoundValue },
			{ "戰鬥寵間隔回合", kBattleCrossActionPetEnable },
			{ "戰鬥寵一般", kBattlePetNormalActionTypeValue },

			{ "攻擊延時", kBattleActionDelayValue },
			{ "動作重發", kBattleResendDelayValue },
#pragma endregion

#pragma region zh_CN

			/*{"战斗道具补血战宠", kBattleItemHealPetValue},
			{ "战斗道具补血队友", kBattleItemHealAllieValue },
			{ "战斗道具补血人物", kBattleItemHealCharValue },*/
			{ "战斗道具补血", kBattleItemHealEnable },//{ "战斗道具补血", kBattleItemHealItemString },//{ "战斗道具肉优先", kBattleItemHealMeatPriorityEnable },{ "BattleItemHealTargetValue", kBattleItemHealTargetValue },

			{ "战斗精灵復活", kBattleMagicReviveEnable },//{ "BattleMagicReviveMagicValue", kBattleMagicReviveMagicValue },{ "BattleMagicReviveTargetValue", kBattleMagicReviveTargetValue },
			{ "战斗道具復活", kBattleItemReviveEnable },//{ "战斗道具復活", kBattleItemReviveItemString },{ "BattleItemReviveTargetValue", kBattleItemReviveTargetValue },

			/*{"ItemHealCharNormalValue", kNormalItemHealCharValue},
			{ "ItemHealPetNormalValue", kNormalItemHealPetValue },
			{ "ItemHealAllieNormalValue", kNormalItemHealAllieValue },*/
			{ "道具补血", kNormalItemHealEnable },//{ "平时道具补血", kNormalItemHealItemString },{ "道具肉优先", kNormalItemHealMeatPriorityEnable },

			{ "自动丢弃宠物", kDropPetEnable },//{ "自动丢弃宠物名单", kDropPetNameString },
			{ "自动丢弃宠物攻", kDropPetStrEnable },//{ "自动丢弃宠物攻数值", kDropPetStrValue },
			{ "自动丢弃宠物防", kDropPetDefEnable },//{ "自动丢弃宠物防数值", kDropPetDefValue },
			{ "自动丢弃宠物敏", kDropPetAgiEnable },//{ "自动丢弃宠物敏数值", kDropPetAgiValue },
			{ "自动丢弃宠物血", kDropPetHpEnable },//{ "自动丢弃宠物血数值", kDropPetHpValue },
			{ "自动丢弃宠物攻防敏", kDropPetAggregateEnable },//{ "自动丢弃宠物攻防敏数值", kDropPetAggregateValue },

			{ "自动加点", kAutoAbilityEnable },


			//ok
			{ "EO命令", kEOCommandString },
			{ "主机", kServerValue },
			{ "副机", kSubServerValue },
			{ "位置", kPositionValue },

			{ "加速", kSpeedBoostValue },
			{ "帐号", kGameAccountString },
			{ "账号", kGameAccountString },
			{ "密码", kGamePasswordString },
			{ "安全码", kGameSecurityCodeString },
			{ "远程白名单", kMailWhiteListString },

			{ "自走延时", kAutoWalkDelayValue },
			{ "自走步长", kAutoWalkDistanceValue },
			{ "自走方向", kAutoWalkDirectionValue },

			{ "脚本速度", kScriptSpeedValue },

			{ "自动登陆", kAutoLoginEnable },
			{ "断线重连", kAutoReconnectEnable },
			{ "隐藏人物", kHideCharacterEnable },
			{ "关闭特效", kCloseEffectEnable },
			{ "资源优化", kOptimizeEnable },
			{ "隐藏石器", kHideWindowEnable },
			{ "屏蔽声音", kMuteEnable },

			{ "自动调整内存", kAutoFreeMemoryEnable },
			{ "快速走路", kFastWalkEnable },
			{ "横冲直撞", kPassWallEnable },
			{ "锁定原地", kLockMoveEnable },
			{ "锁定画面", kLockImageEnable },
			{ "自动丢肉", kAutoDropMeatEnable },
			{ "自动叠加", kAutoStackEnable },
			{ "自动迭加", kAutoStackEnable },
			{ "自动KNPC", kKNPCEnable },
			{ "自动猜谜", kAutoAnswerEnable },
			{ "自动吃豆", kAutoEatBeanEnable },
			{ "走路遇敌", kAutoWalkEnable },
			{ "快速遇敌", kFastAutoWalkEnable },
			{ "快速战斗", kFastBattleEnable },
			{ "自动战斗", kAutoBattleEnable },
			{ "自动捉宠", kAutoCatchEnable },
			{ "自动逃跑", kAutoEscapeEnable },
			{ "战斗99秒", kBattleTimeExtendEnable },
			{ "落马逃跑", kFallDownEscapeEnable },
			{ "显示经验", kShowExpEnable },
			{ "窗口吸附", kWindowDockEnable },
			{ "自动换宠", kBattleAutoSwitchEnable },
			{ "自动EO", kBattleAutoEOEnable },

			{ "队伍开关", kSwitcherTeamEnable },
			{ "PK开关", kSwitcherPKEnable },
			{ "交名开关", kSwitcherCardEnable },
			{ "交易开关", kSwitcherTradeEnable },
			{ "组频开关", kSwitcherGroupEnable },
			{ "家频开关", kSwitcherFamilyEnable },
			{ "职频开关", kSwitcherJobEnable },
			{ "世界开关", kSwitcherWorldEnable },

			{ "锁定战宠", kLockPetEnable },//{ "战后自动锁定战宠编号", kLockPetValue },
			{ "锁定骑宠", kLockRideEnable },//{ "战后自动锁定骑宠编号", kLockRideValue },
			{ "锁宠排程", kLockPetScheduleEnable },
			{ "锁定时间", kLockTimeEnable },//{ "时间", kLockTimeValue },

			{ "捉宠模式", kBattleCatchModeValue },
			{ "捉宠等级", kBattleCatchTargetLevelEnable },//{ "捉宠目标等级", kBattleCatchTargetLevelValue },
			{ "捉宠血量", kBattleCatchTargetMaxHpEnable },//{ "捉宠目标最大耐久力", kBattleCatchTargetMaxHpValue },
			{ "捉宠目标", kBattleCatchPetNameString },
			{ "捉宠宠技能", kBattleCatchPetSkillEnable },////{ "捉宠战宠技能索引", kBattleCatchPetSkillValue },
			{ "捉宠道具", kBattleCatchCharItemEnable },//{ "捉宠使用道具直到血量低于", kBattleCatchTargetItemHpValue },{ "捉宠道具", kBattleCatchCharItemString },
			{ "捉宠精灵", kBattleCatchCharMagicEnable },//{ "捉宠使用精灵直到血量低于", kBattleCatchTargetMagicHpValue },{ "捉宠人物精灵索引", kBattleCatchCharMagicValue },

			{ "自动组队", kAutoJoinEnable },//{ "自动组队名称", kAutoFunNameString },	{ "自动移动功能编号", kAutoFunTypeValue },
			{ "自动丢弃", kAutoDropEnable },//{ "自动丢弃名单", kAutoDropItemString },
			{ "锁定攻击", kLockAttackEnable },//{ "锁定攻击名单", kLockAttackString },
			{ "锁定逃跑", kLockEscapeEnable },//{ "锁定逃跑名单", kLockEscapeString },
			{ "非锁不逃", kBattleNoEscapeWhileLockPetEnable },

			{ "道具补气", kNormalItemHealMpEnable },//{ "ItemHealMpNormalValue", kNormalItemHealMpValue },{ "平时道具补气", kNormalItemHealMpItemString },
			{ "战斗道具补气", kBattleItemHealMpEnable },//{ "战斗道具补气人物", kBattleItemHealMpValue },{ "战斗道具补气 ", kBattleItemHealMpItemString },
			{ "战斗嗜血补气", kBattleSkillMpEnable },//{ "战斗嗜血补气技能", kBattleSkillMpSkillValue },{ "战斗嗜血补气百分比", kBattleSkillMpValue },

			/*{"MagicHealCharNormalValue", kNormalMagicHealCharValue},
			{ "MagicHealPetNormalValue", kNormalMagicHealPetValue },
			{ "MagicHealAllieNormalValue", kNormalMagicHealAllieValue },*/
			{ "精灵补血", kNormalMagicHealEnable },//{ "平时精灵补血精灵索引", kNormalMagicHealMagicValue },
			/*{ "战斗精灵补血人物", kBattleMagicHealCharValue },
			{"战斗精灵补血战宠", kBattleMagicHealPetValue},
			{ "战斗精灵补血队友", kBattleMagicHealAllieValue },*/
			{ "战斗精灵补血", kBattleMagicHealEnable },//kBattleMagicHealMagicValue, kBattleMagicHealTargetValue,

			{ "战斗指定回合", kBattleCharRoundActionRoundValue },
			{ "战斗间隔回合", kBattleCrossActionCharEnable },
			{ "战斗一般", kBattleCharNormalActionTypeValue },

			{ "战斗宠指定回合RoundValue", kBattlePetRoundActionRoundValue },
			{ "战斗宠间隔回合", kBattleCrossActionPetEnable },
			{ "战斗宠一般", kBattlePetNormalActionTypeValue },

			{ "攻击延时", kBattleActionDelayValue },
			{ "动作重发", kBattleResendDelayValue },


			{ "战斗回合目标", kBattleCharRoundActionTargetValue },
			{ "战斗间隔目标",  kBattleCharCrossActionTargetValue },
			{ "战斗一般目标", kBattleCharNormalActionTargetValue },
			{ "战斗宠回合目标", kBattlePetRoundActionTargetValue },
			{ "战斗宠间隔目标", kBattlePetCrossActionTargetValue },
			{ "战斗宠一般目标",  kBattlePetNormalActionTargetValue },
			{ "战斗精灵补血目标", kBattleMagicHealTargetValue },
			{ "战斗道具补血目标", kBattleItemHealTargetValue },
			{ "战斗精灵復活目标", kBattleMagicReviveTargetValue },
			{ "战斗道具復活目标", kBattleItemReviveTargetValue },
	#pragma endregion
	};


	UserSetting type = hash.value(typeStr, kSettingNotUsed);
	if (type == kSettingNotUsed)
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("unknown setting type: '%1'").arg(typeStr));
		return FALSE;
	}

	if (type == kScriptDebugModeEnable)
	{
		__int64 value1 = 0;
		if (p1.is<__int64>())
			value1 = p1.as<__int64>();
		else if (p1.is<bool>())
			value1 = p1.as<bool>() ? 1 : 0;

		injector.setEnableHash(kScriptDebugModeEnable, value1 > 0);
		emit signalDispatcher.applyHashSettingsToUI();
		return TRUE;
	}

	if (type == kBattleCharNormalActionTypeValue)
	{
		__int64 value1 = 0;
		if (p1.is<__int64>())
			value1 = p1.as<__int64>();
		else if (p1.is<bool>())
			value1 = p1.as<bool>() ? 1 : 0;

		--value1;
		if (value1 < 0)
			value1 = 0;

		__int64 value2 = 0;
		if (p2.is<__int64>())
			value2 = p2.as<__int64>();
		else if (p2.is<bool>())
			value2 = p2.as<bool>() ? 1 : 0;
		--value2;
		if (value2 < 0)
			value2 = 0;

		__int64 value3 = 0;
		if (p3.is<__int64>())
			value3 = p3.as<__int64>();
		else if (p2.is<bool>())
			value3 = p3.as<bool>() ? 1 : 0;
		--value3;
		if (value3 < 0)
			value3 = 0;

		injector.setValueHash(kBattleCharNormalActionTypeValue, value1);
		injector.setValueHash(kBattleCharNormalActionEnemyValue, value2);
		injector.setValueHash(kBattleCharNormalActionLevelValue, value3);
		//injector.setValueHash(kBattleCharNormalActionTargetValue, value4);

		emit signalDispatcher.applyHashSettingsToUI();
		return TRUE;
	}
	else if (type == kBattlePetNormalActionTypeValue)
	{
		__int64 value1 = 0;
		if (p1.is<__int64>())
			value1 = p1.as<__int64>();
		else if (p1.is<bool>())
			value1 = p1.as<bool>() ? 1 : 0;

		--value1;
		if (value1 < 0)
			value1 = 0;

		__int64 value2 = 0;
		if (p2.is<__int64>())
			value2 = p2.as<__int64>();
		else if (p2.is<bool>())
			value2 = p2.as<bool>() ? 1 : 0;
		--value2;
		if (value2 < 0)
			value2 = 0;

		__int64 value3 = 0;
		if (p3.is<__int64>())
			value3 = p3.as<__int64>();
		else if (p3.is<bool>())
			value3 = p3.as<bool>() ? 1 : 0;
		--value3;
		if (value3 < 0)
			value3 = 0;

		injector.setValueHash(kBattlePetNormalActionTypeValue, value1);
		injector.setValueHash(kBattlePetNormalActionEnemyValue, value2);
		injector.setValueHash(kBattlePetNormalActionLevelValue, value3);
		//injector.setValueHash(kBattlePetNormalActionTargetValue, value4);

		emit signalDispatcher.applyHashSettingsToUI();
		return TRUE;
	}

	//start with 1
	switch (type)
	{
	case kAutoWalkDirectionValue://自走方向
	case kServerValue://主機
	case kSubServerValue://副機
	case kPositionValue://位置
	case kBattleCatchModeValue://戰鬥捕捉模式
	{
		if (!p1.is<__int64>() && !p1.is<bool>())
		{
			luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("the first parameter of the setting '%1' must be a number or boolean").arg(typeStr));
			return FALSE;
		}

		__int64 value1 = 0;
		if (p1.is<__int64>())
			value1 = p1.as<__int64>();
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
	case kAutoWalkDelayValue://自走延時
	case kAutoWalkDistanceValue://自走步長
	case kSpeedBoostValue://加速
	case kScriptSpeedValue://腳本速度
	case kBattleActionDelayValue://攻擊延時
	case kBattleResendDelayValue://動作重發

	case kBattleCharRoundActionTargetValue:
	case kBattleCharCrossActionTargetValue:
	case kBattleCharNormalActionTargetValue:
	case kBattlePetRoundActionTargetValue:
	case kBattlePetCrossActionTargetValue:
	case kBattlePetNormalActionTargetValue:
	case kBattleMagicHealTargetValue:
	case kBattleItemHealTargetValue:
	case kBattleMagicReviveTargetValue:
	case kBattleItemReviveTargetValue:
	{
		if (!p1.is<__int64>() && !p1.is<bool>())
		{
			luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("the first parameter of the setting '%1' must be a number or boolean").arg(typeStr));
			return FALSE;
		}

		__int64 value1 = 0;
		if (p1.is<__int64>())
			value1 = p1.as<__int64>();
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
	case kAutoLoginEnable:
	case kAutoReconnectEnable:

	case kHideCharacterEnable:
	case kCloseEffectEnable:
	case kOptimizeEnable:
	case kHideWindowEnable:
	case kMuteEnable:

	case kAutoFreeMemoryEnable:
	case kFastWalkEnable:
	case kPassWallEnable:
	case kLockMoveEnable:
	case kLockImageEnable:
	case kAutoDropMeatEnable:
	case kAutoStackEnable:
	case kKNPCEnable:
	case kAutoAnswerEnable:
	case kAutoEatBeanEnable:
	case kAutoWalkEnable:
	case kFastAutoWalkEnable:
	case kFastBattleEnable:
	case kAutoBattleEnable:
	case kAutoCatchEnable:

	case kAutoEscapeEnable:
	case kBattleNoEscapeWhileLockPetEnable:

	case kBattleTimeExtendEnable:
	case kFallDownEscapeEnable:
	case kShowExpEnable:
	case kWindowDockEnable:
	case kBattleAutoSwitchEnable:
	case kBattleAutoEOEnable:

		//switcher
	case kSwitcherTeamEnable:
	case kSwitcherPKEnable:
	case kSwitcherCardEnable:
	case kSwitcherTradeEnable:
	case kSwitcherGroupEnable:
	case kSwitcherFamilyEnable:
	case kSwitcherJobEnable:
	case kSwitcherWorldEnable:
	{
		if (!p1.is<__int64>() && !p1.is<bool>())
		{
			luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("the first parameter of the setting '%1' must be a number or boolean").arg(typeStr));
			return FALSE;
		}

		__int64 value1 = 0;
		if (p1.is<__int64>())
			value1 = p1.as<__int64>();
		else if (p1.is<bool>())
			value1 = p1.as<bool>() ? 1 : 0;

		bool ok = value1 > 0;
		if (value1 < 0)
			value1 = 0;

		injector.setEnableHash(type, ok);
		if (type == kFastBattleEnable && ok)
		{
			injector.setEnableHash(kAutoBattleEnable, !ok);
			if (ok && !injector.server.isNull())
				injector.server->doBattleWork(false);
		}
		else if (type == kAutoBattleEnable && ok)
		{
			injector.setEnableHash(kFastBattleEnable, !ok);
			if (ok && !injector.server.isNull())
				injector.server->doBattleWork(false);
		}
		else if (type == kAutoWalkEnable && ok)
			injector.setEnableHash(kFastAutoWalkEnable, !ok);
		else if (type == kFastAutoWalkEnable && ok)
			injector.setEnableHash(kAutoWalkEnable, !ok);

		emit signalDispatcher.applyHashSettingsToUI();
		return TRUE;
	}
	default:
		break;
	}

	//0:close >1:open string value
	switch (type)
	{
	case kAutoAbilityEnable:
	case kBattleItemHealMpEnable:
	case kNormalItemHealMpEnable:
	case kBattleCatchCharItemEnable:
	case kLockAttackEnable:
	case kLockEscapeEnable:
	case kAutoDropEnable:
	case kAutoJoinEnable:
	case kLockPetScheduleEnable:
	{
		if (!p1.is<__int64>() && !p1.is<bool>())
		{
			luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("the first parameter of the setting '%1' must be a number or boolean").arg(typeStr));
			return FALSE;
		}

		__int64 value1 = 0;
		if (p1.is<__int64>())
			value1 = p1.as<__int64>();
		else if (p1.is<bool>())
			value1 = p1.as<bool>() ? 1 : 0;

		bool ok = value1 > 0;
		if (value1 < 0)
			value1 = 0;

		QString text = toQString(p2);

		injector.setEnableHash(type, ok);
		if (type == kLockPetScheduleEnable && ok)
		{
			injector.setEnableHash(kLockPetEnable, !ok);
			injector.setEnableHash(kLockRideEnable, !ok);
			injector.setStringHash(kLockPetScheduleString, text);
		}
		else if (type == kAutoJoinEnable && ok)
		{
			injector.setStringHash(kAutoFunNameString, text);
			injector.setValueHash(kAutoFunTypeValue, 0);
		}
		else if (type == kLockAttackEnable && ok)
			injector.setStringHash(kLockAttackString, text);
		else if (type == kLockEscapeEnable && ok)
			injector.setStringHash(kLockEscapeString, text);
		else if (type == kAutoDropEnable && ok)
			injector.setStringHash(kAutoDropItemString, text);
		else if (type == kBattleCatchCharItemEnable && ok)
		{
			injector.setStringHash(kBattleCatchCharItemString, text);
			injector.setValueHash(kBattleCatchTargetItemHpValue, value1 + 1);
		}
		else if (type == kNormalItemHealMpEnable && ok)
		{
			injector.setStringHash(kNormalItemHealMpItemString, text);
			injector.setValueHash(kNormalItemHealMpValue, value1 + 1);
		}
		else if (type == kBattleItemHealMpEnable && ok)
		{
			injector.setStringHash(kBattleItemHealMpItemString, text);
			injector.setValueHash(kBattleItemHealMpValue, value1 + 1);
		}
		else if (type == kAutoAbilityEnable)
		{
			injector.setStringHash(kAutoAbilityString, text);
		}

		emit signalDispatcher.applyHashSettingsToUI();
		return TRUE;
	}
	default:
		break;
	}

	//0:close >1:open __int64 value
	switch (type)
	{
	case kBattleSkillMpEnable:
	case kBattleCatchPetSkillEnable:
	case kBattleCatchTargetMaxHpEnable:
	case kBattleCatchTargetLevelEnable:
	case kLockTimeEnable:
	case kLockPetEnable:
	case kLockRideEnable:
	{
		if (!p1.is<__int64>() && !p1.is<bool>())
		{
			luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("the first parameter of the setting '%1' must be a number or boolean").arg(typeStr));
			return FALSE;
		}

		__int64 value1 = 0;
		if (p1.is<__int64>())
			value1 = p1.as<__int64>();
		else if (p1.is<bool>())
			value1 = p1.as<bool>() ? 1 : 0;

		bool ok = value1 > 0;
		--value1;
		if (value1 < 0)
			value1 = 0;

		injector.setEnableHash(type, ok);
		if (type == kLockPetEnable && ok)
		{
			injector.setEnableHash(kLockPetScheduleEnable, !ok);
			injector.setValueHash(kLockPetValue, value1);
		}
		else if (type == kLockRideEnable && ok)
		{
			injector.setEnableHash(kLockPetScheduleEnable, !ok);
			injector.setValueHash(kLockRideValue, value1);
		}
		else if (type == kLockTimeEnable && ok)
			injector.setValueHash(kLockTimeValue, value1 + 1);
		else if (type == kBattleCatchTargetLevelEnable && ok)
			injector.setValueHash(kBattleCatchTargetLevelValue, value1 + 1);
		else if (type == kBattleCatchTargetMaxHpEnable && ok)
			injector.setValueHash(kBattleCatchTargetMaxHpValue, value1 + 1);
		else if (type == kBattleCatchPetSkillEnable && ok)
			injector.setValueHash(kBattleCatchPetSkillValue, value1 + 1);
		else if (type == kBattleSkillMpEnable)
			injector.setValueHash(kBattleSkillMpValue, value1 + 1);

		emit signalDispatcher.applyHashSettingsToUI();
		return TRUE;
	}
	default:
		break;
	}

	//0:close >1:open two __int64 value
	switch (type)
	{
	case kBattleCatchCharMagicEnable:
	{
		if (!p1.is<__int64>() && !p1.is<bool>())
		{
			luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("the first parameter of the setting '%1' must be a number or boolean").arg(typeStr));
			return FALSE;
		}

		__int64 value1 = 0;
		if (p1.is<__int64>())
			value1 = p1.as<__int64>();
		else if (p1.is<bool>())
			value1 = p1.as<bool>() ? 1 : 0;

		bool ok = value1 > 0;
		if (value1 < 0)
			value1 = 0;

		__int64 value2 = 0;
		if (p2.is<__int64>())
			value2 = p2.as<__int64>();

		--value2;
		if (value2 < 0)
			value2 = 0;


		injector.setEnableHash(type, ok);
		if (type == kBattleCatchCharMagicEnable && ok)
		{
			injector.setValueHash(kBattleCatchTargetMagicHpValue, value1);
			injector.setValueHash(kBattleCatchCharMagicValue, value2);
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
	case kBattlePetRoundActionRoundValue:
	case kBattleCharRoundActionRoundValue:
	{
		__int64 value1 = p1.as<__int64>();
		bool ok = value1 > 0;
		if (value1 < 0)
			value1 = 0;

		__int64 value2 = 0;
		if (p2.is<__int64>())
			value2 = p2.as<__int64>();
		else if (p2.is<bool>())
			value2 = p2.as<bool>() ? 1 : 0;

		--value2;
		if (value2 < 0)
			value2 = 0;

		__int64 value3 = 0;
		if (p3.is<__int64>())
			value3 = p3.as<__int64>();
		else if (p2.is<bool>())
			value3 = p3.as<bool>() ? 1 : 0;
		--value3;
		if (value3 < 0)
			value3 = 0;

		__int64 value4 = 0;
		if (p4.is<__int64>())
			value4 = p4.as<__int64>();
		else if (p4.is<bool>())
			value4 = p4.as<bool>() ? 1 : 0;
		--value4;
		if (value4 < 0)
			value4 = 0;

		__int64 value5 = 0;
		if (p5.is<__int64>())
			value5 = p5.as<__int64>();
		else if (p5.is<bool>())
			value5 = p5.as<bool>() ? 1 : 0;
		if (value5 < 0)
			value5 = kSelectEnemyAny;

		injector.setValueHash(type, value1);
		if (type == kBattleCharRoundActionRoundValue && ok)
		{
			injector.setValueHash(kBattleCharRoundActionTypeValue, value2);
			injector.setValueHash(kBattleCharRoundActionEnemyValue, value3);
			injector.setValueHash(kBattleCharRoundActionLevelValue, value4);
			//injector.setValueHash(kBattleCharRoundActionTargetValue, value5);
		}
		else if (type == kBattlePetRoundActionRoundValue && ok)
		{
			injector.setValueHash(kBattlePetRoundActionTypeValue, value2);
			injector.setValueHash(kBattlePetRoundActionEnemyValue, value3);
			injector.setValueHash(kBattlePetRoundActionLevelValue, value4);
			//injector.setValueHash(kBattlePetRoundActionTargetValue, value5);
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
	case kBattleCrossActionPetEnable:
	case kBattleCrossActionCharEnable:
	case kBattleMagicHealEnable:
	case kNormalMagicHealEnable:
	{
		if (!p1.is<__int64>() && !p1.is<bool>())
		{
			luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("the first parameter of the setting '%1' must be a number or boolean").arg(typeStr));
			return FALSE;
		}

		__int64 value1 = 0;
		if (p1.is<__int64>())
			value1 = p1.as<__int64>();
		else if (p1.is<bool>())
			value1 = p1.as<bool>() ? 1 : 0;

		bool ok = value1 > 0;
		--value1;

		if (value1 < 0)
			value1 = 0;
		injector.setEnableHash(type, ok);

		__int64 value2 = 0;
		if (p2.is<__int64>())
			value2 = p2.as<__int64>();
		else if (p2.is<bool>())
			value2 = p2.as<bool>() ? 1 : 0;
		--value2;
		if (value2 < 0)
			value2 = 0;

		__int64 value3 = 0;
		if (p3.is<__int64>())
			value3 = p3.as<__int64>();
		else if (p3.is<bool>())
			value3 = p3.as<bool>() ? 1 : 0;
		--value3;
		if (value3 < 0)
			value3 = 0;

		__int64 value4 = 0;
		if (p4.is<__int64>())
			value4 = p4.as<__int64>();
		else if (p4.is<bool>())
			value4 = p4.as<bool>() ? 1 : 0;
		--value4;
		if (value4 < 0)
			value4 = 0;

		__int64 value5 = kSelectSelf | kSelectPet;
		if (p5.is<__int64>())
			value5 = p5.as<__int64>();

		if (type == kNormalMagicHealEnable && ok)
		{
			injector.setValueHash(kNormalMagicHealMagicValue, value1);
			injector.setValueHash(kNormalMagicHealCharValue, value2 + 1);
			injector.setValueHash(kNormalMagicHealPetValue, value3 + 1);
			injector.setValueHash(kNormalMagicHealAllieValue, value4 + 1);
		}
		else if (type == kBattleMagicHealEnable && ok)
		{
			injector.setValueHash(kBattleMagicHealMagicValue, value1);
			injector.setValueHash(kBattleMagicHealCharValue, value2 + 1);
			injector.setValueHash(kBattleMagicHealPetValue, value3 + 1);
			injector.setValueHash(kBattleMagicHealAllieValue, value4 + 1);
			//injector.setValueHash(kBattleMagicHealTargetValue, value5);
		}
		else if (type == kBattleCrossActionCharEnable && ok)
		{
			injector.setValueHash(kBattleCharCrossActionTypeValue, value1);
			injector.setValueHash(kBattleCharCrossActionRoundValue, value2);
			//injector.setValueHash(kBattleCharCrossActionTargetValue, value3);
		}
		else if (type == kBattleCrossActionPetEnable && ok)
		{
			injector.setValueHash(kBattlePetCrossActionTypeValue, value1);
			injector.setValueHash(kBattlePetCrossActionRoundValue, value2);
			//injector.setValueHash(kBattlePetCrossActionTargetValue, value3);
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
	case kEOCommandString:
	case kGameAccountString:
	case kGamePasswordString:
	case kGameSecurityCodeString:
	case kMailWhiteListString:
	case kBattleCatchPetNameString:
	{
		QString text;
		if (!p1.is<std::string>())
		{
			if (!p1.is<__int64>())
				return FALSE;

			text = toQString(p1.as<__int64>());
		}
		else
			text = toQString(p1);

		injector.setStringHash(type, text);
		emit signalDispatcher.applyHashSettingsToUI();
		return TRUE;
	}
	default:
		break;
	}

	return FALSE;
}