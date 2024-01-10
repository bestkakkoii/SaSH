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
#include <gamedevice.h>
#include "signaldispatcher.h"

// The shared memory key used by the application
constexpr const char* kRemoteGlobalSharedMemoryKey = "RemoteGlobalSharedMemoryKey";
constexpr const char* kRemoteGlobalSemaphoreKey = "RemoteGlobalSystemSemaphore";
constexpr long long kRemoteGlobalSharedMemorySize = 6553600;  // Shared memory size

static QSharedMemory g_remoteGlobalSharedMemory(kRemoteGlobalSharedMemoryKey);

static bool initializeSharedMemory()
{
	if (g_remoteGlobalSharedMemory.isAttached())
		return true;

	// Try to attach to existing shared memory, if it fails, create a new one
	if (!g_remoteGlobalSharedMemory.attach())
	{
		// Create a shared memory segment with the specified size
		if (!g_remoteGlobalSharedMemory.create(kRemoteGlobalSharedMemorySize))
		{
			qDebug() << "Unable to create shared memory segment.";
			return false;
		}
	}
	return true;
}

static bool setRemoteGlobal(const std::string& varname, const QVariant& value)
{
	QSystemSemaphore semaphore(kRemoteGlobalSemaphoreKey, 1, QSystemSemaphore::Open);
	semaphore.acquire();

	do
	{
		// Convert the variable name to a SHA-512 hash
		QString hashedName = util::utf8toSha512String(varname);

		if (!initializeSharedMemory())
		{
			break;
		}

		// Lock the shared memory for exclusive access
		g_remoteGlobalSharedMemory.lock();

		// Read the current data from the shared memory
		QByteArray jsonData = QByteArray::fromRawData(static_cast<const char*>(g_remoteGlobalSharedMemory.constData()), kRemoteGlobalSharedMemorySize);
		long long lastZeroIndex = jsonData.indexOf('\0');
		if (lastZeroIndex != -1)
			jsonData = jsonData.left(lastZeroIndex);

		QJsonParseError error;
		QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonData, &error);
		QJsonObject jsonObject;
		if ((error.error == QJsonParseError::NoError) && !jsonDocument.isNull() && jsonDocument.isObject())
		{
			jsonObject = jsonDocument.object();
		}
		else
		{
			// If JSON is corrupted or cannot be deserialized, we start fresh
			qDebug() << "JSON data is corrupted." << error.errorString() << jsonDocument.isNull() << jsonDocument.isObject();
			jsonObject = QJsonObject();
		}

		// Insert or update the value in the JSON object
		if (value.isValid())
			jsonObject.insert(hashedName, QJsonValue::fromVariant(value));
		else if (jsonObject.contains(hashedName))
			jsonObject.remove(hashedName);

		//set back to jsonDocument
		jsonDocument.setObject(jsonObject);

		jsonData = jsonDocument.toJson(QJsonDocument::Compact);
		qDebug() << "JSON data:" << util::toQString(jsonData);

		const char* from = jsonData.constData();
		memset(g_remoteGlobalSharedMemory.data(), 0, kRemoteGlobalSharedMemorySize);
		_snprintf_s(reinterpret_cast<char*>(g_remoteGlobalSharedMemory.data()), kRemoteGlobalSharedMemorySize, _TRUNCATE, "%s", from);
		g_remoteGlobalSharedMemory.unlock();


		semaphore.release();
		return true;
	} while (false);

	semaphore.release();

	return false;
}

QVariant getRemoteGlobal(const std::string& varname)
{
	QSystemSemaphore semaphore(kRemoteGlobalSemaphoreKey, 1, QSystemSemaphore::Open);
	semaphore.acquire();

	QVariant returnValue;
	do
	{
		QString hashedName = util::utf8toSha512String(varname);

		if (!initializeSharedMemory())
			break;

		// Lock the shared memory for exclusive access
		g_remoteGlobalSharedMemory.lock();

		// Read the current data from the shared memory
		QByteArray jsonData = QByteArray::fromRawData(static_cast<const char*>(g_remoteGlobalSharedMemory.constData()), kRemoteGlobalSharedMemorySize);
		long long lastZeroIndex = jsonData.indexOf('\0');
		if (lastZeroIndex != -1)
			jsonData = jsonData.left(lastZeroIndex);

		qDebug() << "JSON data:" << util::toQString(jsonData);

		// Unlock the shared memory
		g_remoteGlobalSharedMemory.unlock();

		// Deserialize the JSON
		QJsonParseError error;
		QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonData, &error);
		if ((error.error != QJsonParseError::NoError) || jsonDocument.isNull() || !jsonDocument.isObject())
		{
			qDebug() << "JSON data is corrupted." << error.errorString() << jsonDocument.isNull() << jsonDocument.isObject();
			break;
		}

		QJsonObject jsonObject = jsonDocument.object();
		QJsonValue jsonValue = jsonObject.value(hashedName);
		returnValue = jsonValue.toVariant();
	} while (false);

	semaphore.release();
	return returnValue;
}

static bool clearRemoteGlobal()
{
	QSystemSemaphore semaphore(kRemoteGlobalSemaphoreKey, 1, QSystemSemaphore::Open);
	semaphore.acquire();

	if (!initializeSharedMemory())
	{
		semaphore.release();
		return false;
	}

	// Lock the shared memory for exclusive access
	g_remoteGlobalSharedMemory.lock();

	// Clear the shared memory by setting it to zero
	memset(g_remoteGlobalSharedMemory.data(), 0, kRemoteGlobalSharedMemorySize);

	// Unlock the shared memory
	g_remoteGlobalSharedMemory.unlock();

	semaphore.release();

	return true;
}

long long CLuaSystem::clearglobal()
{
	//g_global_custom_variabls.clear();
	return clearRemoteGlobal();
}

long long CLuaSystem::setglobal(std::string sname, sol::object od, sol::this_state s)
{
	sol::state_view lua(s);
	if (sname.empty())
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("global variable name cannot be empty"));
		return FALSE;
	}

	if (od == sol::lua_nil)
	{
		setRemoteGlobal(sname, QVariant());
		return TRUE;
	}

	QVariant value;
	if (od.is<bool>())
		value = od.as<bool>();
	else if (od.is<long long>())
		value = od.as<long long>();
	else if (od.is<double>())
		value = od.as<double>();
	else if (od.is<std::string>())
		value = util::toQString(od);
	else if (od.is<sol::table>() && !od.is<sol::userdata>() && !od.is<sol::function>() && !od.is<sol::lightuserdata>())
	{
		long long depth = 1024;
		QString text = luadebug::getTableVars(s.L, 2, depth);
		value = QString("__table|%1").arg(text);
	}
	else
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("invalid value type"));
		return FALSE;
	}

	return setRemoteGlobal(sname, value);
}

sol::object CLuaSystem::getglobal(std::string sname, sol::this_state s)
{
	sol::state_view lua(s);

	if (sname.empty())
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("global variable name cannot be empty"));
		return sol::lua_nil;
	}

	QVariant value = getRemoteGlobal(sname);

	QVariant::Type type = value.type();
	switch (type)
	{
	case QVariant::Bool: return sol::make_object(lua, value.toBool());
	case QVariant::Int: return sol::make_object(lua, value.toLongLong());
	case QVariant::LongLong: return sol::make_object(lua, value.toLongLong());
	case QVariant::Double: return sol::make_object(lua, value.toDouble());
	case QVariant::String:
	{
		QString text = value.toString();
		if (!text.startsWith("__table|"))
			return sol::make_object(lua, util::toConstData(value.toString()));

		text = text.mid(8);

		QString tempScript = QString(R"(
			return (%1);
		)").arg(text);

		std::string script = util::toConstData(tempScript);

		sol::protected_function_result result = lua.safe_script(script, sol::script_pass_on_error);
		if (!result.valid())
		{
			sol::error err = result;
			luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, err.what());
			return sol::lua_nil;
		}

		sol::object obj = result;

		if (obj.is<sol::table>())
			return obj;
		else
			return sol::lua_nil;
	}
	default: return sol::lua_nil;
	}
}

sol::object CLuaSystem::require(std::string sname, sol::this_state s)
{
	sol::state_view lua(s);
	QString name = util::toQString(sname);
	QStringList paths;
	util::searchFiles(util::applicationDirPath(), name, ".lua", &paths, false, true);
	if (paths.isEmpty())
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("cannot find file '%1'").arg(name));
		return sol::lua_nil;
	}

	QString path = paths.first();
	if (path.isEmpty())
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("cannot find file '%1'").arg(name));
		return sol::lua_nil;
	}

	sol::protected_function loadfile = lua["loadfile"];
	if (!loadfile.valid())
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("cannot find function 'loadfile'"));
		return sol::lua_nil;
	}

	sol::protected_function_result result = loadfile(util::toConstData(path));
	if (!result.valid())
	{
		try
		{
			sol::error err = result;
			qDebug() << util::toQString(err.what());
			luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, err.what());
		}
		catch (...)
		{
			luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("loadfile failed with exception"));
		}
		return sol::lua_nil;
	}

	sol::object	obj = result;

	if (obj.is<sol::function>())
	{
		sol::protected_function moduleFunction = obj.as< sol::function>();
		if (!moduleFunction.valid())
		{
			luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("loadfile('%1') result is not a valid function").arg(path));
			return sol::lua_nil;
		}

		result = moduleFunction();
	}
	else
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("loadfile('%1') result is '#%2' but not a function").arg(path).arg(static_cast<long long>(obj.get_type())));
		return sol::lua_nil;
	}

	if (!result.valid())
	{
		try
		{
			sol::error err = result;
			qDebug() << util::toQString(err.what());
			luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, err.what());
		}
		catch (...)
		{
			luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("result function failed with exception"));
		}
	}

	obj = result;
	if (obj.is<sol::table>())
		return obj;
	else
		return sol::lua_nil;
}

long long CLuaSystem::capture(std::string sfilename, sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	return gamedevice.capture(util::toQString(sfilename));
}

long long CLuaSystem::sleep(long long t, sol::this_state s)
{
	if (t < 0)
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("sleep time must above 0"));
		return FALSE;
	}

	if (t >= 1000)
	{
		long long i = 0;
		long long size = t / 1000;
		for (; i < size; ++i)
		{
			QThread::msleep(1000);
			if (luadebug::checkStopAndPause(s))
				return FALSE;
		}

		if (i % 1000 > 0)
			QThread::msleep(static_cast<DWORD>(i) % 1000UL);

		if (luadebug::checkStopAndPause(s))
			return FALSE;
	}
	else if (t > 0)
	{
		QThread::msleep(t);
		if (luadebug::checkStopAndPause(s))
			return FALSE;
	}

	return TRUE;
}

long long CLuaSystem::logout(sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (!gamedevice.worker.isNull())
	{
		return gamedevice.worker->logOut();
	}

	return FALSE;
}

long long CLuaSystem::logback(sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (!gamedevice.worker.isNull())
	{
		return gamedevice.worker->logBack();
	}

	return FALSE;
}

long long CLuaSystem::eo(sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	util::timer timer;
	if (!gamedevice.worker->EO())
		return FALSE;

	bool bret = luadebug::waitfor(s, 5000, [currentIndex]() { return !GameDevice::getInstance(currentIndex).worker->isEOTTLSend.get(); });

	long long result = bret ? gamedevice.worker->lastEOTime.get() : 0;

	return result;
}

long long CLuaSystem::openlog(std::string sfilename, sol::object oformat, sol::object obuffersize, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	QString filename = util::toQString(sfilename);
	if (filename.isEmpty())
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("log file name cannot be empty"));
		return false;
	}

	QString format = "[%(date) %(time)] | [@%(line)] | %(message)";
	if (oformat.is<std::string>() && !oformat.as<std::string>().empty())
		format = util::toQString(oformat);

	long long buffersize = 1024;
	if (obuffersize.is<long long>())
		buffersize = obuffersize.as<long long>();

	return gamedevice.log.initialize(filename, buffersize, format);
}

//這裡還沒想好format格式怎麼設計，暫時先放著
long long CLuaSystem::print(sol::object ocontent, sol::object ocolor, sol::this_state s)
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
	else if (ocontent.is<sol::table>() && !ocontent.is<sol::userdata>() && !ocontent.is<sol::function>() && !ocontent.is<sol::lightuserdata>())
	{
		//print table
		long long depth = 1024;
		raw = luadebug::getTableVars(s.L, 1, depth);
		msg = raw;
		msg.replace("=[[", "='");
		msg.replace("]],", "',");
		msg.replace("]]}", "'}");
		msg.replace(",", ",\n");
	}
	else
	{
		QString pointerStr = util::toQString(reinterpret_cast<long long>(ocontent.pointer()), 16);
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

	long long color = 4;
	if (ocolor.is<long long>())
	{
		color = ocolor.as<long long>();
		if (color != -2 && (color < 0 || color > 10))
			luadebug::tryPopCustomErrorMsg(s, luadebug::ERROR_PARAM_TYPE, false, 2, QObject::tr("invalid value of 'color'"));
	}
	else if (ocolor == sol::lua_nil || ocolor.is<long long>() && ocolor.as<long long>() == -1)
	{
		util::rnd::get(&color, 0LL, 10LL);
	}

	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	bool split = false;
	bool doNotAnnounce = color == -2 || gamedevice.worker.isNull();
	QStringList l;
	if (msg.contains("\r\n"))
	{
		l = msg.split("\r\n");
		luadebug::logExport(s, l, color, doNotAnnounce);
		split = true;
		return TRUE;
	}
	else if (msg.contains("\n"))
	{
		l = msg.split("\n");
		luadebug::logExport(s, l, color, doNotAnnounce);
		split = true;
		return TRUE;
	}
	else
	{
		luadebug::logExport(s, msg, color, doNotAnnounce);
		return TRUE;
	}

	return FALSE;
}

std::string CLuaSystem::messagebox(sol::object ostr, sol::object otype, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);

	QString text;
	if (ostr.is<long long>())
		text = util::toQString(ostr.as<long long>());
	else if (ostr.is<double>())
		text = util::toQString(ostr.as<double>());
	else if (ostr.is<std::string>())
		text = util::toQString(ostr);
	else
		luadebug::tryPopCustomErrorMsg(s, luadebug::ERROR_PARAM_TYPE, false, 1, QObject::tr("invalid value type"));

	long long type = 0;
	if (ostr.is<long long>())
		type = ostr.as<long long>();
	else if (ostr != sol::lua_nil)
		luadebug::tryPopCustomErrorMsg(s, luadebug::ERROR_PARAM_TYPE, false, 2, QObject::tr("invalid value of 'type'"));

	long long nret = QMessageBox::StandardButton::NoButton;
	emit signalDispatcher.messageBoxShow(text, type, "", &nret);
	if (nret != QMessageBox::StandardButton::NoButton)
	{
		return nret == QMessageBox::StandardButton::Yes ? "yes" : "no";
	}

	return "";
}

long long CLuaSystem::talk(sol::object ostr, sol::object ocolor, sol::object omode, sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	QString text;
	if (ostr.is<long long>())
		text = util::toQString(ostr.as<long long>());
	else if (ostr.is<double>())
		text = util::toQString(ostr.as<double>());
	else if (ostr.is<std::string>())
		text = util::toQString(ostr);
	else if (ostr.is<sol::table>() && !ostr.is<sol::userdata>() && !ostr.is<sol::function>() && !ostr.is<sol::lightuserdata>())
	{
		long long depth = 1024;
		text = luadebug::getTableVars(s.L, 1, depth);
	}
	else
		luadebug::tryPopCustomErrorMsg(s, luadebug::ERROR_PARAM_TYPE, false, 1, QObject::tr("invalid value type"));

	long long color = 0;
	if (ocolor.is<long long>())
		color = ocolor.as<long long>();

	sa::TalkMode mode = sa::kTalkNormal;
	if (omode.is<long long>() && omode.as<long long>() < sa::kTalkModeMax)
		mode = static_cast<sa::TalkMode>(omode.as<long long>());

	return gamedevice.worker->talk(text, color, mode);
}

long long CLuaSystem::cleanchat(sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	return gamedevice.worker->cleanChatHistory();
}

long long CLuaSystem::savesetting(const std::string& sfileName, sol::this_state s)
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

	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	emit signalDispatcher.saveHashSettings(fileName, true);

	return TRUE;
}

long long CLuaSystem::loadsetting(const std::string& sfileName, sol::this_state s)
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
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("file '%1' does not exist").arg(fileName));
		return FALSE;
	}

	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	emit signalDispatcher.loadHashSettings(fileName, true);

	return TRUE;
}

long long CLuaSystem::press(sol::object obutton, sol::object ounitid, sol::object odialogid, sol::object oext, sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	long long ext = 0;
	if (oext.is<long long>())
		ext = oext.as<long long>();

	std::string sbuttonStr;
	long long row = -1;
	if (obutton.is<long long>())
		row = obutton.as<long long>();
	else if (obutton.is<std::string>())
		sbuttonStr = obutton.as<std::string>();
	else
		return FALSE;

	long long unitid = -1;
	if (ounitid.is<long long>())
		unitid = ounitid.as<long long>();
	long long dialogid = -1;
	if (odialogid.is<long long>())
		dialogid = odialogid.as<long long>();

	QString text = util::toQString(sbuttonStr);
	sa::ButtonType button = sa::buttonMap.value(text.toUpper(), sa::kButtonNone);

	if (ounitid.is<std::string>())
	{
		QString searchStr = util::toQString(ounitid.as<std::string>());
		sa::map_unit_t unit;
		if (gamedevice.worker->findUnit(searchStr, sa::kObjectNPC, &unit, "", unitid))
		{
			if (!gamedevice.worker->isDialogVisible())
				gamedevice.worker->setCharFaceToPoint(unit.p);
			if (button == sa::kButtonNone && row == -1)
				QThread::msleep(300);
			unitid = unit.id;
		}
	}

	if (sbuttonStr.empty() && row > 0)
	{
		return gamedevice.worker->press(row, dialogid, unitid);
	}

	if (button == sa::kButtonNone)
	{
		sa::dialog_t dialog = gamedevice.worker->currentDialog.get();
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

			for (long long i = 0; i < textList.size(); ++i)
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

	unitid += ext;

	if (button != sa::kButtonNone)
		return gamedevice.worker->press(button, dialogid, unitid);
	else if (row != -1)
		return gamedevice.worker->press(row, dialogid, unitid);
	else if (!text.isEmpty() && text.startsWith("#"))
	{
		text = text.mid(1);
		return gamedevice.worker->inputtext(text, dialogid, unitid);
	}

	return FALSE;
}

long long CLuaSystem::input(const std::string& str, long long unitid, long long dialogid, sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	QString text = util::toQString(str);

	return gamedevice.worker->inputtext(text, dialogid, unitid);
}

long long CLuaSystem::leftclick(long long x, long long y, sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	gamedevice.leftClick(x, y);
	return TRUE;
}

long long CLuaSystem::rightclick(long long x, long long y, sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	gamedevice.rightClick(x, y);
	return TRUE;
}

long long CLuaSystem::leftdoubleclick(long long x, long long y, sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	gamedevice.leftDoubleClick(x, y);
	return TRUE;
}

long long CLuaSystem::mousedragto(long long x1, long long y1, long long x2, long long y2, sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	gamedevice.dragto(x1, y1, x2, y2);
	return TRUE;
}

long long CLuaSystem::menu(long long index, sol::object otype, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	--index;
	if (index < 0)
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("index must above 0"));
		return FALSE;
	}

	long long type = 1;
	if (otype.is<long long>())
		type = otype.as<long long>();

	--type;
	if (type < 0 || type > 1)
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("type must be 1 or 2"));
		return FALSE;
	}

	if (type == 0)
	{
		return gamedevice.worker->saMenu(index);
	}

	return gamedevice.worker->shopOk(index);
}

long long CLuaSystem::createch(sol::object odataplacenum
	, std::string scharname
	, long long imgno
	, long long faceimgno
	, long long vit
	, long long str
	, long long tgh
	, long long dex
	, long long earth
	, long long water
	, long long fire
	, long long wind
	, sol::object ohometown, sol::object oforcecover, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return FALSE;

	long long dataplacenum = 0;
	if (odataplacenum.is<std::string>())
	{
		QString dataplace = util::toQString(odataplacenum.as<std::string>());
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
	else if (odataplacenum.is<long long>())
	{
		dataplacenum = odataplacenum.as<long long>();
		if (dataplacenum != 1 && dataplacenum != 2)
		{
			luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("Invalid value of 'dataplacenum'"));
			return FALSE;
		}
		--dataplacenum;
	}

	QString charname = util::toQString(scharname);
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

	long long hometown = 1;
	if (ohometown.is<std::string>())
	{
		static const QHash<QString, long long> hash = {
			{ "薩姆吉爾",0 }, { "瑪麗娜絲", 1 }, { "加加", 2 }, { "卡魯它那", 3 },
			{ "萨姆吉尔",0 }, { "玛丽娜丝", 1 }, { "加加", 2 }, { "卡鲁它那", 3 },

			{ "薩姆吉爾村",0 }, { "瑪麗娜絲村", 1 }, { "加加村", 2 }, { "卡魯它那村", 3 },
			{ "萨姆吉尔村",0 }, { "玛丽娜丝村", 1 }, { "加加村", 2 }, { "卡鲁它那村", 3 },
		};

		QString hometownstr = util::toQString(ohometown.as<std::string>());
		if (hometownstr.isEmpty())
		{
			luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("hometown cannot be empty"));
			return FALSE;
		}

		hometown = hash.value(hometownstr, -1);
		if (hometown == -1)
			hometown = 1;
	}
	else if (ohometown.is<long long>())
	{
		hometown = ohometown.as<long long>();
		if (hometown <= 0 || hometown > 4)
			hometown = 1;
		else
			--hometown;
	}

	bool forcecover = false;
	if (oforcecover.is<bool>())
		forcecover = oforcecover.as<bool>();
	else if (oforcecover.is<long long>())
		forcecover = oforcecover.as<long long>() > 0;

	return gamedevice.worker->createCharacter(static_cast<long long>(dataplacenum)
		, charname
		, static_cast<long long>(imgno)
		, static_cast<long long>(faceimgno)
		, static_cast<long long>(vit)
		, static_cast<long long>(str)
		, static_cast<long long>(tgh)
		, static_cast<long long>(dex)
		, static_cast<long long>(earth)
		, static_cast<long long>(water)
		, static_cast<long long>(fire)
		, static_cast<long long>(wind)
		, static_cast<long long>(hometown)
		, forcecover);
}

long long CLuaSystem::delch(long long index, std::string spsw, sol::object option, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return FALSE;

	--index;
	if (index < 0 || index > sa::MAX_CHARACTER)
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("index must between 1 and %1").arg(sa::MAX_CHARACTER));
		return FALSE;
	}

	QString password = util::toQString(spsw);
	if (password.isEmpty())
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("password cannot be empty"));
		return FALSE;
	}

	bool backtofirst = false;
	if (option.is<bool>())
		backtofirst = option.as<bool>();
	else if (option.is<long long>())
		backtofirst = option.as<long long>() > 0;

	return gamedevice.worker->deleteCharacter(index, password, backtofirst);
}

long long CLuaSystem::send(long long funId, sol::variadic_args args, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return FALSE;

	qDebug() << "Received " << args.size() << " arguments:";

	std::vector<std::variant<long long, std::string>> vargs;
	for (const auto arg : args)
	{
		if (arg.is<long long>() || arg.get_type() == sol::type::number)
		{
			vargs.emplace_back(static_cast<int>(arg.as<long long>()));
		}
		else if (arg.is<std::string>() || arg.get_type() == sol::type::string)
		{
			QString str = util::toQString(arg.as<std::string>());
			std::string encodedStr = util::fromUnicode(str);
			vargs.emplace_back(encodedStr);
		}
	}

	return gamedevice.autil.util_SendArgs(funId, vargs);
}

long long CLuaSystem::chname(sol::object oname, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	QString name;
	if (oname.is<long long>())
		name = util::toQString(oname.as<long long>());
	if (oname.is<double>())
		name = util::toQString(oname.as<double>());
	else if (oname.is<std::string>())
		name = util::toQString(oname);
	else
		return FALSE;

	long long size = util::fromUnicode(name).size();
	if (size > sa::CHAR_FREENAME_LEN)
	{
		luadebug::showErrorMsg(s, luadebug::WARN_LEVEL, QObject::tr("name length must below or equal %1 bytes, but got %2 bytes").arg(sa::CHAR_FREENAME_LEN).arg(size));
		return FALSE;
	}

	return gamedevice.worker->setCharFreeName(name);
}

long long CLuaSystem::chpetname(long long index, sol::object oname, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	--index;
	if (index < 0 || index > sa::MAX_PET)
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("index must between 1 and %1").arg(sa::MAX_PET));
		return FALSE;
	}

	QString name;
	if (oname.is<long long>())
		name = util::toQString(oname.as<long long>());
	if (oname.is<double>())
		name = util::toQString(oname.as<double>());
	else if (oname.is<std::string>())
		name = util::toQString(oname);
	else
		return FALSE;

	long long size = util::fromUnicode(name).size();
	if (size > sa::PET_FREENAME_LEN)
	{
		luadebug::showErrorMsg(s, luadebug::WARN_LEVEL, QObject::tr("name length must below or equal %1 bytes, but got %2 bytes").arg(sa::PET_FREENAME_LEN).arg(size));
		return FALSE;
	}

	return gamedevice.worker->setPetFreeName(index, name);
}

long long CLuaSystem::chpet(long long petindex, sol::object ostate, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	petindex -= 1;
	if (petindex < 0 || petindex >= sa::MAX_PET)
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("petindex must between 1 and %1").arg(sa::MAX_PET));
		return FALSE;
	}

	QString stateStr;
	if (ostate.is<std::string>())
		stateStr = util::toQString(ostate);
	if (stateStr.isEmpty())
		stateStr = QString("rest");

	sa::PetState state = sa::petStateMap.value(stateStr.toLower(), sa::PetState::kRest);

	sa::pet_t pet = gamedevice.worker->getPet(petindex);
	if (pet.state == state)
		return TRUE;

	return gamedevice.worker->setPetState(petindex, state);
}

bool CLuaSystem::waitpos(sol::object p1, sol::object p2, sol::object p3, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	QString posStrs;
	QStringList posStrList;
	QList<QPoint> posList;
	long long x = -1, y = -1;

	long long timeoutIndex = 3;
	long long jumpIndex = 4;

	if (p1.is<long long>())
	{
		if (!p2.is<long long>())
		{
			luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("y cannot be empty"));
			return FALSE;
		}

		x = p1.as<long long>();
		y = p2.as<long long>();

		posList.push_back(QPoint(x, y));
	}
	else if (p1.is<std::string>())
	{
		posStrs = util::toQString(p1.as<std::string>()).simplified();

		if (posStrs.isEmpty())
		{
			luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("pos cannot be empty"));
			return FALSE;
		}

		posStrList = posStrs.split(util::rexOR, Qt::SkipEmptyParts);
		if (posStrList.isEmpty())
		{
			luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("pos cannot be empty"));
			return FALSE;
		}

		for (const QString& posStr : posStrList)
		{
			QStringList pos = posStr.split(util::rexComma, Qt::SkipEmptyParts);
			if (pos.size() != 2)
				continue;

			bool ok1, ok2;
			long long px = pos.value(0).toLongLong(&ok1);
			long long py = pos.value(1).toLongLong(&ok2);
			if (ok1 && ok2)
				posList.push_back(QPoint(px, py));
		}

		if (posList.isEmpty())
		{
			//no valid pos
			luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("no valid pos"));
			return FALSE;
		}

		timeoutIndex = 2;
		jumpIndex = 3;
	}
	else
		return FALSE;

	auto check = [&gamedevice, posList]()
		{
			QPoint pos = gamedevice.worker->getPoint();
			for (const QPoint& p : posList)
			{
				if (p == pos)
					return true;
			}
			return false;
		};

	bool bret = false;
	long long timeout = 5000;
	if (p3.is<long long>() && x != -1 && y != -1)
		timeout = p3.as<long long>();
	else if (p2.is<long long>() && p1.is<std::string>())
		timeout = p2.as<long long>();

	bret = luadebug::waitfor(s, timeout, [&check]()->bool
		{
			return check();
		});

	return bret;
}

bool CLuaSystem::waitmap(sol::object p1, sol::object otimeout, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	util::timer timer;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	QString mapname = "";
	long long floor = 0;
	if (p1.is<std::string>())
		mapname = util::toQString(p1.as<std::string>()).simplified();
	else if (p1.is<long long>())
		floor = p1.as<long long>();
	else
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("invalid map name or floor number"));
		return FALSE;
	}

	QStringList mapnames = mapname.split(util::rexOR, Qt::SkipEmptyParts);

	long long timeout = 5000;
	if (otimeout.is<long long>())
		timeout = otimeout.as<long long>();

	auto check = [&gamedevice, floor, mapnames]()
		{
			if (floor != 0)
				return floor == gamedevice.worker->getFloor();
			else
			{
				QString currentFloorName = gamedevice.worker->getFloorName();
				long long currentFloor = gamedevice.worker->getFloor();

				for (const QString& mapname : mapnames)
				{
					bool ok;
					long long fr = mapname.toLongLong(&ok);
					if (ok)
					{
						if (fr == currentFloor)
							return true;
					}
					else
					{
						if (mapname.startsWith("?"))
						{
							QString newName = mapname.mid(1);
							return currentFloorName.contains(newName);
						}
						else
							return mapname == currentFloorName;
					}
				}
			}

			return false;
		};

	bool bret = luadebug::waitfor(s, timeout, [&check]()->bool
		{
			return check();
		});

	if (!bret && timeout > 2000)
		gamedevice.worker->EO();

	return  bret;
}

bool CLuaSystem::waititem(sol::object oname, sol::object omemo, sol::object otimeout, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	long long min = sa::CHAR_EQUIPSLOT_COUNT;
	long long max = sa::MAX_ITEM;

	QString itemName;
	if (oname.is<std::string>())
		itemName = util::toQString(oname.as<std::string>()).simplified();

	QStringList itemNames = itemName.split(util::rexOR, Qt::SkipEmptyParts);

	QString itemMemo;
	if (omemo.is<std::string>())
		itemMemo = util::toQString(omemo.as<std::string>()).simplified();

	QStringList itemMemos = itemMemo.split(util::rexOR, Qt::SkipEmptyParts);

	if (itemName.isEmpty() && itemMemo.isEmpty())
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("item name and memo cannot be empty at the same time"));
		return FALSE;
	}

	long long timeout = 5000;
	if (otimeout.is<long long>())
		timeout = otimeout.as<long long>();

	gamedevice.worker->updateItemByMemory();

	bool bret = luadebug::waitfor(s, timeout, [itemName, &gamedevice, &itemNames, &itemMemos, min, max]()->bool
		{
			if (sa::equipMap.contains(itemName))
			{
				long long index = sa::equipMap.value(itemName);
				if (index >= 0 && index < sa::CHAR_EQUIPSLOT_COUNT)
				{
					if (gamedevice.worker->getItemIndexByName(itemName, true, "", index, index + 1))
						return true;
				}

				return false;
			}

			QVector<long long> vec;
			long long size = 0;
			if (itemNames.size() >= itemMemos.size())
				size = itemNames.size();
			else
				size = itemMemos.size();



			for (long long i = 0; i < size; ++i)
			{
				vec.clear();
				if (gamedevice.worker->getItemIndexsByName(itemNames.value(i), itemMemos.value(i), &vec, min, max) && !vec.isEmpty())
					return true;
			}

			return false;
		});

	return bret;
}

bool CLuaSystem::waitteam(sol::object otimeout, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	sa::character_t pc = gamedevice.worker->getCharacter();

	long long timeout = 5000;
	if (otimeout.is<long long>())
		timeout = otimeout.as<long long>();

	bool bret = luadebug::waitfor(s, timeout, [&pc]()->bool
		{
			return util::checkAND(pc.status, sa::kCharacterStatus_IsLeader) || util::checkAND(pc.status, sa::kCharacterStatus_HasTeam);
		});

	return bret;
}

bool CLuaSystem::waitpet(std::string name, sol::object otimeout, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	QString petName;
	petName = util::toQString(name).simplified();

	if (petName.isEmpty())
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("pet name cannot be empty"));
		return FALSE;
	}

	long long timeout = 5000;
	if (otimeout.is<long long>())
		timeout = otimeout.as<long long>();

	bool bret = luadebug::waitfor(s, timeout, [&gamedevice, petName]()->bool
		{
			QVector<long long> v;
			return gamedevice.worker->getPetIndexsByName(petName, &v);
		});

	return bret;
}

bool CLuaSystem::waitdlg(sol::object p1, sol::object otimeout, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	long long timeout = 5000;
	if (otimeout.is<long long>())
		timeout = otimeout.as<long long>();

	QString cmpStr;
	long long dlgid = -1;
	if (!p1.is<std::string>())
	{
		if (!p1.is<long long>())
		{
			luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("dialog id or string cannot be empty"));
			return FALSE;
		}
		else
		{
			dlgid = p1.as<long long>();
		}
	}
	else
	{
		cmpStr = util::toQString(p1.as<std::string>()).simplified();
	}

	bool bret = false;
	if (dlgid != -1)
	{
		bret = luadebug::waitfor(s, timeout, [&gamedevice, dlgid]()->bool
			{
				if (!gamedevice.worker->isDialogVisible())
					return false;

				return gamedevice.worker->currentDialog.get().dialogid == dlgid;
			});

		return bret;
	}
	else
	{
		QStringList cmpStrs = cmpStr.split(util::rexOR, Qt::SkipEmptyParts);

		long long min = 0;
		long long max = sa::MAX_DIALOG_LINE;
		auto check = [&gamedevice, min, max, cmpStrs]()->bool
			{
				if (!gamedevice.worker->isDialogVisible())
					return false;

				if (cmpStrs.isEmpty() || cmpStrs.front().isEmpty())
					return true;

				QStringList dialogStrList = gamedevice.worker->currentDialog.get().linedatas;
				for (long long i = min; i < max; ++i)
				{
					if (i < 0 || i > dialogStrList.size())
						break;

					QString text = dialogStrList.value(i).simplified();
					if (text.isEmpty())
						continue;

					for (const QString& cmpStr : cmpStrs)
					{
						if (text.contains(cmpStr.simplified(), Qt::CaseInsensitive))
						{
							return true;
						}
					}
				}

				return false;
			};

		bret = luadebug::waitfor(s, timeout, [&check]()->bool
			{
				return check();
			});

		return bret;
	}
}

bool CLuaSystem::waitsay(std::string sstr, sol::object otimeout, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	QString cmpStr = util::toQString(sstr).simplified();
	if (cmpStr.isEmpty())
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("string cannot be empty"));
		return FALSE;
	}

	QStringList cmpStrs = cmpStr.split(util::rexOR, Qt::SkipEmptyParts);

	long long timeout = 5000;
	if (otimeout.is<long long>())
		timeout = otimeout.as<long long>();

	auto check = [&gamedevice, cmpStrs]()->bool
		{
			for (long long i = 0; i < sa::MAX_CHAT_HISTORY; ++i)
			{
				QString text = gamedevice.worker->getChatHistory(i);
				if (text.isEmpty())
					continue;

				for (const QString& cmpStr : cmpStrs)
				{
					if (text.contains(cmpStr.simplified(), Qt::CaseInsensitive))
					{
						return true;
					}
				}
			}

			return false;
		};

	bool bret = luadebug::waitfor(s, timeout, [&check]()->bool
		{
			return check();
		});

	return bret;
}

long long CLuaSystem::set(std::string enumStr,
	sol::object p1, sol::object p2, sol::object p3, sol::object p4, sol::object p5, sol::object p6, sol::object p7, sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(lua["__INDEX"].get<long long>());

	QString typeStr = util::toQString(enumStr);
	if (typeStr.isEmpty())
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("type cannot be empty"));
		return FALSE;
	}

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
			{ "標題", util::kTitleFormatString },
			{ "我方格式", util::kBattleAllieFormatString },
			{ "敵方格式", util::kBattleEnemyFormatString },
			{ "自己格式", util::kBattleSelfMarkString },
			{ "出手格式", util::kBattleActMarkString },
			{ "補位格式", util::kBattleSpaceMarkString },
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
			{ "自動腳本", util::kAutoStartScriptEnable },
			{ "隱藏石器", util::kHideWindowEnable },
			{ "屏蔽聲音", util::kMuteEnable },

			{ "自動重啟", util::kAutoRestartGameEnable },
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
			{ "lua戰鬥託管", util::kBattleLuaModeEnable },

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
			{ "組隊白名單", util::kGroupWhiteListEnable },
			{ "組隊黑名單", util::kGroupWhiteListEnable },
			{ "組隊超時", util::kGroupAutoKickEnable },

			{ "道具補氣", util::kNormalItemHealMpEnable },//{ "ItemHealMpNormalValue", util::kNormalItemHealMpValue },{ "平時道具補氣", util::kNormalItemHealMpItemString },
			{ "戰鬥道具補氣", util::kBattleItemHealMpEnable },//{ "戰鬥道具補氣人物", util::kBattleItemHealMpValue },{ "戰鬥道具補氣 ", util::kBattleItemHealMpItemString },
			{ "戰鬥嗜血補氣", util::kBattleSkillMpEnable },//{ "戰鬥嗜血補氣技能", util::kBattleSkillMpSkillValue },{ "戰鬥嗜血補氣百分比", util::kBattleSkillMpValue },

			/*{"MagicHealCharNormalValue", util::kNormalMagicHealCharValue},
			{ "MagicHealPetNormalValue", util::kNormalMagicHealPetValue },
			{ "MagicHealAllieNormalValue", util::kNormalMagicHealAllieValue },*/
			{ "精靈補血", util::kNormalMagicHealEnable },//{ "平時精靈補血精靈索引", util::kNormalMagicHealMagicValue },
			{ "精靈補血道具", util::kNormalMagicHealItemString },
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
			{ "标题", util::kTitleFormatString },
			{ "我方格式", util::kBattleAllieFormatString },
			{ "敌方格式", util::kBattleEnemyFormatString },
			{ "自己格式", util::kBattleSelfMarkString },
			{ "出手格式", util::kBattleActMarkString },
			{ "补位格式", util::kBattleSpaceMarkString },
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
			{ "自动脚本", util::kAutoStartScriptEnable },
			{ "隐藏石器", util::kHideWindowEnable },
			{ "屏蔽声音", util::kMuteEnable },

			{ "自动重启", util::kAutoRestartGameEnable },
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
			{ "lua战斗託管", util::kBattleLuaModeEnable },

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
			{ "组队白名单", util::kGroupWhiteListEnable },
			{ "组队黑名单", util::kGroupWhiteListEnable },
			{ "组队超时", util::kGroupAutoKickEnable },

			{ "道具补气", util::kNormalItemHealMpEnable },//{ "ItemHealMpNormalValue", util::kNormalItemHealMpValue },{ "平时道具补气", util::kNormalItemHealMpItemString },
			{ "战斗道具补气", util::kBattleItemHealMpEnable },//{ "战斗道具补气人物", util::kBattleItemHealMpValue },{ "战斗道具补气 ", util::kBattleItemHealMpItemString },
			{ "战斗嗜血补气", util::kBattleSkillMpEnable },//{ "战斗嗜血补气技能", util::kBattleSkillMpSkillValue },{ "战斗嗜血补气百分比", util::kBattleSkillMpValue },

			/*{"MagicHealCharNormalValue", util::kNormalMagicHealCharValue},
			{ "MagicHealPetNormalValue", util::kNormalMagicHealPetValue },
			{ "MagicHealAllieNormalValue", util::kNormalMagicHealAllieValue },*/
			{ "精灵补血", util::kNormalMagicHealEnable },//{ "平时精灵补血精灵索引", util::kNormalMagicHealMagicValue },
			{ "精灵补血道具", util::kNormalMagicHealItemString },
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
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("unknown setting type: '%1'").arg(typeStr));
		return FALSE;
	}

	if (type == util::kScriptDebugModeEnable)
	{
		long long value1 = 0;
		if (p1.is<long long>())
			value1 = p1.as<long long>();
		else if (p1.is<bool>())
			value1 = p1.as<bool>() ? 1 : 0;

		gamedevice.setEnableHash(util::kScriptDebugModeEnable, value1 > 0);
		emit signalDispatcher.applyHashSettingsToUI();
		return TRUE;
	}

	if (type == util::kBattleCharNormalActionTypeValue)
	{
		long long value1 = 0;
		if (p1.is<long long>())
			value1 = p1.as<long long>();
		else if (p1.is<bool>())
			value1 = p1.as<bool>() ? 1 : 0;

		--value1;
		if (value1 < 0)
			value1 = 0;

		long long value2 = 0;
		if (p2.is<long long>())
			value2 = p2.as<long long>();
		else if (p2.is<bool>())
			value2 = p2.as<bool>() ? 1 : 0;
		--value2;
		if (value2 < 0)
			value2 = 0;

		long long value3 = 0;
		if (p3.is<long long>())
			value3 = p3.as<long long>();
		else if (p2.is<bool>())
			value3 = p3.as<bool>() ? 1 : 0;
		--value3;
		if (value3 < 0)
			value3 = 0;

		gamedevice.setValueHash(util::kBattleCharNormalActionTypeValue, value1);
		gamedevice.setValueHash(util::kBattleCharNormalActionEnemyValue, value2);
		gamedevice.setValueHash(util::kBattleCharNormalActionLevelValue, value3);
		//gamedevice.setValueHash(util::kBattleCharNormalActionTargetValue, value4);

		emit signalDispatcher.applyHashSettingsToUI();
		return TRUE;
	}
	else if (type == util::kBattlePetNormalActionTypeValue)
	{
		long long value1 = 0;
		if (p1.is<long long>())
			value1 = p1.as<long long>();
		else if (p1.is<bool>())
			value1 = p1.as<bool>() ? 1 : 0;

		--value1;
		if (value1 < 0)
			value1 = 0;

		long long value2 = 0;
		if (p2.is<long long>())
			value2 = p2.as<long long>();
		else if (p2.is<bool>())
			value2 = p2.as<bool>() ? 1 : 0;
		--value2;
		if (value2 < 0)
			value2 = 0;

		long long value3 = 0;
		if (p3.is<long long>())
			value3 = p3.as<long long>();
		else if (p3.is<bool>())
			value3 = p3.as<bool>() ? 1 : 0;
		--value3;
		if (value3 < 0)
			value3 = 0;

		gamedevice.setValueHash(util::kBattlePetNormalActionTypeValue, value1);
		gamedevice.setValueHash(util::kBattlePetNormalActionEnemyValue, value2);
		gamedevice.setValueHash(util::kBattlePetNormalActionLevelValue, value3);
		//gamedevice.setValueHash(util::kBattlePetNormalActionTargetValue, value4);

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
		if (!p1.is<long long>() && !p1.is<bool>())
		{
			luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("the first parameter of the setting '%1' must be a number or boolean").arg(typeStr));
			return FALSE;
		}

		long long value1 = 0;
		if (p1.is<long long>())
			value1 = p1.as<long long>();
		else if (p1.is<bool>())
			value1 = p1.as<bool>() ? 1 : 0;

		--value1;
		if (value1 < 0)
			value1 = 0;

		gamedevice.setValueHash(type, value1);
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
		if (!p1.is<long long>() && !p1.is<bool>())
		{
			luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("the first parameter of the setting '%1' must be a number or boolean").arg(typeStr));
			return FALSE;
		}

		long long value1 = 0;
		if (p1.is<long long>())
			value1 = p1.as<long long>();
		else if (p1.is<bool>())
			value1 = p1.as<bool>() ? 1 : 0;

		if (value1 < 0)
			value1 = 0;

		gamedevice.setValueHash(type, value1);
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
	case util::kAutoStartScriptEnable:
	case util::kHideWindowEnable:
	case util::kMuteEnable:

	case util::kAutoRestartGameEnable:
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
	case util::kBattleLuaModeEnable:

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
		if (!p1.is<long long>() && !p1.is<bool>())
		{
			luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("the first parameter of the setting '%1' must be a number or boolean").arg(typeStr));
			return FALSE;
		}

		long long value1 = 0;
		if (p1.is<long long>())
			value1 = p1.as<long long>();
		else if (p1.is<bool>())
			value1 = p1.as<bool>() ? 1 : 0;

		bool ok = value1 > 0;
		if (value1 < 0)
			value1 = 0;

		gamedevice.setEnableHash(type, ok);
		if (type == util::kFastBattleEnable && ok)
		{
			gamedevice.setEnableHash(util::kAutoBattleEnable, !ok);
			if (!gamedevice.worker.isNull())
			{
				gamedevice.worker->asyncBattleAction(false);
				if (gamedevice.worker->getWorldStatus() == 10)// 強退戰鬥畫面
					gamedevice.worker->setGameStatus(7);
			}
		}
		else if (type == util::kAutoBattleEnable && ok)
		{
			gamedevice.setEnableHash(util::kFastBattleEnable, !ok);
			if (!gamedevice.worker.isNull())
				gamedevice.worker->asyncBattleAction(false);
		}
		else if (type == util::kAutoWalkEnable && ok)
		{
			gamedevice.setEnableHash(util::kFastAutoWalkEnable, !ok);
			gamedevice.setEnableHash(util::kAutoJoinEnable, !ok);
		}
		else if (type == util::kFastAutoWalkEnable && ok)
		{
			gamedevice.setEnableHash(util::kAutoWalkEnable, !ok);
			gamedevice.setEnableHash(util::kAutoJoinEnable, !ok);
		}

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
		if (!p1.is<long long>() && !p1.is<bool>())
		{
			luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("the first parameter of the setting '%1' must be a number or boolean").arg(typeStr));
			return FALSE;
		}

		long long value1 = 0;
		if (p1.is<long long>())
			value1 = p1.as<long long>();
		else if (p1.is<bool>())
			value1 = p1.as<bool>() ? 1 : 0;

		bool ok = value1 > 0;
		if (value1 < 0)
			value1 = 0;

		QString text = util::toQString(p2);

		gamedevice.setEnableHash(type, ok);
		if (type == util::kLockPetScheduleEnable && ok)
		{
			gamedevice.setEnableHash(util::kLockPetEnable, !ok);
			gamedevice.setEnableHash(util::kLockRideEnable, !ok);
			gamedevice.setStringHash(util::kLockPetScheduleString, text);
		}
		else if (type == util::kAutoJoinEnable && ok)
		{
			gamedevice.setStringHash(util::kAutoFunNameString, text);
			gamedevice.setValueHash(util::kAutoFunTypeValue, 0);

			gamedevice.setEnableHash(util::kAutoWalkEnable, false);
			gamedevice.setEnableHash(util::kFastAutoWalkEnable, false);

		}
		else if (type == util::kGroupWhiteListEnable && ok)
		{
			gamedevice.setStringHash(util::kGroupWhiteListString, text);
		}
		else if (type == util::kGroupBlackListEnable && ok)
		{
			gamedevice.setStringHash(util::kGroupBlackListString, text);
		}
		else if (type == util::kLockAttackEnable && ok)
			gamedevice.setStringHash(util::kLockAttackString, text);
		else if (type == util::kLockEscapeEnable && ok)
			gamedevice.setStringHash(util::kLockEscapeString, text);
		else if (type == util::kAutoDropEnable && ok)
			gamedevice.setStringHash(util::kAutoDropItemString, text);
		else if (type == util::kBattleCatchCharItemEnable && ok)
		{
			gamedevice.setStringHash(util::kBattleCatchCharItemString, text);
			gamedevice.setValueHash(util::kBattleCatchTargetItemHpValue, value1 + 1);
		}
		else if (type == util::kNormalItemHealMpEnable && ok)
		{
			gamedevice.setStringHash(util::kNormalItemHealMpItemString, text);
			gamedevice.setValueHash(util::kNormalItemHealMpValue, value1 + 1);
		}
		else if (type == util::kBattleItemHealMpEnable && ok)
		{
			gamedevice.setStringHash(util::kBattleItemHealMpItemString, text);
			gamedevice.setValueHash(util::kBattleItemHealMpValue, value1 + 1);
		}
		else if (type == util::kAutoAbilityEnable)
		{
			gamedevice.setStringHash(util::kAutoAbilityString, text);
		}

		emit signalDispatcher.applyHashSettingsToUI();
		return TRUE;
	}
	default:
		break;
	}

	//0:close >1:open long long value
	switch (type)
	{
	case util::kBattleSkillMpEnable:
	case util::kBattleCatchPetSkillEnable:
	case util::kBattleCatchTargetMaxHpEnable:
	case util::kBattleCatchTargetLevelEnable:
	case util::kLockTimeEnable:
	case util::kLockPetEnable:
	case util::kLockRideEnable:
	case util::kGroupAutoKickEnable:
	{
		if (!p1.is<long long>() && !p1.is<bool>())
		{
			luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("the first parameter of the setting '%1' must be a number or boolean").arg(typeStr));
			return FALSE;
		}

		long long value1 = 0;
		if (p1.is<long long>())
			value1 = p1.as<long long>();
		else if (p1.is<bool>())
			value1 = p1.as<bool>() ? 1 : 0;

		bool ok = value1 > 0;
		--value1;
		if (value1 < 0)
			value1 = 0;

		gamedevice.setEnableHash(type, ok);
		if (type == util::kLockPetEnable && ok)
		{
			gamedevice.setEnableHash(util::kLockPetScheduleEnable, !ok);
			gamedevice.setValueHash(util::kLockPetValue, value1);
		}
		else if (type == util::kLockRideEnable && ok)
		{
			gamedevice.setEnableHash(util::kLockPetScheduleEnable, !ok);
			gamedevice.setValueHash(util::kLockRideValue, value1);
		}
		else if (type == util::kLockTimeEnable && ok)
			gamedevice.setValueHash(util::kLockTimeValue, value1 + 1);
		else if (type == util::kBattleCatchTargetLevelEnable && ok)
			gamedevice.setValueHash(util::kBattleCatchTargetLevelValue, value1 + 1);
		else if (type == util::kBattleCatchTargetMaxHpEnable && ok)
			gamedevice.setValueHash(util::kBattleCatchTargetMaxHpValue, value1 + 1);
		else if (type == util::kBattleCatchPetSkillEnable && ok)
			gamedevice.setValueHash(util::kBattleCatchPetSkillValue, value1);
		else if (type == util::kBattleSkillMpEnable)
			gamedevice.setValueHash(util::kBattleSkillMpValue, value1 + 1);
		else if (type == util::kGroupAutoKickEnable)
			gamedevice.setValueHash(util::kGroupAutoKickTimeoutValue, value1 + 1);
		emit signalDispatcher.applyHashSettingsToUI();
		return TRUE;
	}
	default:
		break;
	}

	//0:close >1:open two long long value
	switch (type)
	{
	case util::kBattleCatchCharMagicEnable:
	{
		if (!p1.is<long long>() && !p1.is<bool>())
		{
			luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("the first parameter of the setting '%1' must be a number or boolean").arg(typeStr));
			return FALSE;
		}

		long long value1 = 0;
		if (p1.is<long long>())
			value1 = p1.as<long long>();
		else if (p1.is<bool>())
			value1 = p1.as<bool>() ? 1 : 0;

		bool ok = value1 > 0;
		if (value1 < 0)
			value1 = 0;

		long long value2 = 0;
		if (p2.is<long long>())
			value2 = p2.as<long long>();

		--value2;
		if (value2 < 0)
			value2 = 0;


		gamedevice.setEnableHash(type, ok);
		if (type == util::kBattleCatchCharMagicEnable && ok)
		{
			gamedevice.setValueHash(util::kBattleCatchTargetMagicHpValue, value1);
			gamedevice.setValueHash(util::kBattleCatchCharMagicValue, value2);
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
		long long value1 = p1.as<long long>();
		bool ok = value1 > 0;
		if (value1 < 0)
			value1 = 0;

		long long value2 = 0;
		if (p2.is<long long>())
			value2 = p2.as<long long>();
		else if (p2.is<bool>())
			value2 = p2.as<bool>() ? 1 : 0;

		--value2;
		if (value2 < 0)
			value2 = 0;

		long long value3 = 0;
		if (p3.is<long long>())
			value3 = p3.as<long long>();
		else if (p2.is<bool>())
			value3 = p3.as<bool>() ? 1 : 0;
		--value3;
		if (value3 < 0)
			value3 = 0;

		long long value4 = 0;
		if (p4.is<long long>())
			value4 = p4.as<long long>();
		else if (p4.is<bool>())
			value4 = p4.as<bool>() ? 1 : 0;
		--value4;
		if (value4 < 0)
			value4 = 0;

		long long value5 = 0;
		if (p5.is<long long>())
			value5 = p5.as<long long>();
		else if (p5.is<bool>())
			value5 = p5.as<bool>() ? 1 : 0;
		if (value5 < 0)
			value5 = util::kSelectEnemyAny;

		gamedevice.setValueHash(type, value1);
		if (type == util::kBattleCharRoundActionRoundValue && ok)
		{
			gamedevice.setValueHash(util::kBattleCharRoundActionTypeValue, value2);
			gamedevice.setValueHash(util::kBattleCharRoundActionEnemyValue, value3);
			gamedevice.setValueHash(util::kBattleCharRoundActionLevelValue, value4);
			//gamedevice.setValueHash(util::kBattleCharRoundActionTargetValue, value5);
		}
		else if (type == util::kBattlePetRoundActionRoundValue && ok)
		{
			gamedevice.setValueHash(util::kBattlePetRoundActionTypeValue, value2);
			gamedevice.setValueHash(util::kBattlePetRoundActionEnemyValue, value3);
			gamedevice.setValueHash(util::kBattlePetRoundActionLevelValue, value4);
			//gamedevice.setValueHash(util::kBattlePetRoundActionTargetValue, value5);
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
		if (!p1.is<long long>() && !p1.is<bool>())
		{
			luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("the first parameter of the setting '%1' must be a number or boolean").arg(typeStr));
			return FALSE;
		}

		long long value1 = 0;
		if (p1.is<long long>())
			value1 = p1.as<long long>();
		else if (p1.is<bool>())
			value1 = p1.as<bool>() ? 1 : 0;

		bool ok = value1 > 0;
		--value1;

		if (value1 < 0)
			value1 = 0;
		gamedevice.setEnableHash(type, ok);

		long long value2 = 0;
		if (p2.is<long long>())
			value2 = p2.as<long long>();
		else if (p2.is<bool>())
			value2 = p2.as<bool>() ? 1 : 0;
		--value2;
		if (value2 < 0)
			value2 = 0;

		long long value3 = 0;
		if (p3.is<long long>())
			value3 = p3.as<long long>();
		else if (p3.is<bool>())
			value3 = p3.as<bool>() ? 1 : 0;
		--value3;
		if (value3 < 0)
			value3 = 0;

		long long value4 = 0;
		if (p4.is<long long>())
			value4 = p4.as<long long>();
		else if (p4.is<bool>())
			value4 = p4.as<bool>() ? 1 : 0;
		--value4;
		if (value4 < 0)
			value4 = 0;

		long long value5 = util::kSelectSelf | util::kSelectPet;
		if (p5.is<long long>())
			value5 = p5.as<long long>();

		if (type == util::kNormalMagicHealEnable && ok)
		{
			gamedevice.setValueHash(util::kNormalMagicHealMagicValue, value1);
			gamedevice.setValueHash(util::kNormalMagicHealCharValue, value2 + 1);
			gamedevice.setValueHash(util::kNormalMagicHealPetValue, value3 + 1);
			gamedevice.setValueHash(util::kNormalMagicHealAllieValue, value4 + 1);
		}
		else if (type == util::kBattleMagicHealEnable && ok)
		{
			gamedevice.setValueHash(util::kBattleMagicHealMagicValue, value1);
			gamedevice.setValueHash(util::kBattleMagicHealCharValue, value2 + 1);
			gamedevice.setValueHash(util::kBattleMagicHealPetValue, value3 + 1);
			gamedevice.setValueHash(util::kBattleMagicHealAllieValue, value4 + 1);
			//gamedevice.setValueHash(util::kBattleMagicHealTargetValue, value5);
		}
		else if (type == util::kBattleCrossActionCharEnable && ok)
		{
			gamedevice.setValueHash(util::kBattleCharCrossActionTypeValue, value1);
			gamedevice.setValueHash(util::kBattleCharCrossActionRoundValue, value2);
			//gamedevice.setValueHash(util::kBattleCharCrossActionTargetValue, value3);
		}
		else if (type == util::kBattleCrossActionPetEnable && ok)
		{
			gamedevice.setValueHash(util::kBattlePetCrossActionTypeValue, value1);
			gamedevice.setValueHash(util::kBattlePetCrossActionRoundValue, value2);
			//gamedevice.setValueHash(util::kBattlePetCrossActionTargetValue, value3);
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
	case util::kBattleAllieFormatString:
	case util::kBattleEnemyFormatString:
	case util::kBattleSelfMarkString:
	case util::kBattleActMarkString:
	case util::kBattleSpaceMarkString:
	case util::kTitleFormatString:
	case util::kEOCommandString:
	case util::kGameAccountString:
	case util::kGamePasswordString:
	case util::kGameSecurityCodeString:
	case util::kMailWhiteListString:
	case util::kBattleCatchPetNameString:
	case util::kNormalMagicHealItemString:
	{
		QString text;
		if (!p1.is<std::string>())
		{
			if (!p1.is<long long>())
				return FALSE;

			text = util::toQString(p1.as<long long>());
		}
		else
			text = util::toQString(p1);

		gamedevice.setStringHash(type, text);
		emit signalDispatcher.applyHashSettingsToUI();
		return TRUE;
	}
	default:
		break;
	}

	return FALSE;
}