#pragma once
#include <QObject>
#include <QScopedPointer>
#include <atomic>
#include "parser.h"
#include "util.h"

typedef struct break_marker_s
{
	int line = 0;
	int count = 0;
	int maker = 0;
	QString content = "\0";
} break_marker_t;

class Interpreter : public QObject
{
	Q_OBJECT
public:

public:
	Interpreter(QObject* parent = nullptr);
	virtual ~Interpreter();

	inline Q_REQUIRED_RESULT bool isInterruptionRequested() const { return isRequestInterrupted.load(std::memory_order_acquire); }

	inline bool isRunning() const { return isRunning_.load(std::memory_order_acquire) && !isInterruptionRequested(); }

	void preview(const QString& fileName);

	void doString(const QString& script);

	void doFileWithThread(int beginLine, const QString& fileName);

	bool doFile(int beginLine, const QString& fileName, Interpreter* parent, bool isVarShared);

	void stop();

	void pause();

	void resume();

	inline bool isPaused() const { return isPaused_.load(std::memory_order_acquire); }

	inline void setSubScript(bool isSub) { isSub = isSub; }

	Q_REQUIRED_RESULT inline bool isSubScript() const { return isSub; }

signals:
	void finished();


public slots:
	inline void requestInterruption() { isRequestInterrupted.store(true, std::memory_order_release); }

	void proc();
private:
	bool readFile(const QString& fileName, QString* pcontent);
	bool loadString(const QString& script, QHash<int, TokenMap>* ptokens, QHash<QString, int>* plabel);
private:
	enum JumpBehavior
	{
		SuccessJump,
		FailedJump,
	};

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

		itemCount,

		kTeamCount,
		kPetCount,

	};

	inline static const QHash<QString, CompareType> compareTypeMap = {
		{ u8"人物name", kPlayerName },
		{ u8"人物freename", kPlayerFreeName },
		{ u8"人物level", kPlayerLevel },
		{ u8"人物hp", kPlayerHp },
		{ u8"人物maxhp", kPlayerMaxHp },
		{ u8"人物hppercent", kPlayerHpPercent },
		{ u8"人物mp", kPlayerMp },
		{ u8"人物maxmp", kPlayerMaxMp },
		{ u8"人物mppercent", kPlayerMpPercent },
		{ u8"人物exp", kPlayerExp },
		{ u8"人物maxexp", kPlayerMaxExp },
		{ u8"人物stone", kPlayerStone },
		{ u8"人物atk", kPlayerAtk },
		{ u8"人物def", kPlayerDef },
		{ u8"人物agi", kPlayerAgi },
		{ u8"人物chasma", kPlayerChasma },

		{ u8"寵物name", kPetName },
		{ u8"寵物freename", kPetFreeName },
		{ u8"寵物level", kPetLevel },
		{ u8"寵物hp", kPetHp },
		{ u8"寵物maxhp", kPetMaxHp },
		{ u8"寵物hppercent", kPetHpPercent },
		{ u8"寵物exp", kPetExp },
		{ u8"寵物maxexp", kPetMaxExp },
		{ u8"寵物atk", kPetAtk },
		{ u8"寵物def", kPetDef },
		{ u8"寵物agi", kPetAgi },
		{ u8"寵物loyal", kPetLoyal },
		{ u8"寵物state", kPetState },

		{ u8"道具數量", itemCount },

		{ u8"組隊人數", kTeamCount },
		{ u8"寵物數量", kPetCount },


		{ u8"人物name", kPlayerName },
		{ u8"人物freename", kPlayerFreeName },
		{ u8"人物level", kPlayerLevel },
		{ u8"人物hp", kPlayerHp },
		{ u8"人物maxhp", kPlayerMaxHp },
		{ u8"人物hppercent", kPlayerHpPercent },
		{ u8"人物mp", kPlayerMp },
		{ u8"人物maxmp", kPlayerMaxMp },
		{ u8"人物mppercent", kPlayerMpPercent },
		{ u8"人物exp", kPlayerExp },
		{ u8"人物maxexp", kPlayerMaxExp },
		{ u8"人物stone", kPlayerStone },
		{ u8"人物atk", kPlayerAtk },
		{ u8"人物def", kPlayerDef },
		{ u8"人物agi", kPlayerAgi },
		{ u8"人物chasma", kPlayerChasma },

		{ u8"宠物name", kPetName },
		{ u8"宠物freename", kPetFreeName },
		{ u8"宠物level", kPetLevel },
		{ u8"宠物hp", kPetHp },
		{ u8"宠物maxhp", kPetMaxHp },
		{ u8"宠物hppercent", kPetHpPercent },
		{ u8"宠物exp", kPetExp },
		{ u8"宠物maxexp", kPetMaxExp },
		{ u8"宠物atk", kPetAtk },
		{ u8"宠物def", kPetDef },
		{ u8"宠物agi", kPetAgi },
		{ u8"宠物loyal", kPetLoyal },
		{ u8"宠物state", kPetState },

		{ u8"道具数量", itemCount },

		{ u8"组队人数", kTeamCount },
		{ u8"宠物数量", kPetCount },
	};

	template<typename Func>
	void registerFunction(const QString functionName, Func fun);
	void openLibsBIG5();
	void openLibsGB2312();
	void openLibsUTF8();
	void openLibs();
private:
	bool checkBattleThenWait();
	bool findPath(QPoint dst, int steplen, int step_cost = 0, int timeout = 180000, std::function<int(QPoint& dst)> callback = nullptr, bool noAnnounce = false);

	bool waitfor(int timeout, std::function<bool()> exprfun) const;
	bool checkString(const TokenMap& TK, int idx, QString* ret) const;
	bool checkInt(const TokenMap& TK, int idx, int* ret) const;
	bool checkDouble(const TokenMap& TK, int idx, double* ret) const;
	bool toVariant(const TokenMap& TK, int idx, QVariant* ret) const;
	int checkJump(const TokenMap& TK, int idx, bool expr, JumpBehavior behavior) const;
	bool checkRange(const TokenMap& TK, int idx, int* min, int* max) const;
	bool checkRelationalOperator(const TokenMap& TK, int idx, RESERVE* ret) const;

	bool compare(const QVariant& a, const QVariant& b, RESERVE type) const;

	bool compare(CompareArea area, const TokenMap& TK) const;


	void checkPause();

	void updateGlobalVariables();

	void logExport(int currentline, const QString& text, int color = 0);
private: //註冊給Parser的函數
	//system
	int test(int currentline, const TokenMap&) const;
	int sleep(int currentline, const TokenMap&);
	int press(int currentline, const TokenMap&);
	int eo(int currentline, const TokenMap& TK);
	int announce(int currentline, const TokenMap& TK);
	int input(int currentline, const TokenMap& TK);
	int messagebox(int currentline, const TokenMap& TK);
	int talk(int currentline, const TokenMap& TK);
	int talkandannounce(int currentline, const TokenMap& TK);
	int logout(int currentline, const TokenMap& TK);
	int logback(int currentline, const TokenMap& TK);
	int cleanchat(int currentline, const TokenMap& TK);
	int set(int currentline, const TokenMap& TK);
	int savesetting(int currentline, const TokenMap& TK);
	int loadsetting(int currentline, const TokenMap& TK);
	int run(int currentline, const TokenMap& TK);

	//check
	int checkdaily(int currentline, const TokenMap& TK);
	int isbattle(int currentline, const TokenMap& TK);
	int checkcoords(int currentline, const TokenMap& TK);
	int checkmap(int currentline, const TokenMap& TK);
	int checkmapnowait(int currentline, const TokenMap& TK);
	int checkdialog(int currentline, const TokenMap& TK);
	int checkchathistory(int currentline, const TokenMap& TK);
	int checkunit(int currentline, const TokenMap& TK);
	int checkplayerstatus(int currentline, const TokenMap& TK);
	int checkpetstatus(int currentline, const TokenMap& TK);
	int checkitemcount(int currentline, const TokenMap& TK);
	int checkpetcount(int currentline, const TokenMap& TK);
	int checkitemfull(int currentline, const TokenMap& TK);
	int checkitem(int currentline, const TokenMap& TK);
	int checkpet(int currentline, const TokenMap& TK);
	//check-group
	int checkteam(int currentline, const TokenMap& TK);
	int checkteamcount(int currentline, const TokenMap& TK);
	int cmp(int currentline, const TokenMap& TK);

	//move
	int setdir(int currentline, const TokenMap& TK);
	int move(int currentline, const TokenMap& TK);
	int fastmove(int currentline, const TokenMap& TK);
	int packetmove(int currentline, const TokenMap& TK);
	int findpath(int currentline, const TokenMap& TK);
	int movetonpc(int currentline, const TokenMap& TK);
	int warp(int currentline, const TokenMap& TK);


	//action
	int useitem(int currentline, const TokenMap& TK);
	int dropitem(int currentline, const TokenMap& TK);
	int playerrename(int currentline, const TokenMap& TK);
	int petrename(int currentline, const TokenMap& TK);
	int setpetstate(int currentline, const TokenMap& TK);
	int droppet(int currentline, const TokenMap& TK);
	int buy(int currentline, const TokenMap& TK);
	int sell(int currentline, const TokenMap& TK);
	int make(int currentline, const TokenMap& TK);
	int cook(int currentline, const TokenMap& TK);
	int usemagic(int currentline, const TokenMap& TK);
	int pickitem(int currentline, const TokenMap& TK);
	int depositgold(int currentline, const TokenMap& TK);
	int withdrawgold(int currentline, const TokenMap& TK);
	int teleport(int currentline, const TokenMap& TK);
	int addpoint(int currentline, const TokenMap& TK);

	int recordequip(int currentline, const TokenMap& TK);
	int wearequip(int currentline, const TokenMap& TK);
	int unwearequip(int currentline, const TokenMap& TK);

	int depositpet(int currentline, const TokenMap& TK);
	int deposititem(int currentline, const TokenMap& TK);
	int withdrawpet(int currentline, const TokenMap& TK);
	int withdrawitem(int currentline, const TokenMap& TK);

	//action-group
	int join(int currentline, const TokenMap& TK);
	int leave(int currentline, const TokenMap& TK);

	int leftclick(int currentline, const TokenMap& TK);
	int rightclick(int currentline, const TokenMap& TK);
	int leftdoubleclick(int currentline, const TokenMap& TK);
	int mousedragto(int currentline, const TokenMap& TK);


private:
	int beginLine_ = 0;
	bool isSub = false;

	std::atomic_bool isRequestInterrupted = false;

	QThread* thread_ = nullptr;

	QScopedPointer<Parser> parser_;

	std::atomic_bool isRunning_ = false;

	//是否暫停
	std::atomic_bool isPaused_ = false;
	mutable QWaitCondition waitCondition_;
	mutable QMutex mutex_;
};