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

	inline Q_REQUIRED_RESULT qint64 getCurrentLine(lua_State* L)
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

	inline Q_REQUIRED_RESULT qint64 getCurrentLine(const sol::this_state& s)
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
	bool checkOnlineThenWait(const sol::this_state& s);

	void processDelay(const sol::this_state& s);

	bool isInterruptionRequested(const sol::this_state& s);

	//根據傳入function的循環執行結果等待超時或條件滿足提早結束
	bool waitfor(const sol::this_state& s, qint64 timeout, std::function<bool()> exprfun);

	//遞歸獲取每一層目錄
	void getPackagePath(const QString base, QStringList* result);

	//從錯誤訊息中擷取行號
	static const QRegularExpression rexGetLine(R"(at[\s]*line[\s]*(\d+))");
	static const QRegularExpression reGetLineEx(R"(\]:(\d+)(?=\s*:))");
	Q_REQUIRED_RESULT QString getErrorMsgLocatedLine(const QString& str, qint64* retline);

	QPair<QString, QVariant> getVars(lua_State*& L, qint64 si, qint64 depth);
	QString getTableVars(lua_State*& L, qint64 si, qint64 depth);

	void hookProc(lua_State* L, lua_Debug* ar);

	void logExport(const sol::this_state& s, const QStringList& datas, qint64 color, bool doNotAnnounce = false);
	void logExport(const sol::this_state& s, const QString& data, qint64 color, bool doNotAnnounce = false);
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

	//qint64 createDialog(qint64 currentline, sol::this_state s);
	//qint64 regex(qint64 currentline, sol::this_state s);
	//qint64 find(qint64 currentline, sol::this_state s);
	//qint64 half(qint64 currentline, sol::this_state s);
	//qint64 full(qint64 currentline, sol::this_state s);
	//qint64 upper(qint64 currentline, sol::this_state s);
	//qint64 lower(qint64 currentline, sol::this_state s);
	//qint64 replace(qint64 currentline, sol::this_state s);
	//qint64 trim(std::string str, sol::this_state s);

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
	qint64 sleep(qint64 value, sol::this_state s);//ok
	qint64 openlog(std::string sfilename, sol::object oformat, sol::object obuffersize, sol::this_state s);
	qint64 print(sol::object ostr, sol::object ocolor, sol::this_state s);//ok
	qint64 messagebox(sol::object ostr, sol::object otype, sol::this_state s);//ok
	qint64 savesetting(const std::string& fileName, sol::this_state s);//ok
	qint64 loadsetting(const std::string& fileName, sol::this_state s);//ok
	qint64 set(std::string enumStr, sol::object p1, sol::object p2, sol::object p3, sol::object p4, sol::object p5, sol::object p6, sol::object p7, sol::this_state s);
	qint64 leftclick(qint64 x, qint64 y, sol::this_state s);//ok
	qint64 rightclick(qint64 x, qint64 y, sol::this_state s);//ok
	qint64 leftdoubleclick(qint64 x, qint64 y, sol::this_state s);//ok
	qint64 mousedragto(qint64 x1, qint64 y1, qint64 x2, qint64 y2, sol::this_state s);//ok

	//meta
	qint64 logout(sol::this_state s);//ok
	qint64 logback(sol::this_state s);//ok
	qint64 eo(sol::this_state s);//ok
	qint64 cleanchat(sol::this_state s);//ok
	qint64 talk(sol::object ostr, sol::this_state s);
	qint64 menu(qint64 index, sol::this_state s);//ok
	qint64 menu(qint64 type, qint64 index, sol::this_state s);//ok
	qint64 press(std::string buttonStr, qint64 unitid, qint64 dialogid, sol::this_state s);//ok
	qint64 press(qint64 row, qint64 unitid, qint64 dialogid, sol::this_state s);//ok
	qint64 input(const std::string& str, qint64 unitid, qint64 dialogid, sol::this_state s);//ok
};

class CLuaItem
{
public:
	CLuaItem() = default;
	~CLuaItem() = default;

	qint64 use(qint64 itemIndex, qint64 target, sol::this_state s);
	qint64 drop(qint64 itemIndex, sol::this_state s);
	qint64 pick(qint64 dir, sol::this_state s);
	qint64 swap(qint64 from, qint64 to, sol::this_state s);
	qint64 craft(qint64 type, sol::table ingreList, sol::this_state s);
	qint64 buy(qint64 productIndex, qint64 amount, qint64 unitid, qint64 dialogid, sol::this_state s);
	qint64 sell(qint64 itemIndex, qint64 unitid, qint64 dialogid, sol::this_state s);

	qint64 deposit(qint64 itemIndex, qint64 unitid, qint64 dialogid, sol::this_state s);
	qint64 withdraw(qint64 itemIndex, qint64 unitid, qint64 dialogid, sol::this_state s);
};

class CLuaTrade
{
public:
	CLuaTrade() = default;
	~CLuaTrade() = default;
	//bool start(std::string sname, qint64 timeout);
	//void comfirm(std::string sname);
	//void cancel();
	//void appendItems(std::string sname, sol::table itemIndexs);
	//void appendGold(std::string sname, qint64 gold);
	//void appendPets(std::string sname, sol::table petIndex);
	//void complete(std::string sname);
};

class CLuaChar
{
public:
	CLuaChar() = default;
	~CLuaChar() = default;

	qint64 rename(std::string sfname, sol::this_state s);
	qint64 useMagic(qint64 magicIndex, qint64 target, sol::this_state s);
	qint64 depositGold(qint64 gold, sol::object oispublic, sol::this_state s);
	qint64 withdrawGold(qint64 gold, sol::object oispublic, sol::this_state s);
	qint64 dropGold(qint64 gold, sol::this_state s);
	qint64 mail(qint64 cardIndex, std::string stext, qint64 petIndex, std::string sitemName, std::string sitemMemo, sol::this_state s);
	qint64 mail(qint64 cardIndex, std::string stext, sol::this_state s);
	qint64 skillUp(qint64 abilityIndex, qint64 amount, sol::this_state s);

	//action-group
	qint64 setTeamState(bool join, sol::this_state s);
	qint64 kick(qint64 teammateIndex, sol::this_state s);
};

class CLuaPet
{
public:
	CLuaPet() = default;
	~CLuaPet() = default;

	qint64 setState(qint64 petIndex, qint64 state, sol::this_state s);
	qint64 drop(qint64 petIndex, sol::this_state s);
	qint64 rename(qint64 petIndex, std::string name, sol::this_state s);
	qint64 learn(qint64 fromSkillIndex, qint64 petIndex, qint64 toSkillIndex, qint64 unitid, qint64 dialogid, sol::this_state s);
	qint64 swap(qint64 petIndex, qint64 from, qint64 to, sol::this_state s);
	qint64 deposit(qint64 petIndex, qint64 unitid, qint64 dialogid, sol::this_state s);
	qint64 withdraw(qint64 petIndex, qint64 unitid, qint64 dialogid, sol::this_state s);
};

class CLuaMap
{
public:
	CLuaMap() = default;
	~CLuaMap() = default;

	qint64 setDir(qint64 dir, sol::this_state s);
	qint64 setDir(qint64 x, qint64 y, sol::this_state s);
	qint64 setDir(std::string sdir, sol::this_state s);

	qint64 move(qint64 x, qint64 y, sol::this_state s);
	qint64 packetMove(qint64 x, qint64 y, std::string sdir, sol::this_state s);
	qint64 teleport(sol::this_state s);
	qint64 findPath(sol::object p1, sol::object p2, sol::object p3, sol::object p4, sol::object p5, sol::object ofunction, sol::object jump, sol::this_state s);
	qint64 downLoad(sol::object floor, sol::this_state s);
	qint64 moveToNPC(sol::object, sol::object, qint64 x, qint64 y, qint64 otimeout, sol::object jump, sol::this_state s);
};

class CLuaBattle
{
public:
	CLuaBattle() = default;
	~CLuaBattle() = default;

	qint64 charUseAttack(qint64 objIndex, sol::this_state s);//atk
	qint64 charUseMagic(qint64 magicIndex, qint64 objIndex, sol::this_state s);//magic
	qint64 charUseSkill(qint64 skillIndex, qint64 objIndex, sol::this_state s);//skill
	qint64 switchPet(qint64 petIndex, sol::this_state s);//switch
	qint64 escape(sol::this_state s);//escape
	qint64 defense(sol::this_state s);//defense
	qint64 useItem(qint64 itemIndex, qint64 objIndex, sol::this_state s);//item
	qint64 catchPet(qint64 objIndex, sol::this_state s);//catch
	qint64 nothing(sol::this_state s);//nothing
	qint64 petUseSkill(qint64 petSkillIndex, qint64 objIndex, sol::this_state s);//petskill
	qint64 petNothing(sol::this_state s);//pet nothing
};

class CLua : public ThreadPlugin
{
	Q_OBJECT
public:
	explicit CLua(qint64 index, QObject* parent = nullptr);
	CLua(qint64 index, const QString& content, QObject* parent = nullptr);
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
	void setMax(qint64 max) { max_ = max; }
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
	qint64 max_ = 0;
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