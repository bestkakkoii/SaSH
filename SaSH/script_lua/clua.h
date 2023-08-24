#pragma once

#include <threadplugin.h>
#include <util.h>

#ifndef SOL_ALL_SAFETIES_ON
#define SOL_ALL_SAFETIES_ON 1
#include <lua/lua.hpp>
#include <sol/sol.hpp>
#endif

namespace luadebug
{
	enum LUA_ERROR_TYPE
	{
		ERROR_NOTUSED,
		ERROR_FLAG_DETECT_STOP,
		ERROR_REQUEST_STOP_FROM_USER,
		ERROR_REQUEST_STOP_FROM_SCRIP,
		ERROR_REQUEST_STOP_FROM_PARENT_SCRIP,
		ERROR_REQUEST_STOP_FROM_DISTRUCTOR,
		ERROR_PARAM_SIZE,
		ERROR_PARAM_SIZE_NONE,
		ERROR_PARAM_SIZE_RANGE,
		ERROR_PARAM_VALUE,
		ERROR_PARAM_TYPE,
	};

	static const QHash<LUA_ERROR_TYPE, QString> errormsg_str = {
		{ ERROR_NOTUSED, QObject::tr("unknown") },
		{ ERROR_FLAG_DETECT_STOP, QObject::tr("FLAG_DETECT_STOP") },
		{ ERROR_REQUEST_STOP_FROM_USER, QObject::tr("REQUEST_STOP_FROM_USER") },
		{ ERROR_REQUEST_STOP_FROM_SCRIP, QObject::tr("REQUEST_STOP_FROM_SCRIP") },
		{ ERROR_REQUEST_STOP_FROM_PARENT_SCRIP, QObject::tr("REQUEST_STOP_FROM_PARENT_SCRIP") },
		{ ERROR_REQUEST_STOP_FROM_DISTRUCTOR, QObject::tr("REQUEST_STOP_FROM_DISTRUCTOR") },
		{ ERROR_PARAM_SIZE, QObject::tr("request param size is %1, but import %2") },
		{ ERROR_PARAM_SIZE_NONE, QObject::tr("no param request, but import %1") },
		{ ERROR_PARAM_SIZE_RANGE, QObject::tr("request param size is between %1 to %2, but import %3") },
	};

	inline Q_REQUIRED_RESULT int getCurrentLine(lua_State* L)
	{
		if (L)
		{
			lua_Debug info = {};
			if (lua_getstack(L, 1, &info))
			{
				lua_getinfo(L, "l", &info);
				return info.currentline;
			}
		}
		return -1;
	}

	inline Q_REQUIRED_RESULT int getCurrentLine(const sol::this_state& s)
	{
		lua_State* L = s.lua_state();
		if (L)
			return getCurrentLine(L);
		else
			return -1;
	}

	inline Q_REQUIRED_RESULT lua_Debug getCurrentInfo(lua_State* L)
	{
		lua_Debug info = {};
		if (L)
		{

			if (lua_getstack(L, 1, &info))
			{
				lua_getinfo(L, "nSltu", &info);
				return info;
			}
		}
		return info;
	}

	inline Q_REQUIRED_RESULT lua_Debug getCurrentInfo(const sol::this_state& s)
	{
		lua_State* L = s.lua_state();
		return getCurrentInfo(L);
	}

	void tryPopCustomErrorMsg(const sol::this_state& s, const LUA_ERROR_TYPE element, const QVariant& p1 = 0, const QVariant& p2 = 0, const QVariant& p3 = 0, const QVariant& p4 = 0);

	void checkStopAndPause(const sol::this_state& s);
	bool checkBattleThenWait(const sol::this_state& s);

	bool isInterruptionRequested(const sol::this_state& s);

	//根據傳入function的循環執行結果等待超時或條件滿足提早結束
	bool waitfor(const sol::this_state& s, qint64 timeout, std::function<bool()> exprfun);

	//遞歸獲取每一層目錄
	void getPackagePath(const QString base, QStringList* result);

	//從錯誤訊息中擷取行號
	static const QRegularExpression rexGetLine(R"(at[\s]*line[\s]*(\d+))");
	static const QRegularExpression reGetLineEx(R"(\]:(\d+)(?=\s*:))");
	Q_REQUIRED_RESULT QString getErrorMsgLocatedLine(const QString& str, int* retline);

	void hookProc(lua_State* L, lua_Debug* ar);

	void logExport(const sol::this_state& s, const QStringList& datas, qint64 color);
	void logExport(const sol::this_state& s, const QString& data, qint64 color);
}

class CLuaTest
{
public:
	CLuaTest(sol::this_state) {};
	CLuaTest(qint64 a, sol::this_state) : a_(a) {};
	~CLuaTest() = default;
	qint64 add(qint64 a, qint64 b) { return a + b; };
	qint64 add(qint64 b) { return a_ + b; };

	qint64 sub(qint64 a, qint64 b) { return a - b; };

private:
	qint64 a_ = 0;
};

class CLuaUtil
{
public:
	CLuaUtil() = default;
	~CLuaUtil() = default;

	qint64 ocr(qint64 currentline, sol::this_state s);
	qint64 dlg(qint64 currentline, sol::this_state s);
	qint64 regex(qint64 currentline, sol::this_state s);
	qint64 find(qint64 currentline, sol::this_state s);
	qint64 half(qint64 currentline, sol::this_state s);
	qint64 full(qint64 currentline, sol::this_state s);
	qint64 upper(qint64 currentline, sol::this_state s);
	qint64 lower(qint64 currentline, sol::this_state s);
	qint64 replace(qint64 currentline, sol::this_state s);
	qint64 toint(std::string str, sol::this_state s);
	qint64 tostr(sol::object onumber, sol::this_state s);

	qint64 trim(std::string str, sol::this_state s);
};

class CLuaSystem
{
public:
	CLuaSystem() = default;
	~CLuaSystem() = default;

	//global
	qint64 sleep(qint64 value, sol::this_state s);
	qint64 announce(sol::object ostr, sol::object ocolor, sol::this_state s);
	qint64 messagebox(sol::object ostr, sol::object otype, sol::this_state s);
	qint64 savesetting(const std::string& fileName, sol::this_state s);
	qint64 loadsetting(const std::string& fileName, sol::this_state s);
	qint64 set(qint64 enumInt, sol::object p1, sol::object p2, sol::object p3, sol::object p4, sol::object p5, sol::object p6, sol::object p7, sol::this_state s);
	qint64 leftclick(qint64 x, qint64 y, sol::this_state s);
	qint64 rightclick(qint64 x, qint64 y, sol::this_state s);
	qint64 leftdoubleclick(qint64 x, qint64 y, sol::this_state s);
	qint64 mousedragto(qint64 x1, qint64 y1, qint64 x2, qint64 y2, sol::this_state s);

	//meta
	qint64 logout(sol::this_state s);
	qint64 logback(sol::this_state s);
	qint64 eo(sol::this_state s);
	qint64 cleanchat(sol::this_state s);
	qint64 talk(sol::object ostr, sol::this_state s);
	qint64 menu(qint64 index, sol::this_state s);
	qint64 menu(qint64 type, qint64 index, sol::this_state s);
	qint64 press(const std::string& buttonStr, sol::object onpcName, sol::object odlgId, sol::this_state s);
	qint64 press(qint64 row, sol::object onpcName, sol::object odlgId, sol::this_state s);
	qint64 input(const std::string& str, sol::object onpcName, sol::object odlgId, sol::this_state s);
};

class CLuaItem
{
public:
	CLuaItem() = default;
	~CLuaItem() = default;

	qint64 use(qint64 currentline, sol::this_state s);
	qint64 drop(qint64 currentline, sol::this_state s);
	qint64 pick(qint64 currentline, sol::this_state s);
	qint64 swap(qint64 currentline, sol::this_state s);
	qint64 make(qint64 currentline, sol::this_state s);
	qint64 cook(qint64 currentline, sol::this_state s);
	qint64 buy(qint64 currentline, sol::this_state s);
	qint64 sell(qint64 currentline, sol::this_state s);
	qint64 wear(qint64 currentline, sol::this_state s);
	qint64 unwear(qint64 currentline, sol::this_state s);
	qint64 trade(qint64 currentline, sol::this_state s);
	qint64 deposit(qint64 currentline, sol::this_state s);
	qint64 withdraw(qint64 currentline, sol::this_state s);
	qint64 record(qint64 currentline, sol::this_state s);//equip
};

class CLuaChar
{
public:
	CLuaChar() = default;
	~CLuaChar() = default;

	qint64 rename(qint64 currentline, sol::this_state s);
	qint64 use(qint64 currentline, sol::this_state s);
	qint64 deposit(qint64 currentline, sol::this_state s);//gold
	qint64 withdraw(qint64 currentline, sol::this_state s);//gold
	qint64 drop(qint64 currentline, sol::this_state s);//gold
	qint64 mail(qint64 currentline, sol::this_state s);
	qint64 add(qint64 currentline, sol::this_state s);//skup point

	//action-group
	qint64 join(qint64 currentline, sol::this_state s);
	qint64 leave(qint64 currentline, sol::this_state s);
	qint64 kick(qint64 currentline, sol::this_state s);
};


class CLuaPet
{
public:
	CLuaPet() = default;
	~CLuaPet() = default;

	qint64 setstate(qint64 currentline, sol::this_state s);
	qint64 drop(qint64 currentline, sol::this_state s);
	qint64 rename(qint64 currentline, sol::this_state s);
	qint64 sell(qint64 currentline, sol::this_state s);
	qint64 learn(qint64 currentline, sol::this_state s);
	qint64 equip(qint64 currentline, sol::this_state s);
	qint64 unequip(qint64 currentline, sol::this_state s);
	qint64 deposit(qint64 currentline, sol::this_state s);
	qint64 withdraw(qint64 currentline, sol::this_state s);
};

class CLuaMap
{
public:
	CLuaMap() = default;
	~CLuaMap() = default;

	qint64 setdir(qint64 currentline, sol::this_state s);
	qint64 move(qint64 currentline, sol::this_state s);
	qint64 fastmove(qint64 currentline, sol::this_state s);
	qint64 packetmove(qint64 currentline, sol::this_state s);
	qint64 findpath(qint64 currentline, sol::this_state s);
	qint64 movetonpc(qint64 currentline, sol::this_state s);
	qint64 warp(qint64 currentline, sol::this_state s);
	qint64 teleport(qint64 currentline, sol::this_state s);
};

class CLuaBattle
{
public:
	CLuaBattle() = default;
	~CLuaBattle() = default;

	qint64 bh(qint64 currentline, sol::this_state s);//atk
	qint64 bj(qint64 currentline, sol::this_state s);//magic
	qint64 bp(qint64 currentline, sol::this_state s);//skill
	qint64 bs(qint64 currentline, sol::this_state s);//switch
	qint64 be(qint64 currentline, sol::this_state s);//escape
	qint64 bd(qint64 currentline, sol::this_state s);//defense
	qint64 bi(qint64 currentline, sol::this_state s);//item
	qint64 bt(qint64 currentline, sol::this_state s);//catch
	qint64 bn(qint64 currentline, sol::this_state s);//nothing
	qint64 bw(qint64 currentline, sol::this_state s);//petskill
	qint64 bwf(qint64 currentline, sol::this_state s);//pet nothing
	qint64 bwait(qint64 currentline, sol::this_state s);//wait
	qint64 bend(qint64 currentline, sol::this_state s);//wait
};

class CLua : public ThreadPlugin
{
	Q_OBJECT
public:
	CLua() = default;
	explicit CLua(QObject* parent = nullptr);
	CLua(const QString& content, QObject* parent = nullptr);
	virtual ~CLua();

	void start();
	void wait();

signals:
	void finished();

private slots:
	void proc();

private:
	void open_enumlibs();
	void open_testlibs();
	void open_systemlibs();

private:
	sol::state lua_;
	CLua* parent_ = nullptr;
	QThread* thread_ = nullptr;
	DWORD tid_ = 0UL;
	int index_ = -1;
	QElapsedTimer scriptTimer_;
	QString scriptContent_;
	bool isSubScript_ = false;
	bool isDebug_ = false;

private:
	CLuaSystem luaSystem_;
};