#pragma once
#include <QObject>
#include <util.h>

class QThread;
class Server;

class MainObject : public QObject
{
	Q_OBJECT
public:
	MainObject(QObject* parent = nullptr);
	virtual ~MainObject();

	void requestInterruption()
	{
		isRequestInterrupted.store(true, std::memory_order_release);
	}

	bool isInterruptionRequested() const
	{
		return isRequestInterrupted.load(std::memory_order_acquire);
	}

signals:
	void finished();

public slots:
	void run();


private:


	void mainProc();

	void updateAfkInfos();
	void setUserDatas();

	int checkAndRunFunctions();

	void checkControl();
	void checkEtcFlag();
	void checkAutoWalk();
	void checkAutoDropItems();
	void checkAutoJoin();
	void checkAutoHeal();
	void checkAutoDropPet();

private:
	void battleTimeThread();

private:
	util::REMOVE_THREAD_REASON remove_thread_reason = util::REASON_NO_ERROR;

	std::atomic_bool isRequestInterrupted = false;

	QFuture<void> autowalk_future_;
	std::atomic_bool autowalk_future_cancel_flag_ = false;

	QFuture<void> autojoin_future_;
	std::atomic_bool autojoin_future_cancel_flag_ = false;

	QFuture<void> battleTime_future_;
	std::atomic_bool battleTime_future_cancel_flag_ = false;

	QFuture<void> autoheal_future_;
	std::atomic_bool autoheal_future_cancel_flag_ = false;

	QFuture<void> autodroppet_future_;
	std::atomic_bool autodroppet_future_cancel_flag_ = false;

	bool login_run_once_flag_ = false;
	bool battle_run_once_flag_ = false;

	bool flagAutoLoginEnable_ = false;
	bool flagAutoReconnectEnable_ = false;
	bool flagLogOutEnable_ = false;
	bool flagLogBackEnable_ = false;
	bool flagHideCharacterEnable_ = false;
	bool flagCloseEffectEnable_ = false;
	bool flagOptimizeEnable_ = false;
	bool flagHideWindowEnable_ = false;
	bool flagMuteEnable_ = false;
	bool flagAutoJoinEnable_ = false;
	bool flagLockTimeEnable_ = false;
	int flagLockTimeValue_ = 0;
	int flagSetBoostValue_ = 0;
	bool flagAutoFreeMemoryEnable_ = false;
	bool flagFastWalkEnable_ = false;
	bool flagPassWallEnable_ = false;
	bool flagLockMoveEnable_ = false;
	bool flagLockImageEnable_ = false;
	bool flagAutoDropMeatEnable_ = false;
	bool flagAutoDropEnable_ = false;
	bool flagAutoStackEnable_ = false;
	bool flagKNPCEnable_ = false;
	bool flagAutoAnswerEnable_ = false;
	bool flagForceLeaveBattleEnable_ = false;
	bool flagAutoWalkEnable_ = false;
	bool flagFastAutoWalkEnable_ = false;
	bool flagFastBattleEnable_ = false;
	bool flagAutoBattleEnable_ = false;
	bool flagAutoCatchEnable_ = false;
	bool flagLockAttackEnable_ = false;
	bool flagAutoEscapeEnable_ = false;
	bool flagLockEscapeEnable_ = false;
	bool flagBattleTimeExtendEnable_ = false;
	bool flagFallDownEscapeEnable_ = false;

	// switcher
	bool flagSwitcherTeamEnable_ = false;
	bool flagSwitcherPKEnable_ = false;
	bool flagSwitcherCardEnable_ = false;
	bool flagSwitcherTradeEnable_ = false;
	bool flagSwitcherFamilyEnable_ = false;
	bool flagSwitcherJobEnable_ = false;
	bool flagSwitcherWorldEnable_ = false;
};

class ThreadManager : public QObject
{
	Q_OBJECT;
	Q_DISABLE_COPY_MOVE(ThreadManager);
public:
	static ThreadManager& getInstance()
	{
		static ThreadManager* instance = new ThreadManager();
		return *instance;
	}
	void close()
	{
		if (thread_ != nullptr)
		{
			object_->requestInterruption();
			thread_->quit();
			thread_->wait();
			delete thread_;
			thread_ = nullptr;
			delete object_;
			object_ = nullptr;
		}
	}
	virtual ~ThreadManager() = default;
private:
	ThreadManager() = default;

public:
	bool createThread(QObject* parent = nullptr);

private:
	QThread* thread_ = nullptr;
	MainObject* object_ = nullptr;

};