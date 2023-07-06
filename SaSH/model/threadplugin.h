#pragma once

#include <QThread>
#include <QObject>
#include <QWaitCondition>
#include <QMutex>
#include <QReadWriteLock>

#include "signaldispatcher.h"
class ThreadPlugin : public QObject
{
	Q_OBJECT
public:
	explicit ThreadPlugin(QObject* parent) : QObject(parent)
	{
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
		connect(&signalDispatcher, &SignalDispatcher::nodifyAllStop, this, &ThreadPlugin::requestInterruption);
	}

	virtual ~ThreadPlugin() = default;

	Q_REQUIRED_RESULT inline bool isInterruptionRequested() const
	{
		QReadLocker locker(&lock_);
		return isInterruptionRequested_.load(std::memory_order_acquire);
	}

	inline bool isPaused() const { return isPaused_.load(std::memory_order_acquire); }

	void checkPause()
	{
		if (isPaused_.load(std::memory_order_acquire))
		{
			pausedMutex_.lock();
			waitCondition_.wait(&pausedMutex_);
			pausedMutex_.unlock();
		}
	}

public slots:
	void requestInterruption()
	{
		QWriteLocker locker(&lock_);
		isInterruptionRequested_.store(true, std::memory_order_release);
	}

	void paused()
	{
		isPaused_.store(true, std::memory_order_release);
	}

	void resumed()
	{
		isPaused_.store(false, std::memory_order_release);
		waitCondition_.wakeAll();
	}

private:
	std::atomic_bool isInterruptionRequested_ = false;
	mutable QReadWriteLock lock_;

	std::atomic_bool isPaused_ = false;
	mutable QWaitCondition waitCondition_;
	mutable QMutex pausedMutex_;
};