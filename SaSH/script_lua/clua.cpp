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

			if (timer.hasExpired(180000))
				break;

			QThread::msleep(100);;
		}

		QThread::msleep(1000);
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

			if (!gamedevice.worker->getBattleFlag())
				break;
			if (timer.hasExpired(180000))
				break;

			QThread::msleep(100);;
		}

		QThread::msleep(1000);
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
	long long currentline = lua["LINE"].get<long long>();//getCurrentLine(s);

	msg = (QString("[%1 | @%2]: %3\0") \
		.arg(timeStr)
		.arg(currentline, 3, 10, QLatin1Char(' ')).arg(data));


	long long currentIndex = lua["__INDEX"].get<long long>();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	emit signalDispatcher.appendScriptLog(msg, color);
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (!gamedevice.worker.isNull() && !doNotAnnounce)
	{
		gamedevice.worker->announce(data, color);
	}

	if (gamedevice.log.isOpen())
		gamedevice.log.write(data, currentline);
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

		QThread::msleep(100);;
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
		if (lua["__HOOKFORSTOP"].is<bool>() && lua["__HOOKFORSTOP"].get<bool>())
		{
			luadebug::checkStopAndPause(s);
			break;
		}

		long long currentLine = ar->currentline;
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(lua["__INDEX"].get<long long>());
		long long max = lua["ROWCOUNT"];

		GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());

		emit signalDispatcher.scriptLabelRowTextChanged(currentLine, max, false);

		processDelay(s);
		if (gamedevice.IS_SCRIPT_DEBUG_ENABLE.get())
		{
			QThread::msleep(1);
		}
		else
		{
			luadebug::checkStopAndPause(s);
			return;
		}

		luadebug::checkStopAndPause(s);

		CLua* pLua = lua["__THIS_CLUA"].get<CLua*>();
		if (pLua == nullptr)
			return;

		QString scriptFileName = gamedevice.currentScriptFileName.get();

		safe::hash<long long, break_marker_t> breakMarkers = gamedevice.break_markers.value(scriptFileName);
		const safe::hash<long long, break_marker_t> stepMarkers = gamedevice.step_markers.value(scriptFileName);
		if (!(breakMarkers.contains(currentLine) || stepMarkers.contains(currentLine)))
		{
			return;//檢查是否有中斷點
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
#pragma endregion

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

void CLua::open_utillibs(sol::state&)
{

}

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

	long long getWindowType() const { GameDevice& gamedevice = GameDevice::getInstance(index_); return gamedevice.worker->currentDialog.get().windowtype; }
	long long getButtonType() const { GameDevice& gamedevice = GameDevice::getInstance(index_); return gamedevice.worker->currentDialog.get().buttontype; }
	long long getDialogId() const { GameDevice& gamedevice = GameDevice::getInstance(index_); return gamedevice.worker->currentDialog.get().dialogid; }
	long long getUnitId() const { GameDevice& gamedevice = GameDevice::getInstance(index_); return gamedevice.worker->currentDialog.get().unitid; }
	std::string getData() const { GameDevice& gamedevice = GameDevice::getInstance(index_); return util::toConstData(gamedevice.worker->currentDialog.get().data); }
	std::string  getLineData(long long index) const { GameDevice& gamedevice = GameDevice::getInstance(index_); return util::toConstData(gamedevice.worker->currentDialog.get().linedatas.join("|")); }
	std::string  getButtonText(long long index) const { GameDevice& gamedevice = GameDevice::getInstance(index_); return util::toConstData(gamedevice.worker->currentDialog.get().linebuttontext.join("|")); }

	bool contains(std::string str, sol::this_state s);

private:
	long long index_ = 0;
};

bool CLuaDialog::contains(std::string str, sol::this_state s)
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

std::string sa::team_t::getFreeName() const
{
	GameDevice& gamedevice = GameDevice::getInstance(controlIndex);
	if (gamedevice.worker.isNull())
		return "";

	gamedevice.worker->updateDatasFromMemory();

	return util::toConstData(gamedevice.worker->mapUnitHash.value(id).freeName);
}

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

	long long count()
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

			if (ovalue.is<std::string>())
			{
				sol::protected_function format = lua["format"];
				if (!format.valid())
					return 0;

				sol::object o = format.call(ovalue.as<std::string>());
				if (!o.valid())
					return 0;

				return print.call(o, ocolor).get<long long>();
			}

			return print.call(ovalue, ocolor).get<long long>();
		});

	//直接覆蓋print會無效,改成在腳本內中轉覆蓋
	lua.safe_script(R"(
		print = printf;
	)", sol::script_pass_on_error);

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

	lua.new_usertype<util::timer>("Timer",
		sol::call_constructor,
		sol::constructors<util::timer()>(),

		"restart", &util::timer::restart,
		"expired", &util::timer::hasExpired,
		"get", &util::timer::cost
	);

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

			static const QHash<long long, QString> hash = {
				{ sa::kTimeNoon, QObject::tr("noon") },
				{ sa::kTimeEvening, QObject::tr("evening") },
				{ sa::kTimeNight , QObject::tr("night") },
				{ sa::kTimeMorning, QObject::tr("morning") },
			};

			long long satime = gamedevice.worker->saCurrentGameTime.get();
			QString timeStr = hash.value(satime, QObject::tr("unknown"));
			return util::toConstData(timeStr);
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
		"contains", &CLuaChat::contains
	);

	lua.safe_script("chat = ChatClass(__INDEX);", sol::script_pass_on_error);
	lua.collect_garbage();

	lua.new_usertype<CLuaTimer>("timer",
		sol::call_constructor,
		sol::constructors<CLuaTimer()>(),

		"restart", &CLuaTimer::restart,
		"expired", &CLuaTimer::hasExpired,
		"getstr", &CLuaTimer::toFormatedString,
		"tostr", &CLuaTimer::toString,
		"todb", &CLuaTimer::toDouble,
		"msec", sol::property(&CLuaTimer::cost),
		"sec", sol::property(&CLuaTimer::costSeconds)
	);

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
		"contains", &CLuaCard::contains
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
		"count", sol::property(&CLuaTeam::count)
	);

	lua.safe_script("team = TeamClass(__INDEX);", sol::script_pass_on_error);
	lua.collect_garbage();
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

		/*custom*/
		"max", sol::readonly(&sa::item_t::maxStack),
		"count", sol::readonly(&sa::item_t::count)
	);

	lua.new_usertype<CLuaItem>("ItemClass",
		sol::call_constructor,
		sol::constructors<CLuaItem(long long)>(),
		sol::meta_function::index, &CLuaItem::operator[],
		"space", sol::property(&CLuaItem::getSpace),
		"isfull", sol::property(&CLuaItem::getIsFull),
		"count", &CLuaItem::count,
		"indexof", &CLuaItem::indexof,
		"find", &CLuaItem::find
	);

	lua.safe_script("item = ItemClass(__INDEX);", sol::script_pass_on_error);
	lua.collect_garbage();
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

	sa::profession_skill_t find(std::string sname)
	{
		GameDevice& gamedevice = GameDevice::getInstance(index_);
		if (gamedevice.worker.isNull())
			return sa::profession_skill_t();

		QString name = util::toQString(sname);

		if (name.isEmpty())
			return sa::profession_skill_t();

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
				return skill;
			else if (!isExist && skillname.contains(newName))
				return skill;
			else if (!memo.isEmpty() && memo.contains(name, Qt::CaseInsensitive))
				return skill;
		}

		return sa::profession_skill_t();
	}

private:
	long long index_ = 0;
};

void CLua::open_charlibs(sol::state& lua)
{
	lua.set_function("join", &CLuaChar::join, &luaChar_);
	lua.set_function("leave", &CLuaChar::leave, &luaChar_);
	lua.set_function("kick", &CLuaChar::kick, &luaChar_);
	lua.set_function("skup", &CLuaChar::skup, &luaChar_);

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
		"data", sol::property(&CLuaChar::getCharacter)
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
		"find", &CLuaSkill::find
	);

	lua.safe_script("skill = SkillClass(__INDEX);", sol::script_pass_on_error);
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
		"count", &CLuaPet::count
	);

	lua.safe_script("pet = PetClass(__INDEX);", sol::script_pass_on_error);
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
}

void CLua::open_battlelibs(sol::state& lua)
{
	//registerFunction("bwait", &Interpreter::bwait);
	//registerFunction("bend", &Interpreter::bend);

	lua.set_function("bh", &CLuaBattle::charUseAttack, &luaBattle_);
	lua.set_function("bj", &CLuaBattle::charUseMagic, &luaBattle_);
	lua.set_function("bp", &CLuaBattle::charUseSkill, &luaBattle_);
	lua.set_function("bs", &CLuaBattle::switchPet, &luaBattle_);
	lua.set_function("be", &CLuaBattle::escape, &luaBattle_);
	lua.set_function("bd", &CLuaBattle::defense, &luaBattle_);
	lua.set_function("bi", &CLuaBattle::useItem, &luaBattle_);
	lua.set_function("bt", &CLuaBattle::catchPet, &luaBattle_);
	lua.set_function("bn", &CLuaBattle::nothing, &luaBattle_);
	lua.set_function("bw", &CLuaBattle::petUseSkill, &luaBattle_);
	lua.set_function("bwn", &CLuaBattle::petNothing, &luaBattle_);
	lua.set_function("bwait", &CLuaBattle::bwait, &luaBattle_);
	lua.set_function("bend", &CLuaBattle::bend, &luaBattle_);

	lua.new_usertype<sa::battle_object_t>("BattleStruct",
		"ready", sol::readonly(&sa::battle_object_t::ready),
		"pos", sol::readonly(&sa::battle_object_t::pos),
		"modelid", sol::readonly(&sa::battle_object_t::modelid),
		"level", sol::readonly(&sa::battle_object_t::level),
		"hp", sol::readonly(&sa::battle_object_t::hp),
		"maxhp", sol::readonly(&sa::battle_object_t::maxHp),
		"hpp", sol::readonly(&sa::battle_object_t::hpPercent),
		"rideflag", sol::readonly(&sa::battle_object_t::rideFlag),
		"ridelevel", sol::readonly(&sa::battle_object_t::rideLevel),
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
		"alliecount", sol::property(&CLuaBattle::alliecount)
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

	//執行短腳本
	lua_.safe_script(R"(
collectgarbage("setpause", 100)
collectgarbage("setstepmul", 100);
collectgarbage("step", 1024);
	)", sol::script_pass_on_error);

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