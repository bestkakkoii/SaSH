#pragma once
#include <QObject>
#include <QScopedPointer>
#include <atomic>
#include "parser.h"
#include "util.h"

using TokenMap = QMap<int, Token>;
Q_DECLARE_METATYPE(TokenMap)

class Interpreter : public QObject
{
	Q_OBJECT
public:
	Interpreter(QObject* parent = nullptr);
	virtual ~Interpreter();

	inline Q_REQUIRED_RESULT bool isInterruptionRequested() const { return isRequestInterrupted.load(std::memory_order_acquire); }

	inline bool isRunning() const { return isRunning_.load(std::memory_order_acquire) && !isInterruptionRequested(); }

	void preview(const QString& fileName);

	void doString(const QString& script);

	void doFileWithThread(int beginLine, const QString& fileName);

	bool doFile(int beginLine, const QString& fileName, Interpreter* parent);

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
		{ u8"人物主名", kPlayerName },
		{ u8"人物副名", kPlayerFreeName },
		{ u8"人物等級", kPlayerLevel },
		{ u8"人物耐久力", kPlayerHp },
		{ u8"人物最大耐久力", kPlayerMaxHp },
		{ u8"人物耐久力百分比", kPlayerHpPercent },
		{ u8"人物氣力", kPlayerMp },
		{ u8"人物最大氣力", kPlayerMaxMp },
		{ u8"人物氣力百分比", kPlayerMpPercent },
		{ u8"人物經驗", kPlayerExp },
		{ u8"人物經驗剩餘", kPlayerMaxExp },
		{ u8"人物石幣", kPlayerStone },
		{ u8"人物攻擊", kPlayerAtk },
		{ u8"人物防禦", kPlayerDef },
		{ u8"人物敏捷", kPlayerAgi },
		{ u8"人物魅力", kPlayerChasma },

		{ u8"寵物主名", kPetName },
		{ u8"寵物副名", kPetFreeName },
		{ u8"寵物等級", kPetLevel },
		{ u8"寵物耐久力", kPetHp },
		{ u8"寵物最大耐久力", kPetMaxHp },
		{ u8"寵物耐久力百分比", kPetHpPercent },
		{ u8"寵物經驗", kPetExp },
		{ u8"寵物剩餘經驗", kPetMaxExp },
		{ u8"寵物攻擊", kPetAtk },
		{ u8"寵物防禦", kPetDef },
		{ u8"寵物敏捷", kPetAgi },
		{ u8"寵物忠誠", kPetLoyal },
		{ u8"寵物狀態", kPetState },

		{ u8"道具數量", itemCount },

		{ u8"組隊人數", kTeamCount },
		{ u8"寵物數量", kPetCount },

	};

	template<typename Func>
	void registerFunction(const QString functionName, Func fun);
	void openLibs();

private:
	bool checkBattleThenWait();
	bool findPath(QPoint dst, int steplen, int step_cost = 0, int timeout = 180000, std::function<int(QPoint& dst)> callback = nullptr);

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
private: //註冊給Parser的函數
	//system
	int test(const TokenMap&) const;
	int sleep(const TokenMap&);
	int press(const TokenMap&);
	int eo(const TokenMap& TK);
	int announce(const TokenMap& TK);
	int input(const TokenMap& TK);
	int messagebox(const TokenMap& TK);
	int talk(const TokenMap& TK);
	int talkandannounce(const TokenMap& TK);
	int logout(const TokenMap& TK);
	int logback(const TokenMap& TK);
	int cleanchat(const TokenMap& TK);
	int set(const TokenMap& TK);
	int savesetting(const TokenMap& TK);
	int loadsetting(const TokenMap& TK);
	int run(const TokenMap& TK);

	//check
	int checkdaily(const TokenMap& TK);
	int isbattle(const TokenMap& TK);
	int checkcoords(const TokenMap& TK);
	int checkmap(const TokenMap& TK);
	int checkmapnowait(const TokenMap& TK);
	int checkdialog(const TokenMap& TK);
	int checkchathistory(const TokenMap& TK);
	int checkunit(const TokenMap& TK);
	int checkplayerstatus(const TokenMap& TK);
	int checkpetstatus(const TokenMap& TK);
	int checkitemcount(const TokenMap& TK);
	int checkpetcount(const TokenMap& TK);
	int checkitemfull(const TokenMap& TK);
	int checkitem(const TokenMap& TK);
	int checkpet(const TokenMap& TK);
	//check-group
	int checkteam(const TokenMap& TK);
	int checkteamcount(const TokenMap& TK);
	int cmp(const TokenMap& TK);

	//move
	int setdir(const TokenMap& TK);
	int move(const TokenMap& TK);
	int fastmove(const TokenMap& TK);
	int packetmove(const TokenMap& TK);
	int findpath(const TokenMap& TK);
	int movetonpc(const TokenMap& TK);


	//action
	int useitem(const TokenMap& TK);
	int dropitem(const TokenMap& TK);
	int playerrename(const TokenMap& TK);
	int petrename(const TokenMap& TK);
	int setpetstate(const TokenMap& TK);
	int droppet(const TokenMap& TK);
	int buy(const TokenMap& TK);
	int sell(const TokenMap& TK);
	int make(const TokenMap& TK);
	int cook(const TokenMap& TK);
	int usemagic(const TokenMap& TK);
	int pickitem(const TokenMap& TK);
	int depositgold(const TokenMap& TK);
	int withdrawgold(const TokenMap& TK);
	int warp(const TokenMap& TK);
	int leftclick(const TokenMap& TK);
	int addpoint(const TokenMap& TK);

	int recordequip(const TokenMap& TK);
	int wearequip(const TokenMap& TK);
	int unwearequip(const TokenMap& TK);

	int depositpet(const TokenMap& TK);
	int deposititem(const TokenMap& TK);
	int withdrawpet(const TokenMap& TK);
	int withdrawitem(const TokenMap& TK);

	//action-group
	int join(const TokenMap& TK);
	int leave(const TokenMap& TK);


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