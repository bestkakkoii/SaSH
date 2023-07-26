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
	explicit CLua(QObject* parent);
	virtual ~CLua();

public slots:
	void run();

private:
	void open_enumlibs();
	void open_testlibs();

private:
	sol::state lua_;
	DWORD tid_ = 0UL;
	int index_ = -1;
	QElapsedTimer scriptTimer_;
	QString scriptContent_;
	bool isSubScript_ = false;
	bool isDebug_ = false;
};