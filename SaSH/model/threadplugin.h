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

#include <QThread>
#include <QObject>
#include <QWaitCondition>
#include <QMutex>
#include <QReadWriteLock>

#include "signaldispatcher.h"
#include <shared_mutex>
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
		std::shared_lock<std::shared_mutex> lock(rwLock_);
		return isInterruptionRequested_.load(std::memory_order_acquire);
	}

	inline bool isPaused() const { return isPaused_; }

	inline void checkPause()
	{
		std::unique_lock<std::mutex> lock(pausedMutex_);
		if (isPaused_.load(std::memory_order_acquire))
		{
			pausedCondition_.wait(lock);
		}
	}

public slots:
	void requestInterruption()
	{
		std::unique_lock<std::shared_mutex> lock(rwLock_);
		isInterruptionRequested_.store(true, std::memory_order_release);
	}

	void paused()
	{
		pausedMutex_.lock();
		isPaused_.store(true, std::memory_order_release);
		pausedMutex_.unlock();
	}

	void resumed()
	{
		{
			pausedMutex_.lock();
			isPaused_.store(false, std::memory_order_release);
			pausedMutex_.unlock();
		}
		pausedCondition_.notify_all();
	}

private:
	std::atomic_bool isInterruptionRequested_ = false;
	mutable std::shared_mutex rwLock_;

	std::atomic_bool isPaused_ = false;
	std::condition_variable pausedCondition_;
	std::mutex pausedMutex_;
};