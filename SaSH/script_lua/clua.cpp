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

#include "signaldispatcher.h"
#include "injector.h"
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

void luadebug::tryPopCustomErrorMsg(const sol::this_state& s, const LUA_ERROR_TYPE element, const QVariant& p1, const QVariant& p2, const QVariant& p3, const QVariant& p4)
{
	Q_UNUSED(p4);//reserved
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
		const std::string str(qmsgstr.toUtf8().constData());
		luaL_error(L, str.c_str());
		break;
	}
	case ERROR_PARAM_SIZE://固定參數數量錯誤
	{
		qint64 topsize = lua_gettop(L);
		const QString qmsgstr(QString(errormsg_str.value(element)).arg(p1.toLongLong()).arg(topsize));
		const std::string str(qmsgstr.toUtf8().constData());
		luaL_argcheck(L, topsize == p1.toLongLong(), topsize, str.c_str());
		break;
	}
	case ERROR_PARAM_SIZE_NONE://無參數數量錯誤
	{
		qint64 topsize = lua_gettop(L);
		const QString qmsgstr(QString(errormsg_str.value(element)).arg(topsize));
		const std::string str(qmsgstr.toUtf8().constData());
		luaL_argcheck(L, topsize == 0, 1, str.c_str());
		break;
	}
	case ERROR_PARAM_SIZE_RANGE://範圍參數數量錯誤
	{
		qint64 topsize = lua_gettop(L);
		const QString qmsgstr(QString(errormsg_str.value(element)).arg(p1.toLongLong()).arg(p2.toLongLong()).arg(topsize));
		const std::string str(qmsgstr.toUtf8().constData());
		luaL_argcheck(L, topsize >= p1.toLongLong() && topsize <= p2.toLongLong(), 1, str.c_str());
		break;
	}
	case ERROR_PARAM_VALUE://數值錯誤 p1 第幾個參數 p2邏輯 str訊息
	{
		const QString qmsgstr(p3.toString());
		const std::string str(qmsgstr.toUtf8().constData());
		luaL_argcheck(L, p2.toBool(), p1.toLongLong(), str.c_str());
		break;
	}
	case ERROR_PARAM_TYPE://參數預期錯誤 p1 第幾個參數 p2邏輯 str訊息
	{
		const QString qmsgstr(p3.toString());
		const std::string str(qmsgstr.toUtf8().constData());
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

QString luadebug::getErrorMsgLocatedLine(const QString& str, qint64* retline)
{
	const QString cmpstr(str.simplified());

	QRegularExpressionMatch match = rexGetLine.match(cmpstr);
	QRegularExpressionMatch match2 = reGetLineEx.match(cmpstr);
	static const auto matchies = [](const QRegularExpressionMatch& m, qint64* retline)->void
	{
		qint64 size = m.capturedTexts().size();
		if (size > 1)
		{
			for (qint64 i = (size - 1); i >= 1; --i)
			{
				const QString s = m.captured(i).simplified();
				if (!s.isEmpty())
				{
					bool ok = false;
					qint64 row = s.simplified().toLongLong(&ok);
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

QString luadebug::getTableVars(lua_State*& L, qint64 si, qint64 depth)
{
	if (!L) return "\0";
	QPair<QString, QVariant> pair;
	qint64 pos_si = si > 0 ? si : (si - 1);
	QString ret;
	qint64 top = lua_gettop(L);
	lua_pushnil(L);
	qint64 empty = 1;
	QStringList varList;
	while (lua_next(L, pos_si) != 0)
	{
		if (empty)
			empty = 0;

		QString key;
		pair = getVars(L, -2, -1);
		if (pair.first == "(string)")
			ret += pair.second.toString();
		else if (pair.first == "(integer)")
		{
			pair.second = pair.second.toString();
			ret += QString("[%1]=").arg(pair.second.toString());
		}
		else
			continue;

		if (depth > 50)
		{
			ret += ("{}");
		}
		else
		{
			pair = getVars(L, -1, depth + 1);
			if (pair.first == "(string)")
				ret += QString("'%1'").arg(pair.second.toString());
			else
				ret += pair.second.toString();
		}
		lua_pop(L, 1);
		varList.append(ret);
		ret.clear();
	}

	lua_settop(L, top);
	ret = QString("{%1}").arg(varList.join(","));
	return ret.simplified();
}

QPair<QString, QVariant> luadebug::getVars(lua_State*& L, qint64 si, qint64 depth)
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
		if (d == static_cast<qint64>(d))
			return qMakePair(QString("(integer)"), util::toQString(static_cast<qint64>(luaL_checkinteger(L, si))));
		else
			return qMakePair(QString("(number)"), util::toQString(d));

	}
	case LUA_TFUNCTION:
	{
		lua_CFunction func = lua_tocfunction(L, si);
		if (func != nullptr)
		{
			return qMakePair(QString("(C function)"), QString("0x%1").arg(util::toQString(reinterpret_cast<qint64>(func), 16)));
		}
		else
		{
			return qMakePair(QString("(function)"), QString("0x%1").arg(util::toQString(reinterpret_cast<qint64>(func), 16)));
		}
		break;
	}
	case LUA_TUSERDATA:
	{
		return qMakePair(QString("(user data)"), QString("0x%1").arg(util::toQString(reinterpret_cast<qint64>(lua_touserdata(L, si)), 16)));
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

bool luadebug::isInterruptionRequested(const sol::this_state& s)
{
	sol::state_view lua(s.lua_state());
	CLua* pLua = lua["_THIS"].get<CLua*>();
	if (pLua == nullptr)
		return false;

	return pLua->isInterruptionRequested();
}

void luadebug::checkStopAndPause(const sol::this_state& s)
{
	sol::state_view lua(s.lua_state());
	lua_State* L = s.lua_state();
	CLua* pLua = lua["_THIS_CLUA"].get<CLua*>();
	if (pLua == nullptr)
		return;

	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (pLua->isInterruptionRequested() || injector.IS_SCRIPT_INTERRUPT.load(std::memory_order_acquire))
	{
		luadebug::tryPopCustomErrorMsg(s, luadebug::ERROR_FLAG_DETECT_STOP);
		return;
	}
	pLua->checkPause();
}

bool luadebug::checkOnlineThenWait(const sol::this_state& s)
{
	checkStopAndPause(s);
	sol::state_view lua(s.lua_state());
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	bool bret = false;
	if (!injector.server->getOnlineFlag())
	{
		QElapsedTimer timer; timer.start();
		bret = true;
		for (;;)
		{
			if (isInterruptionRequested(s))
				break;

			if (!injector.server.isNull())
				break;

			checkStopAndPause(s);

			if (injector.server->getOnlineFlag())
				break;
			if (timer.hasExpired(180000))
				break;

			QThread::msleep(100);
		}

		QThread::msleep(1000UL);
	}
	return bret;
}

bool luadebug::checkBattleThenWait(const sol::this_state& s)
{
	checkStopAndPause(s);

	sol::state_view lua(s.lua_state());
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	bool bret = false;
	if (injector.server->getBattleFlag())
	{
		QElapsedTimer timer; timer.start();
		bret = true;
		for (;;)
		{
			if (isInterruptionRequested(s))
				break;

			if (!injector.server.isNull())
				break;

			checkStopAndPause(s);

			if (!injector.server->getBattleFlag())
				break;
			if (timer.hasExpired(180000))
				break;

			QThread::msleep(100);
		}

		QThread::msleep(1000UL);
	}
	return bret;
}

void luadebug::processDelay(const sol::this_state& s)
{
	sol::state_view lua(s.lua_state());
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	qint64 extraDelay = injector.getValueHash(util::kScriptSpeedValue);
	if (extraDelay > 1000ll)
	{
		//將超過1秒的延時分段
		qint64 i = 0ll;
		qint64 size = extraDelay / 1000ll;
		for (i = 0; i < size; ++i)
		{
			if (luadebug::isInterruptionRequested(s))
				return;
			QThread::msleep(1000L);
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
void luadebug::getPackagePath(const QString base, QStringList* result)
{
	QDir dir(base);
	if (!dir.exists())
		return;
	dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
	dir.setSorting(QDir::DirsFirst);
	QFileInfoList list = dir.entryInfoList();
	for (qint64 i = 0; i < list.size(); ++i)
	{
		QFileInfo fileInfo = list.value(i);
		result->append(fileInfo.filePath());
		getPackagePath(fileInfo.filePath(), result);
	}
}

void luadebug::logExport(const sol::this_state& s, const QStringList& datas, qint64 color, bool doNotAnnounce)
{
	for (const QString& data : datas)
	{
		logExport(s, data, color);
	}
}

void luadebug::logExport(const sol::this_state& s, const QString& data, qint64 color, bool doNotAnnounce)
{

	//打印當前時間
	const QDateTime time(QDateTime::currentDateTime());
	const QString timeStr(time.toString("hh:mm:ss:zzz"));
	QString msg = "\0";
	QString src = "\0";
	sol::state_view lua(s);
	qint64 currentline = lua["_LINE_"].get<qint64>();//getCurrentLine(s);

	msg = (QString("[%1 | @%2]: %3\0") \
		.arg(timeStr)
		.arg(currentline, 3, 10, QLatin1Char(' ')).arg(data));


	qint64 currentIndex = lua["_INDEX"].get<qint64>();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	emit signalDispatcher.appendScriptLog(msg, color);
	Injector& injector = Injector::getInstance(currentIndex);
	if (!injector.server.isNull())
	{
		injector.server->announce(data, color);
	}

	if (injector.log.isOpen())
		injector.log.write(data, currentline);
}

//根據傳入function的循環執行結果等待超時或條件滿足提早結束
bool luadebug::waitfor(const sol::this_state& s, qint64 timeout, std::function<bool()> exprfun)
{
	if (timeout < 0)
		timeout = std::numeric_limits<qint64>::max();

	sol::state_view lua(s.lua_state());
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	bool bret = false;
	QElapsedTimer timer; timer.start();
	for (;;)
	{
		checkStopAndPause(s);

		if (isInterruptionRequested(s))
			break;

		if (timer.hasExpired(timeout))
			break;

		if (injector.server.isNull())
			break;

		if (exprfun())
		{
			bret = true;
			break;
		}

		QThread::msleep(100);
	}
	return bret;
}


//lua函數鉤子 這裡主要用於控制 暫停、終止腳本、獲取棧數據、變量數據...或其他操作
void luadebug::hookProc(lua_State* L, lua_Debug* ar)
{
	if (!L)
		return;

	if (!ar)
		return;

	sol::this_state s = L;
	sol::state_view lua(L);

	switch (ar->event)
	{
	case LUA_HOOKCALL:
	{
		//函數入口
		break;
	}
	case LUA_MASKCOUNT:
	{
		//每 n 個函數執行一次
		break;
	}
	case LUA_HOOKRET:
	{
		//函數返回
		break;
	}
	case LUA_HOOKLINE:
	{
		sol::state_view lua(s.lua_state());

		if (lua["_HOOKFORSTOP"].is<bool>() && lua["_HOOKFORSTOP"].get<bool>())
		{
			luadebug::checkStopAndPause(s);
			break;
		}

		qint64 currentLine = ar->currentline;
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(lua["_INDEX"].get<qint64>());
		qint64 max = lua["_ROWCOUNT_"];

		Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());

		emit signalDispatcher.scriptLabelRowTextChanged(currentLine, max, false);

		processDelay(s);
		if (injector.isScriptDebugModeEnable.load(std::memory_order_acquire))
		{
			QThread::msleep(1);
		}
		else
		{
			luadebug::checkStopAndPause(s);
			return;
		}

		luadebug::checkStopAndPause(s);

		CLua* pLua = lua["_THIS_CLUA"].get<CLua*>();
		if (pLua == nullptr)
			return;

		QString scriptFileName = injector.currentScriptFileName;

		util::SafeHash<qint64, break_marker_t> breakMarkers = injector.break_markers.value(scriptFileName);
		const util::SafeHash<qint64, break_marker_t> stepMarkers = injector.step_markers.value(scriptFileName);
		if (!(breakMarkers.contains(currentLine) || stepMarkers.contains(currentLine)))
		{
			return;//檢查是否有中斷點
		}

		pLua->paused();

		if (breakMarkers.contains(currentLine))
		{
			//叫用次數+1
			break_marker_t mark = breakMarkers.value(currentLine);
			++mark.count;

			//重新插入斷下的紀錄
			breakMarkers.insert(currentLine, mark);
			injector.break_markers.insert(scriptFileName, breakMarkers);
			//所有行插入隱式斷點(用於單步)
			emit signalDispatcher.addStepMarker(currentLine, true);
		}

		emit signalDispatcher.addForwardMarker(currentLine, true);

		//獲取區域變量數值
		qint64 i;
		const char* name = nullptr;
		QVariantHash varhash;
		for (i = 1; (name = lua_getlocal(L, ar, i)) != NULL; ++i)
		{
			QPair<QString, QVariant> vs = getVars(L, i, 5);

			QString key = QString("local|%1").arg(util::toQString(name));
			varhash.insert(key, vs.second.toString());
			//var.type = vs.first.replace("(", "").replace(")", "");
			lua_pop(L, 1);// no match, then pop out the var's value
		}

		Parser parser(lua["_INDEX"].get<qint64>());
		emit signalDispatcher.varInfoImported(&parser, varhash, QStringList{});

		luadebug::checkStopAndPause(s);

		break;
	}
	default:
		break;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CLua::CLua(qint64 index, QObject* parent)
	: ThreadPlugin(index, parent)
{
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(index);
	connect(&signalDispatcher, &SignalDispatcher::nodifyAllStop, this, &CLua::requestInterruption, Qt::UniqueConnection);
	connect(&signalDispatcher, &SignalDispatcher::nodifyAllScriptStop, this, &CLua::requestInterruption, Qt::UniqueConnection);
	qDebug() << "CLua 1";
}

CLua::CLua(qint64 index, const QString& content, QObject* parent)
	: ThreadPlugin(index, parent)
	, scriptContent_(content)
{
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(index);
	connect(&signalDispatcher, &SignalDispatcher::nodifyAllStop, this, &CLua::requestInterruption, Qt::UniqueConnection);
	connect(&signalDispatcher, &SignalDispatcher::nodifyAllScriptStop, this, &CLua::requestInterruption, Qt::UniqueConnection);
	qDebug() << "CLua 2";
}

CLua::~CLua()
{
	qDebug() << "~CLua";
}

void CLua::start()
{
	thread_ = new QThread();
	if (nullptr == thread_)
		return;

	moveToThread(thread_);

	qint64 currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	connect(this, &CLua::finished, thread_, &QThread::quit);
	connect(thread_, &QThread::finished, thread_, &QThread::deleteLater);
	connect(thread_, &QThread::started, this, &CLua::proc);
	connect(this, &CLua::finished, this, [this]()
		{
			requestInterruption();
			thread_->requestInterruption();
			thread_->quit();
			thread_->wait();
			thread_ = nullptr;
			qDebug() << "CLua::finished";
		});


	thread_->start();
}

void CLua::wait()
{
	if (nullptr != thread_)
		thread_->wait();
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

	//kSelectSelf = 0x1,            // 己 (Self)
	//kSelectPet = 0x2,             // 寵 (Pet)
	//kSelectAllieAny = 0x4,        // 我任 (Any ally)
	//kSelectAllieAll = 0x8,        // 我全 (All allies)
	//kSelectEnemyAny = 0x10,       // 敵任 (Any enemy)
	//kSelectEnemyAll = 0x20,       // 敵全 (All enemies)
	//kSelectEnemyFront = 0x40,     // 敵前 (Front enemy)
	//kSelectEnemyBack = 0x80,      // 敵後 (Back enemy)
	//kSelectLeader = 0x100,        // 隊 (Leader)
	//kSelectLeaderPet = 0x200,     // 隊寵 (Leader's pet)
	//kSelectTeammate1 = 0x400,     // 隊1 (Teammate 1)
	//kSelectTeammate1Pet = 0x800,  // 隊1寵 (Teammate 1's pet)
	//kSelectTeammate2 = 0x1000,    // 隊2 (Teammate 2)
	//kSelectTeammate2Pet = 0x2000, // 隊2寵 (Teammate 2's pet)
	//kSelectTeammate3 = 0x4000,    // 隊3 (Teammate 3)
	//kSelectTeammate3Pet = 0x8000, // 隊3寵 (Teammate 3's pet)
	//kSelectTeammate4 = 0x10000,   // 隊4 (Teammate 4)
	//kSelectTeammate4Pet = 0x20000 // 隊4寵 (Teammate 4's pet)

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
		CLuaTest(qint64 a, sol::this_state)>(),

		//成員函數多載
		"add", sol::overload(
			sol::resolve<qint64(qint64 a, qint64 b)>(&CLuaTest::add),
			sol::resolve<qint64(qint64 b)>(&CLuaTest::add)
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
	lua.new_usertype<CLuaUtil>("InfoClass",
		sol::call_constructor,
		sol::constructors<CLuaUtil()>(),
		"sys", &CLuaUtil::getSys,
		"map", &CLuaUtil::getMap,
		"char", &CLuaUtil::getChar,
		"pet", &CLuaUtil::getPet,
		"team", &CLuaUtil::getTeam,
		"card", &CLuaUtil::getCard,
		"chat", &CLuaUtil::getChat,
		"dialog", &CLuaUtil::getDialog,
		"unit", &CLuaUtil::getUnit,
		"battlUnit", &CLuaUtil::getBattleUnit,
		"daily", &CLuaUtil::getDaily,
		"item", &CLuaUtil::getItem,
		"skill", &CLuaUtil::getSkill,
		"magic", &CLuaUtil::getMagic
	);
}

//luasystem.cpp
void CLua::open_syslibs(sol::state& lua)
{
	lua.set_function("sleep", &CLuaSystem::sleep, &luaSystem_);
	lua.set_function("openlog", &CLuaSystem::openlog, &luaSystem_);
	lua.set_function("printf", &CLuaSystem::print, &luaSystem_);
	lua.safe_script(R"(
		_print = print;
		print = printf;
	)");

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

	lua.new_usertype<QElapsedTimer>("Timer",
		sol::call_constructor,
		sol::constructors<QElapsedTimer()>(),

		"start", &QElapsedTimer::start,
		"restart", &QElapsedTimer::restart,
		"expired", &QElapsedTimer::hasExpired,
		"get", &QElapsedTimer::elapsed
	);

	lua.set_function("say", &CLuaSystem::talk, &luaSystem_);
	lua.set_function("cls", &CLuaSystem::cleanchat, &luaSystem_);
	lua.set_function("logout", &CLuaSystem::logout, &luaSystem_);
	lua.set_function("logback", &CLuaSystem::logback, &luaSystem_);
	lua.set_function("eo", &CLuaSystem::eo, &luaSystem_);

	lua.new_usertype<CLuaSystem>("SystemClass",
		sol::call_constructor,
		sol::constructors<CLuaSystem()>(),
		"press", sol::overload(
			sol::resolve<qint64(std::string, qint64, qint64, sol::this_state)>(&CLuaSystem::press),
			sol::resolve<qint64(qint64, qint64, qint64, sol::this_state)>(&CLuaSystem::press)
		)
	);
}

void CLua::open_itemlibs(sol::state& lua)
{
	lua.new_usertype<CLuaItem>("ItemClass",
		sol::call_constructor,
		sol::constructors<CLuaItem()>(),
		"use", &CLuaItem::use,
		"drop", &CLuaItem::drop,
		"pick", &CLuaItem::pick,
		"swap", &CLuaItem::swap,
		"craft", &CLuaItem::craft,
		"buy", &CLuaItem::buy,
		"sell", &CLuaItem::sell,

		"deposit", &CLuaItem::deposit,
		"withdraw", &CLuaItem::withdraw
	);
}

void CLua::open_charlibs(sol::state& lua)
{
	lua.new_usertype<CLuaChar>("CharClass",
		sol::call_constructor,
		sol::constructors<CLuaChar()>(),
		"rename", &CLuaChar::rename,
		"useMagic", &CLuaChar::useMagic,
		"depositGold", &CLuaChar::depositGold,
		"withdrawGold", &CLuaChar::withdrawGold,
		"dropGold", &CLuaChar::dropGold,
		"mail", sol::overload(
			sol::resolve<qint64(qint64, std::string, qint64, std::string, std::string, sol::this_state)>(&CLuaChar::mail),
			sol::resolve<qint64(qint64, std::string, sol::this_state)>(&CLuaChar::mail)
		),
		"skillUp", &CLuaChar::skillUp,

		"setTeamState", &CLuaChar::setTeamState,
		"kick", &CLuaChar::kick
	);
}

void CLua::open_petlibs(sol::state& lua)
{
	lua.new_usertype<CLuaPet>("PetClass",
		sol::call_constructor,
		sol::constructors<CLuaPet()>(),
		"setState", &CLuaPet::setState,
		"drop", &CLuaPet::drop,
		"rename", &CLuaPet::rename,
		"learn", &CLuaPet::learn,
		"swap", &CLuaPet::swap,
		"deposit", &CLuaPet::deposit,
		"withdraw", &CLuaPet::withdraw
	);
}

void CLua::open_maplibs(sol::state& lua)
{
	lua.set_function("findpath", &CLuaMap::findPath, &luaMap_);
	lua.set_function("move", &CLuaMap::move, &luaMap_);
	lua.set_function("w", &CLuaMap::packetMove, &luaMap_);
	lua.set_function("chmap", &CLuaMap::teleport, &luaMap_);
	lua.set_function("download", &CLuaMap::downLoad, &luaMap_);
	lua.set_function("movetonpc", &CLuaMap::moveToNPC, &luaMap_);

	lua.new_usertype<CLuaMap>("MapClass",
		sol::call_constructor,
		sol::constructors<CLuaMap()>(),
		"setDir", sol::overload(
			sol::resolve<qint64(qint64, sol::this_state)>(&CLuaMap::setDir),
			sol::resolve<qint64(qint64, qint64, sol::this_state) >(&CLuaMap::setDir),
			sol::resolve<qint64(std::string, sol::this_state) >(&CLuaMap::setDir)
		)
	);
}

void CLua::open_battlelibs(sol::state& lua)
{
	lua.new_usertype<CLuaBattle>("BattleClass",
		sol::call_constructor,
		sol::constructors<CLuaBattle()>(),
		"charUseAttack", &CLuaBattle::charUseAttack,
		"charUseMagic", &CLuaBattle::charUseMagic,
		"charUseSkill", &CLuaBattle::charUseSkill,
		"switchPet", &CLuaBattle::switchPet,
		"escape", &CLuaBattle::escape,
		"defense", &CLuaBattle::defense,
		"useItem", &CLuaBattle::useItem,
		"catchPet", &CLuaBattle::catchPet,
		"nothing", &CLuaBattle::nothing,
		"petUseSkill", &CLuaBattle::petUseSkill,
		"petNothing", &CLuaBattle::petNothing
	);
}

void CLua::openlibs()
{
	if (!isSubScript_)
	{
		Injector& injector = Injector::getInstance(getIndex());
		injector.scriptThreadId = reinterpret_cast<quint64>(QThread::currentThreadId());
	}


	lua_.set("_THIS_CLUA", this);// 將this指針傳給lua設置全局變量
	lua_.set("_THIS_PARENT", parent_);// 將父類指針傳給lua設置全局變量
	lua_.set("_INDEX", getIndex());
	lua_.set("_INDEX_", getIndex());

	lua_.set("_ROWCOUNT_", max_);

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

	//執行短腳本
	lua_.safe_script(R"(
collectgarbage("setpause", 100)
collectgarbage("setstepmul", 100);
collectgarbage("step", 1024);
	)");

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

	lua_["package"]["path"] = std::string(paths.join(";").toUtf8().constData());

	if (isHookEnabled_)
	{
		lua_State* L = lua_.lua_state();
		lua_sethook(L, &luadebug::hookProc, LUA_MASKLINE | LUA_MASKCALL | LUA_MASKRET, NULL);// | LUA_MASKCOUNT
	}
}

void CLua::proc()
{
	do
	{
		if (scriptContent_.simplified().isEmpty())
			break;

		max_ = scriptContent_.split("\n").size();

		isRunning_.store(true, std::memory_order_release);

		lua_State* L = lua_.lua_state();
		sol::this_state s = L;

		openlibs();

		QStringList tableStrs;
		std::string luaCode = scriptContent_.toUtf8().constData();

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
					qint64 retline = -1;
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
				else if (retObject.is<qint64>())
				{
					tableStrs << "> (integer)" + util::toQString(retObject.as<qint64>());

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
						if (!key.is<std::string>() && !key.is<qint64>()) continue;
						QString qkey = key.is<std::string>() ? util::toQString(key) : util::toQString(key.as<qint64>());

						if (val.is<bool>())
						{
							tableStrs << QString(R"(>     ["%1"] = (boolean)%2,)").arg(qkey).arg(util::toQString(val.as<bool>()));
						}
						else if (val.is<qint64>())
						{
							tableStrs << QString(R"(>     ["%1"] = (integer)%2,)").arg(qkey).arg(val.as<qint64>());
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

	isRunning_.store(false, std::memory_order_release);
	emit finished();

	qint64 currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	emit signalDispatcher.scriptFinished();
}