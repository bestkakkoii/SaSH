#pragma once

#include <QThread>
#include <QObject>
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

public slots:
	void requestInterruption()
	{
		QWriteLocker locker(&lock_);
		isInterruptionRequested_.store(true, std::memory_order_release);
	}

private:
	std::atomic_bool isInterruptionRequested_ = false;
	mutable QReadWriteLock lock_;
};