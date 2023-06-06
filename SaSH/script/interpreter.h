#pragma once
#include <QObject>
#include <QScopedPointer>
#include <atomic>
#include "parser.h"

using TokenMap = QMap<int, Token>;

class Interpreter : public QObject
{
	Q_OBJECT
public:
	Interpreter(QObject* parent = nullptr);
	virtual ~Interpreter();

	inline void requestInterruption() { isRequestInterrupted.store(true, std::memory_order_release); }

	Q_REQUIRED_RESULT bool isInterruptionRequested() const { return isRequestInterrupted.load(std::memory_order_acquire); }

	bool isRunning() const { return isRunning_.load(std::memory_order_acquire) && !isInterruptionRequested(); }
	bool loadFile(const QString& fileName, QHash<int, TokenMap>* hash);
	bool loadString(const QString& script, QHash<int, TokenMap>* hash);
	void doString(const QString& script);

	void start(int beginLine, const QString& fileName);
	void stop();
	void pause();
	void resume();
	inline bool isPaused() const { return isPaused_.load(std::memory_order_acquire); }

private:
	enum JumpBehavior
	{
		SuccessJump,
		FailedJump,
	};


	template<typename Func>
	void registerFunction(const QString functionName, Func fun);
	void openLibs();

private:
	bool checkBattleThenWait();
	bool findPath(const QPoint& dst, int steplen, int step_cost = 0, int timeout = 180000, std::function<int()> callback = nullptr);

	bool waitfor(int timeout, std::function<bool()> exprfun);
	bool checkString(const TokenMap& TK, int idx, QString* ret);
	bool checkInt(const TokenMap& TK, int idx, int* ret);
	int checkJump(const TokenMap& TK, int idx, bool expr, JumpBehavior behavior);
private: //註冊給Parser的函數
	//system
	int test(const TokenMap&);
	int sleep(const TokenMap&);
	int press(const TokenMap&);
	int eo(const TokenMap& TK);
	int announce(const TokenMap& TK);
	int talk(const TokenMap& TK);
	int talkandannounce(const TokenMap& TK);
	int logout(const TokenMap& TK);
	int logback(const TokenMap& TK);
	int cleanchat(const TokenMap& TK);

	//check
	int checkdaily(const TokenMap& TK);
	int isbattle(const TokenMap& TK);
	int checkcoords(const TokenMap& TK);
	int checkmap(const TokenMap& TK);
	int checkmapnowait(const TokenMap& TK);
	int checkdialog(const TokenMap& TK);
	int checkchathistory(const TokenMap& TK);
	int checkunit(const TokenMap& TK);

	//move
	int setdir(const TokenMap& TK);
	int move(const TokenMap& TK);
	int fastmove(const TokenMap& TK);
	int packetmove(const TokenMap& TK);
	int findpath(const TokenMap& TK);
	int movetonpc(const TokenMap& TK);
	int findnpc(const TokenMap& TK);

	//action
	int useitem(const TokenMap& TK);
	int dropitem(const TokenMap& TK);
signals:
	void finished();

public slots:
	void run();

private:
	int beginLine_ = 0;
	std::atomic_bool isRequestInterrupted = false;
	QString currentMainScriptFileName_ = "";
	QString currentMainScriptString_ = "";
	QHash<QString, QHash<int, TokenMap>> alltokens_;//所有已加載過的腳本Tokens
	QThread* thread_ = nullptr;
	QScopedPointer<Lexer> lexer_;
	QScopedPointer<Parser> parser_;

	std::atomic_bool isRunning_ = false;
	//是否暫停
	std::atomic_bool isPaused_ = false;
	QWaitCondition waitCondition_;
	QMutex mutex_;
};