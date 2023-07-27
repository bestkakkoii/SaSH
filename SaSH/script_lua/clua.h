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


	//從錯誤訊息中擷取行號
	static const QRegularExpression rexGetLine(R"(at[\s]*line[\s]*(\d+))");
	static const QRegularExpression reGetLineEx(R"(\]:(\d+)(?=\s*:))");
	Q_REQUIRED_RESULT QString getErrorMsgLocatedLine(const QString& str, int* retline);

	void hookProc(lua_State* L, lua_Debug* ar);
}

class CLuaTest
{
public:
	CLuaTest(sol::this_state) {};
	CLuaTest(lua_Integer a, sol::this_state) : a_(a) {};
	~CLuaTest() = default;
	lua_Integer add(lua_Integer a, lua_Integer b) { return a + b; };
	lua_Integer add(lua_Integer b) { return a_ + b; };

	lua_Integer sub(lua_Integer a, lua_Integer b) { return a - b; };

private:
	lua_Integer a_ = 0;
};

class CLua : public ThreadPlugin
{
	Q_OBJECT
public:
	CLua() = default;
	explicit CLua(QObject* parent = nullptr);
	CLua(const QString& content, QObject* parent = nullptr);
	virtual ~CLua();


private:
	void open_enumlibs();
	void open_testlibs();

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
};