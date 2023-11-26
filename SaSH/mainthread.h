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
#include <QObject>
#include <QSharedMemory>
#include <indexer.h>
#include <util.h>

class QThread;
class Server;

class MissionThread : public QObject, public Indexer
{
	Q_OBJECT
public:
	enum
	{
		kAutoJoin = 0,
		kAutoWalk,
		kAutoSortItem,
		kAutoRecordNPC,
		kMaxAutoMission,
		kAsyncFindPath
	};

	MissionThread(long long index, long long type, QObject* parent = nullptr);
	virtual ~MissionThread();

	void wait();

	inline bool isRunning() { return future_.isRunning(); }
	inline bool isFinished() { return future_.isFinished(); }
	inline void appendArg(const QVariant& arg) { args_.append(arg); }
	inline void appendArgs(const QVariantList& args) { args_.append(args); }

	[[nodiscard]] inline bool __fastcall isMissionInterruptionRequested() const
	{
		return isMissionInterruptionRequested_.get();
	}

	void requestMissionInterruption()
	{
		isMissionInterruptionRequested_.on();
	}

	void start();

private:
	void autoJoin();
	void autoWalk();
	void autoSortItem();
	void autoRecordNPC();
	void asyncFindPath();

	void reset()
	{
		isMissionInterruptionRequested_.off();
	}

private:
	long long type_ = 0;
	QFuture<void> future_;
	std::function<void()> func_;
	QVariantList args_;
	safe::flag isMissionInterruptionRequested_ = false;
};

class MainObject : public QObject, public Indexer
{
	Q_OBJECT
public:
	MainObject(long long index, QObject* parent);
	virtual ~MainObject();

signals:
	void finished();

public slots:
	void run();

private:
	long long __fastcall inGameInitialize() const;

	void __fastcall mainProc();

	void __fastcall updateAfkInfos() const;

	long long __fastcall checkAndRunFunctions();

	void __fastcall checkControl();
	void __fastcall checkEtcFlag() const;
	//void checkAutoAbility();

public:
	QThread thread;

private:

	util::REMOVE_THREAD_REASON remove_thread_reason = util::REASON_NO_ERROR;

	bool login_run_once_flag_ = true;
	bool battle_run_once_flag_ = false;

	bool flagBattleDialogEnable_ = false;
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
	long long flagLockTimeValue_ = 0;
	long long flagSetBoostValue_ = 0;
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

	safe::vector<MissionThread*> autoThreads_;
};

class ThreadManager : public QObject
{
	Q_OBJECT;
	Q_DISABLE_COPY_MOVE(ThreadManager);
public:
	static ThreadManager& getInstance()
	{
		static ThreadManager instance;
		return instance;
	}

	void close(long long index);

	inline void close()
	{
		QMutexLocker locker(&mutex_);
		QList<long long> keys = objects_.keys();
		for (auto& key : keys)
		{
			close(key);
		}
	}

	inline void wait(long long index)
	{
		QMutexLocker locker(&mutex_);
		if (objects_.contains(index))
		{
			auto thread_ = objects_.value(index);
			thread_->thread.wait();
		}
	}

	virtual ~ThreadManager() = default;

private:
	ThreadManager() = default;

public:
	bool createThread(long long index, MainObject** ppObj, QObject* parent);

	[[nodiscard]] inline long long size() const
	{
		QMutexLocker locker(&mutex_);
		return objects_.size();
	}

private:
	mutable QMutex mutex_;
	QHash<long long, MainObject*> objects_;

};

class UniqueIdManager
{
	Q_DISABLE_COPY_MOVE(UniqueIdManager)
private:
	UniqueIdManager()
	{

	}

public:
	static UniqueIdManager& getInstance()
	{
		static UniqueIdManager instance;
		return instance;
	}

	virtual ~UniqueIdManager();

public:
	long long allocateUniqueId(long long id);
	void deallocateUniqueId(long long id);

private:
	void reset()
	{
		clear();
	}

	void clear();

	void write(const char* from);

	bool readSharedMemory(QSet<long long>* pAllocatedIds);

	void updateSharedMemory(const QSet<long long>& allocatedIds);

private:
	const QString jsonKey_ = "ids";
};