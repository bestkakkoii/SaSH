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

import Utility;

#pragma once
#include <threadplugin.h>
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

	enum
	{
		WARN_LEVEL = 0,
		ERROR_LEVEL = 1,
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

	inline [[nodiscard]] __int64 getCurrentLine(lua_State* L)
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

	inline [[nodiscard]] __int64 getCurrentLine(const sol::this_state& s)
	{
		lua_State* L = s.lua_state();
		if (L)
			return getCurrentLine(L);
		else
			return -1;
	}

	inline [[nodiscard]] lua_Debug getCurrentInfo(lua_State* L)
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

	inline [[nodiscard]] lua_Debug getCurrentInfo(const sol::this_state& s)
	{
		lua_State* L = s.lua_state();
		return getCurrentInfo(L);
	}

	void tryPopCustomErrorMsg(const sol::this_state& s, const LUA_ERROR_TYPE element, const QVariant& p1 = 0, const QVariant& p2 = 0, const QVariant& p3 = 0, const QVariant& p4 = 0);


	void checkStopAndPause(const sol::this_state& s);
	bool checkBattleThenWait(const sol::this_state& s);
	bool checkOnlineThenWait(const sol::this_state& s);

	void processDelay(const sol::this_state& s);

	bool isInterruptionRequested(const sol::this_state& s);

	//根據傳入function的循環執行結果等待超時或條件滿足提早結束
	bool waitfor(const sol::this_state& s, __int64 timeout, std::function<bool()> exprfun);

	//遞歸獲取每一層目錄
	void getPackagePath(const QString base, QStringList* result);

	//從錯誤訊息中擷取行號
	static const QRegularExpression rexGetLine(R"(at[\s]*line[\s]*(\d+))");
	static const QRegularExpression reGetLineEx(R"(\]:(\d+)(?=\s*:))");
	[[nodiscard]] QString getErrorMsgLocatedLine(const QString& str, __int64* retline);

	QPair<QString, QVariant> getVars(lua_State*& L, __int64 si, __int64& depth);
	QString getTableVars(lua_State*& L, __int64 si, __int64& depth);

	void hookProc(lua_State* L, lua_Debug* ar);

	void logExport(const sol::this_state& s, const QStringList& datas, __int64 color, bool doNotAnnounce = false);
	void logExport(const sol::this_state& s, const QString& data, __int64 color, bool doNotAnnounce = false);
	void showErrorMsg(const sol::this_state& s, __int64 level, const QString& data);
}

class CLuaTest
{
public:
	CLuaTest(sol::this_state) {};
	CLuaTest(__int64 a, sol::this_state) : a_(a) {};
	~CLuaTest() = default;
	__int64 add(__int64 a, __int64 b) { return a + b; };
	__int64 add(__int64 b) { return a_ + b; };

	__int64 sub(__int64 a, __int64 b) { return a - b; };

private:
	__int64 a_ = 0;
};

class CLuaUtil
{
public:
	CLuaUtil() = default;
	~CLuaUtil() = default;

	//__int64 createDialog(__int64 currentline, sol::this_state s);
	//__int64 regex(__int64 currentline, sol::this_state s);
	//__int64 find(__int64 currentline, sol::this_state s);
	//__int64 half(__int64 currentline, sol::this_state s);
	//__int64 full(__int64 currentline, sol::this_state s);
	//__int64 upper(__int64 currentline, sol::this_state s);
	//__int64 lower(__int64 currentline, sol::this_state s);
	//__int64 replace(__int64 currentline, sol::this_state s);
	//__int64 trim(std::string str, sol::this_state s);

	bool getSys(sol::table dstTable, sol::this_state s);
	bool getMap(sol::table dstTable, sol::this_state s);
	bool getChar(sol::table dstTable, sol::this_state s);
	bool getPet(sol::table dstTable, sol::this_state s);
	bool getTeam(sol::table dstTable, sol::this_state s);
	bool getCard(sol::table dstTable, sol::this_state s);
	bool getChat(sol::table dstTable, sol::this_state s);
	bool getDialog(sol::table dstTable, sol::this_state s);
	bool getUnit(sol::table dstTable, sol::this_state s);
	bool getBattleUnit(sol::table dstTable, sol::this_state s);
	bool getDaily(sol::table dstTable, sol::this_state s);
	bool getItem(sol::table dstTable, sol::this_state s);
	bool getSkill(sol::table dstTable, sol::this_state s);
	bool getMagic(sol::table dstTable, sol::this_state s);
};

class CLuaSystem
{
public:
	CLuaSystem() = default;
	~CLuaSystem() = default;

	//global
	__int64 sleep(__int64 value, sol::this_state s);//ok
	__int64 openlog(std::string sfilename, sol::object oformat, sol::object obuffersize, sol::this_state s);
	__int64 print(sol::object ostr, sol::object ocolor, sol::this_state s);//ok
	__int64 messagebox(sol::object ostr, sol::object otype, sol::this_state s);//ok
	__int64 savesetting(const std::string& fileName, sol::this_state s);//ok
	__int64 loadsetting(const std::string& fileName, sol::this_state s);//ok
	__int64 set(std::string enumStr, sol::object p1, sol::object p2, sol::object p3, sol::object p4, sol::object p5, sol::object p6, sol::object p7, sol::this_state s);
	__int64 leftclick(__int64 x, __int64 y, sol::this_state s);//ok
	__int64 rightclick(__int64 x, __int64 y, sol::this_state s);//ok
	__int64 leftdoubleclick(__int64 x, __int64 y, sol::this_state s);//ok
	__int64 mousedragto(__int64 x1, __int64 y1, __int64 x2, __int64 y2, sol::this_state s);//ok
	__int64 createch(sol::object odataplacenum
		, std::string scharname
		, __int64 imgno
		, __int64 faceimgno
		, __int64 vit
		, __int64 str
		, __int64 tgh
		, __int64 dex
		, __int64 earth
		, __int64 water
		, __int64 fire
		, __int64 wind
		, sol::object ohometown, sol::object oforcecover, sol::this_state s);
	__int64 delch(__int64 index, std::string spsw, sol::object option, sol::this_state s);

	__int64 menu(__int64 index, sol::object otype, sol::this_state s);

	//meta
	__int64 logout(sol::this_state s);//ok
	__int64 logback(sol::this_state s);//ok
	__int64 eo(sol::this_state s);//ok
	__int64 cleanchat(sol::this_state s);//ok
	__int64 talk(sol::object ostr, sol::this_state s);
	__int64 press(std::string buttonStr, __int64 unitid, __int64 dialogid, sol::this_state s);//ok
	__int64 press(__int64 row, __int64 unitid, __int64 dialogid, sol::this_state s);//ok
	__int64 input(const std::string& str, __int64 unitid, __int64 dialogid, sol::this_state s);//ok
};

class CLuaItem
{
public:
	CLuaItem() = default;
	~CLuaItem() = default;

	__int64 use(__int64 itemIndex, __int64 target, sol::this_state s);
	__int64 drop(__int64 itemIndex, sol::this_state s);
	__int64 pick(__int64 dir, sol::this_state s);
	__int64 swap(__int64 from, __int64 to, sol::this_state s);
	__int64 craft(__int64 type, sol::table ingreList, sol::this_state s);
	__int64 buy(__int64 productIndex, __int64 amount, __int64 unitid, __int64 dialogid, sol::this_state s);
	__int64 sell(__int64 itemIndex, __int64 unitid, __int64 dialogid, sol::this_state s);

	__int64 deposit(__int64 itemIndex, __int64 unitid, __int64 dialogid, sol::this_state s);
	__int64 withdraw(__int64 itemIndex, __int64 unitid, __int64 dialogid, sol::this_state s);
};

class CLuaTrade
{
public:
	CLuaTrade() = default;
	~CLuaTrade() = default;
	//bool start(std::string sname, __int64 timeout);
	//void comfirm(std::string sname);
	//void cancel();
	//void appendItems(std::string sname, sol::table itemIndexs);
	//void appendGold(std::string sname, __int64 gold);
	//void appendPets(std::string sname, sol::table petIndex);
	//void complete(std::string sname);
};

class CLuaChar
{
public:
	CLuaChar() = default;
	~CLuaChar() = default;

	__int64 rename(std::string sfname, sol::this_state s);
	__int64 useMagic(__int64 magicIndex, __int64 target, sol::this_state s);
	__int64 depositGold(__int64 gold, sol::object oispublic, sol::this_state s);
	__int64 withdrawGold(__int64 gold, sol::object oispublic, sol::this_state s);
	__int64 dropGold(__int64 gold, sol::this_state s);
	__int64 mail(__int64 cardIndex, std::string stext, __int64 petIndex, std::string sitemName, std::string sitemMemo, sol::this_state s);
	__int64 mail(__int64 cardIndex, std::string stext, sol::this_state s);
	__int64 skillUp(__int64 abilityIndex, __int64 amount, sol::this_state s);

	//action-group
	__int64 setTeamState(bool join, sol::this_state s);
	__int64 kick(__int64 teammateIndex, sol::this_state s);
};

class CLuaPet
{
public:
	CLuaPet() = default;
	~CLuaPet() = default;

	__int64 setState(__int64 petIndex, __int64 state, sol::this_state s);
	__int64 drop(__int64 petIndex, sol::this_state s);
	__int64 rename(__int64 petIndex, std::string name, sol::this_state s);
	__int64 learn(__int64 fromSkillIndex, __int64 petIndex, __int64 toSkillIndex, __int64 unitid, __int64 dialogid, sol::this_state s);
	__int64 swap(__int64 petIndex, __int64 from, __int64 to, sol::this_state s);
	__int64 deposit(__int64 petIndex, __int64 unitid, __int64 dialogid, sol::this_state s);
	__int64 withdraw(__int64 petIndex, __int64 unitid, __int64 dialogid, sol::this_state s);
};

class CLuaMap
{
public:
	CLuaMap() = default;
	~CLuaMap() = default;

	__int64 setDir(__int64 dir, sol::this_state s);
	__int64 setDir(__int64 x, __int64 y, sol::this_state s);
	__int64 setDir(std::string sdir, sol::this_state s);

	__int64 move(sol::object obj, __int64 y, sol::this_state s);
	__int64 packetMove(__int64 x, __int64 y, std::string sdir, sol::this_state s);
	__int64 teleport(sol::this_state s);
	__int64 findPath(sol::object p1, sol::object p2, sol::object p3, sol::object p4, sol::object p5, sol::object ofunction, sol::object jump, sol::this_state s);
	__int64 downLoad(sol::object floor, sol::this_state s);
	__int64 findNPC(sol::object, sol::object, __int64 x, __int64 y, __int64 otimeout, sol::object jump, sol::this_state s);
};

class CLuaBattle
{
public:
	CLuaBattle() = default;
	~CLuaBattle() = default;

	__int64 charUseAttack(__int64 objIndex, sol::this_state s);//atk
	__int64 charUseMagic(__int64 magicIndex, __int64 objIndex, sol::this_state s);//magic
	__int64 charUseSkill(__int64 skillIndex, __int64 objIndex, sol::this_state s);//skill
	__int64 switchPet(__int64 petIndex, sol::this_state s);//switch
	__int64 escape(sol::this_state s);//escape
	__int64 defense(sol::this_state s);//defense
	__int64 useItem(__int64 itemIndex, __int64 objIndex, sol::this_state s);//item
	__int64 catchPet(__int64 objIndex, sol::this_state s);//catch
	__int64 nothing(sol::this_state s);//nothing
	__int64 petUseSkill(__int64 petSkillIndex, __int64 objIndex, sol::this_state s);//petskill
	__int64 petNothing(sol::this_state s);//pet nothing
};

class CLua : public ThreadPlugin
{
	Q_OBJECT
public:
	explicit CLua(__int64 index, QObject* parent = nullptr);
	CLua(__int64 index, const QString& content, QObject* parent = nullptr);
	virtual ~CLua();

	void start();
	void wait();
	inline bool isRunning() const { return isRunning_.load(std::memory_order_acquire) && !isInterruptionRequested(); }

signals:
	void finished();

private slots:
	void proc();

public:
	void openlibs();
	sol::state& getLua() { return lua_; }
	void setMax(__int64 max) { max_ = max; }
	void setSubScript(bool isSubScript) { isSubScript_ = isSubScript; }
	void setHookForStop(bool isHookForStop) { lua_["_HOOKFORSTOP"] = isHookForStop; }
	void setHookEnabled(bool isHookEnabled) { isHookEnabled_ = isHookEnabled; }
private:
	void open_enumlibs();
	void open_testlibs();
	void open_utillibs(sol::state& lua);
	void open_syslibs(sol::state& lua);
	void open_itemlibs(sol::state& lua);
	void open_charlibs(sol::state& lua);
	void open_petlibs(sol::state& lua);
	void open_maplibs(sol::state& lua);
	void open_battlelibs(sol::state& lua);

public:
	sol::state lua_;

private:
	__int64 max_ = 0;
	CLua* parent_ = nullptr;
	QThread* thread_ = nullptr;
	DWORD tid_ = 0UL;
	QElapsedTimer scriptTimer_;
	QString scriptContent_;
	bool isSubScript_ = false;
	bool isDebug_ = false;
	bool isHookEnabled_ = true;
	std::atomic_bool isRunning_ = false;

private:
	CLuaSystem luaSystem_;
	CLuaItem luaItem_;
	CLuaChar luaChar_;
	CLuaPet luaPet_;
	CLuaMap luaMap_;
	CLuaBattle luaBattle_;
};