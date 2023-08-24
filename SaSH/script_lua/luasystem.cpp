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
qint64 CLuaSystem::announce(sol::object ostr, sol::object ocolor, sol::this_state s)
{
	QString text;
	if (ostr.is<qint64>())
		text = QString::number(ostr.as<qint64>());
	else if (ostr.is<double>())
		text = QString::number(ostr.as<double>(), 'f', 16);
	else if (ostr.is<std::string>())
		text = QString::fromUtf8(ostr.as<std::string>().c_str());
	else if (ostr.is<sol::table>())
	{
		QStringList list;
		sol::table t = ostr.as<sol::table>();
		list << "{";

		//print key : value
		for (auto& v : t)
		{
			if (v.second.is<qint64>())
				list << QString("%1:%2").arg(v.first.as<qint64>()).arg(v.second.as<qint64>());
			else if (v.second.is<double>())
				list << QString("%1:%2").arg(v.first.as<qint64>()).arg(v.second.as<double>(), 0, 'f', 16);
			else if (v.second.is<std::string>())
				list << QString("%1:%2").arg(v.first.as<qint64>()).arg(QString::fromUtf8(v.second.as<const char*>()));
			else
				list << "(invalid type)";
		}

		list << "}";

		text = list.join(" ");
	}
	else
		luadebug::tryPopCustomErrorMsg(s, luadebug::ERROR_PARAM_TYPE, false, 1, QObject::tr("invalid value type"));

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
	luadebug::logExport(s, text, color);
	if (color != -2 && !injector.server.isNull())
	{
		injector.server->announce(text, color);
		return TRUE;
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

qint64 CLuaSystem::press(const std::string& buttonStr, sol::object onpcName, sol::object odlgId, sol::this_state s)
{
	return 0;
}
qint64 CLuaSystem::press(qint64 row, sol::object onpcName, sol::object odlgId, sol::this_state s)
{
	return 0;
}

qint64 CLuaSystem::input(const std::string& str, sol::object onpcName, sol::object odlgId, sol::this_state s)
{
	return 0;
}

qint64 CLuaSystem::leftclick(qint64 x, qint64 y, sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	injector.leftClick(x, y);
}

qint64 CLuaSystem::rightclick(qint64 x, qint64 y, sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	injector.rightClick(x, y);
}
qint64 CLuaSystem::leftdoubleclick(qint64 x, qint64 y, sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	injector.leftDoubleClick(x, y);
}

qint64 CLuaSystem::mousedragto(qint64 x1, qint64 y1, qint64 x2, qint64 y2, sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	injector.dragto(x1, y1, x2, y2);
}

qint64 CLuaSystem::set(qint64 enumInt, sol::object p1, sol::object p2, sol::object p3, sol::object p4, sol::object p5, sol::object p6, sol::object p7, sol::this_state s)
{
	return 0;
}