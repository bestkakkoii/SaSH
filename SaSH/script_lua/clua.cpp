/*
				GNU GENERAL PUBLIC LICENSE
				   Version 2, June 1991
COPYRIGHT (C) Bestkakkoii 2024 All Rights Reserved.
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

#include "signaldispatcher.h"
#include <gamedevice.h>
#include "../script/parser.h"


#ifdef _WIN64
#ifdef _DEBUG
#pragma comment(lib, "lua-5.4.4_x64d.lib")
#else
#pragma comment(lib, "lua-5.4.4_x64.lib")
#endif
#else
#ifdef _DEBUG
#pragma comment(lib, "lua-5.4.4d.lib")
#else
#pragma comment(lib, "lua-5.4.4.lib")
#endif
#endif

#define OPEN_HOOK

#pragma region luadebug
void __fastcall luadebug::tryPopCustomErrorMsg(const sol::this_state& s, const LUA_ERROR_TYPE element, const QVariant& p1, const QVariant& p2, const QVariant& p3, const QVariant& p4)
{
	std::ignore = p4;//reserved
	lua_State* L = s;

	switch (element)
	{
	case ERROR_FLAG_DETECT_STOP:
	case ERROR_REQUEST_STOP_FROM_USER:
	case ERROR_REQUEST_STOP_FROM_SCRIP:
	case ERROR_REQUEST_STOP_FROM_DISTRUCTOR:
	case ERROR_REQUEST_STOP_FROM_PARENT_SCRIP:
	{
		//用戶請求停止
		const QString qmsgstr(errormsg_str.value(element));
		const std::string str(util::toConstData(qmsgstr));
		luaL_error(L, str.c_str());
		break;
	}
	case ERROR_PARAM_SIZE://固定參數數量錯誤
	{
		long long topsize = lua_gettop(L);
		const QString qmsgstr(QString(errormsg_str.value(element)).arg(p1.toLongLong()).arg(topsize));
		const std::string str(util::toConstData(qmsgstr));
		luaL_argcheck(L, topsize == p1.toLongLong(), topsize, str.c_str());
		break;
	}
	case ERROR_PARAM_SIZE_NONE://無參數數量錯誤
	{
		long long topsize = lua_gettop(L);
		const QString qmsgstr(QString(errormsg_str.value(element)).arg(topsize));
		const std::string str(util::toConstData(qmsgstr));
		luaL_argcheck(L, topsize == 0, 1, str.c_str());
		break;
	}
	case ERROR_PARAM_SIZE_RANGE://範圍參數數量錯誤
	{
		long long topsize = lua_gettop(L);
		const QString qmsgstr(QString(errormsg_str.value(element)).arg(p1.toLongLong()).arg(p2.toLongLong()).arg(topsize));
		const std::string str(util::toConstData(qmsgstr));
		luaL_argcheck(L, topsize >= p1.toLongLong() && topsize <= p2.toLongLong(), 1, str.c_str());
		break;
	}
	case ERROR_PARAM_VALUE://數值錯誤 p1 第幾個參數 p2邏輯 str訊息
	{
		const QString qmsgstr(p3.toString());
		const std::string str(util::toConstData(qmsgstr));
		luaL_argcheck(L, p2.toBool(), p1.toLongLong(), str.c_str());
		break;
	}
	case ERROR_PARAM_TYPE://參數預期錯誤 p1 第幾個參數 p2邏輯 str訊息
	{
		const QString qmsgstr(p3.toString());
		const std::string str(util::toConstData(qmsgstr));
		luaL_argexpected(L, p2.toBool(), p1.toLongLong(), str.c_str());
		break;
	}
	default:
	{
		luaL_error(L, "UNKNOWN ERROR TYPE");
		break;
	}
	}
}

QString __fastcall luadebug::getErrorMsgLocatedLine(const QString& str, long long* retline)
{
	const QString cmpstr(str.simplified());

	QRegularExpressionMatch match = rexGetLine.match(cmpstr);
	QRegularExpressionMatch match2 = reGetLineEx.match(cmpstr);
	static const auto matchies = [](const QRegularExpressionMatch& m, long long* retline)->void
		{
			long long size = m.capturedTexts().size();
			if (size > 1)
			{
				for (long long i = (size - 1); i >= 1; --i)
				{
					const QString s = m.captured(i).simplified();
					if (!s.isEmpty())
					{
						bool ok = false;
						long long row = s.simplified().toLongLong(&ok);
						if (ok)
						{
							if (retline)
								*retline = row - 1;

							break;
						}
					}
				}
			}
		};

	if (match.hasMatch())
	{
		matchies(match, retline);
	}
	else if (match2.hasMatch())
	{
		matchies(match2, retline);
	}

	return cmpstr;
}

QString __fastcall luadebug::getTableVars(lua_State*& L, long long si, long long& depth)
{
	if (!L)
		return "\0";

	QPair<QString, QVariant> pair;
	long long pos_si = si > 0 ? si : (si - 1);
	QString ret;
	long long top = lua_gettop(L);
	lua_pushnil(L);
	long long empty = 1;
	QStringList varList;

	--depth;

	while (lua_next(L, pos_si) != 0)
	{
		if (empty)
			empty = 0;

		QString key;
		pair = getVars(L, -2, depth);
		if (pair.second.toString().startsWith("_"))
		{
			lua_pop(L, 1);
			continue;
		}

		if (pair.first == "(string)")
			ret += QString("%1=").arg(pair.second.toString());
		else if (pair.first == "(integer)")
		{
			pair.second = pair.second.toString();
			ret += QString("[%1]=").arg(pair.second.toString());
		}
		else
			continue;

		if (depth < 0)
		{
			ret += ("{}");
		}
		else
		{
			pair = getVars(L, -1, depth);
			if (pair.first == "(string)")
				ret += QString("[[%1]]").arg(pair.second.toString());
			else
				ret += pair.second.toString();
		}
		lua_pop(L, 1);
		varList.append(ret);
		ret.clear();
	}

	lua_settop(L, top);

	std::sort(varList.begin(), varList.end());
	ret = QString("{%1}").arg(varList.join(","));
	return ret.simplified();
}

QPair<QString, QVariant> __fastcall luadebug::getVars(lua_State*& L, long long si, long long& depth)
{
	switch (lua_type(L, si))
	{
	case LUA_TNIL:
	{
		return qMakePair(QString("(nil)"), QString("nil"));
	}
	case LUA_TBOOLEAN:
	{
		return qMakePair(QString("(boolean)"), util::toQString(lua_toboolean(L, si) > 0));
	}
	case LUA_TSTRING:
	{
		return qMakePair(QString("(string)"), util::toQString(luaL_checklstring(L, si, nullptr)));
	}
	case LUA_TNUMBER:
	{
		double d = static_cast<double>(luaL_checknumber(L, si));
		if (d == static_cast<long long>(d))
			return qMakePair(QString("(integer)"), util::toQString(static_cast<long long>(luaL_checkinteger(L, si))));
		else
			return qMakePair(QString("(number)"), util::toQString(d));

	}
	case LUA_TFUNCTION:
	{
		lua_CFunction func = lua_tocfunction(L, si);
		if (func != nullptr)
		{
			return qMakePair(QString("(C function)"), QString("0x%1").arg(util::toQString(reinterpret_cast<long long>(func), 16)));
		}
		else
		{
			return qMakePair(QString("(function)"), QString("0x%1").arg(util::toQString(reinterpret_cast<long long>(func), 16)));
		}
		break;
	}
	case LUA_TUSERDATA:
	{
		return qMakePair(QString("(user data)"), QString("0x%1").arg(util::toQString(reinterpret_cast<long long>(lua_touserdata(L, si)), 16)));
	}
	case LUA_TTABLE:
	{
		return qMakePair(QString("(table)"), getTableVars(L, si, depth));
	}
	default:
		break;
	}

	return qMakePair(QString("(nil)"), QString("nil"));
}

bool __fastcall luadebug::isInterruptionRequested(const sol::this_state& s)
{
	sol::state_view lua(s.lua_state());
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.IS_SCRIPT_INTERRUPT.get())
		return true;

	return false;
}

bool __fastcall luadebug::checkStopAndPause(const sol::this_state& s)
{
	sol::state_view lua(s.lua_state());

	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());


	gamedevice.checkScriptPause();

	bool isStop = gamedevice.IS_SCRIPT_INTERRUPT.get();
	if (isStop)
	{
		tryPopCustomErrorMsg(s, ERROR_FLAG_DETECT_STOP);
	}

	return isStop;
}

bool __fastcall luadebug::checkOnlineThenWait(const sol::this_state& s)
{
	checkStopAndPause(s);

	sol::state_view lua(s.lua_state());
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	bool bret = false;
	if (!gamedevice.worker->getOnlineFlag())
	{
		util::timer timer;
		bret = true;
		for (;;)
		{
			if (checkStopAndPause(s))
				break;

			if (!gamedevice.worker.isNull())
				break;

			if (gamedevice.worker->getOnlineFlag())
				break;

			if (timer.hasExpired(300000))
				break;

			QThread::msleep(500);
		}

		QThread::msleep(2000);
	}
	return bret;
}

bool __fastcall luadebug::checkBattleThenWait(const sol::this_state& s)
{
	checkStopAndPause(s);

	sol::state_view lua(s.lua_state());
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	bool bret = false;
	if (gamedevice.worker->getBattleFlag())
	{
		util::timer timer;
		bret = true;
		for (;;)
		{
			if (checkStopAndPause(s))
				break;

			if (!gamedevice.worker.isNull())
				break;

			if (!gamedevice.worker->getBattleFlag() && gamedevice.worker->isColloectionFinished())
				break;
			if (timer.hasExpired(180000))
				break;

			QThread::msleep(100);
		}

		QThread::msleep(500);
	}
	return bret;
}

void __fastcall luadebug::processDelay(const sol::this_state& s)
{
	sol::state_view lua(s.lua_state());
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	long long extraDelay = gamedevice.getValueHash(util::kScriptSpeedValue);
	if (extraDelay > 1000ll)
	{
		//將超過1秒的延時分段
		long long i = 0ll;
		long long size = extraDelay / 1000ll;
		for (i = 0; i < size; ++i)
		{
			if (checkStopAndPause(s))
				return;

			QThread::msleep(1000);
		}
		if (extraDelay % 1000ll > 0ll)
			QThread::msleep(extraDelay % 1000ll);
	}
	else if (extraDelay > 0ll)
	{
		QThread::msleep(extraDelay);
	}
}

//遞歸獲取每一層目錄
void __fastcall luadebug::getPackagePath(const QString base, QStringList* result)
{
	QDir dir(base);
	if (!dir.exists())
		return;
	dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
	dir.setSorting(QDir::DirsFirst);
	QFileInfoList list = dir.entryInfoList();
	for (long long i = 0; i < list.size(); ++i)
	{
		QFileInfo fileInfo = list.value(i);
		result->append(fileInfo.filePath());
		getPackagePath(fileInfo.filePath(), result);
	}
}

void __fastcall luadebug::logExport(const sol::this_state& s, const QStringList& datas, long long color, bool doNotAnnounce)
{
	for (const QString& data : datas)
	{
		logExport(s, data, color, doNotAnnounce);
	}
}

void __fastcall luadebug::logExport(const sol::this_state& s, const QString& data, long long color, bool doNotAnnounce)
{

	//打印當前時間
	const QDateTime time(QDateTime::currentDateTime());
	const QString timeStr(time.toString("hh:mm:ss:zzz"));
	QString msg = "\0";
	QString src = "\0";
	sol::state_view lua(s);

	long long currentline = -1;
	if (lua["LINE"].valid() && lua["LINE"].is<long long>())
		currentline = lua["LINE"].get<long long>();
	else
		currentline = getCurrentLine(s);

	QString newData = data;
	if (lua["__HOOKFORBATTLE"].valid() && lua["__HOOKFORBATTLE"].is<bool>() && lua["__HOOKFORBATTLE"].get<bool>())
	{
		const QString prefix = "[" + QObject::tr("battle") + "]";
		if (newData.contains(prefix))
			newData.prepend(prefix);
	}

	msg = (QString("[%1 | @%2]: %3\0") \
		.arg(timeStr)
		.arg(currentline, 3, 10, QLatin1Char(' ')).arg(newData));

	long long currentIndex = lua["__INDEX"].get<long long>();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	emit signalDispatcher.appendScriptLog(msg, color);
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (!gamedevice.worker.isNull() && !doNotAnnounce)
	{
		gamedevice.worker->announce(newData, color);
	}

	if (gamedevice.log.isOpen())
		gamedevice.log.write(newData, currentline);
}

void __fastcall luadebug::showErrorMsg(const sol::this_state& s, long long level, const QString& data)
{
	QString newText = QString("%1%2").arg(level == 0 ? QObject::tr("[warn]") : QObject::tr("[error]")).arg(data);
	logExport(s, newText, 0, true);
}

//根據傳入function的循環執行結果等待超時或條件滿足提早結束
bool __fastcall luadebug::waitfor(const sol::this_state& s, long long timeout, std::function<bool()> exprfun)
{
	if (timeout < 0)
		timeout = std::numeric_limits<long long>::max();
	else if (timeout == 0)
	{
		return exprfun();
	}

	sol::state_view lua(s.lua_state());
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	bool bret = false;
	util::timer timer;
	for (;;)
	{
		if (checkStopAndPause(s))
			break;

		if (timer.hasExpired(timeout))
			break;

		if (gamedevice.worker.isNull())
			break;

		if (exprfun())
		{
			bret = true;
			break;
		}

		QThread::msleep(200);
	}
	return bret;
}

//lua函數鉤子 這裡主要用於控制 暫停、終止腳本、獲取棧數據、變量數據...或其他操作
void luadebug::hookProc(lua_State* L, lua_Debug* ar)
{
	if (nullptr == L)
	{
		return;
	}

	if (nullptr == ar)
	{
		return;
	}

	sol::this_state s = L;
	sol::state_view lua(L);

	switch (ar->event)
	{
	case LUA_HOOKLINE:
	{
		long long currentLine = ar->currentline;
		lua["LINE"] = currentLine;
		Q_FALLTHROUGH();
	}
	case LUA_HOOKCALL: //函數入口
	case LUA_MASKCOUNT: //每 n 個函數執行一次
	case LUA_HOOKRET: //函數返回
	{
		if (lua["__HOOKFORBATTLE"].is<bool>() && lua["__HOOKFORBATTLE"].get<bool>())
		{
			GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
			if (gamedevice.worker.isNull() || !gamedevice.worker->getOnlineFlag() || !gamedevice.worker->getBattleFlag())
			{
				tryPopCustomErrorMsg(s, ERROR_FLAG_DETECT_STOP);
				break;
			}

			break;
		}

		if (lua["__HOOKFORSTOP"].is<bool>() && lua["__HOOKFORSTOP"].get<bool>())
		{
			luadebug::checkStopAndPause(s);
			break;
		}

		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(lua["__INDEX"].get<long long>());
		long long max = lua["ROWCOUNT"];

		GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());

		long long currentLine = ar->currentline;

		emit signalDispatcher.scriptLabelRowTextChanged(currentLine, max, false);

		//processDelay(s);
		if (!gamedevice.IS_SCRIPT_DEBUG_ENABLE.get())
		{
			luadebug::checkStopAndPause(s);
			return;
		}

		luadebug::checkStopAndPause(s);

		CLua* pLua = lua["__THIS_CLUA"].get<CLua*>();
		if (pLua == nullptr)
		{
			return;
		}

		QString scriptFileName = gamedevice.currentScriptFileName.get();

		safe::hash<long long, break_marker_t> breakMarkers = gamedevice.break_markers.value(scriptFileName);
		const safe::hash<long long, break_marker_t> stepMarkers = gamedevice.step_markers.value(scriptFileName);
		if (!(breakMarkers.contains(currentLine) || stepMarkers.contains(currentLine)))
		{
			return;
		}

		gamedevice.paused();

		if (breakMarkers.contains(currentLine))
		{
			//叫用次數+1
			break_marker_t mark = breakMarkers.value(currentLine);
			++mark.count;

			//重新插入斷下的紀錄
			breakMarkers.insert(currentLine, mark);
			gamedevice.break_markers.insert(scriptFileName, breakMarkers);
			//所有行插入隱式斷點(用於單步)
			emit signalDispatcher.addStepMarker(currentLine, true);
		}

		emit signalDispatcher.addForwardMarker(currentLine, true);

		//獲取區域變量數值
		long long i;
		const char* name = nullptr;
		QVariantHash varhash;
		for (i = 1; (name = lua_getlocal(L, ar, i)) != NULL; ++i)
		{
			long long depth = 1024;
			QPair<QString, QVariant> vs = getVars(L, i, depth);

			QString key = QString("local|%1").arg(util::toQString(name));
			varhash.insert(key, vs.second.toString());
			lua_pop(L, 1);// no match, then pop out the var's value
		}

		Parser parser(lua["__INDEX"].get<long long>());

		emit signalDispatcher.varInfoImported(&parser, varhash, QStringList{});

		luadebug::checkStopAndPause(s);

		break;
	}
	default:
		break;
	}
}
#pragma endregion

#pragma region luatool
static std::vector<std::string> Unique(const std::vector<std::string>& v)
{
	std::vector<std::string> result = v;
#if _MSVC_LANG > 201703L
	std::ranges::stable_sort(result, std::less<std::string>());
#else
	std::sort(result.begin(), result.end(), std::less<std::string>());
#endif
	result.erase(std::unique(result.begin(), result.end()), result.end());
	return result;
}

static std::vector<long long> Unique(const std::vector<long long>& v)
{
	std::vector<long long> result = v;
#if _MSVC_LANG > 201703L
	std::ranges::stable_sort(result, std::less<long long>());
#else
	std::sort(result.begin(), result.end(), std::less<long long>());
#endif
	result.erase(std::unique(result.begin(), result.end()), result.end());
	return result;
}

template<typename T>
std::vector<T> ShiftLeft(const std::vector<T>& v, long long i)
{
	std::vector<T> result = v;
	std::rotate(result.begin(), result.begin() + i, result.end());
	return result;

}

template<typename T>
std::vector<T> ShiftRight(const std::vector<T>& v, long long i)
{
	std::vector<T> result = v;
	std::rotate(result.begin(), result.end() - i, result.end());
	return result;
}

template<typename T>
std::vector<T> Shuffle(const std::vector<T>& v)
{
	std::vector<T> result = v;
	std::random_device rd;
	std::mt19937_64 gen(rd());
	std::shuffle(result.begin(), result.end(), gen);
	return result;
}

template<typename T>
std::vector<T> Rotate(const std::vector<T>& v, long long len)//true = right, false = left
{
	std::vector<T> result = v;
	if (len >= 0)
#if _MSVC_LANG > 201703L
		std::ranges::rotate(result, result.begin() + len);
#else
		std::rotate(result.begin(), result.begin() + len, result.end());
#endif
	else
#if _MSVC_LANG > 201703L
		std::ranges::rotate(result, result.end() + len);
#else
		std::rotate(result.begin(), result.end() + len, result.end());
#endif
	return result;
}


bool luatool::checkRange(sol::object o, long long& min, long long& max, QVector<long long>* pindexs)
{
	if (o == sol::lua_nil)
		return true;

	if (o.is<std::string>())
	{
		QString str = util::toQString(o.as<std::string>());
		if (str.isEmpty() || str == "?")
		{
			return true;
		}

		bool ok = false;
		long long tmp = str.toLongLong(&ok) - 1;
		if (ok)
		{
			if (tmp < 0)
				return false;

			min = tmp;
			max = tmp;
			return true;
		}

		QStringList range = str.split("-", Qt::SkipEmptyParts);
		if (range.isEmpty())
		{
			return true;
		}

		if (range.size() == 2)
		{
			bool ok1, ok2;
			long long tmp1 = range.value(0).toLongLong(&ok1);
			long long tmp2 = range.value(1).toLongLong(&ok2);
			if (ok1 && ok2)
			{
				if (tmp1 < 0 || tmp2 < 0)
					return false;

				min = tmp1 - 1;
				max = tmp2 - 1;
				return true;
			}
		}
		else
		{
			if (pindexs != nullptr)
			{
				for (const QString& str : range)
				{
					bool ok;
					long long tmp = str.toLongLong(&ok) - 1;
					if (ok && tmp >= 0)
						pindexs->append(tmp);
				}

				return pindexs->size() > 0;
			}
		}
	}
	else if (o.is<long long>())
	{
		long long tmp = o.as<long long>() - 1;
		if (tmp < 0)
			return false;

		min = tmp;
		max = tmp;
		return true;
	}
	return false;
}

sol::object luatool::getVar(sol::state_view& lua, const std::string& varName)
{
	// Accessing local variables
	lua_Debug ar;
	int index = 1; // Stack level
	lua_State* L = lua.lua_state();

	while (lua_getstack(L, index, &ar))
	{
		int i = 1;
		const char* name;
		while ((name = lua_getlocal(L, &ar, i)) != nullptr)
		{
			if (varName == name)
			{
				sol::stack_object value(L, -1); // Get the value from the stack
				lua_pop(L, 1); // Pop the value
				return value;
			}

			lua_pop(L, 1); // Pop the variable name
			++i;
		}
		++index;
	}

	// If not found in local, try global
	return lua[varName];
}

std::string luatool::format(std::string sformat, sol::this_state s)
{
	sol::state_view lua(s);
	QString formatStr = util::toQString(sformat);
	if (formatStr.isEmpty())
		return sformat;

	static const QRegularExpression rexFormat(R"(\{\s*(?:([CT]?))\s*:\s*([^}]+)\s*\})");
	if (!formatStr.contains(rexFormat))
		return sformat;

	enum FormatType
	{
		kNothing,
		kTime,
		kConst,
	};

	constexpr long long MAX_FORMAT_DEPTH = 10;

	for (long long i = 0; i < MAX_FORMAT_DEPTH; ++i)
	{
		QRegularExpressionMatchIterator matchIter = rexFormat.globalMatch(formatStr);
		//Group 1: T or C or nothing
		//Group 2: var or var table

		QMap<QString, QPair<FormatType, QString>> formatVarList;

		for (;;)
		{
			if (!matchIter.hasNext())
				break;

			const QRegularExpressionMatch match = matchIter.next();
			if (!match.hasMatch())
				break;

			QString type = match.captured(1);
			QString varStr = match.captured(2);
			//QVariant varValue;

			if (type == "T")
			{
				sol::object obj = getVar(lua, util::toConstData(varStr));

				if (obj != sol::lua_nil && (obj.is<long long>() || obj.is<double>()))
				{
					long long sec = 0;
					if (obj.is<long long>())
						sec = obj.as<long long>();
					else if (obj.is<double>())
						sec = static_cast<long long>(obj.as<double>());
					else
						continue;

					formatVarList.insert(varStr, qMakePair(FormatType::kTime, util::formatSeconds(obj.as<long long>())));
				}
				else
					formatVarList.insert(varStr, qMakePair(FormatType::kNothing, QString()));
			}
			else if (type == "C")
			{
				QString str;
				if (varStr.toLower() == "date")
				{
					const QDateTime dt = QDateTime::currentDateTime();
					str = dt.toString("yyyy-MM-dd");

				}
				else if (varStr.toLower() == "time")
				{
					const QDateTime dt = QDateTime::currentDateTime();
					str = dt.toString("hh:mm:ss:zzz");
				}

				formatVarList.insert(varStr, qMakePair(FormatType::kConst, str));
			}
			else
			{
				sol::object obj = getVar(lua, util::toConstData(varStr));
				if (obj != sol::lua_nil)
				{
					QString str;
					if (obj.is<long long>())
						str = QString::number(obj.as<long long>());
					else if (obj.is<std::string>())
						str = util::toQString(obj.as<std::string>());
					else if (obj.is<bool>())
						str = util::toQString(obj.as<bool>());
					else if (obj.is<double>())
						str = QString::number(obj.as<double>());
					else
						str = util::toQString(obj.as<std::string>());
					formatVarList.insert(varStr, qMakePair(FormatType::kNothing, str));
				}
				else
					formatVarList.insert(varStr, qMakePair(FormatType::kNothing, QString()));
			}
		}

		if (formatVarList.isEmpty())
			continue;

		for (auto it = formatVarList.constBegin(); it != formatVarList.constEnd(); ++it)
		{
			const QString key = it.key();
			const QPair<FormatType, QString> varValue = it.value();
			QString preReplace = "";
			switch (varValue.first)
			{
			case FormatType::kNothing:
				preReplace = QString("{:%1}").arg(key);
				break;
			case FormatType::kTime:
				preReplace = QString("{T:%1}").arg(key);
				break;
			case FormatType::kConst:
				preReplace = QString("{C:%1}").arg(key);
				break;
			default:
				continue;
			}

			formatStr.replace(preReplace, varValue.second);
		}
	}

	return util::toConstData(formatStr);
}
#pragma endregion


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma region luaclasses
std::tuple<std::string, long long> CLuaChar::getServer()
{
	GameDevice& gamedevice = GameDevice::getInstance(index_);
	if (gamedevice.worker.isNull())
		return std::make_tuple("", -1);

	long long subServer = static_cast<long long>(mem::read<int>(gamedevice.getProcess(), gamedevice.getProcessModule() + sa::kOffsetSubServerIndex));
	long long position = static_cast<long long>(mem::read<int>(gamedevice.getProcess(), gamedevice.getProcessModule() + sa::kOffsetPositionIndex));

	QString subServerName;
	long long size = gamedevice.subServerNameList.get().size();
	if (subServer >= 0 && subServer < size)
		subServerName = gamedevice.subServerNameList.get().value(subServer);
	else
		subServerName = "????";

	return std::make_tuple(util::toConstData(subServerName), position + 1);
}

class CLuaLog
{
public:
	CLuaLog(std::string filename, sol::this_state s)
	{
		QString path = util::toQString(filename);
		if (!path.endsWith(".log"))
			path += ".log";

		file_.setFileName(path);
		file_.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append);

		textStream_.setFile(&file_);
	}

	~CLuaLog()
	{
		if (file_.isOpen())
			file_.close();

		qDebug() << "~CLuaLog";
	}

	bool write(sol::object odata)
	{
		if (!file_.isOpen())
			return false;

		QString data;
		if (odata.is<std::string>())
			data = util::toQString(odata.as<std::string>());
		else if (odata.is<long long>())
			data = util::toQString(odata.as<long long>());
		else if (odata.is<double>())
			data = util::toQString(odata.as<double>());
		else if (odata.is<bool>())
			data = odata.as<bool>() ? "true" : "false";
		else
			return false;

		QString line = QString("[%1] [%2] [%3] %4\n")
			.arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"))
			.arg("sash")
			.arg("debug")
			.arg(data);

		textStream_ << line;
		textStream_.flush();
		file_.flush();
		return true;
	}

private:
	util::TextStream textStream_;
	QFile file_;
};

class CLuaChat
{
public:
	CLuaChat(long long index) : index_(index) {}

	std::string operator[](long long index)
	{
		--index;

		GameDevice& gamedevice = GameDevice::getInstance(index_);
		if (gamedevice.worker.isNull())
			return "";

		QString text = gamedevice.worker->getChatHistory(index);
		return util::toConstData(text);
	}

	bool contains(std::string str, sol::this_state s) const
	{
		GameDevice& gamedevice = GameDevice::getInstance(index_);
		if (gamedevice.worker.isNull())
			return false;

		if (str.empty())
			return false;

		QStringList list = util::toQString(str).split(util::rexOR, Qt::SkipEmptyParts);
		QString text = util::toQString(str);
		for (long long i = 0; i < sa::MAX_CHAT_HISTORY; ++i)
		{
			QString cmptext = gamedevice.worker->getChatHistory(i);
			if (cmptext.isEmpty())
				continue;

			for (const QString& it : list)
			{
				if (it.isEmpty())
					continue;

				if (cmptext.contains(it))
					return true;
			}
		}

		return false;
	}

private:
	long long index_ = 0;
};

class CLuaCard
{
public:
	CLuaCard(long long index) : index_(index) {}

	sa::address_bool_t operator[](long long index)
	{
		--index;

		GameDevice& gamedevice = GameDevice::getInstance(index_);
		if (gamedevice.worker.isNull())
			return sa::address_bool_t();

		sa::address_bool_t card = gamedevice.worker->getAddressBook(index);
		return card;
	}

	bool contains(std::string str, sol::this_state s) const
	{
		GameDevice& gamedevice = GameDevice::getInstance(index_);
		if (gamedevice.worker.isNull())
			return false;

		if (str.empty())
			return false;

		QStringList list = util::toQString(str).split(util::rexOR, Qt::SkipEmptyParts);
		for (long long i = 0; i < sa::MAX_ADDRESS_BOOK; ++i)
		{
			sa::address_bool_t card = gamedevice.worker->getAddressBook(i);
			if (card.name.isEmpty())
				continue;

			for (const QString& it : list)
			{
				if (it.isEmpty())
					continue;

				if (card.name.contains(it))
					return true;
			}
		}

		return false;
	}

private:
	long long index_ = 0;
};

class CLuaMailProxy
{
public:
	CLuaMailProxy(long long gameDeviceIndex, long long addressBookIndex)
		: index_(gameDeviceIndex), addressBookIndex_(addressBookIndex) {}

	std::string operator[](long long index)
	{
		--index;

		GameDevice& gamedevice = GameDevice::getInstance(index_);
		if (gamedevice.worker.isNull())
			return "";

		if (index < 0 || index >= sa::MAIL_MAX_HISTORY)
			return "";

		sa::mail_history_t hisory = gamedevice.worker->getMailHistory(addressBookIndex_);
		return util::toConstData(hisory.dateStr[index]);
	}

private:
	long long index_ = 0;
	long long addressBookIndex_ = 0;
};

class CLuaMail
{
public:
	CLuaMail(long long index) : index_(index) {}

	CLuaMailProxy operator[](long long index)
	{
		--index;

		return CLuaMailProxy(index_, index);
	}

private:
	long long index_ = 0;
};

class CLuaPoint
{
public:
	CLuaPoint(long long index) : index_(index) {}

	long long getExp() const { GameDevice& gamedevice = GameDevice::getInstance(index_); return gamedevice.worker->currencyData.get().expbufftime; }
	long long getRep() const { GameDevice& gamedevice = GameDevice::getInstance(index_); return gamedevice.worker->currencyData.get().prestige; }
	long long getEne() const { GameDevice& gamedevice = GameDevice::getInstance(index_); return gamedevice.worker->currencyData.get().energy; }
	long long getShl() const { GameDevice& gamedevice = GameDevice::getInstance(index_); return gamedevice.worker->currencyData.get().shell; }
	long long getVit() const { GameDevice& gamedevice = GameDevice::getInstance(index_); return gamedevice.worker->currencyData.get().vitality; }
	long long getPts() const { GameDevice& gamedevice = GameDevice::getInstance(index_); return gamedevice.worker->currencyData.get().points; }
	long long getVip() const { GameDevice& gamedevice = GameDevice::getInstance(index_); return gamedevice.worker->currencyData.get().VIPPoints; }

private:
	long long index_ = 0;
};

class CLuaTeam
{
public:
	CLuaTeam(long long index) : index_(index) {}

	sa::team_t operator[](long long index)
	{
		--index;

		GameDevice& gamedevice = GameDevice::getInstance(index_);
		if (gamedevice.worker.isNull())
			return sa::team_t();

		gamedevice.worker->updateDatasFromMemory();

		sa::team_t team = gamedevice.worker->getTeam(index);
		team.controlIndex = index_;
		return team;
	}

	bool contains(std::string names, sol::this_state s) const
	{
		GameDevice& gamedevice = GameDevice::getInstance(index_);
		if (gamedevice.worker.isNull())
			return false;

		QStringList list = util::toQString(names).split(util::rexOR, Qt::SkipEmptyParts);
		QString cmpName = list.value(0);
		QString cmpFreeName = list.value(1);

		gamedevice.worker->updateDatasFromMemory();

		for (long long i = 0; i < sa::MAX_TEAM; ++i)
		{
			sa::team_t team = gamedevice.worker->getTeam(i);
			if (team.name.isEmpty())
				continue;

			QString freeName = gamedevice.worker->mapUnitHash.value(team.id).freeName;

			if (!cmpFreeName.isEmpty() && freeName.contains(cmpFreeName) && !cmpName.isEmpty() && team.name == cmpName)
				return true;
			if (cmpFreeName.isEmpty() && !cmpName.isEmpty() && team.name == cmpName)
				return true;
		}

		return false;
	}

	long long size()
	{
		GameDevice& gamedevice = GameDevice::getInstance(index_);
		if (gamedevice.worker.isNull())
			return 0;

		gamedevice.worker->updateDatasFromMemory();

		return gamedevice.worker->getTeamSize();
	}

private:
	long long index_ = 0;

};

class CLuaPetEquipProxy
{
public:
	CLuaPetEquipProxy(long long gameDeviceIndex, long long petIndex) : index_(gameDeviceIndex), petIndex_(petIndex) {}

	sa::item_t operator[](long long index)
	{
		--index;

		GameDevice& gamedevice = GameDevice::getInstance(index_);
		if (gamedevice.worker.isNull())
			return sa::item_t();

		return gamedevice.worker->getPetEquip(petIndex_, index);
	}

	sol::object find(std::string sname, sol::this_state s)
	{
		sol::state_view lua(s.lua_state());
		GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
		if (gamedevice.worker.isNull())
			return sol::lua_nil;

		QString name = util::toQString(sname);
		bool isExact = false;
		if (name.startsWith("?"))
		{
			name = name.mid(1);
			isExact = true;
		}

		for (long long i = 0; i < sa::MAX_PET_ITEM; ++i)
		{
			sa::item_t item = gamedevice.worker->getPetEquip(petIndex_, i);
			if (!item.valid)
				continue;

			if (isExact)
			{
				if (item.name == name)
					return sol::make_object(lua, item);
			}
			else
			{
				if (item.name.contains(name))
					return sol::make_object(lua, item);
			}
		}

		return sol::lua_nil;
	}

private:
	long long index_ = 0;
	long long petIndex_ = -1;
};

class CLuaPetEquip
{
public:
	CLuaPetEquip(long long index) : index_(index) {}

	CLuaPetEquipProxy operator[](long long index)
	{
		--index;

		CLuaPetEquipProxy proxy(index_, index);
		return proxy;
	}

private:
	long long index_ = 0;
};

class CLuaDialog
{
public:
	CLuaDialog(long long index) : index_(index) {}
	~CLuaDialog() = default;

	std::string operator[](long long index)
	{
		GameDevice& gamedevice = GameDevice::getInstance(index_);

		QStringList dialogstrs = gamedevice.worker->currentDialog.get().linedatas;

		long long size = dialogstrs.size();

		bool visible = gamedevice.worker->isDialogVisible();

		QString text;
		if (index >= size || !visible)
			text = "";
		else
			text = dialogstrs.value(index);

		if (visible)
			return util::toConstData(text);
		else
			return "";
	}

	bool isValid() const { GameDevice& gamedevice = GameDevice::getInstance(index_); return gamedevice.worker->isDialogVisible(); }
	long long getWindowType() const { GameDevice& gamedevice = GameDevice::getInstance(index_); return gamedevice.worker->currentDialog.get().windowtype; }
	long long getButtonType() const { GameDevice& gamedevice = GameDevice::getInstance(index_); return gamedevice.worker->currentDialog.get().buttontype; }
	long long getDialogId() const { GameDevice& gamedevice = GameDevice::getInstance(index_); return gamedevice.worker->currentDialog.get().dialogid; }
	long long getUnitId() const { GameDevice& gamedevice = GameDevice::getInstance(index_); return gamedevice.worker->currentDialog.get().unitid; }
	std::string getData() const { GameDevice& gamedevice = GameDevice::getInstance(index_); return util::toConstData(gamedevice.worker->currentDialog.get().data); }
	std::string  getLineData(long long index) const { GameDevice& gamedevice = GameDevice::getInstance(index_); return util::toConstData(gamedevice.worker->currentDialog.get().linedatas.join("|")); }
	std::string  getButtonText(long long index) const { GameDevice& gamedevice = GameDevice::getInstance(index_); return util::toConstData(gamedevice.worker->currentDialog.get().linebuttontext.join("|")); }

	bool contains(std::string str, sol::this_state s)
	{
		sol::state_view lua(s.lua_state());
		GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
		if (gamedevice.worker.isNull())
			return false;

		if (str.empty())
			return false;

		if (!gamedevice.worker->isDialogVisible())
			return false;

		QString text = util::toQString(str);
		QStringList list = text.split(util::rexOR, Qt::SkipEmptyParts);
		QStringList dialogstrs = gamedevice.worker->currentDialog.get().linedatas;
		long long size = dialogstrs.size();
		for (long long i = 0; i < size; ++i)
		{
			QString cmptext = dialogstrs.value(i);
			for (const QString& it : list)
			{
				if (cmptext.contains(it))
					return true;
			}
		}

		return false;
	}
private:
	long long index_ = 0;
};

std::string sa::team_t::getFreeName() const
{
	GameDevice& gamedevice = GameDevice::getInstance(controlIndex);
	if (gamedevice.worker.isNull())
		return "";

	gamedevice.worker->updateDatasFromMemory();

	return util::toConstData(gamedevice.worker->mapUnitHash.value(id).freeName);
}

class CLuaSkill
{
public:
	CLuaSkill(long long index) : index_(index) {}

	sa::profession_skill_t operator[](long long index)
	{
		--index;

		GameDevice& gamedevice = GameDevice::getInstance(index_);
		if (gamedevice.worker.isNull())
			return sa::profession_skill_t();


		return gamedevice.worker->getSkill(index);
	}

	sol::object find(std::string sname, sol::this_state s)
	{
		GameDevice& gamedevice = GameDevice::getInstance(index_);
		if (gamedevice.worker.isNull())
			return sol::lua_nil;

		QString name = util::toQString(sname);

		if (name.isEmpty())
			return sol::lua_nil;

		bool isExist = true;
		QString newName = name;
		if (name.startsWith("?"))
		{
			newName = name.mid(1);
			isExist = false;
		}

		QHash<long long, sa::profession_skill_t> skill = gamedevice.worker->getSkills();

		for (auto it = skill.begin(); it != skill.end(); ++it)
		{
			sa::profession_skill_t skill = it.value();
			skill.index = it.key() + 1;

			QString skillname = skill.name;
			QString memo = skill.memo;
			if (skillname.isEmpty())
				continue;

			if (isExist && skillname == newName)
				return sol::make_object(s, skill);
			else if (!isExist && skillname.contains(newName))
				return sol::make_object(s, skill);
			else if (!memo.isEmpty() && memo.contains(name, Qt::CaseInsensitive))
				return sol::make_object(s, skill);
		}

		return sol::lua_nil;
	}

private:
	long long index_ = 0;
};

class CLuaMagic
{
public:
	CLuaMagic(long long index) : index_(index) {}

	sa::magic_t operator[](long long index)
	{
		--index;

		GameDevice& gamedevice = GameDevice::getInstance(index_);
		if (gamedevice.worker.isNull())
			return sa::magic_t();

		return gamedevice.worker->getMagic(index);
	}

	sol::object find(std::string sname, sol::this_state s)
	{
		GameDevice& gamedevice = GameDevice::getInstance(index_);
		if (gamedevice.worker.isNull())
			return sol::lua_nil;

		QString name = util::toQString(sname);

		if (name.isEmpty())
			return sol::lua_nil;

		bool isExist = true;
		QString newName = name;
		if (name.startsWith("?"))
		{
			newName = name.mid(1);
			isExist = false;
		}

		for (long long i = 0; i < sa::MAX_MAGIC; ++i)
		{
			sa::magic_t magic = gamedevice.worker->getMagic(i);

			QString magicname = magic.name;
			QString memo = magic.memo;
			if (magicname.isEmpty())
				continue;

			if (isExist && magicname == newName)
				return sol::make_object(s, magic);
			else if (!isExist && magicname.contains(newName))
				return sol::make_object(s, magic);
			else if (!memo.isEmpty() && memo.contains(name, Qt::CaseInsensitive))
				return sol::make_object(s, magic);
		}

		return sol::lua_nil;
	}

private:
	long long index_ = 0;
};

class CLuaPetSkillProxy
{
public:
	CLuaPetSkillProxy(long long gameDeviceIndex, long long petIndex)
		: index_(gameDeviceIndex), petIndex_(petIndex) {}

	sa::pet_skill_t operator[](long long index)
	{
		--index;

		GameDevice& gamedevice = GameDevice::getInstance(index_);
		if (gamedevice.worker.isNull())
			return sa::pet_skill_t();

		return gamedevice.worker->getPetSkill(petIndex_, index);
	}

	sol::object find(std::string sname, sol::this_state s)
	{
		GameDevice& gamedevice = GameDevice::getInstance(index_);
		if (gamedevice.worker.isNull())
			return sol::lua_nil;

		QString name = util::toQString(sname);

		if (name.isEmpty())
			return sol::lua_nil;

		bool isExist = true;
		QString newName = name;
		if (name.startsWith("?"))
		{
			newName = name.mid(1);
			isExist = false;
		}

		QHash<long long, sa::pet_skill_t> skills = gamedevice.worker->getPetSkills(petIndex_);

		for (auto it = skills.begin(); it != skills.end(); ++it)
		{
			sa::pet_skill_t skill = it.value();

			QString skillname = skill.name;
			QString memo = skill.memo;
			if (skillname.isEmpty())
				continue;

			if (isExist && skillname == newName)
				return sol::make_object(s, skill);
			else if (!isExist && skillname.contains(newName))
				return sol::make_object(s, skill);
			else if (!memo.isEmpty() && memo.contains(name, Qt::CaseInsensitive))
				return sol::make_object(s, skill);
		}

		return sol::lua_nil;
	}

private:
	long long index_ = 0;
	long long petIndex_ = -1;
};

class CLuaPetSkill
{
public:
	CLuaPetSkill(long long index) : index_(index) {}

	CLuaPetSkillProxy operator[](long long index)
	{
		--index;

		CLuaPetSkillProxy proxy(index_, index);

		return proxy;
	}

	sol::object find(long long petIndex, std::string sname, sol::this_state s)
	{
		GameDevice& gamedevice = GameDevice::getInstance(index_);
		if (gamedevice.worker.isNull())
			return sol::lua_nil;

		QString name = util::toQString(sname);

		if (name.isEmpty())
			return sol::lua_nil;

		bool isExist = true;
		QString newName = name;
		if (name.startsWith("?"))
		{
			newName = name.mid(1);
			isExist = false;
		}

		--petIndex;

		QHash<long long, sa::pet_skill_t> skills = gamedevice.worker->getPetSkills(petIndex);

		for (long long i = 0; i < sa::MAX_PET_SKILL; ++i)
		{
			sa::pet_skill_t skill = skills.value(i);

			QString skillname = skill.name;
			QString memo = skill.memo;
			if (skillname.isEmpty())
				continue;

			if (isExist && skillname == newName)
				return sol::make_object(s, skill);
			else if (!isExist && skillname.contains(newName))
				return sol::make_object(s, skill);
			else if (!memo.isEmpty() && memo.contains(name, Qt::CaseInsensitive))
				return sol::make_object(s, skill);
		}

		return sol::lua_nil;
	}

private:
	long long index_ = 0;
};

class CLuaMapUnit
{
public:
	CLuaMapUnit(long long index) : index_(index) {}

	sol::object find(sol::object p1, sol::object p2, sol::this_state s)
	{
		GameDevice& gamedevice = GameDevice::getInstance(index_);
		if (gamedevice.worker.isNull())
			return sol::lua_nil;

		QString name = "";
		long long modelid = -1;
		if (p1.is<std::string>())
			name = util::toQString(p1);
		else if (p1.is<long long>())
			modelid = p1.as<long long>();
		else
			return sol::lua_nil;

		sa::map_unit_t unit = {};
		do
		{
			if (!p2.is<long long>())
			{
				if (-1 == modelid)
				{
					if (gamedevice.worker->findUnit(name, sa::kObjectNPC, &unit, "", -1))
					{
						break;
					}

					if (gamedevice.worker->findUnit(name, sa::kObjectHuman, &unit, "", -1))
					{
						break;
					}

					if (gamedevice.worker->findUnit("", sa::kObjectNPC, &unit, name, -1))
					{
						break;
					}

					if (gamedevice.worker->findUnit("", sa::kObjectHuman, &unit, name, -1))
					{
						break;
					}
				}
				else
				{
					if (gamedevice.worker->findUnit("", sa::kObjectNPC, &unit, "", modelid))
					{
						break;
					}

					if (gamedevice.worker->findUnit("", sa::kObjectHuman, &unit, "", modelid))
					{
						break;
					}
				}

			}
			else
			{
				if (p2.as<long long>() >= sa::kObjectMaxCount)
				{
					modelid = p2.as<long long>();

					if (gamedevice.worker->findUnit(name, sa::kObjectNPC, &unit, "", modelid))
					{
						break;
					}

					if (gamedevice.worker->findUnit(name, sa::kObjectHuman, &unit, "", modelid))
					{
						break;
					}
				}
				else
				{
					sa::ObjectType type = static_cast<sa::ObjectType>(p2.as<long long>());
					if (-1 == modelid)
					{
						if (gamedevice.worker->findUnit(name, type, &unit, "", -1))
						{
							break;
						}

						if (gamedevice.worker->findUnit("", type, &unit, name, -1))
						{
							break;
						}
					}
					else
					{
						if (gamedevice.worker->findUnit("", type, &unit, "", modelid))
						{
							break;
						}
					}
				}
			}

			return sol::lua_nil;
		} while (false);

		return sol::make_object(s, unit);
	}

private:
	long long index_ = 0;
};

class CLuaDebug
{
public:
	CLuaDebug(sol::this_state s)
		: s_(s)
	{}
	~CLuaDebug()
	{
		CLuaSystem luasystem;
		std::string output_string;
		output_string += output_buffer_.str();

		sol::object ostr = sol::make_object(s_, output_string);
		sol::object ocolor = sol::make_object(s_, 4);
		luasystem.print(ostr, ocolor, s_);
	}

	// <<
	CLuaDebug& operator<<(sol::object o)
	{
		std::ostringstream ss;
		if (o.is<std::string>())
		{
			ss << luatool::format(o.as<std::string>(), s_);
		}
		else if (o.is<long long>())
			ss << std::to_string(o.as<long long>());
		else if (o.is<double>())
			ss << std::fixed << std::setprecision(std::numeric_limits<double>::digits10 + 1) << o.as<double>();
		else if (o.is<bool>())
			ss << o.as<bool>() ? "true" : "false";
		else if (o.is<sol::table>() && !o.is<sol::userdata>() && !o.is<sol::function>() && !o.is<sol::lightuserdata>())
		{
			//print table
			long long depth = 1024;
			QString msg = luadebug::getTableVars(s_.L, 1, depth);
			msg.replace("=[[", "='");
			msg.replace("]],", "',");
			msg.replace("]]}", "'}");
			msg.replace(",", ",\n");

			ss << util::toConstData(msg);
		}
		else if (o.is<sol::metatable>())
		{
			//print key and type name   like: QString("%1(%2)").arg(key).arg(typeName);
			sol::userdata ud = o.as<sol::userdata>();
			//打印出所有成員函數名
			sol::table metaTable = ud[sol::metatable_key];

			QString msg;
			QStringList l;
			for (auto& pair : metaTable)
			{
				QString name = util::toQString(pair.first.as<std::string>());
				if (name.startsWith("__") || name.startsWith("class_"))
					continue;

				l.append(name);
			}

			if (!l.isEmpty())
			{
				std::sort(l.begin(), l.end());
				msg = l.join("\n");
			}
			else
			{
				QString pointerStr = util::toQString(reinterpret_cast<long long>(o.pointer()), 16);
				msg = "(userdata) 0x" + pointerStr;
			}

			ss << util::toConstData(msg);
		}


		output_buffer_ << ss.str();
		return maybeSpace();
	}

private:
	std::ostringstream output_buffer_;
	sol::this_state s_;

	CLuaDebug& maybeSpace() { output_buffer_ << " "; return *this; }

};
#pragma endregion

CLua::CLua(long long index, QObject* parent)
	: Indexer(index)
{
	std::ignore = parent;
	qDebug() << "CLua 1";
}

CLua::CLua(long long index, const QString& content, QObject* parent)
	: Indexer(index)
	, scriptContent_(content)
{
	std::ignore = parent;
	qDebug() << "CLua 2";
}

CLua::~CLua()
{
	qDebug() << "~CLua";
}

void CLua::start()
{
	moveToThread(&thread_);

	connect(this, &CLua::finished, &thread_, &QThread::quit);
	connect(&thread_, &QThread::finished, &thread_, &QThread::deleteLater);
	connect(&thread_, &QThread::started, this, &CLua::proc);
	connect(this, &CLua::finished, this, [this]()
		{
			thread_.requestInterruption();
			thread_.quit();
			thread_.wait();
			qDebug() << "CLua::finished";
		});

	thread_.start();
}

void CLua::wait()
{
	thread_.wait();
}

void CLua::open_enumlibs()
{
	/*
	//添加lua枚舉值示範
	enum TemplateEnum
	{
		EnumValue1,
		EnumValue2,
		EnumValue3
	};

	lua_.new_enum<TemplateEnum>("TEST",
		{
			{ "EnumValue1", TemplateEnum::EnumValue1 },
			{ "EnumValue2", TemplateEnum::EnumValue2 },
			{ "EnumValue3", TemplateEnum::EnumValue3 }
		}
	);

	在lua中的使用方法: TEST.EnumValue1
	*/

	lua_.new_enum <util::SelectTarget>("TARGET",
		{
			{ "SELF", util::kSelectSelf },
			{ "PET", util::kSelectPet },
			{ "ALLIE_ANY", util::kSelectAllieAny },
			{ "ALLIE_ALL", util::kSelectAllieAll },
			{ "ENEMY_ANY", util::kSelectEnemyAny },
			{ "ENEMY_ALL", util::kSelectEnemyAll },
			{ "ENEMY_FRONT", util::kSelectEnemyFront },
			{ "ENEMY_BACK", util::kSelectEnemyBack },
			{ "LEADER", util::kSelectLeader },
			{ "LEADER_PET", util::kSelectLeaderPet },
			{ "TEAM", util::kSelectTeammate1 },
			{ "TEAM1_PET", util::kSelectTeammate1Pet },
			{ "TEAM2", util::kSelectTeammate2 },
			{ "TEAM2_PET", util::kSelectTeammate2Pet },
			{ "TEAM3", util::kSelectTeammate3 },
			{ "TEAM3_PET", util::kSelectTeammate3Pet },
			{ "TEAM4", util::kSelectTeammate4 },
			{ "TEAM4_PET", util::kSelectTeammate4Pet }
		}
	);
	//在腳本中的使用方法: Target.Self Target.Pet


	lua_.new_enum< sa::DailyJobState>("DAILY",
		{
			{ "FAILED", sa::DailyJobState::kFailed },
			{ "NONE", sa::DailyJobState::kNone },
			{ "ONGOING", sa::DailyJobState::kOnGoing },
			{ "FINISHED", sa::DailyJobState::kFinished }
		});
}

//以Metatable方式註冊函數 支援函數多載、運算符重載，面向對象，常量
void CLua::open_testlibs()
{
	/*
	sol::usertype<CLuaTest> test = lua_.new_usertype<CLuaTest>("Test",
		sol::call_constructor,
		sol::constructors<
		//建構函數多載
		CLuaTest(sol::this_state),
		CLuaTest(long long a, sol::this_state)>(),

		//成員函數多載
		"add", sol::overload(
			sol::resolve<long long(long long a, long long b)>(&CLuaTest::add),
			sol::resolve<long long(long long b)>(&CLuaTest::add)
		),

		//成員函數聲明
		"sub", &CLuaTest::sub
	);


	在lua中的使用方法:

	local test = Test(1)
	print(test:add(1, 2))
	print(test:add(2))
	print(test:sub(1, 2))

	*/
}

void CLua::open_utillibs(sol::state& lua)
{
	lua.new_usertype<CLuaLog>("Log",
		sol::call_constructor,
		sol::constructors<
		CLuaLog(std::string, sol::this_state)>(),
		"write", &CLuaLog::write
	);

	lua.set_function("format", &luatool::format);

	lua.set_function("checkdaily", [this](std::string smisson, sol::object otimeout)->long long
		{
			GameDevice& gamedevice = GameDevice::getInstance(getIndex());
			QString missionStr = util::toQString(smisson);

			QStringList missions = missionStr.split(util::rexOR, Qt::SkipEmptyParts);
			if (gamedevice.worker.isNull())
				return sa::DailyJobState::kFailed;

			long long timeout = 5000;
			if (otimeout.is<long long>())
				timeout = otimeout.as<long long>();

			bool doNotWait = false;

			QVector<long long> results;
			for (const auto& mission : missions)
			{
				QVector<long long> v = gamedevice.worker->checkJobDailyState(mission, timeout, doNotWait);

				if (v.isEmpty())
					continue;

				if (!doNotWait)
					doNotWait = true;

				if (v.contains(sa::DailyJobState::kFinished))
					results.append(sa::DailyJobState::kFinished);
				else if (v.contains(sa::DailyJobState::kOnGoing))
					results.append(sa::DailyJobState::kOnGoing);
				else
					results.append(sa::DailyJobState::kNone);
			}

			if (results.contains(sa::DailyJobState::kNone))
				return sa::DailyJobState::kNone;
			else if (results.contains(sa::DailyJobState::kOnGoing))
				return sa::DailyJobState::kOnGoing;
			else if (results.contains(sa::DailyJobState::kFinished))
				return sa::DailyJobState::kFinished;

			return sa::DailyJobState::kNone;
		});


	lua.set_function("getgamestate", [this](long long id)->long long
		{
			GameDevice& gamedevice = GameDevice::getInstance(getIndex());
			InterfaceSender sender(gamedevice.getParentWidget());

			return sender.GetGameState(id);
		});


	lua.set_function("setlogin", [this](long long id, long long server, long long subserver, long long position, std::string account, std::string password)->long long
		{
			GameDevice& gamedevice = GameDevice::getInstance(getIndex());
			InterfaceSender sender(gamedevice.getParentWidget());

			QString acc = util::toQString(account);
			QString pwd = util::toQString(password);

			return sender.SetAutoLogin(id, server - 1, subserver - 1, position - 1, acc, pwd);
		});

	//检查某串字符串或表内是否包含某个字符串或元素
	lua.set_function("contains", [this](sol::object data, sol::object ocmp_data, sol::this_state s)->bool
		{
			auto toVariant = [](const sol::object& o)->QVariant
				{
					QVariant out;
					if (o.is<std::string>())
						out = util::toQString(o).simplified();
					else if (o.is<long long>())
						out = o.as<long long>();
					else if (o.is<double>())
						out = o.as<double>();
					else if (o.is<bool>())
						out = o.as<bool>();

					return out;
				};

			if (data.is<sol::table>() && ocmp_data.is<sol::table>())
			{
				sol::table table = data.as<sol::table>();
				sol::table table2 = ocmp_data.as<sol::table>();
				QStringList a;
				QStringList b;

				for (const std::pair<sol::object, sol::object>& pair : table)
					a.append(toVariant(pair.second).toString().simplified());

				for (const std::pair<sol::object, sol::object>& pair : table2)
					b.append(toVariant(pair.second).toString().simplified());

				return QSet<QString>(a.begin(), a.end()).contains(QSet<QString>(b.begin(), b.end())) || QSet<QString>(b.begin(), b.end()).contains(QSet<QString>(a.begin(), a.end()));
			}
			else if (data.is<sol::table>() && !ocmp_data.is<sol::table>())
			{
				QStringList a;
				QString b = toVariant(ocmp_data).toString().simplified();
				sol::table table = data.as<sol::table>();
				for (const std::pair<sol::object, sol::object>& pair : table)
					a.append(toVariant(pair.second).toString().simplified());

				if (b.isEmpty())
					return false;

				return a.contains(b);
			}

			QString qcmp_data;
			QVariant adata = toVariant(ocmp_data);
			QVariant bdata = toVariant(data);

			if (adata.type() == QVariant::Type::String)
				qcmp_data = adata.toString().simplified();
			else if (adata.type() == QVariant::Type::LongLong)
				qcmp_data = util::toQString(adata.toLongLong());
			else if (adata.type() == QVariant::Type::Double)
				qcmp_data = util::toQString(adata.toDouble());
			else if (adata.type() == QVariant::Type::Bool)
				qcmp_data = util::toQString(adata.toBool());
			else
				return false;

			if (adata.toString().isEmpty() && !bdata.toString().isEmpty())
				return false;
			else if (!adata.toString().isEmpty() && bdata.toString().isEmpty())
				return false;

			if (bdata.type() == QVariant::Type::String)
				return bdata.toString().simplified().contains(qcmp_data) || qcmp_data.contains(bdata.toString().simplified());
			else if (bdata.type() == QVariant::Type::LongLong)
				return util::toQString(bdata.toLongLong()).contains(qcmp_data) || qcmp_data.contains(util::toQString(bdata.toLongLong()));
			else if (bdata.type() == QVariant::Type::Double)
				return util::toQString(bdata.toDouble()).contains(qcmp_data) || qcmp_data.contains(util::toQString(bdata.toDouble()));
			else if (bdata.type() == QVariant::Type::Bool)
				return util::toQString(bdata.toBool()).contains(qcmp_data) || qcmp_data.contains(util::toQString(bdata.toBool()));
			else
				return false;

			return false;

		}
	);

	//检查某串字符串或或数值在一个字符串或表内的出现的次数
	lua.set_function("count", [this](sol::object data, sol::object ocmp_data, sol::this_state s)->long long
		{
			long long count = 0;
			auto toVariant = [](const sol::object& o)->QVariant
				{
					QVariant out;
					if (o.is<std::string>())
						out = util::toQString(o).simplified();
					else if (o.is<long long>())
						out = o.as<long long>();
					else if (o.is<double>())
						out = o.as<double>();
					else if (o.is<bool>())
						out = o.as<bool>();

					return out;
				};

			if (data.is<sol::table>() && ocmp_data.is<sol::table>())
			{
				sol::table table = data.as<sol::table>();
				sol::table table2 = ocmp_data.as<sol::table>();
				QStringList a;
				QStringList b;

				for (const std::pair<sol::object, sol::object>& pair : table)
					a.append(toVariant(pair.second).toString().simplified());

				for (const std::pair<sol::object, sol::object>& pair : table2)
					b.append(toVariant(pair.second).toString().simplified());

				for (const QString& it : b)
					count += a.count(it);

				return count;
			}
			else if (data.is<sol::table>() && !ocmp_data.is<sol::table>())
			{
				QStringList a;
				QString b = toVariant(ocmp_data).toString().simplified();
				sol::table table = data.as<sol::table>();
				for (const std::pair<sol::object, sol::object>& pair : table)
					a.append(toVariant(pair.second).toString().simplified());

				if (b.isEmpty())
					return 0;

				return a.count(b);
			}

			QString qcmp_data;
			QVariant adata = toVariant(ocmp_data);
			QVariant bdata = toVariant(data);

			if (adata.type() == QVariant::Type::String)
				qcmp_data = adata.toString().simplified();
			else if (adata.type() == QVariant::Type::LongLong)
				qcmp_data = util::toQString(adata.toLongLong());
			else if (adata.type() == QVariant::Type::Double)
				qcmp_data = util::toQString(adata.toDouble());
			else if (adata.type() == QVariant::Type::Bool)
				qcmp_data = util::toQString(adata.toBool());
			else
				return 0;

			if (adata.toString().isEmpty() && !bdata.toString().isEmpty())
				return 0;
			else if (!adata.toString().isEmpty() && bdata.toString().isEmpty())
				return 0;

			if (bdata.type() == QVariant::Type::String)
				return bdata.toString().simplified().count(qcmp_data);
			else if (bdata.type() == QVariant::Type::LongLong)
				return util::toQString(bdata.toLongLong()).count(qcmp_data);
			else if (bdata.type() == QVariant::Type::Double)
				return util::toQString(bdata.toDouble()).count(qcmp_data);
			else if (bdata.type() == QVariant::Type::Bool)
				return util::toQString(bdata.toBool()).count(qcmp_data);
			else
				return 0;

			return 0;
		}
	);

	lua["mkpath"] = [](std::string sfilename, sol::object osuffix, sol::object obj, sol::this_state s)->std::string
		{
			QString retstring = "\0";
			QString fileName = util::toQString(sfilename);
			fileName.replace("\\", "/");

			QFileInfo fileinfo(fileName);
			QString suffix = fileinfo.suffix();

			if (suffix.isEmpty() && osuffix.is<std::string>())
			{
				suffix = util::toQString(osuffix);
				if (!suffix.startsWith("."))
					suffix.prepend(".");
				fileName.append(suffix);
			}
			else if (suffix.isEmpty())
				fileName += ".txt";

			if (obj == sol::lua_nil)
			{
				retstring = util::findFileFromName(fileName);
			}
			else if (obj.is<std::string>())
			{
				QString dir = util::toQString(obj);
				dir.replace("\\", "/");
				if (dir.endsWith("/"))
					dir.chop(1);
				retstring = util::findFileFromName(fileName, util::toQString(obj));
			}

			return util::toConstData(retstring);
		};

	lua["findfiles"] = [](std::string sname, sol::object osuffix, sol::object obasedir, sol::this_state s)->sol::table
		{
			sol::state_view lua(s);
			QString name = util::toQString(sname);
			name.replace("\\", "/");

			sol::table result = lua.create_table();
			QStringList paths;

			QString basedir = util::applicationDirPath();
			if (obasedir.is<std::string>())
				basedir = util::toQString(obasedir);

			basedir.replace("\\", "/");
			if (basedir.endsWith("/"))
				basedir.chop(1);

			QString suffix;
			if (osuffix.is<std::string>())
			{
				suffix = util::toQString(osuffix);
				if (!suffix.startsWith("."))
					suffix.prepend(".");
			}

			util::searchFiles(basedir, name, suffix, &paths, false);

			for (long long i = 0; i < paths.size(); ++i)
			{
				result[i + 1] = util::toConstData(paths.at(i));
			}

			return result;
		};


	lua["mktable"] = [](long long a, sol::object ob, sol::this_state s)->sol::object
		{
			sol::state_view lua(s);

			sol::table t = lua.create_table();

			if (ob.is<long long>() && a > ob.as<long long>())
			{
				for (long long i = ob.as<long long>(); i < (a + 1); ++i)
				{
					t.add(i);
				}
			}
			else if (ob.is<long long>() && a < ob.as<long long>())
			{
				for (long long i = a; i < (ob.as<long long>() + 1); ++i)
				{
					t.add(i);
				}
			}
			else if (ob.is<long long>() && a == ob.as<long long>())
			{
				t.add(a);
			}
			else if (ob == sol::lua_nil && a >= 0)
			{
				for (long long i = 1; i < a + 1; ++i)
				{
					t.add(i);
				}
			}
			else if (ob == sol::lua_nil && a < 0)
			{
				for (long long i = a; i < 2; ++i)
				{
					t.add(i);
				}
			}
			else
				t.add(a);

			return t;
		};

	static const auto Is_1DTable = [](sol::table t)->bool
		{
			for (const std::pair<sol::object, sol::object>& i : t)
			{
				if (i.second.is<sol::table>())
				{
					return false;
				}
			}
			return true;
		};

	lua["tshuffle"] = [](sol::object t, sol::this_state s)->sol::object
		{
			if (!t.is<sol::table>())
				return sol::lua_nil;
			//檢查是否為1維
			sol::table test = t.as<sol::table>();
			if (test.size() == 0)
				return sol::lua_nil;
			if (!Is_1DTable(test))
				return sol::lua_nil;

			sol::state_view lua(s);
			std::vector<sol::object> v = t.as<std::vector<sol::object>>();
			std::vector<sol::object> v2 = Shuffle(v);
			sol::table t2 = lua.create_table();
			for (const sol::object& i : v2) { t2.add(i); }
			auto copy = [&t](sol::table src)
				{
					//清空原表
					t.as<sol::table>().clear();
					//將篩選後的表複製到原表
					for (const std::pair<sol::object, sol::object>& i : src)
					{
						t.as<sol::table>().add(i.second);
					}
				};
			copy(t2);
			return t2;
		};

	lua["trotate"] = [](sol::object t, sol::object oside, sol::this_state s)->sol::object
		{
			if (!t.is<sol::table>())
				return sol::lua_nil;

			long long len = 1;
			if (oside == sol::lua_nil)
				len = 1;
			else if (oside.is<long long>())
				len = oside.as<long long>();
			else
				return sol::lua_nil;

			sol::table test = t.as<sol::table>();
			if (test.size() == 0)
				return sol::lua_nil;
			if (!Is_1DTable(test))
				return sol::lua_nil;

			sol::state_view lua(s);
			std::vector<sol::object> v = t.as<std::vector<sol::object>>();
			std::vector<sol::object> v2 = Rotate(v, len);
			sol::table t2 = lua.create_table();
			for (const sol::object& i : v2) { t2.add(i); }
			auto copy = [&t](sol::table src)
				{
					//清空原表
					t.as<sol::table>().clear();
					//將篩選後的表複製到原表
					for (const std::pair<sol::object, sol::object>& i : src)
					{
						t.as<sol::table>().add(i.second);
					}
				};
			copy(t2);
			return t2;
		};

	lua["tsleft"] = [](sol::object t, long long i, sol::this_state s)->sol::object
		{
			if (!t.is<sol::table>())
				return sol::lua_nil;
			if (i < 0)
				return sol::lua_nil;

			sol::table test = t.as<sol::table>();
			if (test.size() == 0)
				return sol::lua_nil;
			if (!Is_1DTable(test))
				return sol::lua_nil;

			sol::state_view lua(s);
			std::vector<sol::object> v = t.as<std::vector<sol::object>>();
			std::vector<sol::object> v2 = ShiftLeft(v, i);
			sol::table t2 = lua.create_table();
			for (const sol::object& it : v2) { t2.add(it); }
			auto copy = [&t](sol::table src)
				{
					//清空原表
					t.as<sol::table>().clear();
					//將篩選後的表複製到原表
					for (const std::pair<sol::object, sol::object>& i : src)
					{
						t.as<sol::table>().add(i.second);
					}
				};
			copy(t2);
			return t2;
		};

	lua["tsright"] = [](sol::object t, long long i, sol::this_state s)->sol::object
		{
			if (!t.is<sol::table>())
				return sol::lua_nil;
			if (i < 0)
				return sol::lua_nil;

			sol::table test = t.as<sol::table>();
			if (test.size() == 0)
				return sol::lua_nil;
			if (!Is_1DTable(test))
				return sol::lua_nil;

			sol::state_view lua(s);
			std::vector<sol::object> v = t.as<std::vector<sol::object>>();
			std::vector<sol::object> v2 = ShiftRight(v, i);
			sol::table t2 = lua.create_table();
			for (const sol::object& o : v2) { t2.add(o); }
			auto copy = [&t](sol::table src)
				{
					//清空原表
					t.as<sol::table>().clear();
					//將篩選後的表複製到原表
					for (const std::pair<sol::object, sol::object>& o : src)
					{
						t.as<sol::table>().add(o.second);
					}
				};
			copy(t2);
			return t2;
		};

	lua["tunique"] = [](sol::object t, sol::this_state s)->sol::object
		{
			if (!t.is<sol::table>())
				return sol::lua_nil;

			sol::table test = t.as<sol::table>();
			if (test.size() == 0)
				return sol::lua_nil;

			auto isIntTable = [&test]()->bool
				{
					for (const std::pair<sol::object, sol::object>& i : test)
					{
						if (!i.second.is<long long>())
							return false;
					}
					return true;
				};

			auto isStringTable = [&test]()->bool
				{
					for (const std::pair<sol::object, sol::object>& i : test)
					{
						if (!i.second.is<std::string>())
							return false;
					}
					return true;
				};

			auto copy = [&t](sol::table src)
				{
					//清空原表
					t.as<sol::table>().clear();
					//將篩選後的表複製到原表
					for (const std::pair<sol::object, sol::object>& i : src)
					{
						t.as<sol::table>().add(i.second);
					}
				};

			sol::state_view lua(s);
			sol::table t2 = lua.create_table();
			if (isIntTable())
			{
				std::vector<long long> v = t.as<std::vector<long long>>();
				std::vector<long long> v2 = Unique(v);
				for (const long long& i : v2) { t2.add(i); }
				copy(t2);
				return t2;
			}
			else if (isStringTable())
			{
				std::vector<std::string> v = t.as<std::vector<std::string>>();
				std::vector<std::string> v2 = Unique(v);
				for (const std::string& i : v2) { t2.add(i); }
				copy(t2);
				return t2;
			}
			else
				return sol::lua_nil;
		};

	lua["tsort"] = [](sol::object t, sol::this_state s)->sol::object
		{
			sol::state_view lua(s);
			if (!t.is<sol::table>())
				return sol::lua_nil;

			sol::protected_function sort = lua["table"]["sort"];
			if (sort.valid())
			{
				sort(t);
			}
			return t;
		};

	lua.safe_script(R"(
		trsort = function(t)
			local function reverseSort(a, b)
				if type(a) == "string" and type(b) == "string" then
					return utf8.codepoint(a) > utf8.codepoint(b)
				elseif type(a) == "number" and type(b) == "number" then
					return a > b
				elseif type(a) == "boolean" and type(b) == "boolean" then
					return a > b
				elseif type(a) == "table" and type(b) == "table" then
					return #a > #b
				end

				return a > b
			end

			table.sort(t, reverseSort)
			return t
		end

	)", sol::script_pass_on_error);

	lua.collect_garbage();

#pragma region _copyfunstr
	std::string _copyfunstr = R"(
		function copy(object)
			local lookup_table = {};
			local function _copy(object)
				if (type(object) ~= ("table")) then
					
					return object;
				elseif (lookup_table[object]) then
					return lookup_table[object];
				end
				local newObject = {};
				lookup_table[object] = newObject;
				for key, value in pairs(object) do
					newObject[_copy(key)] = _copy(value);
				end
				return setmetatable(newObject, getmetatable(object));
			end

			local ret = _copy(object);
			return ret;
		end
	)";

	sol::load_result lr = lua_.load(_copyfunstr);
	if (lr.valid())
	{
		sol::protected_function target = lr.get<sol::protected_function>();
		sol::bytecode target_bc = target.dump();
		lua.safe_script(target_bc.as_string_view(), sol::script_pass_on_error);
		lua.collect_garbage();
	}
#pragma endregion

	//表合併
	lua["tmerge"] = [](sol::object t1, sol::object t2, sol::this_state s)->sol::object
		{
			if (!t1.is<sol::table>() || !t2.is<sol::table>())
				return sol::lua_nil;

			sol::state_view lua(s);
			sol::table t3 = lua.create_table();
			sol::table lookup_table_1 = lua.create_table();
			sol::table lookup_table_2 = lua.create_table();
			//sol::protected_function _copy = lua["copy"];
			sol::table test1 = t1.as<sol::table>();//_copy(t1, lookup_table_1, lua);
			sol::table test2 = t2.as<sol::table>();//_copy(t2, lookup_table_2, lua);
			if (!test1.valid() || !test2.valid())
				return sol::lua_nil;
			for (const std::pair<sol::object, sol::object>& i : test1)
			{
				t3.add(i.second);
			}
			for (const std::pair<sol::object, sol::object>& i : test2)
			{
				t3.add(i.second);
			}
			auto copy = [&t1](sol::table src)
				{
					//清空原表
					t1.as<sol::table>().clear();
					//將篩選後的表複製到原表
					for (const std::pair<sol::object, sol::object>& i : src)
					{
						t1.as<sol::table>().add(i.second);
					}
				};
			copy(t3);
			return t3;
		};

	lua.set_function("split", [](std::string src, std::string del, sol::object skipEmpty, sol::object orex, sol::this_state s)->sol::object
		{
			sol::state_view lua(s);
			sol::table t = lua.create_table();
			QString qsrc = util::toQString(src);
			QString qdel = util::toQString(del);
			QStringList v;
			bool useRex = false;
			if (orex.is<bool>())
				useRex = orex.as<bool>();
			else if (orex.is<long long>())
				useRex = orex.as<long long>() > 0;
			else if (orex.is<double>())
				useRex = orex.as<double>() > 0.0;

			bool skip = true;
			if (skipEmpty.is<bool>())
				skip = skipEmpty.as<bool>();
			else if (skipEmpty.is<long long>())
				skip = skipEmpty.as<long long>() > 0;
			else if (skipEmpty.is<double>())
				skip = skipEmpty.as<double>() > 0.0;

			if (useRex)
			{
				const QRegularExpression re(qdel);
				v = qsrc.split(re, skip ? Qt::SkipEmptyParts : Qt::KeepEmptyParts);
			}
			else
				v = qsrc.split(qdel, skip ? Qt::SkipEmptyParts : Qt::KeepEmptyParts);

			if (v.size() > 1)
			{
				for (const QString& i : v)
				{
					t.add(util::toConstData(i));
				}
				return t;
			}
			else
				return sol::lua_nil;
		});

	//根據key交換表中的兩個元素
	lua["tswap"] = [](sol::table t, sol::object key1, sol::object key2, sol::this_state s)->sol::object
		{
			if (!t.valid())
				return sol::lua_nil;
			if (!t[key1].valid() || !t[key2].valid())
				return sol::lua_nil;
			sol::object temp = t[key1];
			t[key1] = t[key2];
			t[key2] = temp;
			return t;
		};

	lua["tadd"] = [](sol::table t, sol::object value, sol::this_state s)->sol::object
		{
			if (!t.valid())
				return sol::lua_nil;
			t.add(value);
			return t;
		};

	lua["tpadd"] = [](sol::table t, sol::object value, sol::this_state s)->sol::object
		{
			sol::state_view lua(s);
			if (!t.valid())
				return sol::lua_nil;
			sol::function insert = lua["table"]["insert"];
			insert(t, 1, value);
			return t;
		};

	lua["tpopback"] = [](sol::table t, sol::this_state s)->sol::object
		{
			sol::state_view lua(s);
			if (!t.valid())
				return sol::lua_nil;
			sol::function remove = lua["table"]["remove"];
			sol::object temp = t[t.size()];
			remove(t, t.size());
			return t;
		};

	lua["tpopfront"] = [](sol::table t, sol::this_state s)->sol::object
		{
			sol::state_view lua(s);
			if (!t.valid())
				return sol::lua_nil;
			sol::function remove = lua["table"]["remove"];
			sol::object temp = t[1];
			remove(t, 1);
			return t;
		};

	lua["tfront"] = [](sol::table t, sol::this_state s)->sol::object
		{
			if (!t.valid())
				return sol::lua_nil;
			return t[1];
		};

	lua["tback"] = [](sol::table t, sol::this_state s)->sol::object
		{
			if (!t.valid())
				return sol::lua_nil;
			return t[t.size()];
		};
#pragma endregion

	lua.set_function("rungame", [this](long long id)->long long
		{
			GameDevice& gamedevice = GameDevice::getInstance(getIndex());
			InterfaceSender sender(gamedevice.getParentWidget());

			return sender.RunGame(id);
		});

	lua.set_function("closegame", [this](long long id)->long long
		{
			GameDevice& gamedevice = GameDevice::getInstance(getIndex());
			InterfaceSender sender(gamedevice.getParentWidget());

			return sender.CloseGame(id);
		});

	lua.set_function("openwindow", [this](long long id)->long long
		{
			GameDevice& gamedevice = GameDevice::getInstance(getIndex());
			InterfaceSender sender(gamedevice.getParentWidget());

			return sender.OpenNewWindow(id);
		});

	lua.set_function("runex", [this](long long id, std::string path)->long long
		{
			GameDevice& gamedevice = GameDevice::getInstance(getIndex());
			InterfaceSender sender(gamedevice.getParentWidget());

			return sender.RunFile(id, util::toQString(path));
		});

	lua.set_function("stoprunex", [this](long long id)->long long
		{
			GameDevice& gamedevice = GameDevice::getInstance(getIndex());
			InterfaceSender sender(gamedevice.getParentWidget());

			return sender.StopFile(id);
		});

	lua.set_function("dostrex", [this](long long id, std::string content)->long long
		{
			GameDevice& gamedevice = GameDevice::getInstance(getIndex());
			InterfaceSender sender(gamedevice.getParentWidget());

			return sender.RunScript(id, util::toQString(content));
		});

	lua.set_function("loadsetex", [this](long long id, std::string content)->long long
		{
			GameDevice& gamedevice = GameDevice::getInstance(getIndex());
			InterfaceSender sender(gamedevice.getParentWidget());

			return sender.LoadSettings(id, util::toQString(content));
		});

	lua_.set_function("msg", [this](sol::object otext, sol::object otitle, sol::object otype, sol::this_state s)->std::string
		{
			QString text;
			if (otext.is<std::string>())
				text = util::toQString(otext);
			else if (otext.is<long long>())
				text = util::toQString(otext.as<long long>());
			else if (otext.is<double>())
				text = util::toQString(otext.as<double>());
			else if (otext.is<bool>())
				text = util::toQString(otext.as<bool>());
			else
			{
				return "";
			}

			QString title;
			if (otitle.is<std::string>())
				title = util::toQString(otitle);
			else if (otitle.is<long long>())
				title = util::toQString(otitle.as<long long>());
			else if (otitle.is<double>())
				title = util::toQString(otitle.as<double>());
			else if (otitle.is<bool>())
				title = util::toQString(otitle.as<bool>());
			else
			{
				return "";
			}

			long long type = 1;
			if (otype.is<long long>())
				type = otype.as<long long>();

			SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(getIndex());
			long long nret = QMessageBox::StandardButton::NoButton;
			emit signalDispatcher.messageBoxShow(text, type, title, &nret);
			return nret == QMessageBox::StandardButton::Yes ? "yes" : "no";
		});

	lua.set_function("dlg", [this](std::string buttonstr, std::string stext, sol::object otype, sol::object otimeout, sol::this_state s)->sol::object
		{
			GameDevice& gamedevice = GameDevice::getInstance(getIndex());
			sol::state_view lua_(s);

			if (gamedevice.worker.isNull())
				return sol::lua_nil;

			QString buttonStrs = util::toQString(buttonstr);

			QString text = util::toQString(stext);

			long long timeout = 5000;
			if (otype.is<long long>())
				timeout = otype.as<long long>();

			if (otimeout.is<long long>())
				timeout = otimeout.as<long long>();

			unsigned long long type = 2;
			if (otype.is<unsigned long long>())
				type = otype.as<unsigned long long>();


			luadebug::checkOnlineThenWait(s);
			luadebug::checkBattleThenWait(s);

			buttonStrs = buttonStrs.toUpper();
			QStringList buttonStrList = buttonStrs.split(util::rexOR, Qt::SkipEmptyParts);
			safe::vector<long long> buttonVec;
			unsigned long long buttonFlag = 0;
			for (const QString& str : buttonStrList)
			{
				if (!sa::buttonMap.contains(str))
				{
					luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("invalid button string: %1").arg(str));
					return sol::lua_nil;
				}
				unsigned long long value = sa::buttonMap.value(str);
				buttonFlag |= value;
			}

			gamedevice.worker->IS_WAITFOR_CUSTOM_DIALOG_FLAG.on();
			gamedevice.worker->createRemoteDialog(type, buttonFlag, text);
			bool bret = false;
			util::timer timer;
			for (;;)
			{
				if (gamedevice.IS_SCRIPT_INTERRUPT.get())
					return sol::lua_nil;
				if (timer.hasExpired(timeout))
					break;

				if (!gamedevice.worker->IS_WAITFOR_CUSTOM_DIALOG_FLAG.get())
				{
					bret = true;
					break;
				}
				QThread::msleep(200);
			}
			gamedevice.worker->IS_WAITFOR_CUSTOM_DIALOG_FLAG.off();

			if (!bret)
			{
				return sol::lua_nil;
			}

			QHash<QString, sa::ButtonType> big5 = {
				{ "OK", sa::kButtonOk},
				{ "CANCEL", sa::kButtonCancel },
				{ "CLOSE",sa::kButtonClose },
				{ "關閉",sa::kButtonClose },
				//big5
				{ "確定", sa::kButtonYes },
				{ "取消", sa::kButtonNo },
				{ "上一頁",sa::kButtonPrevious },
				{ "下一頁",sa::kButtonNext },
				{ "取消",sa::kButtonNo },
			};

			QHash<QString, sa::ButtonType> gb2312 = {
				{ "OK",sa::kButtonOk},
				{ "CANCEL",sa::kButtonCancel },
				{ "关闭", sa::kButtonClose },
				//gb2312
				{ "确定", sa::kButtonYes },
				{ "取消", sa::kButtonNo },
				{ "上一页",sa::kButtonPrevious },
				{ "下一页", sa::kButtonNext },
			};

			sa::custom_dialog_t dialog = gamedevice.worker->customDialog.get();

			QString sbtype;
			sbtype = big5.key(dialog.button, "");
			if (sbtype.isEmpty())
				sbtype = gb2312.key(dialog.button, "");

			if (sbtype.isEmpty())
				sbtype = "0x" + util::toQString(static_cast<long long>(dialog.rawbutton), 16);

			sol::table t = lua_.create_table();

			t["row"] = dialog.row;

			t["button"] = util::toConstData(sbtype);

			t["data"] = util::toConstData(dialog.rawdata);

			return t;
		});

	lua.set_function("input", [this](sol::object oargs, sol::this_state s)->sol::object
		{
			SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(getIndex());

			std::string sargs = "";
			if (oargs.is<std::string>())
				sargs = oargs.as<std::string>();

			QString args = util::toQString(sargs);

			QStringList argList = args.split(util::rexOR, Qt::SkipEmptyParts);
			long long type = QInputDialog::InputMode::TextInput;
			QString msg;
			QVariant var;
			bool ok = false;
			if (argList.size() > 0)
			{
				type = argList.front().toLongLong(&ok) - 1ll;
				if (type < 0 || type > 2)
					type = QInputDialog::InputMode::TextInput;
			}

			if (argList.size() > 1)
			{
				msg = argList.value(1);
			}

			if (argList.size() > 2)
			{
				if (type == QInputDialog::IntInput)
					var = QVariant(argList.value(2).toLongLong(&ok));
				else if (type == QInputDialog::DoubleInput)
					var = QVariant(argList.value(2).toDouble(&ok));
				else
					var = QVariant(argList.value(2));
			}

			emit signalDispatcher.inputBoxShow(msg, type, &var);

			if (var.toLongLong() == 987654321ll)
			{
				luadebug::showErrorMsg(s, luadebug::WARN_LEVEL, QObject::tr("force stop by user input stop code"));
				GameDevice& gamedevice = GameDevice::getInstance(getIndex());
				gamedevice.stopScript();
				return sol::lua_nil;
			}

			if (type == QInputDialog::IntInput)
			{
				return sol::make_object(s, var.toLongLong());
			}
			else if (type == QInputDialog::DoubleInput)
			{
				return sol::make_object(s, var.toDouble());
			}
			else
			{
				QString str = var.toString();
				if (str.toLower() == "true" || str.toLower() == "false" || str.toLower() == "真" || str.toLower() == "假")
					return sol::make_object(s, var.toBool());

				return sol::make_object(s, util::toConstData(var.toString()));
			}
		});

	lua.set_function("half", [this](std::string sstr, sol::this_state s)->std::string
		{
			const QString FullWidth = "０１２３４５６７８９"
				"ａｂｃｄｅｆｇｈｉｊｋｌｍｎｏｐｑｒｓｔｕｖｗｘｙｚ"
				"ＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯＰＱＲＳＴＵＶＷＸＹＺ"
				"、～！＠＃＄％︿＆＊（）＿－＝＋［］｛｝＼｜；：’＂，＜．＞／？【】《》　";
			const QString HalfWidth = "0123456789"
				"abcdefghijklmnopqrstuvwxyz"
				"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
				"'~!@#$%^&*()_-+=[]{}\\|;:'\",<.>/? []<>";

			if (sstr.empty())
				return sstr;

			QString result = util::toQString(sstr);
			long long size = FullWidth.size();
			for (long long i = 0; i < size; ++i)
			{
				result.replace(FullWidth.at(i), HalfWidth.at(i));
			}

			return util::toConstData(result);
		});

	lua.set_function("full", [this](std::string sstr, sol::this_state s)->std::string
		{
			const QString FullWidth = "０１２３４５６７８９"
				"ａｂｃｄｅｆｇｈｉｊｋｌｍｎｏｐｑｒｓｔｕｖｗｘｙｚ"
				"ＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯＰＱＲＳＴＵＶＷＸＹＺ"
				"、～！＠＃＄％︿＆＊（）＿－＝＋［］｛｝＼｜；：’＂，＜．＞／？【】《》　";
			const QString HalfWidth = "0123456789"
				"abcdefghijklmnopqrstuvwxyz"
				"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
				"'~!@#$%^&*()_-+=[]{}\\|;:'\",<.>/?[]<> ";

			if (sstr.empty())
				return sstr;

			QString result = util::toQString(sstr);
			long long size = FullWidth.size();
			for (long long i = 0; i < size; ++i)
			{
				result.replace(HalfWidth.at(i), FullWidth.at(i));
			}

			return util::toConstData(result);
		});

	lua.set_function("lower", [this](std::string sstr, sol::this_state s)->std::string
		{
			QString result = util::toQString(sstr).toLower();

			return util::toConstData(result);
		});

	lua.set_function("upper", [this](std::string sstr, sol::this_state s)->std::string
		{
			QString result = util::toQString(sstr).toUpper();

			return util::toConstData(result);
		});

	lua.set_function("trim", [this](std::string sstr, sol::object oisSimplified, sol::this_state s)->std::string
		{
			bool isSimplified = false;

			if (oisSimplified.is<bool>())
				isSimplified = oisSimplified.as<bool>();
			else if (oisSimplified.is<long long>())
				isSimplified = oisSimplified.as<long long>() > 0;
			else if (oisSimplified.is<double>())
				isSimplified = oisSimplified.as<double>() > 0.0;

			QString result = util::toQString(sstr).trimmed();

			if (isSimplified)
				result = result.simplified();

			return util::toConstData(result);
		});

	lua.set_function("todb", [this](sol::object ovalue, sol::object len, sol::this_state s)->double
		{
			double result = 0.0;
			if (ovalue.is<std::string>())
				result = util::toQString(ovalue).toDouble();
			else if (ovalue.is<long long>())
				result = static_cast<double>(ovalue.as<long long>() * 1.0);
			else if (ovalue.is<double>())
				result = ovalue.as<double>();

			if (len.is<long long>() && len.as<long long>() >= 0 && len.as<long long>() <= 16)
			{
				QString str = QString::number(result, 'f', len.as<long long>());
				result = str.toDouble();
			}

			return result;
		});

	lua.set_function("tostr", [this](sol::object ovalue, sol::this_state s)->std::string
		{
			QString result = "";
			if (ovalue.is<std::string>())
				result = util::toQString(ovalue);
			else if (ovalue.is<long long>())
				result = util::toQString(ovalue.as<long long>());
			else if (ovalue.is<double>())
			{
				result = util::toQString(ovalue.as<double>());
				while (!result.isEmpty() && result.back() == '0')
					result.chop(1);
			}

			return util::toConstData(result);
		});

	lua.set_function("toint", [this](sol::object ovalue, sol::this_state s)->long long
		{
			long long result = 0;
			if (ovalue.is<std::string>())
				result = util::toQString(ovalue).toLongLong();
			else if (ovalue.is<long long>())
				result = ovalue.as<long long>();
			else if (ovalue.is<double>())
				result = static_cast<long long>(qFloor(ovalue.as<double>()));

			return result;
		});

	lua.set_function("replace", [this](std::string ssrc, std::string sfrom, std::string sto, sol::object oisRex, sol::this_state s)->std::string
		{
			QString src = util::toQString(ssrc);
			QString from = util::toQString(sfrom);
			QString to = util::toQString(sto);

			bool isRex = false;
			if (oisRex.is<bool>())
				isRex = oisRex.as<bool>();
			else if (oisRex.is<long long>())
				isRex = oisRex.as<long long>() > 0;
			else if (oisRex.is<double>())
				isRex = oisRex.as<double>() > 0.0;

			QString result;
			if (!isRex)
				result = src.replace(from, to);
			else
			{
				const QRegularExpression regex(from);
				result = src.replace(regex, to);
			}

			return util::toConstData(result);
		});

	lua.set_function("find", [this](std::string ssrc, std::string sfrom, sol::object osto, sol::this_state s)->std::string
		{
			QString varValue = util::toQString(ssrc);
			QString text1 = util::toQString(sfrom);
			if (ssrc.empty() || sfrom.empty())
				return "";

			QString text2 = "";
			if (osto.is<std::string>())
				text2 = util::toQString(osto);

			QString result = varValue;

			//查找 src 中 text1 到 text2 之间的文本 如果 text2 为空 则查找 text1 到行尾的文本

			long long pos1 = varValue.indexOf(text1);
			if (pos1 < 0)
				pos1 = 0;

			long long pos2 = -1;
			if (text2.isEmpty())
				pos2 = varValue.length();
			else
			{
				pos2 = static_cast<long long>(varValue.indexOf(text2, pos1 + text1.length()));
				if (pos2 < 0)
					pos2 = varValue.length();
			}

			result = varValue.mid(pos1 + text1.length(), pos2 - pos1 - text1.length());

			return util::toConstData(result);
		});

	//参数1:字符串内容, 参数2:正则表达式, 参数3(选填):第几个匹配项, 参数4(选填):是否为全局匹配, 参数5(选填):第几组
	lua.set_function("regex", [this](std::string ssrc, std::string rexstr, sol::object oidx, sol::object oisglobal, sol::object ogidx, sol::this_state s)->std::string
		{
			QString varValue = util::toQString(ssrc);

			QString result = varValue;

			QString text = util::toQString(rexstr);

			long long capture = 1;
			if (oidx.is<long long>())
				capture = oidx.as<long long>();

			bool isGlobal = false;
			if (oisglobal.is<long long>())
				isGlobal = oisglobal.as<long long>() > 0;
			else if (oisglobal.is<bool>())
				isGlobal = oisglobal.as<bool>();

			long long maxCapture = 0;
			if (ogidx.is<long long>())
				maxCapture = ogidx.as<long long>();

			const QRegularExpression regex(text);

			if (!isGlobal)
			{
				QRegularExpressionMatch match = regex.match(varValue);
				if (match.hasMatch())
				{
					if (capture < 0 || capture > match.lastCapturedIndex())
					{

						return util::toConstData(result);
					}

					result = match.captured(capture);
				}
			}
			else
			{
				QRegularExpressionMatchIterator matchs = regex.globalMatch(varValue);
				long long n = 0;
				while (matchs.hasNext())
				{
					QRegularExpressionMatch match = matchs.next();
					if (++n != maxCapture)
						continue;

					if (capture < 0 || capture > match.lastCapturedIndex())
						continue;

					result = match.captured(capture);
					break;
				}
			}

			return util::toConstData(result);
		});

	//rex 参数1:来源字符串, 参数2:正则表达式
	lua.set_function("rex", [this](std::string ssrc, std::string rexstr, sol::this_state s)->sol::table
		{
			QString src = util::toQString(ssrc);
			sol::state_view lua(s);
			sol::table result = lua.create_table();

			QString expr = util::toQString(rexstr);

			const QRegularExpression regex(expr);

			QRegularExpressionMatch match = regex.match(src);

			long long n = 1;
			if (match.hasMatch())
			{
				for (long long i = 0; i <= match.lastCapturedIndex(); ++i)
				{
					result[n] = util::toConstData(match.captured(i));
				}
			}

			long long maxdepth = kMaxLuaTableDepth;

			return result;
		});

	lua.set_function("rexg", [this](std::string ssrc, std::string rexstr, sol::this_state s)->sol::table
		{
			QString src = util::toQString(ssrc);
			sol::state_view lua(s);
			sol::table result = lua.create_table();

			QString expr = util::toQString(rexstr);

			const QRegularExpression regex(expr);

			QRegularExpressionMatchIterator matchs = regex.globalMatch(src);

			long long n = 1;
			while (matchs.hasNext())
			{
				QRegularExpressionMatch match = matchs.next();

				if (!match.hasMatch())
					continue;

				for (const auto& capture : match.capturedTexts())
				{
					result[n] = util::toConstData(capture);
					++n;
				}
			}

			long long maxdepth = kMaxLuaTableDepth;

			return result;
		});

	lua.set_function("rnd", [this](sol::object omin, sol::object omax, sol::this_state s)->long long
		{
			long long result = 0;
			long long min = 0;
			if (omin.is<long long>())
				min = omin.as<long long>();

			long long max = 0;
			if (omax.is<long long>())
				max = omax.as<long long>();

			if (omin == sol::lua_nil && omax == sol::lua_nil || min == 0 && max == 0)
			{
				util::rnd::get(&result);

				return result;
			}

			if ((min > 0 && max == 0) || (min == max))
			{
				util::rnd::get(&result, 0LL, min);
			}
			else if (min > max)
			{
				util::rnd::get(&result, max, min);
			}
			else
			{
				util::rnd::get(&result, min, max);
			}

			return result;
		});

	lua["tjoin"] = [this](sol::table t, std::string del, sol::this_state s)->std::string
		{
			QStringList l = {};
			for (const std::pair<sol::object, sol::object>& i : t)
			{
				if (i.second.is<long long>())
				{
					l.append(util::toQString(i.second.as<long long>()));
				}
				else if (i.second.is<double>())
				{
					l.append(util::toQString(i.second.as<double>()));
				}
				else if (i.second.is<bool>())
				{
					l.append(util::toQString(i.second.as<bool>()));
				}
				else if (i.second.is<std::string>())
				{
					l.append(util::toQString(i.second));
				}
			}
			QString result = l.join(util::toQString(del));

			return  util::toConstData(result);
		};
}

//luasystem.cpp
void CLua::open_syslibs(sol::state& lua)
{
	lua.set_function("capture", &CLuaSystem::capture, &luaSystem_);

	lua.set_function("__require", &CLuaSystem::require, &luaSystem_);
	lua.safe_script("require = __require;", sol::script_pass_on_error);


	lua.set_function("setglobal", &CLuaSystem::setglobal, &luaSystem_);
	lua.set_function("getglobal", &CLuaSystem::getglobal, &luaSystem_);
	lua.set_function("clearglobal", &CLuaSystem::clearglobal, &luaSystem_);

	lua.set_function("send", &CLuaSystem::send, &luaSystem_);
	lua.set_function("sleep", &CLuaSystem::sleep, &luaSystem_);
	lua.set_function("openlog", &CLuaSystem::openlog, &luaSystem_);
	lua.set_function("__print", &CLuaSystem::print, &luaSystem_);

	lua.set_function("printf", [](sol::object ovalue, sol::object ocolor, sol::this_state s)->long long
		{
			sol::state_view lua(s);
			if (!ocolor.is<long long>() || ocolor.as<long long>() < -2 || ocolor.as<long long>() > 11)
			{
				ocolor = sol::make_object(lua, sol::lua_nil);
			}

			sol::protected_function print = lua["__print"];
			if (!print.valid())
				return 0;

			sol::protected_function_result result;

			do
			{
				if (!ovalue.is<std::string>())
					break;

				std::string src = ovalue.as<std::string>();

				sol::protected_function format = lua["format"];
				if (!format.valid()
					|| (src.find("{:") == std::string::npos && src.find("}") == std::string::npos))
					break;

				sol::object o = format.call(src);
				if (!o.valid())
					break;

				result = print.call(o, ocolor);
				return result.valid() ? 1 : 0;
			} while (false);

			result = print.call(ovalue, ocolor);

			return result.valid() ? 1 : 0;
		});

	lua.set_function("setThreadAffinityMask", [](DWORD_PTR  mask)->DWORD_PTR
		{
			if (mask == 0)
				return 0;
			return SetThreadAffinityMask(GetCurrentThread(), mask);
		});

	//直接覆蓋print會無效,改成在腳本內中轉覆蓋
	lua.safe_script(R"(
		__oldprint = print;
		print = printf;
	)", sol::script_pass_on_error);
	lua.collect_garbage();

	lua.set_function("msg", &CLuaSystem::messagebox, &luaSystem_);
	lua.set_function("saveset", &CLuaSystem::savesetting, &luaSystem_);
	lua.set_function("loadset", &CLuaSystem::loadsetting, &luaSystem_);
	lua.set_function("set", &CLuaSystem::set, &luaSystem_);
	lua.set_function("lclick", &CLuaSystem::leftclick, &luaSystem_);
	lua.set_function("rclick", &CLuaSystem::rightclick, &luaSystem_);
	lua.set_function("dbclick", &CLuaSystem::leftdoubleclick, &luaSystem_);
	lua.set_function("dragto", &CLuaSystem::mousedragto, &luaSystem_);

	lua.set_function("createch", &CLuaSystem::createch, &luaSystem_);
	lua.set_function("delch", &CLuaSystem::delch, &luaSystem_);
	lua.set_function("menu", &CLuaSystem::menu, &luaSystem_);

	lua.set_function("say", &CLuaSystem::talk, &luaSystem_);
	lua.set_function("cls", &CLuaSystem::cleanchat, &luaSystem_);
	lua.set_function("logout", &CLuaSystem::logout, &luaSystem_);
	lua.set_function("logback", &CLuaSystem::logback, &luaSystem_);
	lua.set_function("eo", &CLuaSystem::eo, &luaSystem_);
	lua.set_function("button", &CLuaSystem::press, &luaSystem_);

	lua.set_function("chname", &CLuaSystem::chname, &luaSystem_);
	lua.set_function("chpetname", &CLuaSystem::chpetname, &luaSystem_);
	lua.set_function("chpet", &CLuaSystem::chpet, &luaSystem_);

	lua.set_function("waitpos", &CLuaSystem::waitpos, &luaSystem_);
	lua.set_function("waitmap", &CLuaSystem::waitmap, &luaSystem_);
	lua.set_function("waitpet", &CLuaSystem::waitpet, &luaSystem_);
	lua.set_function("waititem", &CLuaSystem::waititem, &luaSystem_);
	lua.set_function("waitteam", &CLuaSystem::waitteam, &luaSystem_);
	lua.set_function("waitdlg", &CLuaSystem::waitdlg, &luaSystem_);
	lua.set_function("waitsay", &CLuaSystem::waitsay, &luaSystem_);

	lua.set_function("WORLD", [](sol::this_state s) ->long long
		{
			sol::state_view lua(s);
			GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
			if (gamedevice.worker.isNull())
				return 0;

			return gamedevice.worker->getWorldStatus();
		});

	lua.set_function("GAME", [](sol::this_state s) ->long long
		{
			sol::state_view lua(s);
			GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
			if (gamedevice.worker.isNull())
				return 0;

			return gamedevice.worker->getGameStatus();
		});

	lua.set_function("isonline", [](sol::this_state s) ->bool
		{
			sol::state_view lua(s);
			GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
			if (gamedevice.worker.isNull())
				return false;

			return gamedevice.worker->getOnlineFlag();
		});

	lua.set_function("isbattle", [](sol::this_state s) ->bool
		{
			sol::state_view lua(s);
			GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
			if (gamedevice.worker.isNull())
				return false;

			return gamedevice.worker->getBattleFlag();
		});

	lua.set_function("isnormal", [](sol::this_state s) ->bool
		{
			sol::state_view lua(s);
			GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
			if (gamedevice.worker.isNull())
				return false;

			return !gamedevice.worker->getBattleFlag();
		});

	lua.set_function("isdialog", [](sol::this_state s) ->bool
		{
			sol::state_view lua(s);
			GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
			if (gamedevice.worker.isNull())
				return false;

			return gamedevice.worker->isDialogVisible();
		});

	lua.set_function("gettime", [](sol::this_state s) ->std::string
		{
			sol::state_view lua(s);
			GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
			if (gamedevice.worker.isNull())
				return "";

			long long satime = gamedevice.worker->getSaTime();

			//0~255 sa::kAfternoon
			if (satime >= sa::kAfternoon && satime < sa::kDusk)
				return util::toConstData("afternoon");
			else if (satime >= sa::kDusk && satime < sa::kMidnight)
				return util::toConstData("dusk");
			else if (satime >= sa::kMidnight && satime < sa::kMorning)
				return util::toConstData("midnight");
			else if (satime >= sa::kMorning && satime < sa::kNoon)
				return util::toConstData("morning");
			else if (satime >= sa::kNoon)
				return util::toConstData("noon");

			return util::toConstData(QObject::tr("unknown"));
		});

	lua.set_function("isvalid", [](sol::this_state s) ->bool
		{
			sol::state_view lua(s);
			GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
			return gamedevice.worker.isNull();
		});

	lua.new_usertype<sa::dialog_t>("DialogStruct",
		"type", sol::readonly(&sa::dialog_t::windowtype),
		"button", sol::readonly(&sa::dialog_t::buttontype),
		"id", sol::readonly(&sa::dialog_t::dialogid),
		"unitid", sol::readonly(&sa::dialog_t::unitid),
		"data", sol::property(&sa::dialog_t::getData),
		"buttontext", sol::property(&sa::dialog_t::getLineButtonText)
	);

	lua.new_usertype<CLuaDialog>("DialogClass",
		sol::call_constructor,
		sol::constructors<CLuaDialog(long long)>(),
		sol::meta_function::index, &CLuaDialog::operator[],
		"contains", &CLuaDialog::contains,
		"valid", sol::property(&CLuaDialog::isValid),
		"type", sol::property(&CLuaDialog::getWindowType),
		"button", sol::property(&CLuaDialog::getButtonType),
		"id", sol::property(&CLuaDialog::getDialogId),
		"unitid", sol::property(&CLuaDialog::getUnitId),
		"data", sol::property(&CLuaDialog::getData),
		"buttontext", sol::property(&CLuaDialog::getButtonText)
	);


	lua.safe_script("dialog = DialogClass(__INDEX);", sol::script_pass_on_error);
	lua.collect_garbage();

	lua.new_usertype<CLuaChat>("ChatClass",
		sol::call_constructor,
		sol::constructors<CLuaChat(long long)>(),
		sol::meta_function::index, &CLuaChat::operator[],
		"contains", &CLuaChat::contains,
		"包含", &CLuaChat::contains
	);

	lua.safe_script("chat = ChatClass(__INDEX);", sol::script_pass_on_error);
	lua.collect_garbage();

	lua.new_usertype<CLuaTimer>("Timer",
		sol::call_constructor,
		sol::constructors<CLuaTimer()>(),

		"restart", &CLuaTimer::restart,
		"expired", &CLuaTimer::hasExpired,
		"getstr", &CLuaTimer::toFormatedString,
		"tostr", &CLuaTimer::toString,
		"todb", &CLuaTimer::toDouble,
		"msec", sol::property(&CLuaTimer::cost),
		"sec", sol::property(&CLuaTimer::costSeconds),
		"重置", &CLuaTimer::restart,
		"超时", &CLuaTimer::hasExpired,
		"取格式", &CLuaTimer::toFormatedString,
		"到字符串", &CLuaTimer::toString,
		"到浮点数", &CLuaTimer::toDouble,
		"毫秒", sol::property(&CLuaTimer::cost),
		"秒", sol::property(&CLuaTimer::costSeconds)
	);

	lua.new_usertype<CLuaTimerZhs>("计时器",
		sol::call_constructor,
		sol::constructors<CLuaTimerZhs()>(),

		"restart", &CLuaTimerZhs::restart,
		"expired", &CLuaTimerZhs::hasExpired,
		"getstr", &CLuaTimerZhs::toFormatedString,
		"tostr", &CLuaTimerZhs::toString,
		"todb", &CLuaTimerZhs::toDouble,
		"msec", sol::property(&CLuaTimerZhs::cost),
		"sec", sol::property(&CLuaTimerZhs::costSeconds),
		"重置", &CLuaTimerZhs::restart,
		"超时", &CLuaTimerZhs::hasExpired,
		"取格式", &CLuaTimerZhs::toFormatedString,
		"到字符串", &CLuaTimerZhs::toString,
		"到浮点数", &CLuaTimerZhs::toDouble,
		"毫秒", sol::property(&CLuaTimerZhs::cost),
		"秒", sol::property(&CLuaTimerZhs::costSeconds)
	);

	//lua.new_usertype<CLuaTimer>("计时器",
	//	sol::call_constructor,
	//	sol::constructors<CLuaTimer()>(),

	//);

	lua.new_usertype<sa::address_bool_t>("CardStruct",
		"valid", sol::readonly(&sa::address_bool_t::valid),
		"index", sol::readonly(&sa::address_bool_t::index),
		"name", sol::property(&sa::address_bool_t::getName),
		"online", sol::readonly(&sa::address_bool_t::onlineFlag),
		"turn", sol::readonly(&sa::address_bool_t::transmigration),
		"dp", sol::readonly(&sa::address_bool_t::dp),
		"lv", sol::readonly(&sa::address_bool_t::level)
	);

	lua.new_usertype<CLuaCard>("CardClass",
		sol::call_constructor,
		sol::constructors<CLuaCard(long long)>(),
		sol::meta_function::index, &CLuaCard::operator[],
		"contains", &CLuaCard::contains,
		"包含", &CLuaCard::contains
	);

	lua.safe_script("card = CardClass(__INDEX);", sol::script_pass_on_error);
	lua.collect_garbage();

	lua.new_usertype<sa::team_t>("TeamStruct",
		"valid", sol::readonly(&sa::team_t::valid),
		"index", sol::readonly(&sa::team_t::index),
		"id", sol::readonly(&sa::team_t::id),
		"name", sol::property(&sa::team_t::getName),
		"fname", sol::property(&sa::team_t::getFreeName),
		"lv", sol::readonly(&sa::team_t::level),
		"hp", sol::readonly(&sa::team_t::hp),
		"maxhp", sol::readonly(&sa::team_t::maxHp),
		"hpp", sol::readonly(&sa::team_t::hpPercent),
		"mp", sol::readonly(&sa::team_t::mp)
	);

	lua.new_usertype<CLuaTeam>("TeamClass",
		sol::call_constructor,
		sol::constructors<CLuaTeam(long long)>(),
		sol::meta_function::index, &CLuaTeam::operator[],
		"contains", &CLuaTeam::contains,
		"size", sol::property(&CLuaTeam::size),
		"包含", &CLuaTeam::contains,
		"大小", sol::property(&CLuaTeam::size)
	);

	lua.safe_script("team = TeamClass(__INDEX);", sol::script_pass_on_error);
	lua.collect_garbage();

	lua.new_usertype<CLuaMailProxy>("MailProxyClass",
		sol::call_constructor,
		sol::constructors<CLuaMailProxy(long long, long long)>(),
		sol::meta_function::index, &CLuaMailProxy::operator[]
	);

	lua.new_usertype<CLuaMail>("MailClass",
		sol::call_constructor,
		sol::constructors<CLuaMail(long long)>(),
		sol::meta_function::index, &CLuaMail::operator[]
	);

	lua.safe_script("mails = MailClass(__INDEX);", sol::script_pass_on_error);
	lua.collect_garbage();

	lua.new_usertype <CLuaPoint>("PointClass",
		sol::call_constructor,
		sol::constructors<CLuaPoint(long long)>(),
		"exp", sol::property(&CLuaPoint::getExp),
		"rep", sol::property(&CLuaPoint::getRep),
		"ene", sol::property(&CLuaPoint::getEne),
		"shl", sol::property(&CLuaPoint::getShl),
		"vit", sol::property(&CLuaPoint::getVit),
		"pts", sol::property(&CLuaPoint::getPts),
		"vip", sol::property(&CLuaPoint::getVip)
	);

	lua.safe_script("point = PointClass(__INDEX);", sol::script_pass_on_error);
	lua.collect_garbage();

	lua.new_usertype<CLuaDebug>("cout",
		sol::call_constructor,
		sol::constructors<CLuaDebug(sol::this_state)>(),
		sol::meta_function::bitwise_left_shift, &CLuaDebug::operator<<
	);
}

void CLua::open_itemlibs(sol::state& lua)
{
	lua.set_function("swapitem", &CLuaItem::swapitem, &luaItem_);
	lua.set_function("make", &CLuaItem::make, &luaItem_);
	lua.set_function("cook", &CLuaItem::cook, &luaItem_);
	lua.set_function("getstone", &CLuaItem::withdrawgold, &luaItem_);
	lua.set_function("putstone", &CLuaItem::depositgold, &luaItem_);
	lua.set_function("doffstone", &CLuaItem::dropgold, &luaItem_);
	lua.set_function("buy", &CLuaItem::buy, &luaItem_);
	lua.set_function("sell", &CLuaItem::sell, &luaItem_);
	lua.set_function("sellpet", &CLuaItem::sellpet, &luaItem_);
	lua.set_function("doffpet", &CLuaItem::droppet, &luaItem_);

	lua.set_function("useitem", &CLuaItem::useitem, &luaItem_);
	lua.set_function("doffitem", &CLuaItem::doffitem, &luaItem_);
	lua.set_function("pickup", &CLuaItem::pickitem, &luaItem_);
	lua.set_function("putitem", &CLuaItem::deposititem, &luaItem_);
	lua.set_function("getitem", &CLuaItem::withdrawitem, &luaItem_);
	lua.set_function("putpet", &CLuaItem::depositpet, &luaItem_);
	lua.set_function("getpet", &CLuaItem::withdrawpet, &luaItem_);
	lua.set_function("requip", &CLuaItem::recordequip, &luaItem_);
	lua.set_function("wequip", &CLuaItem::wearrecordedequip, &luaItem_);
	lua.set_function("uequip", &CLuaItem::unwearequip, &luaItem_);
	lua.set_function("pequip", &CLuaItem::petequip, &luaItem_);
	lua.set_function("puequip", &CLuaItem::petunequip, &luaItem_);

	lua.set_function("trade", &CLuaItem::trade, &luaItem_);

	lua.new_usertype<sa::item_t>("ItemStruct",
		"valid", sol::readonly(&sa::item_t::valid),
		"color", sol::readonly(&sa::item_t::color),
		"modelid", sol::readonly(&sa::item_t::modelid),
		"lv", sol::readonly(&sa::item_t::level),
		"stack", sol::readonly(&sa::item_t::stack),
		"type", sol::readonly(&sa::item_t::type),
		"field", sol::readonly(&sa::item_t::field),
		"target", sol::readonly(&sa::item_t::target),
		"flag", sol::readonly(&sa::item_t::deadTargetFlag),
		"sflag", sol::readonly(&sa::item_t::sendFlag),
		"itemup", sol::readonly(&sa::item_t::itemup),
		"counttime", sol::readonly(&sa::item_t::counttime),
		"dura", sol::readonly(&sa::item_t::damage),

		"name", sol::property(&sa::item_t::getName),
		"name2", sol::property(&sa::item_t::getName2),
		"memo", sol::property(&sa::item_t::getMemo),
		"hash", sol::property(&sa::item_t::getHash),

		/*custom*/
		"max", sol::readonly(&sa::item_t::maxStack),
		"count", sol::readonly(&sa::item_t::count)
	);

	lua.new_usertype<CLuaItem>("ItemClass",
		sol::call_constructor,
		sol::constructors<CLuaItem(long long)>(),
		sol::meta_function::index, &CLuaItem::operator[],
		"space", sol::property(&CLuaItem::getSpace),
		"spaceindex", sol::property(&CLuaItem::getSpaceIndex),
		"isfull", sol::property(&CLuaItem::getIsFull),
		"size", sol::property(&CLuaItem::getSize),
		"count", &CLuaItem::count,
		"indexof", &CLuaItem::indexof,
		"find", &CLuaItem::find
	);

	lua.safe_script("item = ItemClass(__INDEX);", sol::script_pass_on_error);
	lua.collect_garbage();

	lua.new_usertype < CLuaPetEquipProxy>("PetEquipClass",
		sol::call_constructor,
		sol::constructors<CLuaPetEquipProxy(long long, long long)>(),
		sol::meta_function::index, &CLuaPetEquipProxy::operator[]
	);

	lua.new_usertype<CLuaPetEquip>("PetEquipClass",
		sol::call_constructor,
		sol::constructors<CLuaPetEquip(long long)>(),
		sol::meta_function::index, &CLuaPetEquip::operator[]
	);

	lua.safe_script("petequip = PetEquipClass(__INDEX);", sol::script_pass_on_error);
	lua.collect_garbage();
}

void CLua::open_charlibs(sol::state& lua)
{
	lua.set_function("join", &CLuaChar::join, &luaChar_);
	lua.set_function("leave", &CLuaChar::leave, &luaChar_);
	lua.set_function("kick", &CLuaChar::kick, &luaChar_);
	lua.set_function("skup", &CLuaChar::skup, &luaChar_);
	lua.set_function("mail", &CLuaChar::mail, &luaChar_);
	lua.set_function("usemagic", &CLuaChar::usemagic, &luaChar_);

	lua.new_usertype<sa::character_t>("CharacterStruct",
		"battlepet", sol::readonly(&sa::character_t::battlePetNo),
		"mailpet", sol::readonly(&sa::character_t::mailPetNo),
		"standbypet", sol::readonly(&sa::character_t::standbyPet),
		"ridepet", sol::readonly(&sa::character_t::ridePetNo),
		"modelid", sol::readonly(&sa::character_t::modelid),
		"faceid", sol::readonly(&sa::character_t::faceid),
		"unitid", sol::readonly(&sa::character_t::id),
		"dir", sol::readonly(&sa::character_t::dir),
		"hp", sol::readonly(&sa::character_t::hp),
		"maxhp", sol::readonly(&sa::character_t::maxHp),
		"hpp", sol::readonly(&sa::character_t::hpPercent),
		"mp", sol::readonly(&sa::character_t::mp),
		"maxmp", sol::readonly(&sa::character_t::maxMp),
		"mpp", sol::readonly(&sa::character_t::mpPercent),
		"vit", sol::readonly(&sa::character_t::vit),
		"str", sol::readonly(&sa::character_t::str),
		"tgh", sol::readonly(&sa::character_t::tgh),
		"dex", sol::readonly(&sa::character_t::dex),
		"exp", sol::readonly(&sa::character_t::exp),
		"maxexp", sol::readonly(&sa::character_t::maxExp),
		"lv", sol::readonly(&sa::character_t::level),
		"atk", sol::readonly(&sa::character_t::atk),
		"def", sol::readonly(&sa::character_t::def),
		"agi", sol::readonly(&sa::character_t::agi),
		"chasma", sol::readonly(&sa::character_t::chasma),
		"luck", sol::readonly(&sa::character_t::luck),
		"earth", sol::readonly(&sa::character_t::earth),
		"water", sol::readonly(&sa::character_t::water),
		"fire", sol::readonly(&sa::character_t::fire),
		"wind", sol::readonly(&sa::character_t::wind),
		"gold", sol::readonly(&sa::character_t::gold),
		"stone", sol::readonly(&sa::character_t::gold),
		"fame", sol::readonly(&sa::character_t::fame),
		"titleid", sol::readonly(&sa::character_t::titleNo),
		"dp", sol::readonly(&sa::character_t::dp),
		"namecolor", sol::readonly(&sa::character_t::nameColor),
		"status", sol::readonly(&sa::character_t::status),
		"etc", sol::readonly(&sa::character_t::etcFlag),
		"battleid", sol::readonly(&sa::character_t::battleNo),
		"side", sol::readonly(&sa::character_t::sideNo),
		"ishelp", sol::readonly(&sa::character_t::helpMode),
		"charnamecolor", sol::readonly(&sa::character_t::pcNameColor),
		"turn", sol::readonly(&sa::character_t::transmigration),
		"fmleader", sol::readonly(&sa::character_t::familyleader),
		"channel", sol::readonly(&sa::character_t::channel),
		"quickchannel", sol::readonly(&sa::character_t::quickChannel),
		"bankgold", sol::readonly(&sa::character_t::personal_bankgold),
		"learnride", sol::readonly(&sa::character_t::learnride),//學習騎乘
		"lowsride", sol::readonly(&sa::character_t::lowsride),
		"ridelv", sol::readonly(&sa::character_t::ridePetLevel),
		"fmsprite", sol::readonly(&sa::character_t::familySprite),
		"basemodelid", sol::readonly(&sa::character_t::baseGraNo),
		"big4fm", sol::readonly(&sa::character_t::big4fm),
		"tradestate", sol::readonly(&sa::character_t::trade_confirm),         // 1 -> 初始值
		"professionclass", sol::readonly(&sa::character_t::profession_class),
		"professionlv", sol::readonly(&sa::character_t::profession_level),
		"professionexp", sol::readonly(&sa::character_t::profession_exp),
		"professionpoint", sol::readonly(&sa::character_t::profession_skill_point),

		"herofloor", sol::readonly(&sa::character_t::herofloor),// (不可開)排行榜NPC
		"streetvendor", sol::readonly(&sa::character_t::iOnStreetVendor),		// 擺攤模式
		"skywalker", sol::readonly(&sa::character_t::skywalker), // GM天行者
		"theatermode", sol::readonly(&sa::character_t::iTheaterMode),		// 劇場模式
		"scenertno", sol::readonly(&sa::character_t::iSceneryNumber),		// 記錄劇院背景圖號

		"dancestate", sol::readonly(&sa::character_t::iDanceMode),			// 動一動模式
		"newfame", sol::readonly(&sa::character_t::newfame), // 討伐魔軍積分
		"ftype", sol::readonly(&sa::character_t::ftype),

		//custom
		"maxload", sol::readonly(&sa::character_t::maxload),
		"point ", sol::readonly(&sa::character_t::point),

		"name", sol::property(&sa::character_t::getName),
		"fname", sol::property(&sa::character_t::getFreeName),
		"proname", sol::property(&sa::character_t::getProfessionClassName),
		"gmname", sol::property(&sa::character_t::getGmName),
		"chatroomnum", sol::property(&sa::character_t::getChatRoomNum),
		"chusheng", sol::property(&sa::character_t::getChusheng),
		"family", sol::property(&sa::character_t::getFamily),
		"ridepetname", sol::property(&sa::character_t::getRidePetName),
		"hash", sol::property(&sa::character_t::getHash)
	);


	lua.new_usertype<CLuaChar>("CharacterClass",
		sol::call_constructor,
		sol::constructors<CLuaChar(long long)>(),
		"data", sol::property(&CLuaChar::getCharacter),
		"数据", sol::property(&CLuaChar::getCharacter),

		"battlepet", sol::property(&CLuaChar::getBattlePetNo),
		"mailpet", sol::property(&CLuaChar::getMailPetNo),
		"standbypet", sol::property(&CLuaChar::getStandbyPet),
		"ridepet", sol::property(&CLuaChar::getRidePetNo),
		"modelid", sol::property(&CLuaChar::getModelId),
		"faceid", sol::property(&CLuaChar::getFaceId),
		"unitid", sol::property(&CLuaChar::getUnitId),
		"dir", sol::property(&CLuaChar::getDir),
		"hp", sol::property(&CLuaChar::getHp),
		"maxhp", sol::property(&CLuaChar::getMaxHp),
		"hpp", sol::property(&CLuaChar::getHpPercent),
		"mp", sol::property(&CLuaChar::getMp),
		"maxmp", sol::property(&CLuaChar::getMaxMp),
		"mpp", sol::property(&CLuaChar::getMpPercent),
		"vit", sol::property(&CLuaChar::getVit),
		"str", sol::property(&CLuaChar::getStr),
		"tgh", sol::property(&CLuaChar::getTgh),
		"dex", sol::property(&CLuaChar::getDex),
		"exp", sol::property(&CLuaChar::getExp),
		"maxexp", sol::property(&CLuaChar::getMaxExp),
		"lv", sol::property(&CLuaChar::getLevel),
		"atk", sol::property(&CLuaChar::getAtk),
		"def", sol::property(&CLuaChar::getDef),
		"agi", sol::property(&CLuaChar::getAgi),
		"chasma", sol::property(&CLuaChar::getChasma),
		"luck", sol::property(&CLuaChar::getLuck),
		"earth", sol::property(&CLuaChar::getEarth),
		"water", sol::property(&CLuaChar::getWater),
		"fire", sol::property(&CLuaChar::getFire),
		"wind", sol::property(&CLuaChar::getWind),
		"gold", sol::property(&CLuaChar::getGold),
		"stone", sol::property(&CLuaChar::getGold),
		"fame", sol::property(&CLuaChar::getFame),
		"titleid", sol::property(&CLuaChar::getTitleNo),
		"dp", sol::property(&CLuaChar::getDp),
		"namecolor", sol::property(&CLuaChar::getNameColor),
		"status", sol::property(&CLuaChar::getStatus),
		"etc", sol::property(&CLuaChar::getEtcFlag),
		"turn", sol::property(&CLuaChar::getTransmigration),
		"fmleader", sol::property(&CLuaChar::getFamilyleader),
		"channel", sol::property(&CLuaChar::getChannel),
		"bankgold", sol::property(&CLuaChar::getPersonalBankgold),
		"ridelv", sol::property(&CLuaChar::getRidePetLevel),
		"tradestate", sol::property(&CLuaChar::getTradeState),
		"professionclass", sol::property(&CLuaChar::getProfessionClass),
		"professionlv", sol::property(&CLuaChar::CLuaChar::getProfessionLevel),
		"professionexp", sol::property(&CLuaChar::getProfessionExp),
		"professionpoint", sol::property(&CLuaChar::getProfessionSkillPoint),

		//custom
		"maxload", sol::property(&CLuaChar::getMaxload),
		"point", sol::property(&CLuaChar::getPoint),

		"name", sol::property(&CLuaChar::getName),
		"fname", sol::property(&CLuaChar::getFreeName),
		"proname", sol::property(&CLuaChar::getProfessionClassName),
		"chatroomnum", sol::property(&CLuaChar::getChatRoomNum),
		"chusheng", sol::property(&CLuaChar::getChusheng),

		"family", sol::property(&CLuaChar::getFamily),
		"ridepetname", sol::property(&CLuaChar::getRidePetName),
		"hash", sol::property(&CLuaChar::getHash),
		"getserver", &CLuaChar::getServer
	);

	lua.safe_script("char = CharacterClass(__INDEX);", sol::script_pass_on_error);
	lua.collect_garbage();

	lua.new_usertype<sa::profession_skill_t>("SkillStruct",
		"valid", sol::readonly(&sa::profession_skill_t::valid),
		"skillid", sol::readonly(&sa::profession_skill_t::skillId),
		"target", sol::readonly(&sa::profession_skill_t::target),
		"kind", sol::readonly(&sa::profession_skill_t::kind),
		"icon", sol::readonly(&sa::profession_skill_t::icon),
		"costmp", sol::readonly(&sa::profession_skill_t::costmp),
		"lv", sol::readonly(&sa::profession_skill_t::skill_level),
		"cooltime", sol::readonly(&sa::profession_skill_t::cooltime),
		"index", sol::readonly(&sa::profession_skill_t::index),
		"name", sol::property(&sa::profession_skill_t::getName),
		"memo", sol::property(&sa::profession_skill_t::getMemo)
	);

	lua.new_usertype<CLuaSkill>("SkillClass",
		sol::call_constructor,
		sol::constructors<CLuaSkill(long long)>(),
		sol::meta_function::index, &CLuaSkill::operator[],
		"find", &CLuaSkill::find,
		"查找", &CLuaSkill::find
	);

	lua.safe_script("skill = SkillClass(__INDEX);", sol::script_pass_on_error);
	lua.collect_garbage();

	lua.new_usertype <sa::magic_t>("MagicStruct",
		"valid", sol::readonly(&sa::magic_t::valid),
		"costmp", sol::readonly(&sa::magic_t::costmp),
		"field", sol::readonly(&sa::magic_t::field),
		"target", sol::readonly(&sa::magic_t::target),
		"flag", sol::readonly(&sa::magic_t::deadTargetFlag),
		"name", sol::property(&sa::magic_t::getName),
		"memo", sol::property(&sa::magic_t::getMemo)
	);

	lua.new_usertype<CLuaMagic>("MagicClass",
		sol::call_constructor,
		sol::constructors<CLuaMagic(long long)>(),
		sol::meta_function::index, &CLuaMagic::operator[],
		"find", &CLuaMagic::find,
		"查找", &CLuaMagic::find
	);

	lua.safe_script("magic = MagicClass(__INDEX);", sol::script_pass_on_error);
	lua.collect_garbage();
}

void CLua::open_petlibs(sol::state& lua)
{
	lua.set_function("learn", &CLuaPet::learn, &luaPet_);

	lua.new_usertype <sa::pet_t>("PetStruct",
		"valid", sol::readonly(&sa::pet_t::valid),
		"state", sol::property(&sa::pet_t::getState),
		"index", sol::readonly(&sa::pet_t::index),
		"modelid", sol::readonly(&sa::pet_t::modelid),
		"hp", sol::readonly(&sa::pet_t::hp),
		"maxhp", sol::readonly(&sa::pet_t::maxHp),
		"hpp", sol::readonly(&sa::pet_t::hpPercent),
		"mp", sol::readonly(&sa::pet_t::mp),
		"maxmp", sol::readonly(&sa::pet_t::maxMp),
		"mpp", sol::readonly(&sa::pet_t::mpPercent),
		"exp", sol::readonly(&sa::pet_t::exp),
		"maxexp", sol::readonly(&sa::pet_t::maxExp),
		"lv", sol::readonly(&sa::pet_t::level),
		"atk", sol::readonly(&sa::pet_t::atk),
		"def", sol::readonly(&sa::pet_t::def),
		"agi", sol::readonly(&sa::pet_t::agi),
		"loyal", sol::readonly(&sa::pet_t::loyal),
		"earth", sol::readonly(&sa::pet_t::earth),
		"water", sol::readonly(&sa::pet_t::water),
		"fire", sol::readonly(&sa::pet_t::fire),
		"wind", sol::readonly(&sa::pet_t::wind),
		"maxskill", sol::readonly(&sa::pet_t::maxSkill),
		"turn", sol::readonly(&sa::pet_t::transmigration),
		"fusion", sol::readonly(&sa::pet_t::fusion),
		"status", sol::readonly(&sa::pet_t::status),
		"oldlv", sol::readonly(&sa::pet_t::oldlevel),
		"oldhp", sol::readonly(&sa::pet_t::oldhp),
		"oldatk", sol::readonly(&sa::pet_t::oldatk),
		"oldagi", sol::readonly(&sa::pet_t::oldagi),
		"olddef", sol::readonly(&sa::pet_t::olddef),
		"rideflg", sol::readonly(&sa::pet_t::rideflg),
		"blessflg", sol::readonly(&sa::pet_t::blessflg),
		"blesshp", sol::readonly(&sa::pet_t::blesshp),
		"blessatk", sol::readonly(&sa::pet_t::blessatk),
		"blessquick", sol::readonly(&sa::pet_t::blessquick),
		"blessdef", sol::readonly(&sa::pet_t::blessdef),
		"changenameflag", sol::readonly(&sa::pet_t::changeNameFlag),
		"power", sol::readonly(&sa::pet_t::power),
		"growth", sol::readonly(&sa::pet_t::growth),
		"name", sol::property(&sa::pet_t::getName),
		"fname", sol::property(&sa::pet_t::getFreeName),
		"hash", sol::property(&sa::pet_t::getHash)
	);

	lua.new_usertype<CLuaPet>("PetClass",
		sol::call_constructor,
		sol::constructors<CLuaPet(long long)>(),
		sol::meta_function::index, &CLuaPet::operator[],
		"size", sol::property(&CLuaPet::size),
		"大小", sol::property(&CLuaPet::size),
		"count", &CLuaPet::count,
		"数量", &CLuaPet::count
	);

	lua.safe_script("pet = PetClass(__INDEX);", sol::script_pass_on_error);
	lua.collect_garbage();

	lua.new_usertype <sa::pet_skill_t>("PetSkillStruct",
		"valid", sol::readonly(&sa::pet_skill_t::valid),
		"skillid", sol::readonly(&sa::pet_skill_t::skillId),
		"field", sol::readonly(&sa::pet_skill_t::field),
		"target", sol::readonly(&sa::pet_skill_t::target),
		"name", sol::property(&sa::pet_skill_t::getName),
		"memo", sol::property(&sa::pet_skill_t::getMemo)
	);

	lua.new_usertype < CLuaPetSkillProxy>("PetSkillProxyClass",
		sol::call_constructor,
		sol::constructors<CLuaPetSkillProxy(long long, long long)>(),
		sol::meta_function::index, &CLuaPetSkillProxy::operator[],
		"find", &CLuaPetSkillProxy::find,
		"查找", &CLuaPetSkillProxy::find
	);

	lua.new_usertype<CLuaPetSkill>("PetSkillClass",
		sol::call_constructor,
		sol::constructors<CLuaPetSkill(long long)>(),
		sol::meta_function::index, &CLuaPetSkill::operator[],
		"find", &CLuaPetSkill::find,
		"查找", &CLuaPetSkill::find
	);

	lua.safe_script("petskill = PetSkillClass(__INDEX);", sol::script_pass_on_error);
	lua.collect_garbage();
}

void CLua::open_maplibs(sol::state& lua)
{
	lua.set_function("findpath", &CLuaMap::findPath, &luaMap_);
	lua.set_function("move", &CLuaMap::move, &luaMap_);
	lua.set_function("w", &CLuaMap::packetMove, &luaMap_);
	lua.set_function("chmap", &CLuaMap::teleport, &luaMap_);
	lua.set_function("download", &CLuaMap::downLoad, &luaMap_);
	lua.set_function("findnpc", &CLuaMap::findNPC, &luaMap_);
	lua.set_function("dir", &CLuaMap::setdir, &luaMap_);
	lua.set_function("walkpos", &CLuaMap::walkpos, &luaMap_);

	lua.new_usertype<CLuaMap>("MapClass",
		sol::call_constructor,
		sol::constructors<CLuaMap(long long)>(),
		"x", sol::property(&CLuaMap::x),
		"y", sol::property(&CLuaMap::y),
		"xy", &CLuaMap::xy,
		"floor", sol::property(&CLuaMap::floor),
		"name", sol::property(&CLuaMap::getName),
		"ground", sol::property(&CLuaMap::getGround),
		"ismap", &CLuaMap::ismap,
		"isxy", &CLuaMap::isxy,
		"isrect", &CLuaMap::isrect
	);

	lua.safe_script("map = MapClass(__INDEX);", sol::script_pass_on_error);
	lua.collect_garbage();

	lua.new_usertype <sa::map_unit_t>("UnitStruct",
		"valid", sol::readonly(&sa::map_unit_t::isVisible),
		"visible", sol::readonly(&sa::map_unit_t::isVisible),
		"walkable", sol::readonly(&sa::map_unit_t::walkable),
		"id", sol::readonly(&sa::map_unit_t::id),
		"modelid", sol::readonly(&sa::map_unit_t::modelid),
		"x", sol::readonly(&sa::map_unit_t::x),
		"y", sol::readonly(&sa::map_unit_t::y),
		"xy", &sa::map_unit_t::xy,
		"dir", sol::readonly(&sa::map_unit_t::dir),
		"level", sol::readonly(&sa::map_unit_t::level),
		"namecolor", sol::readonly(&sa::map_unit_t::nameColor),
		"height", sol::readonly(&sa::map_unit_t::height),
		"charnamecolor", sol::readonly(&sa::map_unit_t::charNameColor),
		"petlevel", sol::readonly(&sa::map_unit_t::petlevel),
		"classno", sol::readonly(&sa::map_unit_t::classNo),
		"gold", sol::readonly(&sa::map_unit_t::gold),
		"professionclass", sol::readonly(&sa::map_unit_t::profession_class),
		"professionlv", sol::readonly(&sa::map_unit_t::profession_level),
		"professionpoint", sol::readonly(&sa::map_unit_t::profession_skill_point),
		"name", sol::property(&sa::map_unit_t::getName),
		"fname", sol::property(&sa::map_unit_t::getFreeName),
		"family", sol::property(&sa::map_unit_t::getFamily),
		"petname", sol::property(&sa::map_unit_t::getPetName),
		"itemname", sol::property(&sa::map_unit_t::getItemName)
	);

	lua.new_usertype<CLuaMapUnit>("UnitClass",
		sol::call_constructor,
		sol::constructors<CLuaMapUnit(long long)>(),
		"find", &CLuaMapUnit::find,
		"查找", &CLuaMapUnit::find
	);

	lua.safe_script("unit = UnitClass(__INDEX);", sol::script_pass_on_error);
	lua.collect_garbage();
}

void CLua::open_battlelibs(sol::state& lua)
{
	lua.new_usertype<sa::battle_object_t>("BattleStruct",
		"ready", sol::readonly(&sa::battle_object_t::ready),
		"pos", sol::readonly(&sa::battle_object_t::pos),
		"modelid", sol::readonly(&sa::battle_object_t::modelid),
		"lv", sol::readonly(&sa::battle_object_t::level),
		"hp", sol::readonly(&sa::battle_object_t::hp),
		"maxhp", sol::readonly(&sa::battle_object_t::maxHp),
		"hpp", sol::readonly(&sa::battle_object_t::hpPercent),
		"rideflag", sol::readonly(&sa::battle_object_t::rideFlag),
		"ridelv", sol::readonly(&sa::battle_object_t::rideLevel),
		"ridehp", sol::readonly(&sa::battle_object_t::rideHp),
		"ridemaxhp", sol::readonly(&sa::battle_object_t::rideMaxHp),
		"ridehpp", sol::readonly(&sa::battle_object_t::rideHpPercent),
		"name", sol::property(&sa::battle_object_t::getName),
		"freename", sol::property(&sa::battle_object_t::getFreeName),
		"ridename", sol::property(&sa::battle_object_t::getRideName),
		"status", sol::property(&sa::battle_object_t::getStatus)
	);

	lua.new_usertype<CLuaBattle>("BattleClass",
		sol::call_constructor,
		sol::constructors<CLuaBattle(long long)>(),
		sol::meta_function::index, &CLuaBattle::operator[],
		"count", sol::property(&CLuaBattle::count),
		"dura", sol::property(&CLuaBattle::dura),
		"time", sol::property(&CLuaBattle::time),
		"cost", sol::property(&CLuaBattle::cost),
		"round", sol::property(&CLuaBattle::round),
		"field", sol::property(&CLuaBattle::field),
		"charpos", sol::property(&CLuaBattle::charpos),
		"petpos", sol::property(&CLuaBattle::petpos),
		"size", sol::property(&CLuaBattle::size),
		"enemycount", sol::property(&CLuaBattle::enemycount),
		"alliecount", sol::property(&CLuaBattle::alliecount),

		"atk", &CLuaBattle::charUseAttack,
		"magic", sol::overload(
			sol::resolve< long long(long long, long long, sol::this_state)>(&CLuaBattle::charUseMagic),
			sol::resolve< long long(std::string, long long, sol::this_state)>(&CLuaBattle::charUseMagic)
		),
		"skill", sol::overload(
			sol::resolve< long long(long long, long long, sol::this_state)>(&CLuaBattle::charUseSkill),
			sol::resolve< long long(std::string, long long, sol::this_state)>(&CLuaBattle::charUseSkill)
		),
		"switch", &CLuaBattle::switchPet,
		"escape", &CLuaBattle::escape,
		"def", &CLuaBattle::defense,
		"item", sol::overload(
			sol::resolve< long long(long long, long long, sol::this_state)>(&CLuaBattle::useItem),
			sol::resolve< long long(std::string, long long, sol::this_state)>(&CLuaBattle::useItem)
		),
		"catch", sol::overload(
			sol::resolve< long long(long long, sol::this_state)>(&CLuaBattle::catchPet),
			sol::resolve< long long(std::string, sol::object, sol::object, sol::object, sol::this_state)>(&CLuaBattle::catchPet)
		),
		"none", &CLuaBattle::nothing,
		"petskill", sol::overload(
			sol::resolve< long long(long long, long long, sol::this_state)>(&CLuaBattle::petUseSkill),
			sol::resolve< long long(std::string, long long, sol::this_state)>(&CLuaBattle::petUseSkill)
		),
		"petnone", &CLuaBattle::petNothing,
		"wait", &CLuaBattle::bwait,
		"done", &CLuaBattle::bend
	);

	lua.safe_script("battle = BattleClass(__INDEX);", sol::script_pass_on_error);
	lua.collect_garbage();

}

void CLua::openlibs()
{
	if (!isSubScript_)
	{
		GameDevice& gamedevice = GameDevice::getInstance(getIndex());
		gamedevice.scriptThreadId = reinterpret_cast<unsigned long long>(QThread::currentThreadId());
	}

	lua_.set("__THIS_CLUA", this);// 將this指針傳給lua設置全局變量
	lua_.set("__THIS_PARENT", parent_);// 將父類指針傳給lua設置全局變量
	lua_.set("__INDEX", getIndex());

	lua_.set("ROWCOUNT", max_);

	//打開lua原生庫
	lua_.open_libraries(
		sol::lib::base,
		sol::lib::package,
		sol::lib::os,
		sol::lib::string,
		sol::lib::math,
		sol::lib::table,
		sol::lib::debug,
		sol::lib::utf8,
		sol::lib::coroutine,
		sol::lib::io
	);

	open_enumlibs();
	//open_testlibs();
	open_utillibs(lua_);
	open_syslibs(lua_);
	open_itemlibs(lua_);
	open_charlibs(lua_);
	open_petlibs(lua_);
	open_maplibs(lua_);
	open_battlelibs(lua_);

	lua_.set("SCRIPTDIR", util::toConstData(QString("%1/script").arg(util::applicationDirPath())));

	lua_.set("SETTINGDIR", util::toConstData(QString("%1/settings").arg(util::applicationDirPath())));

	lua_.set("CURRENTDIR", util::toConstData(util::applicationDirPath()));

	lua_.set("PID", static_cast<long long>(_getpid()));

	lua_.set("TID", reinterpret_cast<long long>(QThread::currentThreadId()));

	lua_.set("INFINITE", std::numeric_limits<long long>::max());

	lua_.set("MAXTHREAD", SASH_MAX_THREAD);

	lua_.set("MAXCHAR", sa::MAX_CHARACTER);

	lua_.set("MAXDIR", sa::MAX_DIR);

	lua_.set("MAXITEM", sa::MAX_ITEM - sa::CHAR_EQUIPSLOT_COUNT);

	lua_.set("MAXEQUIP", sa::CHAR_EQUIPSLOT_COUNT);

	lua_.set("MAXCARD", sa::MAX_ADDRESS_BOOK);

	lua_.set("MAXMAGIC", sa::MAX_MAGIC);

	lua_.set("MAXSKILL", sa::MAX_PROFESSION_SKILL);

	lua_.set("MAXPET", sa::MAX_PET);

	lua_.set("MAXPETSKILL", sa::MAX_PET_SKILL);

	lua_.set("MAXCHAT", sa::MAX_CHAT_HISTORY);

	lua_.set("MAXDLG", sa::MAX_DIALOG_LINE);

	lua_.set("MAXENEMY", sa::MAX_ENEMY / 2);

	GameDevice& gamedevice = GameDevice::getInstance(getIndex());

	lua_.set("HWND", reinterpret_cast<long long>(gamedevice.getParentWidget()));

	lua_.set("GAMEPID", gamedevice.getProcessId());

	lua_.set("GAMEHWND", reinterpret_cast<long long>(gamedevice.getProcessWindow()));

	lua_.set("GAMEHANDLE", reinterpret_cast<long long>(gamedevice.getProcess()));

	lua_.set("INDEX", gamedevice.getIndex());

	//執行短腳本
	lua_.safe_script(R"(
collectgarbage("setpause", 1000)
collectgarbage("setstepmul", 50);
collectgarbage("step", 1024);
	)", sol::script_pass_on_error);

	lua_.collect_garbage();

	//Add additional package path.
	QStringList paths;
	std::string package_path = lua_["package"]["path"];
	paths.append(util::toQString(package_path).replace("\\", "/"));

	QStringList dirs;
	luadebug::getPackagePath(util::applicationDirPath() + "/", &dirs);
	for (const QString& it : dirs)
	{
		QString path = it + "/?.lua";
		paths.append(path.replace("\\", "/"));
	}

	lua_["package"]["path"] = std::string(util::toConstData(paths.join(";")));

	//lua_.safe_script(R"(
	//	-- require('translations');
	//	set('init');
	//)", sol::script_pass_on_error);
	//lua_.collect_garbage();

	if (isHookEnabled_)
	{
		lua_State* L = lua_.lua_state();
		lua_sethook(L, &luadebug::hookProc, LUA_MASKLINE | LUA_MASKCALL | LUA_MASKRET, NULL);// | LUA_MASKCOUNT
	}
}

bool CLua::doString(const std::string& sstr)
{
	bool bret = false;

	do
	{
		isRunning_.on();

		sol::protected_function_result loaded_chunk = lua_.safe_script(sstr, sol::script_pass_on_error);
		lua_.collect_garbage();

		if (loaded_chunk.valid())
		{
			sol::type tp = loaded_chunk.get_type();

			if (tp == sol::type::boolean)
			{
				bret = loaded_chunk.get<bool>();
				break;
			}
			else if (tp == sol::type::number)
			{
				bret = loaded_chunk.get<double>() > 0.0;
				break;
			}
		}
		else
		{
			try
			{
				sol::error err = loaded_chunk;
				qDebug() << err.what();

				QString errStr = util::toQString(err.what());

				sol::this_state s = lua_.lua_state();

				luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, errStr);
			}
			catch (...) {}
		}
	} while (false);

	isRunning_.off();

	return bret;
}

void CLua::proc()
{
	do
	{
		if (scriptContent_.simplified().isEmpty())
			break;

		safe::auto_flag autoFlag(&isRunning_);

		max_ = scriptContent_.split("\n").size();

		lua_State* L = lua_.lua_state();
		sol::this_state s = L;

		openlibs();

		QStringList tableStrs;
		std::string luaCode(util::toConstData(scriptContent_));

		//安全模式下執行lua腳本
		sol::protected_function_result loaded_chunk = lua_.safe_script(luaCode.c_str(), sol::script_pass_on_error);
		lua_.collect_garbage();

		if (!loaded_chunk.valid())
		{
			sol::error err("\0");
			QString qstrErr("\0");
			bool errOk = false;
			try
			{
				err = loaded_chunk;
				qstrErr = util::toQString(err.what());
				errOk = true;
			}
			catch (...)
			{
				if (!isSubScript_)
					tableStrs << "[fatal]:" + tr("========== lua error result with an EXCEPTION ==========");
				else
					tableStrs << "[fatal]: SUBTHREAD | " + tr("========== lua error result with an EXCEPTION ==========");

				if (errOk)
				{
					long long retline = -1;
					QString msg(luadebug::getErrorMsgLocatedLine(qstrErr, &retline));

					if (msg.contains("FLAG_DETECT_STOP")
						|| msg.contains("REQUEST_STOP_FROM_USER")
						|| msg.contains("REQUEST_STOP_FROM_SCRIP")
						|| msg.contains("REQUEST_STOP_FROM_PARENT_SCRIP")
						|| msg.contains("REQUEST_STOP_FROM_DISTRUCTOR"))
					{
						if (msg.contains("FLAG_DETECT_STOP"))
							tableStrs << tr("> lua script stop by flag change to false");
						else if (msg.contains("REQUEST_STOP_FROM_USER"))
							tableStrs << tr("> lua script stop with user request");
						else if (msg.contains("REQUEST_STOP_FROM_SCRIP"))
							tableStrs << tr("> lua script stop from script request");
						else if (msg.contains("REQUEST_STOP_FROM_PARENT_SCRIP"))
							tableStrs << tr("> lua script stop from parent script request");
						else if (msg.contains("REQUEST_STOP_FROM_DISTRUCTOR"))
							tableStrs << tr("> lua script stop from it's distructor");
						//SPD_LOG(GLOBAL_LOG_ID, QString("%1 [index:%2] Lua Info:%3").arg(__FUNCTION__).arg(m_index).arg(msg));
						//emit this->addErrorMarker(retline, true);
						if (isDebug_)
							tableStrs << tr("> message: ");
						tableStrs << "> [info]:" + msg;
					}
					else
					{
						tableStrs << tr("========== lua script stop with an ERROR ==========");
						//SPD_LOG(GLOBAL_LOG_ID, QString("%1 [index:%2] Lua Warn:%3").arg(__FUNCTION__).arg(m_index).arg(msg), SPD_WARN);
						if (isDebug_)
							tableStrs << tr("> reason: ");
						tableStrs << "> [error]:" + msg;
					}

					//emit this->logMessageExport(s, );
					if (isDebug_ && !isSubScript_)
						tableStrs << ">";
				}
			}

			tableStrs.append(qstrErr);
		}
		else
		{
#ifdef _DEBUG
			isDebug_ = true;
#endif

			if (isDebug_)
			{
				//get script return value
				tableStrs << tr("========== lua script normally end ==========");
				tableStrs << tr("> return value:");
				sol::object retObject;

				try
				{
					retObject = loaded_chunk;
				}
				catch (...)
				{
					if (!isSubScript_)
						tableStrs << "[fatal]:" + tr("========== lua normal result with EXCEPTION ==========");
					else
						tableStrs << "[fatal]: SUBTHREAD | " + tr("========== lua normal result with EXCEPTION ==========");
				}
				if (retObject.is<bool>())
				{
					tableStrs << QString("> (boolean)%1").arg(util::toQString(retObject.as<bool>()));
				}
				else if (retObject.is<long long>())
				{
					tableStrs << "> (integer)" + util::toQString(retObject.as<long long>());

				}
				else if (retObject.is<double>())
				{
					tableStrs << "> (number)" + util::toQString(retObject.as<double>());
				}
				else if (retObject.is<std::string>())
				{
					tableStrs << "> (string)" + util::toQString(retObject);
				}
				else if (retObject == sol::lua_nil)
				{
					tableStrs << "> (nil)";
				}
				else if (retObject.is<sol::table>())
				{
					sol::table t = retObject.as<sol::table>();

					tableStrs << "> {";
					for (const std::pair<sol::object, sol::object>& it : t)
					{
						sol::object key = it.first;
						sol::object val = it.second;
						if (!key.valid() || !val.valid()) continue;
						if (!key.is<std::string>() && !key.is<long long>()) continue;
						QString qkey = key.is<std::string>() ? util::toQString(key) : util::toQString(key.as<long long>());

						if (val.is<bool>())
						{
							tableStrs << QString(R"(>     ["%1"] = (boolean)%2,)").arg(qkey).arg(util::toQString(val.as<bool>()));
						}
						else if (val.is<long long>())
						{
							tableStrs << QString(R"(>     ["%1"] = (integer)%2,)").arg(qkey).arg(val.as<long long>());
						}
						else if (val.is<double>())
						{
							tableStrs << QString(R"(>     ["%1"] = (number)%2,)").arg(qkey).arg(val.as<double>());
						}
						else if (val.is<std::string>())
						{
							tableStrs << QString(R"(>     ["%1"] = (string)%2,)").arg(qkey).arg(util::toQString(val));
						}
						else if (val.is<sol::table>())
						{
							tableStrs << QString(R"(>     ["%1"] = %2,)").arg(qkey).arg("table");
						}
						else
						{
							tableStrs << QString(R"(>     ["%1"] = %2,)").arg(qkey).arg("unknown");
						}

					}
					tableStrs << "> }";
				}
				else
				{
					tableStrs << tr("> (unknown type of data)");
				}
				tableStrs << ">";
		}
	}

		luadebug::logExport(s, tableStrs, 0);
} while (false);

	emit finished();

	long long currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	emit signalDispatcher.scriptFinished();
}