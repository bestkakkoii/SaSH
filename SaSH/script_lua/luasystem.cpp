#include "stdafx.h"
#include "clua.h"
#include "injector.h"
#include "signaldispatcher.h"

lua_Integer CLuaSystem::sleep(lua_Integer t, sol::this_state s)
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
			if (luadebug::isInterruptionRequested(s))
				return FALSE;
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

lua_Integer CLuaSystem::logout(sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (!injector.server.isNull())
	{
		injector.server->logOut();
		return TRUE;
	}

	return FALSE;
}

lua_Integer CLuaSystem::logback(sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (!injector.server.isNull())
	{
		injector.server->logBack();
		return TRUE;
	}

	return FALSE;
}

lua_Integer CLuaSystem::eo(sol::this_state s)
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

lua_Integer CLuaSystem::announce(sol::object ostr, sol::this_state s)
{
	//1~11
	lua_Integer color = QRandomGenerator64::global()->bounded(1, 11);
	return announce(ostr, color - 1, s);
}

lua_Integer CLuaSystem::announce(sol::object ostr, lua_Integer color, sol::this_state s)
{
	return announce("{:msg}", ostr, color, s);
}

//這裡還沒想好format格式怎麼設計，暫時先放著
lua_Integer CLuaSystem::announce(std::string format, sol::object ostr, lua_Integer color, sol::this_state s)
{
	QString text;
	if (ostr.is<lua_Integer>())
		text = QString::number(ostr.as<lua_Integer>());
	else if (ostr.is<double>())
		text = QString::number(ostr.as<double>(), 'f', 16);
	else if (ostr.is<std::string>())
		text = QString::fromUtf8(ostr.as<const char*>());
	else if (ostr.is<sol::table>())
	{
		QStringList list;
		sol::table t = ostr.as<sol::table>();
		list << "{";

		//print key : value
		for (auto& v : t)
		{
			if (v.second.is<lua_Integer>())
				list << QString("%1:%2").arg(v.first.as<lua_Integer>()).arg(v.second.as<lua_Integer>());
			else if (v.second.is<double>())
				list << QString("%1:%2").arg(v.first.as<lua_Integer>()).arg(v.second.as<double>(), 0, 'f', 16);
			else if (v.second.is<std::string>())
				list << QString("%1:%2").arg(v.first.as<lua_Integer>()).arg(QString::fromUtf8(v.second.as<const char*>()));
			else
				list << "(invalid type)";
		}

		list << "}";

		text = list.join(" ");
	}
	else
		luadebug::tryPopCustomErrorMsg(s, luadebug::ERROR_PARAM_TYPE, false, 1, QObject::tr("invalid value type"));

	Injector& injector = Injector::getInstance();
	if (!injector.server.isNull())
	{
		injector.server->announce(text, color);
		//logExport(currentline, text, color);
		return TRUE;
	}
	return FALSE;
}

lua_Integer CLuaSystem::messagebox(sol::object ostr, sol::object otype, sol::this_state s)
{
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();

	QString text;
	if (ostr.is<lua_Integer>())
		text = QString::number(ostr.as<lua_Integer>());
	else if (ostr.is<double>())
		text = QString::number(ostr.as<double>(), 'f', 16);
	else if (ostr.is<std::string>())
		text = QString::fromUtf8(ostr.as<const char*>());
	else
		luadebug::tryPopCustomErrorMsg(s, luadebug::ERROR_PARAM_TYPE, false, 1, QObject::tr("invalid value type"));

	qint64 type = 0;
	if (ostr.is<lua_Integer>())
		type = ostr.as<lua_Integer>();
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

lua_Integer CLuaSystem::talk(sol::object ostr, sol::this_state s)
{
	return 0;
}

lua_Integer CLuaSystem::cleanchat(sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	injector.server->cleanChatHistory();

	return TRUE;
}

lua_Integer CLuaSystem::menu(lua_Integer index, sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->saMenu(index);

	return TRUE;
}

lua_Integer CLuaSystem::menu(lua_Integer type, lua_Integer index, sol::this_state s)
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

lua_Integer CLuaSystem::savesetting(const std::string& sfileName, sol::this_state s)
{
	QString fileName = QString::fromUtf8(sfileName.c_str());
	fileName.replace("\\", "/");
	fileName = QCoreApplication::applicationDirPath() + "/settings/" + fileName;
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

lua_Integer CLuaSystem::loadsetting(const std::string& sfileName, sol::this_state s)
{
	QString fileName = QString::fromUtf8(sfileName.c_str());
	fileName.replace("\\", "/");
	fileName = QCoreApplication::applicationDirPath() + "/settings/" + fileName;
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

lua_Integer CLuaSystem::press(const std::string& buttonStr, sol::object onpcName, sol::object odlgId, sol::this_state s)
{
	return 0;
}
lua_Integer CLuaSystem::press(lua_Integer row, sol::object onpcName, sol::object odlgId, sol::this_state s)
{
	return 0;
}

lua_Integer CLuaSystem::input(const std::string& str, sol::object onpcName, sol::object odlgId, sol::this_state s)
{
	return 0;
}

lua_Integer CLuaSystem::set(lua_Integer enumInt, sol::object p1, sol::object p2, sol::object p3, sol::object p4, sol::object p5, sol::object p6, sol::object p7, sol::this_state s)
{
	return 0;
}