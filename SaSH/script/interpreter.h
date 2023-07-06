#pragma once
#include <QObject>
#include <QScopedPointer>
#include <atomic>
#include "parser.h"
#include "util.h"

typedef struct break_marker_s
{
	qint64 line = 0;
	qint64 count = 0;
	qint64 maker = 0;
	QString content = "\0";
} break_marker_t;

constexpr qint64 DEFAULT_FUNCTION_TIMEOUT = 5000;

class Interpreter : public ThreadPlugin
{
	Q_OBJECT
public:
	enum RunFileMode
	{
		kSync,
		kAsync,
	};

	enum VarShareMode
	{
		kNotShare,
		kShare,
	};

public:
	Interpreter();
	virtual ~Interpreter();

	inline bool isRunning() const { return isRunning_.load(std::memory_order_acquire) && !isInterruptionRequested(); }

	void preview(const QString& fileName);

	void doString(const QString& script, Interpreter* parent, VarShareMode shareMode);

	void doFileWithThread(qint64 beginLine, const QString& fileName);

	bool doFile(qint64 beginLine, const QString& fileName, Interpreter* parent, VarShareMode shareMode, RunFileMode noShow = kSync);

	void stop();

	inline void setSubScript(bool is) { parser_.setSubScript(is); }

	Q_REQUIRED_RESULT inline bool isSubScript() const { return parser_.isSubScript(); }

signals:
	void finished();


public slots:
	void proc();

private:
	bool readFile(const QString& fileName, QString* pcontent, bool* isPrivate);
	bool loadString(const QString& script, QHash<qint64, TokenMap>* ptokens, QHash<QString, qint64>* plabel);

	qint64 mainScriptCallBack(qint64 currentLine, const TokenMap& token);

private:
	enum CompareArea
	{
		kAreaPlayer,
		kAreaPet,
		kAreaItem,
		kAreaCount,
	};

	enum CompareType
	{
		kCompareTypeNone,
		kPlayerName,
		kPlayerFreeName,
		kPlayerLevel,
		kPlayerHp,
		kPlayerMaxHp,
		kPlayerHpPercent,
		kPlayerMp,
		kPlayerMaxMp,
		kPlayerMpPercent,
		kPlayerExp,
		kPlayerMaxExp,
		kPlayerStone,
		kPlayerAtk,
		kPlayerDef,
		kPlayerAgi,
		kPlayerChasma,
		kPlayerEarth,
		kPlayerWater,
		kPlayerFire,
		kPlayerWind,

		kPetName,
		kPetFreeName,
		kPetLevel,
		kPetHp,
		kPetMaxHp,
		kPetHpPercent,
		kPetExp,
		kPetMaxExp,
		kPetAtk,
		kPetDef,
		kPetAgi,
		kPetLoyal,
		kPetState,
		kPetEarth,
		kPetWater,
		kPetFire,
		kPetWind,

		itemCount,

		kTeamCount,
		kPetCount,

	};

	inline static const QHash<QString, CompareType> compareTypeMap = {
		{ u8"人物name", kPlayerName },
		{ u8"人物fname", kPlayerFreeName },
		{ u8"人物lv", kPlayerLevel },
		{ u8"人物hp", kPlayerHp },
		{ u8"人物maxhp", kPlayerMaxHp },
		{ u8"人物hpp", kPlayerHpPercent },
		{ u8"人物mp", kPlayerMp },
		{ u8"人物maxmp", kPlayerMaxMp },
		{ u8"人物mpp", kPlayerMpPercent },
		{ u8"人物exp", kPlayerExp },
		{ u8"人物maxexp", kPlayerMaxExp },
		{ u8"人物stone", kPlayerStone },
		{ u8"人物atk", kPlayerAtk },
		{ u8"人物def", kPlayerDef },
		{ u8"人物agi", kPlayerAgi },
		{ u8"人物chasma", kPlayerChasma },
		{ u8"人物earth", kPlayerEarth },
		{ u8"人物water", kPlayerWater },
		{ u8"人物fire", kPlayerFire },
		{ u8"人物wind", kPlayerWind },

		{ u8"寵物name", kPetName },
		{ u8"寵物fname", kPetFreeName },
		{ u8"寵物lv", kPetLevel },
		{ u8"寵物hp", kPetHp },
		{ u8"寵物maxhp", kPetMaxHp },
		{ u8"寵物hpp", kPetHpPercent },
		{ u8"寵物exp", kPetExp },
		{ u8"寵物maxexp", kPetMaxExp },
		{ u8"寵物atk", kPetAtk },
		{ u8"寵物def", kPetDef },
		{ u8"寵物agi", kPetAgi },
		{ u8"寵物loyal", kPetLoyal },
		{ u8"寵物state", kPetState },
		{ u8"寵物earth", kPetEarth },
		{ u8"寵物water", kPetWater },
		{ u8"寵物fire", kPetFire },
		{ u8"寵物wind", kPetWind },

		{ u8"道具數量", itemCount },

		{ u8"組隊人數", kTeamCount },
		{ u8"寵物數量", kPetCount },


		{ u8"人物name", kPlayerName },
		{ u8"人物fname", kPlayerFreeName },
		{ u8"人物lv", kPlayerLevel },
		{ u8"人物hp", kPlayerHp },
		{ u8"人物maxhp", kPlayerMaxHp },
		{ u8"人物hpp", kPlayerHpPercent },
		{ u8"人物mp", kPlayerMp },
		{ u8"人物maxmp", kPlayerMaxMp },
		{ u8"人物mpp", kPlayerMpPercent },
		{ u8"人物exp", kPlayerExp },
		{ u8"人物maxexp", kPlayerMaxExp },
		{ u8"人物stone", kPlayerStone },
		{ u8"人物atk", kPlayerAtk },
		{ u8"人物def", kPlayerDef },
		{ u8"人物agi", kPlayerAgi },
		{ u8"人物chasma", kPlayerChasma },
		{ u8"人物earth", kPlayerEarth },
		{ u8"人物water", kPlayerWater },
		{ u8"人物fire", kPlayerFire },
		{ u8"人物wind", kPlayerWind },

		{ u8"宠物name", kPetName },
		{ u8"宠物fname", kPetFreeName },
		{ u8"宠物lv", kPetLevel },
		{ u8"宠物hp", kPetHp },
		{ u8"宠物maxhp", kPetMaxHp },
		{ u8"宠物hpp", kPetHpPercent },
		{ u8"宠物exp", kPetExp },
		{ u8"宠物maxexp", kPetMaxExp },
		{ u8"宠物atk", kPetAtk },
		{ u8"宠物def", kPetDef },
		{ u8"宠物agi", kPetAgi },
		{ u8"宠物loyal", kPetLoyal },
		{ u8"宠物state", kPetState },
		{ u8"宠物earth", kPetEarth },
		{ u8"宠物water", kPetWater },
		{ u8"宠物fire", kPetFire },
		{ u8"宠物wind", kPetWind },

		{ u8"道具数量", itemCount },

		{ u8"组队人数", kTeamCount },
		{ u8"宠物数量", kPetCount },

		{ u8"ifitem", itemCount },

		{ u8"ifteam", kTeamCount },
		{ u8"ifpet", kPetCount },
	};

	template<typename Func>
	void registerFunction(const QString functionName, Func fun);
	void openLibsBIG5();
	void openLibsGB2312();
	void openLibsUTF8();
	void openLibs();

private:
	bool checkBattleThenWait();
	bool findPath(QPoint dst, qint64 steplen, qint64 step_cost = 0, qint64 timeout = DEFAULT_FUNCTION_TIMEOUT * 36, std::function<qint64(QPoint& dst)> callback = nullptr, bool noAnnounce = false);

	bool waitfor(qint64 timeout, std::function<bool()> exprfun);
	bool checkString(const TokenMap& TK, qint64 idx, QString* ret);
	bool checkInteger(const TokenMap& TK, qint64 idx, qint64* ret);
	bool toVariant(const TokenMap& TK, qint64 idx, QVariant* ret);

	qint64 checkJump(const TokenMap& TK, qint64 idx, bool expr, JumpBehavior behavior);
	bool checkRange(const TokenMap& TK, qint64 idx, qint64* min, qint64* max);
	bool checkRelationalOperator(const TokenMap& TK, qint64 idx, RESERVE* ret) const;

	bool compare(const QVariant& a, const QVariant& b, RESERVE type) const;

	bool compare(CompareArea area, const TokenMap& TK);

	void logExport(qint64 currentline, const QString& text, qint64 color = 0);

	void setError(const QString& error) { parser_.setLastErrorMessage(error); }
private: //註冊給Parser的函數
	//system
	qint64 test(qint64 currentline, const TokenMap& TK) const;
	qint64 sleep(qint64 currentline, const TokenMap& TK);
	qint64 press(qint64 currentline, const TokenMap& TK);
	qint64 eo(qint64 currentline, const TokenMap& TK);
	qint64 announce(qint64 currentline, const TokenMap& TK);
	qint64 input(qint64 currentline, const TokenMap& TK);
	qint64 messagebox(qint64 currentline, const TokenMap& TK);
	qint64 talk(qint64 currentline, const TokenMap& TK);
	qint64 talkandannounce(qint64 currentline, const TokenMap& TK);
	qint64 logout(qint64 currentline, const TokenMap& TK);
	qint64 logback(qint64 currentline, const TokenMap& TK);
	qint64 cleanchat(qint64 currentline, const TokenMap& TK);
	qint64 set(qint64 currentline, const TokenMap& TK);
	qint64 savesetting(qint64 currentline, const TokenMap& TK);
	qint64 loadsetting(qint64 currentline, const TokenMap& TK);
	qint64 run(qint64 currentline, const TokenMap& TK);
	qint64 dostring(qint64 currentline, const TokenMap& TK);
	qint64 reg(qint64 currentline, const TokenMap& TK);
	qint64 timer(qint64 currentline, const TokenMap& TK);

	//check
	qint64 checkdaily(qint64 currentline, const TokenMap& TK);
	qint64 isbattle(qint64 currentline, const TokenMap& TK);
	qint64 isnormal(qint64 currentline, const TokenMap& TK);
	qint64 isonline(qint64 currentline, const TokenMap& TK);
	qint64 checkcoords(qint64 currentline, const TokenMap& TK);
	qint64 checkmap(qint64 currentline, const TokenMap& TK);
	qint64 checkmapnowait(qint64 currentline, const TokenMap& TK);
	qint64 checkdialog(qint64 currentline, const TokenMap& TK);
	qint64 checkchathistory(qint64 currentline, const TokenMap& TK);
	qint64 checkunit(qint64 currentline, const TokenMap& TK);
	qint64 checkplayerstatus(qint64 currentline, const TokenMap& TK);
	qint64 checkpetstatus(qint64 currentline, const TokenMap& TK);
	qint64 checkitemcount(qint64 currentline, const TokenMap& TK);
	qint64 checkpetcount(qint64 currentline, const TokenMap& TK);
	qint64 checkitemfull(qint64 currentline, const TokenMap& TK);
	qint64 checkitem(qint64 currentline, const TokenMap& TK);
	qint64 checkpet(qint64 currentline, const TokenMap& TK);
	//check-group
	qint64 checkteam(qint64 currentline, const TokenMap& TK);
	qint64 checkteamcount(qint64 currentline, const TokenMap& TK);


	//move
	qint64 setdir(qint64 currentline, const TokenMap& TK);
	qint64 move(qint64 currentline, const TokenMap& TK);
	qint64 fastmove(qint64 currentline, const TokenMap& TK);
	qint64 packetmove(qint64 currentline, const TokenMap& TK);
	qint64 findpath(qint64 currentline, const TokenMap& TK);
	qint64 movetonpc(qint64 currentline, const TokenMap& TK);
	qint64 warp(qint64 currentline, const TokenMap& TK);


	//action
	qint64 useitem(qint64 currentline, const TokenMap& TK);
	qint64 dropitem(qint64 currentline, const TokenMap& TK);
	qint64 playerrename(qint64 currentline, const TokenMap& TK);
	qint64 petrename(qint64 currentline, const TokenMap& TK);
	qint64 setpetstate(qint64 currentline, const TokenMap& TK);
	qint64 droppet(qint64 currentline, const TokenMap& TK);
	qint64 buy(qint64 currentline, const TokenMap& TK);
	qint64 sell(qint64 currentline, const TokenMap& TK);
	qint64 sellpet(qint64 currentline, const TokenMap& TK);
	qint64 make(qint64 currentline, const TokenMap& TK);
	qint64 cook(qint64 currentline, const TokenMap& TK);
	qint64 usemagic(qint64 currentline, const TokenMap& TK);
	qint64 pickitem(qint64 currentline, const TokenMap& TK);
	qint64 depositgold(qint64 currentline, const TokenMap& TK);
	qint64 withdrawgold(qint64 currentline, const TokenMap& TK);
	qint64 teleport(qint64 currentline, const TokenMap& TK);
	qint64 addpoint(qint64 currentline, const TokenMap& TK);
	qint64 learn(qint64 currentline, const TokenMap& TK);
	qint64 trade(qint64 currentline, const TokenMap& TK);

	qint64 recordequip(qint64 currentline, const TokenMap& TK);
	qint64 wearequip(qint64 currentline, const TokenMap& TK);
	qint64 unwearequip(qint64 currentline, const TokenMap& TK);
	qint64 petequip(qint64 currentline, const TokenMap& TK);
	qint64 petunequip(qint64 currentline, const TokenMap& TK);

	qint64 depositpet(qint64 currentline, const TokenMap& TK);
	qint64 deposititem(qint64 currentline, const TokenMap& TK);
	qint64 withdrawpet(qint64 currentline, const TokenMap& TK);
	qint64 withdrawitem(qint64 currentline, const TokenMap& TK);

	qint64 mail(qint64 currentline, const TokenMap& TK);

	//action-group
	qint64 join(qint64 currentline, const TokenMap& TK);
	qint64 leave(qint64 currentline, const TokenMap& TK);
	qint64 kick(qint64 currentline, const TokenMap& TK);

	qint64 leftclick(qint64 currentline, const TokenMap& TK);
	qint64 rightclick(qint64 currentline, const TokenMap& TK);
	qint64 leftdoubleclick(qint64 currentline, const TokenMap& TK);
	qint64 mousedragto(qint64 currentline, const TokenMap& TK);

	//hide
	qint64 ocr(qint64 currentline, const TokenMap& TK);
	qint64 dlg(qint64 currentline, const TokenMap& TK);
	qint64 regex(qint64 currentline, const TokenMap& TK);
	qint64 find(qint64 currentline, const TokenMap& TK);
	qint64 half(qint64 currentline, const TokenMap& TK);
	qint64 full(qint64 currentline, const TokenMap& TK);
	qint64 upper(qint64 currentline, const TokenMap& TK);
	qint64 lower(qint64 currentline, const TokenMap& TK);
	qint64 replace(qint64 currentline, const TokenMap& TK);
	qint64 toint(qint64 currentline, const TokenMap& TK);
	qint64 tostr(qint64 currentline, const TokenMap& TK);
	//qint64 todb(qint64 currentline, const TokenMap& TK);

	//battle
	qint64 bh(qint64 currentline, const TokenMap& TK);//atk
	qint64 bj(qint64 currentline, const TokenMap& TK);//magic
	qint64 bp(qint64 currentline, const TokenMap& TK);//skill
	qint64 bs(qint64 currentline, const TokenMap& TK);//switch
	qint64 be(qint64 currentline, const TokenMap& TK);//escape
	qint64 bd(qint64 currentline, const TokenMap& TK);//defense
	qint64 bi(qint64 currentline, const TokenMap& TK);//item
	qint64 bt(qint64 currentline, const TokenMap& TK);//catch
	qint64 bn(qint64 currentline, const TokenMap& TK);//nothing
	qint64 bw(qint64 currentline, const TokenMap& TK);//petskill
	qint64 bwf(qint64 currentline, const TokenMap& TK);//pet nothing
	qint64 bwait(qint64 currentline, const TokenMap& TK);//wait
	qint64 bend(qint64 currentline, const TokenMap& TK);//wait


private:
	qint64 beginLine_ = 0;

	QThread* thread_ = nullptr;

	Parser parser_;

	std::atomic_bool isRunning_ = false;

	QString scriptFileName_;

	QList<QSharedPointer<Interpreter>> subInterpreterList_;
	QFutureSynchronizer<bool> futureSync_;
	ParserCallBack pCallback = nullptr;

	QHash<QString, QSharedPointer<QElapsedTimer>> customTimer_;
};