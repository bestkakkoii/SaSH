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
#include <threadplugin.h>
#include <util.h>

class QThread;
class Server;

class MainObject : public ThreadPlugin
{
	Q_OBJECT
public:
	MainObject(qint64 index, QObject* parent);
	virtual ~MainObject();

signals:
	void finished();

public slots:
	void run();

private:
	void mainProc();

	void updateAfkInfos();

	void setUserDatas();

	qint64 checkAndRunFunctions();

	void checkControl();
	void checkEtcFlag();
	void checkAutoSortItem();
	void checkAutoWalk();
	void checkAutoDropItems();
	void checkAutoDropMeat(const QStringList& items);
	void checkAutoJoin();
	void checkAutoHeal();
	void checkAutoDropPet();
	//void checkAutoLockPet();
	void checkAutoLockSchedule();
	void checkAutoEatBoostExpItem();
	void checkRecordableNpcInfo();
private:
	void battleTimeThread();

private:

	util::REMOVE_THREAD_REASON remove_thread_reason = util::REASON_NO_ERROR;

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

	QFuture<void> autosortitem_future_;
	std::atomic_bool autosortitem_future_cancel_flag_ = false;

	QFutureSynchronizer <void> pointerWriterSync_;

	bool login_run_once_flag_ = false;
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
	qint64 flagLockTimeValue_ = 0;
	qint64 flagSetBoostValue_ = 0;
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
		static ThreadManager instance;
		return instance;
	}

	void close(qint64 index);

	inline void close()
	{
		QMutexLocker locker(&mutex_);
		QList<qint64> keys = threads_.keys();
		for (auto& key : keys)
		{
			close(key);
		}
	}

	inline void wait(qint64 index)
	{
		QMutexLocker locker(&mutex_);
		if (threads_.contains(index))
		{
			auto thread_ = threads_.value(index);
			thread_->wait();
		}
	}

	virtual ~ThreadManager() = default;

private:
	ThreadManager() = default;

public:
	bool createThread(qint64 index, MainObject** ppObj, QObject* parent);

	Q_REQUIRED_RESULT inline qint64 size() const
	{
		QMutex mutex_;
		return threads_.size();
	}

private:
	mutable QMutex mutex_;
	QHash<qint64, QThread*> threads_;
	QHash<qint64, MainObject*> objects_;

};

class UniqueIdManager
{
	Q_DISABLE_COPY_MOVE(UniqueIdManager)
private:
	UniqueIdManager()
	{
		QStringList list;
		for (int i = 0; i < SASH_MAX_THREAD; ++i)
			list << QString::number(i);

		QString combined = QString(R"({"ids":[%1]})").arg(list.join(","));
		QByteArray byteArray = combined.toUtf8();
		totalBytes_ = byteArray.size();
	}

public:
	static UniqueIdManager& getInstance()
	{
		static UniqueIdManager instance;
		return instance;
	}

	virtual ~UniqueIdManager()
	{
		if (sharedMemory_.isAttached())
			sharedMemory_.detach();
		qDebug() << "~UniqueIdManager()";
	}

public:
	qint64 allocateUniqueId(qint64 id)
	{
		if (id < 0 || id >= SASH_MAX_THREAD)
			id = -1;

		QSystemSemaphore semaphore("UniqueIdManagerSystemSemaphore", 1, QSystemSemaphore::Open);
		semaphore.acquire();

		qint64 allocatedId = -1;

		// 嘗試連接到共享內存，如果不存在則創建
		sharedMemory_.setKey("UniqueIdManagerSharedMemory");
		if (!sharedMemory_.attach())
		{

			sharedMemory_.create(totalBytes_);
			reset();
		}
		else
		{
			// 如果共享內存已經存在，則檢查大小是否足夠
			if (sharedMemory_.size() < totalBytes_)
			{
				sharedMemory_.detach();
				sharedMemory_.create(totalBytes_);
				reset();
			}
		}

		QSet<qint64> allocatedIds;

		do
		{
			bool bret = readSharedMemory(&allocatedIds);
			if (!bret)
				id = -1;

			if (id >= 0 && id < SASH_MAX_THREAD)
			{
				// 分配指定的ID
				if (!allocatedIds.contains(id))
				{
					allocatedIds.insert(id);
					updateSharedMemory(allocatedIds);
					allocatedId = id;
					break;
				}
			}

			// 分配唯一的ID
			for (qint64 i = 0; i < SASH_MAX_THREAD; ++i)
			{
				if (!allocatedIds.contains(i))
				{
					allocatedIds.insert(i);
					updateSharedMemory(allocatedIds);
					allocatedId = i;
					break;
				}
			}
		} while (false);

		semaphore.release();

		return  allocatedId;
	}

private:
	void reset()
	{
		clear();
	}

	void clear()
	{
		memset(sharedMemory_.data(), 0, sharedMemory_.size());
	}

	void write(const char* from)
	{
		clear();
		_snprintf_s(reinterpret_cast<char*>(sharedMemory_.data()), sharedMemory_.size(), _TRUNCATE, "%s", from);
	}

	bool readSharedMemory(QSet<qint64>* pAllocatedIds)
	{
		do
		{
			if (!sharedMemory_.lock())
				break;

			qDebug() << "sharedMemory_.size():" << sharedMemory_.size();

			QByteArray data = QByteArray::fromRawData(static_cast<const char*>(sharedMemory_.constData()), sharedMemory_.size());
			sharedMemory_.unlock();

			if (data.isEmpty())
				break;
			else if (data.front() == '\0')
				break;

			qint64 indexEof = data.indexOf('\0');
			if (indexEof != -1)
			{
				data = data.left(indexEof);
			}

			qDebug() << data;

			QJsonParseError error;
			QJsonDocument doc = QJsonDocument::fromJson(data, &error);
			if (error.error != QJsonParseError::NoError)
			{
				qDebug() << "QJsonParseError:" << error.errorString();
				break;
			}

			if (!doc.isObject())
			{
				qDebug() << "doc.isObject() == false";
				break;
			}

			QJsonObject obj = doc.object();
			QJsonValue value = obj[jsonKey_];
			if (!value.isArray())
			{
				qDebug() << "!value.isArray()";
				break;
			}

			QJsonArray idArray = value.toArray();
			for (const QJsonValue& idValue : idArray)
			{
				pAllocatedIds->insert(idValue.toInt());
			}

			return true;
		} while (false);

		return false;
	}

	void updateSharedMemory(const QSet<qint64>& allocatedIds)
	{
		// 將分配的ID保存為JSON字符串，並寫入共享內存
		QJsonObject obj;
		QJsonArray idArray;
		for (qint64 id : allocatedIds)
		{
			idArray.append(static_cast<int>(id));
		}

		obj[jsonKey_] = idArray;

		QJsonDocument doc(obj);

		// 將 JSON 文檔轉換為 UTF-8 編碼的 QByteArray
		QByteArray data = doc.toJson(QJsonDocument::Compact);
		QString str = QString::fromUtf8(data).simplified();
		data = str.toUtf8();
		data.replace(" ", "");

		if (sharedMemory_.lock())
		{
			const char* from = data.constData();
			write(from);
			sharedMemory_.unlock();
		}
	}

private:

	const QString jsonKey_ = "ids";
	qint64 totalBytes_ = 0;
	QSharedMemory sharedMemory_;
};