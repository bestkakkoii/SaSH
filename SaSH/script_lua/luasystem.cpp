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
	Injector& injector = Injector::getInstance();
	if (!injector.server.isNull())
	{
		injector.server->logOut();
		return TRUE;
	}

	return FALSE;
}

qint64 CLuaSystem::logback(sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (!injector.server.isNull())
	{
		injector.server->logBack();
		return TRUE;
	}

	return FALSE;
}

qint64 CLuaSystem::eo(sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return 0;

	luadebug::checkBattleThenWait(s);

	QElapsedTimer timer; timer.start();
	injector.server->EO();

	bool bret = luadebug::waitfor(s, 5000, []() { return !Injector::getInstance().server->isEOTTLSend.load(std::memory_order_acquire); });

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
		raw = QString(value ? "true" : "false");
		msg = "(boolean)" + raw;
	}
	else if (maybe_defaulted.is<qlonglong>())
	{
		const qlonglong value = maybe_defaulted.as<qlonglong>();
		raw = (QString::number(value));
		msg = "(integer)" + raw;
	}
	else if (maybe_defaulted.is<qreal>())
	{
		const qreal value = maybe_defaulted.as<qreal>();
		raw = (QString::number(value, 'f', 16));
		msg = "(number)" + raw;
	}
	else if (maybe_defaulted.is<std::string>())
	{
		raw = QString::fromUtf8(maybe_defaulted.as<std::string>().c_str());
		msg = "(string)" + raw;
	}
	else if (maybe_defaulted.is<const char*>())
	{
		raw = (maybe_defaulted.as<const char*>());
		msg = "(string)" + raw;
	}
	else if (maybe_defaulted.is<std::wstring>())
	{
		raw = QString::fromStdWString(maybe_defaulted.as<std::wstring>());
		msg = "(utf8-string)" + raw;
	}
	else if (maybe_defaulted.is<sol::table>())
	{
		//print table
		raw = luadebug::getTableVars(s.L, 1, 10);
		msg = "(table)\r\n" + raw;

	}
	else
	{
		//print type name
		switch (maybe_defaulted.get_type())
		{
		case sol::type::function: msg = "(function)"; break;
		case sol::type::userdata: msg = "(userdata)"; break;
		case sol::type::thread: msg = "(thread)"; break;
		case sol::type::lightuserdata: msg = "(lightuserdata)"; break;
		case sol::type::none: msg = "(none)"; break;
		case sol::type::poly: msg = "(poly)"; break;
		default: msg = "print error: other type, unable to show"; break;
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

	Injector& injector = Injector::getInstance();
	sol::safe_function print = lua["_print"];

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
			if (print.valid())
				print(it.toUtf8().constData());
			if (announceOk)
				injector.server->announce(it, color);
		}
	}
	else
	{
		if (print.valid())
			print(msg.toUtf8().constData());
		if (announceOk)
			injector.server->announce(msg, color);
	}

	return FALSE;
}

qint64 CLuaSystem::messagebox(sol::object ostr, sol::object otype, sol::this_state s)
{
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();

	QString text;
	if (ostr.is<qint64>())
		text = QString::number(ostr.as<qint64>());
	else if (ostr.is<double>())
		text = QString::number(ostr.as<double>(), 'f', 16);
	else if (ostr.is<std::string>())
		text = QString::fromUtf8(ostr.as<std::string>().c_str());
	else
		luadebug::tryPopCustomErrorMsg(s, luadebug::ERROR_PARAM_TYPE, false, 1, QObject::tr("invalid value type"));

	qint64 type = 0;
	if (ostr.is<qint64>())
		type = ostr.as<qint64>();
	else if (ostr != sol::lua_nil)
		luadebug::tryPopCustomErrorMsg(s, luadebug::ERROR_PARAM_TYPE, false, 2, QObject::tr("invalid value of 'type'"));

	qint32 nret = QMessageBox::StandardButton::NoButton;
	emit signalDispatcher.messageBoxShow(text, type, &nret);
	if (nret != QMessageBox::StandardButton::NoButton)
	{
		return nret;
	}
	return 0;
}

qint64 CLuaSystem::talk(sol::object ostr, sol::this_state s)
{
	return 0;
}

qint64 CLuaSystem::cleanchat(sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	injector.server->cleanChatHistory();

	return TRUE;
}

qint64 CLuaSystem::menu(qint64 index, sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->saMenu(index);

	return TRUE;
}

qint64 CLuaSystem::menu(qint64 type, qint64 index, sol::this_state s)
{
	Injector& injector = Injector::getInstance();
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
	QString fileName = QString::fromUtf8(sfileName.c_str());
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

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	emit signalDispatcher.saveHashSettings(fileName, true);

	return TRUE;
}

qint64 CLuaSystem::loadsetting(const std::string& sfileName, sol::this_state s)
{
	QString fileName = QString::fromUtf8(sfileName.c_str());
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

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	emit signalDispatcher.loadHashSettings(fileName, true);

	return TRUE;
}

qint64 CLuaSystem::press(std::string sbuttonStr, qint64 unitid, qint64 dialogid, sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	QString text = QString::fromUtf8(sbuttonStr.c_str());
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
		injector.server->press(button, unitid, dialogid);
	else if (row != -1)
		injector.server->press(row, unitid, dialogid);
	else
		return FALSE;

	return TRUE;
}

qint64 CLuaSystem::press(qint64 row, qint64 unitid, qint64 dialogid, sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->press(row, unitid, dialogid);

	return TRUE;
}

qint64 CLuaSystem::input(const std::string& str, qint64 unitid, qint64 dialogid, sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	QString text = QString::fromUtf8(str.c_str());

	injector.server->inputtext(text, unitid, dialogid);

	return TRUE;
}

qint64 CLuaSystem::leftclick(qint64 x, qint64 y, sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	injector.leftClick(x, y);
	return TRUE;
}

qint64 CLuaSystem::rightclick(qint64 x, qint64 y, sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	injector.rightClick(x, y);
	return TRUE;
}
qint64 CLuaSystem::leftdoubleclick(qint64 x, qint64 y, sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	injector.leftDoubleClick(x, y);
	return TRUE;
}

qint64 CLuaSystem::mousedragto(qint64 x1, qint64 y1, qint64 x2, qint64 y2, sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	injector.dragto(x1, y1, x2, y2);
	return TRUE;
}

qint64 CLuaSystem::set(qint64 enumInt, sol::object p1, sol::object p2, sol::object p3, sol::object p4, sol::object p5, sol::object p6, sol::object p7, sol::this_state s)
{
	return TRUE;
}