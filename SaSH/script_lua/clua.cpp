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


#if QT_NO_DEBUG
#pragma comment(lib, "lua-5.4.4.lib")
#else
#pragma comment(lib, "lua-5.4.4d.lib")
#endif

#define OPEN_HOOK

extern util::SafeHash<QString, util::SafeHash<qint64, break_marker_t>> break_markers;//interpreter.cpp//用於標記自訂義中斷點(紅點)
extern util::SafeHash<QString, util::SafeHash<qint64, break_marker_t>> forward_markers;//interpreter.cpp//用於標示當前執行中斷處(黃箭頭)
extern util::SafeHash<QString, util::SafeHash<qint64, break_marker_t>> error_markers;//interpreter.cpp//用於標示錯誤發生行(紅線)
extern util::SafeHash<QString, util::SafeHash<qint64, break_marker_t>> step_markers;//interpreter.cpp//隱式標記中斷點用於單步執行(無)

void luadebug::tryPopCustomErrorMsg(const sol::this_state& s, const LUA_ERROR_TYPE element, const QVariant& p1, const QVariant& p2, const QVariant& p3, const QVariant& p4)
{
	Q_UNUSED(p4);
	lua_State* L = s;
	////檢查當前行是否包含在#param中
	//if (QLuaDebuger::CHECK_PROGMA(L, "argument"))
		//return;

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
		int topsize = lua_gettop(L);
		const QString qmsgstr(QString(errormsg_str.value(element)).arg(p1.toInt()).arg(topsize));
		const std::string str(qmsgstr.toUtf8().constData());
		luaL_argcheck(L, topsize == p1.toInt(), topsize, str.c_str());
		break;
	}
	case ERROR_PARAM_SIZE_NONE://無參數數量錯誤
	{
		int topsize = lua_gettop(L);
		const QString qmsgstr(QString(errormsg_str.value(element)).arg(topsize));
		const std::string str(qmsgstr.toUtf8().constData());
		luaL_argcheck(L, topsize == 0, 1, str.c_str());
		break;
	}
	case ERROR_PARAM_SIZE_RANGE://範圍參數數量錯誤
	{
		int topsize = lua_gettop(L);
		const QString qmsgstr(QString(errormsg_str.value(element)).arg(p1.toInt()).arg(p2.toInt()).arg(topsize));
		const std::string str(qmsgstr.toUtf8().constData());
		luaL_argcheck(L, topsize >= p1.toInt() && topsize <= p2.toInt(), 1, str.c_str());
		break;
	}
	case ERROR_PARAM_VALUE://數值錯誤 p1 第幾個參數 p2邏輯 str訊息
	{
		const QString qmsgstr(p3.toString());
		const std::string str(qmsgstr.toUtf8().constData());
		luaL_argcheck(L, p2.toBool(), p1.toInt(), str.c_str());
		break;
	}
	case ERROR_PARAM_TYPE://參數預期錯誤 p1 第幾個參數 p2邏輯 str訊息
	{
		const QString qmsgstr(p3.toString());
		const std::string str(qmsgstr.toUtf8().constData());
		luaL_argexpected(L, p2.toBool(), p1.toInt(), str.c_str());
		break;
	}
	default:
	{
		luaL_error(L, "UNKNOWN ERROR TYPE");
		break;
	}
	}
}

QString luadebug::getErrorMsgLocatedLine(const QString& str, int* retline)
{
	const QString cmpstr(str.simplified());

	QRegularExpressionMatch match = rexGetLine.match(cmpstr);
	QRegularExpressionMatch match2 = reGetLineEx.match(cmpstr);
	static const auto matchies = [](const QRegularExpressionMatch& m, int* retline)->void
	{
		int size = m.capturedTexts().size();
		if (size > 1)
		{
			for (int i = (size - 1); i >= 1; --i)
			{
				const QString s = m.captured(i).simplified();
				if (!s.isEmpty())
				{
					bool ok = false;
					int row = s.simplified().toInt(&ok);
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

QString luadebug::getTableVars(lua_State*& L, int si, int depth)
{
	if (!L) return "\0";
	QPair<QString, QString> pa;
	int pos_si = si > 0 ? si : (si - 1);
	QString ret("{");
	int top = lua_gettop(L);
	lua_pushnil(L);
	int empty = 1;
	while (lua_next(L, pos_si) != 0)
	{
		if (empty)
		{
			ret += ("\r\n");
			empty = 0;
		}

		int i;
		for (i = 0; i < depth; i++)
		{
			ret += (" ");
		}

		ret += ("[");
		pa = getVars(L, -2, -1);
		ret += (R"(")" + pa.second + R"(")");
		ret += ("] = ");
		if (depth > 20)
		{
			ret += ("{...}");
		}
		else
		{
			pa = getVars(L, -1, depth + 1);
			ret += (pa.first + " " + pa.second);
		}
		lua_pop(L, 1);
		ret += (",\r\n");
	}

	if (empty)
	{
		ret += (" }");
	}
	else
	{
		int i;
		for (i = 0; i < depth - 1; i++)
		{
			ret += (" ");
		}
		ret += ("}");
	}
	lua_settop(L, top);
	return ret;
}

QPair<QString, QString> luadebug::getVars(lua_State*& L, int si, int depth)
{
	switch (lua_type(L, si))
	{
	case LUA_TNIL:
	{
		return { "(nil)" , "nil" };
	}

	case LUA_TNUMBER:
	{
		return { "(integer)", QString::number(luaL_checkinteger(L, si)) };
	}

	case LUA_TBOOLEAN:
	{
		return { "(boolean)", lua_toboolean(L, si) ? "true" : "false" };
	}

	case LUA_TFUNCTION:
	{
		lua_CFunction func = lua_tocfunction(L, si);
		if (func != NULL)
		{
			return { "(C function)", QString("0x%1").arg(QString::number(reinterpret_cast<qint64>(func)),16) };
		}
		else
		{
			return { "(function)", QString("0x%1").arg(QString::number(reinterpret_cast<qint64>(func)),16) };
		}
		break;
	}


	case LUA_TUSERDATA:
	{
		return { "(user data)", QString("0x%1").arg(QString::number(reinterpret_cast<qint64>(lua_touserdata(L, si)),16)) };
	}

	case LUA_TSTRING:
	{
		return { "(string)", luaL_checkstring(L, si) };
	}

	case LUA_TTABLE:
	{
		//print_table_var(state, si, depth);
		return { "(table)" , getTableVars(L, si, depth) };
	}

	default:
		break;
	}
	return { "", "" };
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
	CLua* pLua = lua["_THIS"].get<CLua*>();
	if (pLua == nullptr)
		return;

	if (pLua->isInterruptionRequested())
	{
		luadebug::tryPopCustomErrorMsg(s, luadebug::ERROR_FLAG_DETECT_STOP);
		return;
	}
	pLua->checkPause();
}

bool luadebug::checkOnlineThenWait(const sol::this_state& s)
{
	checkStopAndPause(s);

	Injector& injector = Injector::getInstance();
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

		QThread::msleep(500);
	}
	return bret;
}

bool luadebug::checkBattleThenWait(const sol::this_state& s)
{
	checkStopAndPause(s);

	Injector& injector = Injector::getInstance();
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

		QThread::msleep(500);
	}
	return bret;
}

void luadebug::processDelay(const sol::this_state& s)
{
	Injector& injector = Injector::getInstance();
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
		qint64 currentLine = ar->currentline;
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
		qint64 max = lua["_ROWCOUNT"];

		Injector& injector = Injector::getInstance();

		emit signalDispatcher.scriptLabelRowTextChanged(currentLine, max, false);

		processDelay(s);
		if (!lua["_DEBUG"].is<bool>() || lua["_DEBUG"].get<bool>() || injector.isScriptDebugModeEnable.load(std::memory_order_acquire))
		{
			QThread::msleep(1);
		}
		else
		{
			luadebug::checkStopAndPause(s);
			return;
		}

		luadebug::checkStopAndPause(s);

		if (!injector.isScriptDebugModeEnable.load(std::memory_order_acquire))
			return;

		CLua* pLua = lua["_THIS"].get<CLua*>();
		if (pLua == nullptr)
			return;

		QString scriptFileName = injector.currentScriptFileName;

		util::SafeHash<qint64, break_marker_t> breakMarkers = break_markers.value(scriptFileName);
		const util::SafeHash<qint64, break_marker_t> stepMarkers = step_markers.value(scriptFileName);
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
			break_markers.insert(scriptFileName, breakMarkers);
			//所有行插入隱式斷點(用於單步)
			emit signalDispatcher.addStepMarker(currentLine, true);
		}

		emit signalDispatcher.addForwardMarker(currentLine, true);

		////獲取區域變量數值
		//int i;
		//const char* name = nullptr;
		//QVariantHash varhash;
		//for (i = 1; (name = lua_getlocal(L, ar, i)) != NULL; i++)
		//{
		//	QPair<QString, QString> vs = getVars(L, i, 5);

		//	QString key = QString("local|%1").arg(QString::fromUtf8(name));
		//	varhash.insert(key, vs.second);
		//	//var.type = vs.first.replace("(", "").replace(")", "");
		//	lua_pop(L, 1);// no match, then pop out the var's value
		//}

		//emit signalDispatcher.varInfoImported(varhash);

		luadebug::checkStopAndPause(s);

		break;
	}
	default:
		break;
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
	for (int i = 0; i < list.size(); ++i)
	{
		QFileInfo fileInfo = list.at(i);
		result->append(fileInfo.filePath());
		getPackagePath(fileInfo.filePath(), result);
	}
}

void luadebug::logExport(const sol::this_state& s, const QStringList& datas, qint64 color)
{
	for (const QString& data : datas)
	{
		logExport(s, data, color);
	}
}

void luadebug::logExport(const sol::this_state& s, const QString& data, qint64 color)
{

	//打印當前時間
	const QDateTime time(QDateTime::currentDateTime());
	const QString timeStr(time.toString("hh:mm:ss:zzz"));
	QString msg = "\0";
	QString src = "\0";

	qint64 currentline = getCurrentLine(s);

	msg = (QString("[%1 | @%2]: %3\0") \
		.arg(timeStr)
		.arg(currentline + 1, 3, 10, QLatin1Char(' ')).arg(data));

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	emit signalDispatcher.appendScriptLog(msg, color);
}

//根據傳入function的循環執行結果等待超時或條件滿足提早結束
bool luadebug::waitfor(const sol::this_state& s, qint64 timeout, std::function<bool()> exprfun)
{
	if (timeout < 0)
		timeout = std::numeric_limits<qint64>::max();

	Injector& injector = Injector::getInstance();
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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CLua::CLua(QObject* parent)
	: ThreadPlugin(parent)
{

}

CLua::CLua(const QString& content, QObject* parent)
	: ThreadPlugin(parent)
	, scriptContent_(content)
{
	qDebug() << "CLua";
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

	//在lua中的使用方法: TEST.EnumValue1
}

//以Metatable方式註冊函數 支援函數多載、運算符重載，面向對象，常量
void CLua::open_testlibs()
{
	sol::usertype<CLuaTest> test = lua_.new_usertype<CLuaTest>("Test",
		sol::call_constructor,
		sol::constructors<
		/*建構函數多載*/
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

	/*
	在lua中的使用方法:

	local test = Test(1)
	print(test:add(1, 2))
	print(test:add(2))
	print(test:sub(1, 2))

	*/
}

void CLua::open_utillibs()
{
	lua_.new_usertype<CLuaUtil>("InfoClass",
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


	lua_.safe_script(R"(
			info = InfoClass();
		)");
}

//luasystem.cpp
void CLua::open_syslibs()
{
	lua_.set_function("sleep", &CLuaSystem::sleep, &luaSystem_);
	lua_.set_function("printf", &CLuaSystem::announce, &luaSystem_);
	lua_.safe_script(R"(
		_print = print;
		print = printf;
	)");

	lua_.set_function("msg", &CLuaSystem::messagebox, &luaSystem_);
	lua_.set_function("saveset", &CLuaSystem::savesetting, &luaSystem_);
	lua_.set_function("loadset", &CLuaSystem::loadsetting, &luaSystem_);
	lua_.set_function("set", &CLuaSystem::set, &luaSystem_);
	lua_.set_function("lclick", &CLuaSystem::leftclick, &luaSystem_);
	lua_.set_function("rclick", &CLuaSystem::rightclick, &luaSystem_);
	lua_.set_function("dbclick", &CLuaSystem::leftdoubleclick, &luaSystem_);
	lua_.set_function("dragto", &CLuaSystem::mousedragto, &luaSystem_);

	lua_.new_usertype<CLuaSystem>("SystemClass",
		sol::call_constructor,
		sol::constructors<CLuaSystem()>(),

		"out", &CLuaSystem::logout,
		"back", &CLuaSystem::logback,
		"eo", &CLuaSystem::eo,
		"say", &CLuaSystem::talk,
		"clearChat", &CLuaSystem::cleanchat,
		"menu", sol::overload(
			sol::resolve<qint64(qint64, sol::this_state)>(&CLuaSystem::menu),
			sol::resolve<qint64(qint64, qint64, sol::this_state)>(&CLuaSystem::menu)
		),

		"press", sol::overload(
			sol::resolve<qint64(std::string, qint64, qint64, sol::this_state)>(&CLuaSystem::press),
			sol::resolve<qint64(qint64, qint64, qint64, sol::this_state)>(&CLuaSystem::press)
		),

		"input", &CLuaSystem::input
	);

	lua_.safe_script(R"(
			sys = SystemClass();
		)");
}

void CLua::open_itemlibs()
{
	lua_.new_usertype<CLuaItem>("ItemClass",
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

	lua_.safe_script(R"(
			item = ItemClass();
		)");
}

void CLua::open_charlibs()
{
	lua_.new_usertype<CLuaChar>("CharClass",
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

	lua_.safe_script(R"(
			char = CharClass();
		)");
}

void CLua::open_petlibs()
{
	lua_.new_usertype<CLuaPet>("PetClass",
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

	lua_.safe_script(R"(
			pet = PetClass();
		)");
}

void CLua::open_maplibs()
{
	lua_.new_usertype<CLuaMap>("MapClass",
		sol::call_constructor,
		sol::constructors<CLuaMap()>(),
		"setDir", sol::overload(
			sol::resolve<qint64(qint64, sol::this_state)>(&CLuaMap::setDir),
			sol::resolve<qint64(qint64, qint64, sol::this_state) >(&CLuaMap::setDir),
			sol::resolve<qint64(std::string, sol::this_state) >(&CLuaMap::setDir)
		),
		"move", &CLuaMap::move,
		"packetMove", &CLuaMap::packetMove,
		"teleport", &CLuaMap::teleport,
		"findPath", &CLuaMap::findPath,
		"download", &CLuaMap::downLoad
	);

	lua_.safe_script(R"(
			map = MapClass();
		)");
}

void CLua::open_battlelibs()
{
	lua_.new_usertype<CLuaBattle>("BattleClass",
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

	lua_.safe_script(R"(
			battle = BattleClass();
		)");
}

void CLua::proc()
{
	do
	{
		if (scriptContent_.simplified().isEmpty())
			break;

		isRunning_.store(true, std::memory_order_release);

		lua_State* L = lua_.lua_state();
		sol::this_state s = L;

		lua_.set("_THIS", this);// 將this指針傳給lua設置全局變量
		lua_.set("_THIS_PARENT", parent_);// 將父類指針傳給lua設置全局變量
		lua_.set("_INDEX", index_);
		lua_.set("_ROWCOUNT", scriptContent_.split("\n").size());

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
		open_utillibs();
		open_syslibs();
		open_itemlibs();
		open_charlibs();
		open_petlibs();
		open_maplibs();
		open_battlelibs();

		//執行短腳本
		lua_.safe_script(R"(
			collectgarbage("setpause", 100)
			collectgarbage("setstepmul", 100);
			collectgarbage("step", 1024);
		)");

		//Add additional package path.
		QStringList paths;
		std::string package_path = lua_["package"]["path"];
		paths.append(QString::fromUtf8(package_path.c_str()).replace("\\", "/"));

		QStringList dirs;
		luadebug::getPackagePath(util::applicationDirPath() + "/", &dirs);
		for (const QString& it : dirs)
		{
			QString path = it + "/?.lua";
			paths.append(path.replace("\\", "/"));
		}

		lua_["package"]["path"] = std::string(paths.join(";").toUtf8().constData());

#ifdef OPEN_HOOK
		lua_sethook(L, &luadebug::hookProc, LUA_MASKLINE | LUA_MASKCALL | LUA_MASKRET, NULL);// | LUA_MASKCOUNT 
#endif

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
				qstrErr = QString::fromUtf8(err.what());
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
					int retline = -1;
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
						//emit this->addErrorMarker(retline, true);
						if (isDebug_)
							tableStrs << tr("> message: ");
						tableStrs << "> [info]:" + msg;
					}
					else
					{
						tableStrs << tr("========== lua script stop with an ERROR ==========");
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
					tableStrs << QString("> (boolean)%1").arg(retObject.as<bool>() ? "true" : "false");
				}
				else if (retObject.is<qint64>())
				{
					tableStrs << "> (integer)" + QString::number(retObject.as<qint64>());

				}
				else if (retObject.is<double>())
				{
					tableStrs << "> (number)" + QString::number(retObject.as<double>(), 'f', 16);
				}
				else if (retObject.is<std::string>())
				{
					tableStrs << "> (string)" + QString::fromUtf8(retObject.as<std::string>().c_str());
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
						if (!key.is<std::string>() && !key.is<int>()) continue;
						QString qkey = key.is<std::string>() ? QString::fromUtf8(key.as<std::string>().c_str()) : QString::number(key.as<int>());

						if (val.is<bool>())
						{
							tableStrs << QString(R"(>     ["%1"] = (boolean)%2,)").arg(qkey).arg(val.as<bool>() ? "true" : "false");
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
							tableStrs << QString(R"(>     ["%1"] = (string)%2,)").arg(qkey).arg(QString::fromUtf8(val.as<std::string>().c_str()));
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
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	emit signalDispatcher.scriptFinished();
}