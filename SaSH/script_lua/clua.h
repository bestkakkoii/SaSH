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

#pragma once

#include <indexer.h>
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


	bool __fastcall checkStopAndPause(const sol::this_state& s);
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

namespace luatool
{
	bool checkRange(sol::object o, long long& min, long long& max, QVector<long long>* pindexs);

	sol::object getVar(sol::state_view& lua, const std::string& varName);

	std::string format(std::string sformat, sol::this_state s);
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

class CLuaSystem
{
public:
	CLuaSystem() = default;
	~CLuaSystem() = default;

	//global
	long long setglobal(std::string sname, sol::object od, sol::this_state s);
	sol::object getglobal(std::string sname, sol::this_state s);
	long long clearglobal();

	sol::object require(std::string sname, sol::this_state s);

	long long send(long long funId, sol::variadic_args args, sol::this_state s);
	long long sleep(long long value, sol::this_state s);//ok
	long long openlog(std::string sfilename, sol::object oformat, sol::object obuffersize, sol::this_state s);
	long long print(sol::object ostr, sol::object ocolor, sol::this_state s);//ok
	std::string messagebox(sol::object ostr, sol::object otype, sol::this_state s);//ok
	long long savesetting(const std::string& fileName, sol::this_state s);//ok
	long long loadsetting(const std::string& fileName, sol::this_state s);//ok
	long long chname(sol::object oname, sol::this_state s);
	long long chpetname(long long index, sol::object oname, sol::this_state s);
	long long chpet(long long petindex, sol::object ostate, sol::this_state s);
	long long set(sol::object oenumStr,
		sol::object p1, sol::object p2, sol::object p3, sol::object p4, sol::object p5, sol::object p6, sol::object p7,
		sol::this_state s);
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
	long long press(sol::object obutton, sol::object ounitid, sol::object odialogid, sol::object oext, sol::this_state s);//ok
	long long input(const std::string& str, long long unitid, long long dialogid, sol::this_state s);//ok

	long long capture(std::string sfilename, sol::this_state s);


	bool waitpos(sol::object p1, sol::object p2, sol::object p3, sol::this_state s);
	bool waitmap(sol::object p1, sol::object otimeout, sol::this_state s);
	bool waititem(sol::object oname, sol::object omemo, sol::object otimeout, sol::this_state s);
	bool waitteam(sol::object otimeout, sol::this_state s);
	bool waitpet(std::string name, sol::object otimeout, sol::this_state s);
	bool waitdlg(sol::object p1, sol::object otimeout, sol::this_state s);
	bool waitsay(std::string sstr, sol::object otimeout, sol::this_state s);
};

class CLuaTimer
{
public:
	CLuaTimer()
	{
		timer_.start();
	}

	long long cost() const
	{
		return timer_.elapsed();
	}

	long long costSeconds() const
	{
		return timer_.elapsed() / 1000;
	}

	bool hasExpired(long long milliseconds) const
	{
		return timer_.elapsed() >= milliseconds;
	}

	void restart()
	{
		timer_.restart();
	}

	std::string toFormatedString()
	{
		QString formated = util::formatMilliseconds(timer_.elapsed());
		return util::toConstData(formated);
	}

	std::string toString()
	{
		QString str = util::toQString(timer_.elapsed());
		return util::toConstData(str);
	}

	double toDouble()
	{
		return static_cast<double>(timer_.elapsed());
	}

private:
	QElapsedTimer timer_;
};

class CLuaTimerZhs
{
public:
	CLuaTimerZhs()
	{
		timer_.start();
	}

	long long cost() const
	{
		return timer_.elapsed();
	}

	long long costSeconds() const
	{
		return timer_.elapsed() / 1000;
	}

	bool hasExpired(long long milliseconds) const
	{
		return timer_.elapsed() >= milliseconds;
	}

	void restart()
	{
		timer_.restart();
	}

	std::string toFormatedString()
	{
		QString formated = util::formatMilliseconds(timer_.elapsed());
		return util::toConstData(formated);
	}

	std::string toString()
	{
		QString str = util::toQString(timer_.elapsed());
		return util::toConstData(str);
	}

	double toDouble()
	{
		return static_cast<double>(timer_.elapsed());
	}

private:
	QElapsedTimer timer_;
};


class CLuaItem
{
public:
	CLuaItem() = default;
	CLuaItem(long long index) : index_(index) {}
	~CLuaItem() = default;

	sa::item_t operator[](long long index);

	long long swapitem(long long fromIndex, long long toIndex, sol::this_state s);
	long long cook(std::string singre, sol::this_state s);
	long long make(std::string singre, sol::this_state s);
	long long withdrawgold(long long gold, sol::object oispublic, sol::this_state s);
	long long depositgold(long long gold, sol::object oispublic, sol::this_state s);
	long long buy(long long productindex, long long count, sol::object ounit, sol::object odialogid, sol::this_state s);
	long long sell(std::string sname, sol::object ounit, sol::object odialogid, sol::this_state s);


	long long dropgold(long long goldamount, sol::this_state s);

	long long useitem(sol::object p1, sol::object p2, sol::object p3, sol::object p4, sol::this_state s);
	long long pickitem(sol::object odir, sol::this_state s);
	long long doffitem(sol::object oitem, sol::object p1, sol::object p2, sol::this_state s);
	long long sellpet(sol::object range, sol::this_state s);
	long long droppet(sol::object oname, sol::this_state s);

	long long deposititem(sol::object orange, std::string sname, sol::this_state s);
	long long withdrawitem(std::string sname, sol::object omemo, sol::object oisall, sol::this_state s);
	long long recordequip(sol::this_state s);
	long long wearrecordedequip(sol::this_state s);
	long long unwearequip(sol::object opart, sol::this_state s);
	long long petequip(long long petIndex, sol::object oname, sol::object omemo, sol::this_state s);
	long long petunequip(long long petIndex, sol::object opart, sol::this_state s);
	long long depositpet(sol::object oslots, sol::this_state s);
	long long withdrawpet(std::string sname, sol::object olevel, sol::object omaxhp, sol::this_state s);

	long long trade(std::string sname, sol::object oitem, sol::object opet, sol::object ogold, sol::object oitemout, sol::this_state s);

	long long getSpace();
	long long getSize();
	long long getSpaceIndex();
	bool getIsFull();

public:
	long long count(sol::object oitemnames, sol::object oitemmemos, sol::object oincludeEequip, sol::object ostartFrom, sol::this_state s);
	long long indexof(sol::object oitemnames, sol::object oitemmemos, sol::object oincludeEequip, sol::object ostartFrom, sol::this_state s);
	sol::object find(sol::object oitemnames, sol::object oitemmemos, sol::object oincludeEequip, sol::object ostartFrom, sol::this_state s);

public:
	long long space = 0;
	bool isfull = false;

private:
	long long index_ = -1;
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
	CLuaChar(long long index) : index_(index) {}

	long long rename(std::string sfname, sol::this_state s);
	long long skup(sol::object otype, long long count, sol::this_state s);

	long long join(sol::this_state s);
	long long leave(sol::this_state s);
	long long kick(sol::object o, sol::this_state s);
	long long mail(sol::object oaddrIndex, sol::object omessage, sol::object opetindex, sol::object sitemname, sol::object sitemmemo, sol::this_state s);
	long long usemagic(sol::object omagic, sol::object otarget, sol::this_state s);
	sa::character_t getCharacter() const;

	long long getBattlePetNo() { return getCharacter().battlePetNo + 1; }
	long long getMailPetNo() { return getCharacter().mailPetNo + 1; }
	long long getStandbyPet() { return getCharacter().standbyPet + 1; }
	long long getRidePetNo() { return getCharacter().ridePetNo + 1; }
	long long getModelId() { return getCharacter().modelid; }
	long long getFaceId() { return getCharacter().faceid; }
	long long getUnitId() { return getCharacter().id; }
	long long getDir() { return getCharacter().dir; }
	long long getHp() { return getCharacter().hp; }
	long long getMaxHp() { return getCharacter().maxHp; }
	long long getHpPercent() { return getCharacter().hpPercent; }
	long long getMp() { return getCharacter().mp; }
	long long getMaxMp() { return getCharacter().maxMp; }
	long long getMpPercent() { return getCharacter().mpPercent; }
	long long getVit() { return getCharacter().vit; }
	long long getStr() { return getCharacter().str; }
	long long getTgh() { return getCharacter().tgh; }
	long long getDex() { return getCharacter().dex; }
	long long getExp() { return getCharacter().exp; }
	long long getMaxExp() { return getCharacter().maxExp; }
	long long getLevel() { return getCharacter().level; }
	long long getAtk() { return getCharacter().atk; }
	long long getDef() { return getCharacter().def; }
	long long getAgi() { return getCharacter().agi; }
	long long getChasma() { return getCharacter().chasma; }
	long long getLuck() { return getCharacter().luck; }
	long long getEarth() { return getCharacter().earth; }
	long long getWater() { return getCharacter().water; }
	long long getFire() { return getCharacter().fire; }
	long long getWind() { return getCharacter().wind; }
	long long getGold() { return getCharacter().gold; }
	long long getFame() { return getCharacter().fame; }
	long long getTitleNo() { return getCharacter().titleNo; }
	long long getDp() { return getCharacter().dp; }
	long long getNameColor() { return getCharacter().nameColor; }
	long long getStatus() { return getCharacter().status; }
	long long getEtcFlag() { return getCharacter().etcFlag; }
	long long getBattleNo() { return getCharacter().battleNo; }
	long long getSideNo() { return getCharacter().sideNo; }
	long long getHelpMode() { return getCharacter().helpMode; }
	long long getPcNameColor() { return getCharacter().pcNameColor; }
	long long getTransmigration() { return getCharacter().transmigration; }
	long long getFamilyleader() { return getCharacter().familyleader; }
	long long getChannel() { return getCharacter().channel; }
	long long getQuickChannel() { return getCharacter().quickChannel; }
	long long getPersonalBankgold() { return getCharacter().personal_bankgold; }
	long long getLearnride() { return getCharacter().learnride; }
	long long getLowsride() { return getCharacter().lowsride; }
	long long getRidePetLevel() { return getCharacter().ridePetLevel; }
	long long getFamilySprite() { return getCharacter().familySprite; }
	long long getBaseGraNo() { return getCharacter().baseGraNo; }
	long long getBig4fm() { return getCharacter().big4fm; }
	long long getTradeConfirm() { return getCharacter().trade_confirm; }
	long long getProfessionClass() { return getCharacter().profession_class; }
	long long getProfessionLevel() { return getCharacter().profession_level; }
	long long getProfessionExp() { return getCharacter().profession_exp; }
	long long getProfessionSkillPoint() { return getCharacter().profession_skill_point; }
	long long getHerofloor() { return getCharacter().herofloor; }
	long long getIOnStreetVendor() { return getCharacter().iOnStreetVendor; }
	long long getSkywalker() { return getCharacter().skywalker; }
	long long getITheaterMode() { return getCharacter().iTheaterMode; }
	long long getISceneryNumber() { return getCharacter().iSceneryNumber; }
	long long getIDanceMode() { return getCharacter().iDanceMode; }
	long long getNewfame() { return getCharacter().newfame; }
	long long getFtype() { return getCharacter().ftype; }
	long long getMaxload() { return getCharacter().maxload; }
	long long getPoint() { return getCharacter().point; }
	long long getTradeState() { return getCharacter().trade_confirm; }

	std::string getName() { return util::toConstData(getCharacter().name); }
	std::string getFreeName() { return util::toConstData(getCharacter().freeName); }
	std::string getProfessionClassName() { return util::toConstData(getCharacter().profession_class_name); }
	std::string getChatRoomNum() { return util::toConstData(getCharacter().chatRoomNum); }
	std::string getChusheng() { return util::toConstData(getCharacter().chusheng); }
	std::string getFamily() { return util::toConstData(getCharacter().family); }
	std::string getRidePetName() { return util::toConstData(getCharacter().ridePetName); }
	std::string getHash() { return getCharacter().getHash(); }
	std::tuple<std::string, long long> getServer();

private:
	long long index_ = -1;
};

class CLuaPet
{
public:
	CLuaPet() = default;
	CLuaPet(long long index) : index_(index) {}
	~CLuaPet() = default;

	sa::pet_t operator[](long long index);

	long long size();

	long long count(std::string sname);

	long long learn(long long petIndex, long long fromSkillIndex, long long toSkillIndex, sol::object ounitid, sol::object odialogid, sol::this_state s);

private:
	long long index_ = -1;
};

class CLuaMap
{
public:
	CLuaMap() = default;
	CLuaMap(long long index) : index_(index) {}
	~CLuaMap() = default;

	long long setdir(sol::object p1, sol::object p2, sol::object p3, sol::this_state s);
	long long walkpos(long long x, long long y, sol::object otimeout, sol::this_state s);

	long long move(sol::object obj, long long y, sol::this_state s);
	long long packetMove(long long x, long long y, std::string sdir, sol::this_state s);
	long long teleport(sol::this_state s);
	long long findPath(sol::object p1, sol::object p2, sol::object p3, sol::object p4, sol::object p5, sol::object ofunction, sol::object jump, sol::this_state s);
	long long downLoad(sol::object floor, sol::this_state s);
	long long findNPC(sol::object p1, sol::object nicknames, long long x, long long y, sol::object otimeout, sol::object ostepcost, sol::object enableCrossWall, sol::this_state s);

	long long x();
	long long y();
	std::tuple<long long, long long> xy();
	long long floor();
	std::string getName();
	std::string getGround();
	bool isxy(long long x, long long y, sol::this_state s);
	bool isrect(long long x1, long long y1, long long x2, long long y2, sol::this_state s);
	bool ismap(sol::object omap, sol::this_state s);

private:
	long long index_ = -1;

};

class CLuaBattle
{
public:
	CLuaBattle() = default;
	CLuaBattle(long long index) : index_(index) {}
	~CLuaBattle() = default;

	sa::battle_object_t operator[](long long index);

	long long charUseAttack(long long objIndex, sol::this_state s);//atk

	long long charUseMagic(long long magicIndex, long long objIndex, sol::this_state s);//magic
	long long charUseMagic(std::string smagig, long long objIndex, sol::this_state s);//magic

	long long charUseSkill(long long skillIndex, long long objIndex, sol::this_state s);//skill
	long long charUseSkill(std::string sskill, long long objIndex, sol::this_state s);//skill

	long long switchPet(long long petIndex, sol::this_state s);//switch

	long long escape(sol::this_state s);//escape

	long long defense(sol::this_state s);//defense

	long long useItem(long long itemIndex, long long objIndex, sol::this_state s);//item
	long long useItem(std::string sitem, long long objIndex, sol::this_state s);//item

	long long catchPet(long long objIndex, sol::this_state s);//catch
	long long catchPet(std::string sname, sol::object olevel, sol::object omaxhp, sol::object omodelid, sol::this_state s);//catch

	long long nothing(sol::this_state s);//nothing

	long long petUseSkill(long long petSkillIndex, long long objIndex, sol::this_state s);//petskill
	long long petUseSkill(std::string spetskill, long long objIndex, sol::this_state s);//petskill

	long long petNothing(sol::this_state s);//pet nothing

	long long bend(sol::this_state s);
	long long bwait(sol::object otimeout, sol::object jump, sol::this_state s);


	double count();
	double dura();
	double time();
	long long cost();
	long long round();
	std::string field();
	long long charpos();
	long long petpos();
	long long size();
	long long enemycount();
	long long alliecount();

private:
	long long index_ = -1;
};

class CLua : public QObject, public Indexer
{
	Q_OBJECT
public:
	explicit CLua(long long index, QObject* parent = nullptr);
	CLua(long long index, const QString& content, QObject* parent = nullptr);
	virtual ~CLua();

	void __fastcall start();
	void __fastcall wait();
	inline bool __fastcall isRunning() const { return isRunning_.get() && !QThread::currentThread()->isInterruptionRequested(); }

signals:
	void finished();

private slots:
	void proc();

public:
	void __fastcall openlibs();
	sol::state& getLua() { return lua_; }
	void __fastcall setMax(long long max) { max_ = max; }
	void __fastcall setSubScript(bool isSubScript) { isSubScript_ = isSubScript; }
	void __fastcall setHookForStop(bool isHookForStop) { lua_["__HOOKFORSTOP"] = isHookForStop; }
	void __fastcall setHookEnabled(bool isHookEnabled) { isHookEnabled_ = isHookEnabled; }
	void __fastcall setHookForBattle(bool isHookForBattlle) { lua_["__HOOKFORBATTLE"] = isHookForBattlle; }
	void __fastcall setRunningState(bool isRunning) { if (isRunning) isRunning_.on(); else isRunning_.off(); }

	bool __fastcall doString(const std::string& sstr);

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
	QString scriptContent_;
	bool isSubScript_ = false;
	bool isDebug_ = false;
	bool isHookEnabled_ = true;
	safe::flag isRunning_ = false;

public:
	CLuaSystem luaSystem_;
	CLuaItem luaItem_;
	CLuaChar luaChar_;
	CLuaPet luaPet_;
	CLuaMap luaMap_;
	CLuaBattle luaBattle_;
};