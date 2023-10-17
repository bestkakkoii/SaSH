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

#pragma once

#include <threadplugin.h>
#include <util.h>

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

	inline [[nodiscard]] long long __fastcall getCurrentLine(lua_State* L)
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

	inline [[nodiscard]] long long __fastcall getCurrentLine(const sol::this_state& s)
	{
		lua_State* L = s.lua_state();
		if (L)
			return getCurrentLine(L);
		else
			return -1;
	}

	inline [[nodiscard]] lua_Debug __fastcall getCurrentInfo(lua_State* L)
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

	inline [[nodiscard]] lua_Debug __fastcall getCurrentInfo(const sol::this_state& s)
	{
		lua_State* L = s.lua_state();
		return getCurrentInfo(L);
	}

	void __fastcall tryPopCustomErrorMsg(const sol::this_state& s, const LUA_ERROR_TYPE element, const QVariant& p1 = 0, const QVariant& p2 = 0, const QVariant& p3 = 0, const QVariant& p4 = 0);


	void __fastcall checkStopAndPause(const sol::this_state& s);
	bool __fastcall checkBattleThenWait(const sol::this_state& s);
	bool __fastcall checkOnlineThenWait(const sol::this_state& s);

	void __fastcall processDelay(const sol::this_state& s);

	bool __fastcall isInterruptionRequested(const sol::this_state& s);

	//根據傳入function的循環執行結果等待超時或條件滿足提早結束
	bool __fastcall waitfor(const sol::this_state& s, long long timeout, std::function<bool()> exprfun);

	//遞歸獲取每一層目錄
	void __fastcall getPackagePath(const QString base, QStringList* result);

	//從錯誤訊息中擷取行號
	static const QRegularExpression rexGetLine(R"(at[\s]*line[\s]*(\d+))");
	static const QRegularExpression reGetLineEx(R"(\]:(\d+)(?=\s*:))");
	[[nodiscard]] QString __fastcall getErrorMsgLocatedLine(const QString& str, long long* retline);

	QPair<QString, QVariant> __fastcall getVars(lua_State*& L, long long si, long long& depth);
	QString __fastcall getTableVars(lua_State*& L, long long si, long long& depth);

	void hookProc(lua_State* L, lua_Debug* ar);

	void __fastcall logExport(const sol::this_state& s, const QStringList& datas, long long color, bool doNotAnnounce = false);
	void __fastcall logExport(const sol::this_state& s, const QString& data, long long color, bool doNotAnnounce = false);
	void __fastcall showErrorMsg(const sol::this_state& s, long long level, const QString& data);
}

class CLuaTest
{
public:
	CLuaTest(sol::this_state) {};
	CLuaTest(long long a, sol::this_state) : a_(a) {};
	~CLuaTest() = default;
	long long add(long long a, long long b) { return a + b; };
	long long add(long long b) { return a_ + b; };

	long long sub(long long a, long long b) { return a - b; };

private:
	long long a_ = 0;
};

class CLuaUtil
{
public:
	CLuaUtil() = default;
	~CLuaUtil() = default;

	//long long createDialog(long long currentline, sol::this_state s);
	//long long regex(long long currentline, sol::this_state s);
	//long long find(long long currentline, sol::this_state s);
	//long long half(long long currentline, sol::this_state s);
	//long long full(long long currentline, sol::this_state s);
	//long long upper(long long currentline, sol::this_state s);
	//long long lower(long long currentline, sol::this_state s);
	//long long replace(long long currentline, sol::this_state s);
	//long long trim(std::string str, sol::this_state s);

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
	long long sleep(long long value, sol::this_state s);//ok
	long long openlog(std::string sfilename, sol::object oformat, sol::object obuffersize, sol::this_state s);
	long long print(sol::object ostr, sol::object ocolor, sol::this_state s);//ok
	long long messagebox(sol::object ostr, sol::object otype, sol::this_state s);//ok
	long long savesetting(const std::string& fileName, sol::this_state s);//ok
	long long loadsetting(const std::string& fileName, sol::this_state s);//ok
	long long set(std::string enumStr, sol::object p1, sol::object p2, sol::object p3, sol::object p4, sol::object p5, sol::object p6, sol::object p7, sol::this_state s);
	long long leftclick(long long x, long long y, sol::this_state s);//ok
	long long rightclick(long long x, long long y, sol::this_state s);//ok
	long long leftdoubleclick(long long x, long long y, sol::this_state s);//ok
	long long mousedragto(long long x1, long long y1, long long x2, long long y2, sol::this_state s);//ok
	long long createch(sol::object odataplacenum
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
		, sol::object ohometown, sol::object oforcecover, sol::this_state s);
	long long delch(long long index, std::string spsw, sol::object option, sol::this_state s);

	long long menu(long long index, sol::object otype, sol::this_state s);

	//meta
	long long logout(sol::this_state s);//ok
	long long logback(sol::this_state s);//ok
	long long eo(sol::this_state s);//ok
	long long cleanchat(sol::this_state s);//ok
	long long talk(sol::object ostr, sol::object ocolor, sol::object omode, sol::this_state s);
	long long press(std::string buttonStr, long long unitid, long long dialogid, sol::this_state s);//ok
	long long press(long long row, long long unitid, long long dialogid, sol::this_state s);//ok
	long long input(const std::string& str, long long unitid, long long dialogid, sol::this_state s);//ok
};

class CLuaItem
{
public:
	CLuaItem() = default;
	~CLuaItem() = default;

	long long use(long long itemIndex, long long target, sol::this_state s);
	long long drop(long long itemIndex, sol::this_state s);
	long long pick(long long dir, sol::this_state s);
	long long swap(long long from, long long to, sol::this_state s);
	long long craft(long long type, sol::table ingreList, sol::this_state s);
	long long buy(long long productIndex, long long amount, long long unitid, long long dialogid, sol::this_state s);
	long long sell(long long itemIndex, long long unitid, long long dialogid, sol::this_state s);

	long long deposit(long long itemIndex, long long unitid, long long dialogid, sol::this_state s);
	long long withdraw(long long itemIndex, long long unitid, long long dialogid, sol::this_state s);
};

class CLuaTrade
{
public:
	CLuaTrade() = default;
	~CLuaTrade() = default;
	//bool start(std::string sname, long long timeout);
	//void comfirm(std::string sname);
	//void cancel();
	//void appendItems(std::string sname, sol::table itemIndexs);
	//void appendGold(std::string sname, long long gold);
	//void appendPets(std::string sname, sol::table petIndex);
	//void complete(std::string sname);
};

class CLuaChar
{
public:
	CLuaChar() = default;
	~CLuaChar() = default;

	long long rename(std::string sfname, sol::this_state s);
	long long useMagic(long long magicIndex, long long target, sol::this_state s);
	long long depositGold(long long gold, sol::object oispublic, sol::this_state s);
	long long withdrawGold(long long gold, sol::object oispublic, sol::this_state s);
	long long dropGold(long long gold, sol::this_state s);
	long long mail(long long cardIndex, std::string stext, long long petIndex, std::string sitemName, std::string sitemMemo, sol::this_state s);
	long long mail(long long cardIndex, std::string stext, sol::this_state s);
	long long skillUp(long long abilityIndex, long long amount, sol::this_state s);

	//action-group
	long long setTeamState(bool join, sol::this_state s);
	long long kick(long long teammateIndex, sol::this_state s);
};

class CLuaPet
{
public:
	CLuaPet() = default;
	~CLuaPet() = default;

	long long setState(long long petIndex, long long state, sol::this_state s);
	long long drop(long long petIndex, sol::this_state s);
	long long rename(long long petIndex, std::string name, sol::this_state s);
	long long learn(long long fromSkillIndex, long long petIndex, long long toSkillIndex, long long unitid, long long dialogid, sol::this_state s);
	long long swap(long long petIndex, long long from, long long to, sol::this_state s);
	long long deposit(long long petIndex, long long unitid, long long dialogid, sol::this_state s);
	long long withdraw(long long petIndex, long long unitid, long long dialogid, sol::this_state s);
};

class CLuaMap
{
public:
	CLuaMap() = default;
	~CLuaMap() = default;

	long long setDir(long long dir, sol::this_state s);
	long long setDir(long long x, long long y, sol::this_state s);
	long long setDir(std::string sdir, sol::this_state s);

	long long move(sol::object obj, long long y, sol::this_state s);
	long long packetMove(long long x, long long y, std::string sdir, sol::this_state s);
	long long teleport(sol::this_state s);
	long long findPath(sol::object p1, sol::object p2, sol::object p3, sol::object p4, sol::object p5, sol::object ofunction, sol::object jump, sol::this_state s);
	long long downLoad(sol::object floor, sol::this_state s);
	long long findNPC(sol::object, sol::object, long long x, long long y, long long otimeout, sol::object jump, sol::this_state s);
};

class CLuaBattle
{
public:
	CLuaBattle() = default;
	~CLuaBattle() = default;

	long long charUseAttack(long long objIndex, sol::this_state s);//atk
	long long charUseMagic(long long magicIndex, long long objIndex, sol::this_state s);//magic
	long long charUseSkill(long long skillIndex, long long objIndex, sol::this_state s);//skill
	long long switchPet(long long petIndex, sol::this_state s);//switch
	long long escape(sol::this_state s);//escape
	long long defense(sol::this_state s);//defense
	long long useItem(long long itemIndex, long long objIndex, sol::this_state s);//item
	long long catchPet(long long objIndex, sol::this_state s);//catch
	long long nothing(sol::this_state s);//nothing
	long long petUseSkill(long long petSkillIndex, long long objIndex, sol::this_state s);//petskill
	long long petNothing(sol::this_state s);//pet nothing
};

class CLua : public ThreadPlugin
{
	Q_OBJECT
public:
	explicit CLua(long long index, QObject* parent = nullptr);
	CLua(long long index, const QString& content, QObject* parent = nullptr);
	virtual ~CLua();

	void __fastcall start();
	void __fastcall wait();
	inline bool __fastcall isRunning() const { return isRunning_.load(std::memory_order_acquire) && !isInterruptionRequested(); }

signals:
	void finished();

private slots:
	void proc();

public:
	void __fastcall openlibs();
	sol::state& getLua() { return lua_; }
	void __fastcall setMax(long long max) { max_ = max; }
	void __fastcall setSubScript(bool isSubScript) { isSubScript_ = isSubScript; }
	void __fastcall setHookForStop(bool isHookForStop) { lua_["_HOOKFORSTOP"] = isHookForStop; }
	void __fastcall setHookEnabled(bool isHookEnabled) { isHookEnabled_ = isHookEnabled; }
private:
	void __fastcall open_enumlibs();
	void __fastcall open_testlibs();
	void __fastcall open_utillibs(sol::state& lua);
	void __fastcall open_syslibs(sol::state& lua);
	void __fastcall open_itemlibs(sol::state& lua);
	void __fastcall open_charlibs(sol::state& lua);
	void __fastcall open_petlibs(sol::state& lua);
	void __fastcall open_maplibs(sol::state& lua);
	void __fastcall open_battlelibs(sol::state& lua);

public:
	sol::state lua_;

private:
	long long max_ = 0;
	CLua* parent_ = nullptr;
	QThread thread_;
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