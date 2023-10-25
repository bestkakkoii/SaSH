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

#include "stdafx.h"
#include "mainthread.h"
#include "signaldispatcher.h"
#include "net/tcpserver.h"
#include "net/autil.h"
#include <injector.h>
#include "map/mapanalyzer.h"
#include "update/downloader.h"

QSharedMemory g_sharedMemory;

UniqueIdManager::~UniqueIdManager()
{
	if (g_sharedMemory.isAttached())
		g_sharedMemory.detach();
	qDebug() << "~UniqueIdManager()";
}

long long UniqueIdManager::allocateUniqueId(long long id)
{
	if (id < 0 || id >= SASH_MAX_THREAD)
		id = -1;

	QSystemSemaphore semaphore("UniqueIdManagerSystemSemaphore", 1, QSystemSemaphore::Open);
	semaphore.acquire();

	long long allocatedId = -1;

	// 嘗試連接到共享內存，如果不存在則創建
	if (g_sharedMemory.key().isEmpty())
	{
		g_sharedMemory.setKey("UniqueIdManagerSharedMemory");
		if (!g_sharedMemory.isAttached() && !g_sharedMemory.attach())
		{
			qDebug() << "sharedMemory not exist, create new one now.";
			g_sharedMemory.create(SASH_MAX_SHARED_MEM);
			reset();
		}
	}

	QSet<long long> allocatedIds;

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
			else
			{
				allocatedId = -1;
				break;
			}
		}

		// 分配唯一的ID
		for (long long i = 0; i < SASH_MAX_THREAD; ++i)
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

void UniqueIdManager::deallocateUniqueId(long long id)
{
	if (id < 0 || id >= SASH_MAX_THREAD)
		return;

	QSystemSemaphore semaphore("UniqueIdManagerSystemSemaphore", 1, QSystemSemaphore::Open);
	semaphore.acquire();

	// 嘗試連接到共享內存，如果不存在則創建
	if (g_sharedMemory.key().isEmpty())
	{
		g_sharedMemory.setKey("UniqueIdManagerSharedMemory");
		if (!g_sharedMemory.isAttached() && !g_sharedMemory.attach())
		{
			semaphore.release();
			return;
		}
	}

	QSet<long long> allocatedIds;

	do
	{
		bool bret = readSharedMemory(&allocatedIds);
		if (!bret)
			break;

		if (allocatedIds.contains(id))
		{
			allocatedIds.remove(id);
			updateSharedMemory(allocatedIds);
		}
	} while (false);

	semaphore.release();
}

void UniqueIdManager::clear()
{
	memset(g_sharedMemory.data(), 0, g_sharedMemory.size());
}

void UniqueIdManager::write(const char* from)
{
	clear();
	_snprintf_s(reinterpret_cast<char*>(g_sharedMemory.data()), g_sharedMemory.size(), _TRUNCATE, "%s", from);
}

bool UniqueIdManager::readSharedMemory(QSet<long long>* pAllocatedIds)
{
	do
	{
		if (!g_sharedMemory.lock())
		{
			qDebug() << "g_sharedMemory.lock() == false";
			break;
		}

		qDebug() << "g_sharedMemory.size():" << g_sharedMemory.size();

		QByteArray data = QByteArray::fromRawData(static_cast<const char*>(g_sharedMemory.constData()), g_sharedMemory.size());
		g_sharedMemory.unlock();

		if (data.isEmpty())
		{
			qDebug() << "data.isEmpty() == true";
			break;
		}
		else if (data.front() == '\0')
		{
			qDebug() << "data.front() == '\\0'";
			break;
		}

		long long indexEof = data.indexOf('\0');
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

void UniqueIdManager::updateSharedMemory(const QSet<long long>& allocatedIds)
{
	// 將分配的ID保存為JSON字符串，並寫入共享內存
	QJsonObject obj;
	QJsonArray idArray;
	for (long long id : allocatedIds)
	{
		idArray.append(id);
	}

	obj[jsonKey_] = idArray;

	QJsonDocument doc(obj);

	// 將 JSON 文檔轉換為 UTF-8 編碼的 QByteArray
	QByteArray data = doc.toJson(QJsonDocument::Compact);
	QString str = QString::fromUtf8(data).simplified();
	data = str.toUtf8();
	data.replace(" ", "");

	if (g_sharedMemory.lock())
	{
		const char* from = data.constData();
		write(from);
		g_sharedMemory.unlock();
	}
}

bool ThreadManager::createThread(long long index, MainObject** ppObj, QObject* parent)
{
	std::ignore = parent;
	QMutexLocker locker(&mutex_);
	if (threads_.contains(index))
		return false;

	if (objects_.contains(index))
		return false;

	do
	{
		MainObject* object = q_check_ptr(new MainObject(index, nullptr));
		if (nullptr == object)
			break;

		QThread* thread = q_check_ptr(new QThread(this));
		if (nullptr == thread)
			break;

		thread->setObjectName(QString("thread_%1").arg(index));

		object->moveToThread(thread);

		threads_.insert(index, thread);
		objects_.insert(index, object);

		connect(thread, &QThread::started, object, &MainObject::run, Qt::QueuedConnection);
		//after delete must set nullptr
		connect(object, &MainObject::finished, this, [this]()
			{
				MainObject* object = qobject_cast<MainObject*>(sender());
				if (nullptr == object)
					return;

				QThread* thread_ = object->thread();
				if (nullptr == thread_)
					return;

				long long index = object->getIndex();
				threads_.remove(index);
				objects_.remove(index);

				qDebug() << "recv MainObject::finished, start cleanning";
				thread_->quit();
				thread_->wait();
				thread_->deleteLater();
				thread_ = nullptr;
				object->deleteLater();
				object = nullptr;
				Injector::reset(index);

			}, Qt::QueuedConnection);
		thread->start();

		if (ppObj != nullptr)
			*ppObj = object;
		return true;
	} while (false);

	return false;
}

void ThreadManager::close(long long index)
{
	QMutexLocker locker(&mutex_);
	if (threads_.contains(index) && objects_.contains(index))
	{
		auto thread_ = threads_.take(index);
		auto object_ = objects_.take(index);
		object_->requestInterruption();
		thread_->quit();
		thread_->wait();
		thread_->deleteLater();
		thread_ = nullptr;
		object_->deleteLater();
		object_ = nullptr;
		Injector::reset(index);
	}
}

MainObject::MainObject(long long index, QObject* parent)
	: ThreadPlugin(index, parent)
	, autoThreads_(MissionThread::kMaxAutoMission, nullptr)
{

}

MainObject::~MainObject()
{
	qDebug() << "MainObject is destroyed!!";
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(getIndex());
	emit signalDispatcher.setStartButtonEnabled(true);
}

void MainObject::run()
{
	long long currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);

	Injector::process_information_t process_info;
	remove_thread_reason = util::REASON_NO_ERROR;
	QElapsedTimer timer; timer.start();
	do
	{
		//檢查服務端是否實例化
		//檢查服務端是否正在運行並且正在監聽
		if (!injector.server.isListening())
			break;

		emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusOpening);

		//創建遊戲進程
		Injector::CreateProcessResult createResult = injector.createProcess(process_info);
		if (createResult == Injector::CreateAboveWindow8Failed || createResult == Injector::CreateBelowWindow8Failed)
			break;

		if (remove_thread_reason != util::REASON_NO_ERROR)
			break;

		emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusOpened);

		if (createResult == Injector::CreateAboveWindow8Success)
		{
			//注入dll 並通知客戶端要連入的port
			if (!injector.injectLibrary(process_info, injector.server.serverPort(), &remove_thread_reason)
				|| (remove_thread_reason != util::REASON_NO_ERROR))
			{
				qDebug() << "injectLibrary failed. reason:" << remove_thread_reason;
				break;
			}
		}

		//等待客戶端連入
		for (;;)
		{
			//檢查TCP是否握手成功
			if (injector.IS_TCP_CONNECTION_OK_TO_USE.load(std::memory_order_acquire))
				break;

			if (isInterruptionRequested())
			{
				remove_thread_reason = util::REASON_REQUEST_STOP;
				break;
			}

			if (!injector.isWindowAlive())
			{
				remove_thread_reason = util::REASON_TARGET_WINDOW_DISAPPEAR;
				break;
			}

			if (timer.hasExpired(15000))
			{
				remove_thread_reason = util::REASON_TCP_CONNECTION_TIMEOUT;
				break;
			}
			QThread::msleep(100);
		}

		if (remove_thread_reason != util::REASON_NO_ERROR)
			break;

		//進入主循環
		mainProc();
	} while (false);

	//開始逐步停止所有功能
	requestInterruption();
	//強制關閉遊戲進程
	injector.close();
	if (SignalDispatcher::contains(getIndex()))
	{
		emit signalDispatcher.scriptStoped();
		emit signalDispatcher.nodifyAllStop();
		emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusNotOpen);
	}

	//關閉自動組隊線程
	for (auto& pthread : autoThreads_)
	{
		if (pthread != nullptr)
		{
			pthread->wait();
			delete pthread;
			pthread = nullptr;
		}
	}
	autoThreads_.clear();

	while (injector.IS_SCRIPT_FLAG.load(std::memory_order_acquire))
	{
		QThread::msleep(100);
	}

	//通知線程結束
	emit finished();
	qDebug() << "emit finished()";
}

void MainObject::mainProc()
{
	Injector& injector = Injector::getInstance(getIndex());
	QElapsedTimer freeMemTimer; freeMemTimer.start();

	mem::freeUnuseMemory(injector.getProcess());

	bool nodelay = false;
	for (;;)
	{
		if (!nodelay)
			QThread::msleep(100);
		else
			nodelay = false;

		//檢查是否接收到停止執行的訊號
		if (isInterruptionRequested())
		{
			remove_thread_reason = util::REASON_REQUEST_STOP;
			break;
		}

		//檢查遊戲是否消失
		if (!injector.isWindowAlive())
		{
			remove_thread_reason = util::REASON_TARGET_WINDOW_DISAPPEAR;
			qDebug() << "window is disappear!";
			break;
		}

		if (injector.worker.isNull())
		{
			QThread::yieldCurrentThread();
			continue;
		}

		//自動釋放記憶體
		if (injector.getEnableHash(util::kAutoFreeMemoryEnable) && freeMemTimer.hasExpired(5ll * 60ll * 1000ll))
		{
			freeMemTimer.restart();
			mem::freeUnuseMemory(injector.getProcess());
		}
		else
			freeMemTimer.restart();

		//有些數據需要和客戶端內存同步
		injector.worker->updateDatasFromMemory();

		injector.worker->setWindowTitle(injector.getStringHash(util::kTitleFormatString));

		//異步加速
		long long value = injector.getValueHash(util::kSpeedBoostValue);
		if (flagSetBoostValue_ != value)
		{
			flagSetBoostValue_ = value;
			injector.postMessage(kSetBoostSpeed, true, flagSetBoostValue_);
		}

		checkControl();

		bool bCheckedFastBattle = injector.getEnableHash(util::kFastBattleEnable);
		bool bCheckedAutoBattle = injector.getEnableHash(util::kAutoBattleEnable);
		long long W = injector.worker->getWorldStatus();
		// 允許 自動戰鬥
		if (bCheckedAutoBattle)
		{
			if (W == 10) // 位於戰鬥畫面
				injector.postMessage(kSetBlockPacket, false, NULL); // 禁止阻擋戰鬥封包
			else if (W == 9) // 位於非戰鬥畫面
				injector.postMessage(kSetBlockPacket, injector.worker->getBattleFlag(), NULL); // 禁止阻擋戰鬥封包
		}
		// 允許 快速戰鬥
		else if (bCheckedFastBattle)
		{
			if (W == 10)// 強退戰鬥畫面
				injector.worker->setGameStatus(7);
			injector.postMessage(kSetBlockPacket, true, NULL); // 允許阻擋戰鬥封包
		}
		else // 不允許 快速戰鬥 和 自動戰鬥
			injector.postMessage(kSetBlockPacket, false, NULL); // 禁止阻擋戰鬥封包

		//登出按下，異步登出
		if (injector.getEnableHash(util::kLogOutEnable))
		{
			injector.setEnableHash(util::kLogOutEnable, false);
			if (!injector.worker.isNull())
				injector.worker->logOut();
		}

		//EO按下，異步發送EO
		if (injector.getEnableHash(util::kEchoEnable))
		{
			injector.setEnableHash(util::kEchoEnable, false);
			if (!injector.worker.isNull())
				injector.worker->EO();
		}

		//回點按下，異步回點
		if (injector.getEnableHash(util::kLogBackEnable))
		{
			injector.setEnableHash(util::kLogBackEnable, false);
			if (!injector.worker.isNull())
				injector.worker->logBack();
		}

		//異步快速走路
		bool bChecked = injector.getEnableHash(util::kFastWalkEnable);
		if (flagFastWalkEnable_ != bChecked)
		{
			flagFastWalkEnable_ = bChecked;
			injector.postMessage(kEnableFastWalk, bChecked, NULL);
		}

		//異步橫衝直撞 (穿牆)
		bChecked = injector.getEnableHash(util::kPassWallEnable);
		if (flagPassWallEnable_ != bChecked)
		{
			flagPassWallEnable_ = bChecked;
			injector.postMessage(kEnablePassWall, bChecked, NULL);
		}

		//異步鎖定畫面
		bChecked = injector.getEnableHash(util::kLockImageEnable);
		if (flagLockImageEnable_ != bChecked)
		{
			flagLockImageEnable_ = bChecked;
			injector.postMessage(kEnableImageLock, bChecked, NULL);
		}

		//異步資源優化
		bChecked = injector.getEnableHash(util::kOptimizeEnable);
		if (flagOptimizeEnable_ != bChecked)
		{
			flagOptimizeEnable_ = bChecked;
			injector.postMessage(kEnableOptimize, bChecked, NULL);
		}

		//異步關閉特效
		bChecked = injector.getEnableHash(util::kCloseEffectEnable);
		if (flagCloseEffectEnable_ != bChecked)
		{
			flagCloseEffectEnable_ = bChecked;
			injector.postMessage(kEnableEffect, !bChecked, NULL);
		}

		//異步鎖定時間
		bChecked = injector.getEnableHash(util::kLockTimeEnable);
		value = injector.getValueHash(util::kLockTimeValue);
		if (flagLockTimeEnable_ != bChecked || flagLockTimeValue_ != value)
		{
			flagLockTimeEnable_ = bChecked;
			flagLockTimeValue_ = value;
			injector.postMessage(kSetTimeLock, bChecked, flagLockTimeValue_);
		}

		//隱藏人物按下，異步隱藏
		bChecked = injector.getEnableHash(util::kHideCharacterEnable);
		if (flagHideCharacterEnable_ != bChecked)
		{
			flagHideCharacterEnable_ = bChecked;
			injector.postMessage(kEnableCharShow, !bChecked, NULL);
		}

		//異步戰鬥99秒
		bChecked = injector.getEnableHash(util::kBattleTimeExtendEnable);
		if (flagBattleTimeExtendEnable_ != bChecked)
		{
			flagBattleTimeExtendEnable_ = bChecked;
			injector.postMessage(kBattleTimeExtend, bChecked, NULL);
		}

		if (bCheckedFastBattle || bCheckedAutoBattle)
			injector.postMessage(kEnableBattleDialog, false, NULL);//禁止戰鬥面板出現
		else
			injector.postMessage(kEnableBattleDialog, true, NULL);//允許戰鬥面板出現

		//其他所有功能
		long long status = checkAndRunFunctions();

		//這裡是預留的暫時沒作用
		if (status == 1)//非登入狀態
		{
			if (!isFirstLogin_)
				QThread::msleep(1000);
			else
				QThread::msleep(50);
			nodelay = true;
		}
		else if (status == 2)//平時
		{
			//檢查開關 (隊伍、交易、名片...等等)
			checkEtcFlag();
		}
		else if (status == 3)//戰鬥中
		{
			injector.worker->doBattleWork(true);
		}
		else//錯誤
		{
			break;
		}

		QThread::yieldCurrentThread();
	}
}

void MainObject::inGameInitialize()
{
	Injector& injector = Injector::getInstance(getIndex());
	if (injector.worker.isNull())
		return;

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(getIndex());

	//等待完全進入登入後的遊戲畫面
	QElapsedTimer timer; timer.start();
	for (;;)
	{
		if (isInterruptionRequested())
			return;

		if (injector.worker.isNull())
			return;

		if (timer.hasExpired(10000))
			return;

		if (injector.worker->checkWG(9, 3))
			break;

		QThread::msleep(100);
	}

	if (!injector.worker->getBattleFlag())
		emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusInNormal);

	QDateTime current = QDateTime::currentDateTime();
	QDateTime due = current.addYears(99);
	const QString dueStr(due.toString("yyyy-MM-dd hh:mm:ss"));
	const QString url("https://www.lovesa.cc");
	QString currentVerStr;
	QString newestVerStr;

	if (!Downloader::checkUpdate(&currentVerStr, &newestVerStr, nullptr))
	{
		newestVerStr = "nil";
	}

	//登入後的廣告公告
	constexpr bool isbeta = true;
	const QString version = QString("%1.%2.%3")
		.arg(SASH_VERSION_MAJOR) \
		.arg(SASH_VERSION_MINOR) \
		.arg(newestVerStr);
	injector.worker->announce(tr("Welcome to use SaSH，For more information please visit %1").arg(url));
	injector.worker->announce(tr("You are using %1 account, due date is:%2").arg(isbeta ? tr("trial") : tr("subscribed")).arg(dueStr));
	injector.worker->announce(tr("StoneAge SaSH forum url:%1, newest version is %2").arg(url).arg(version));
	injector.sendMessage(kDistoryDialog, NULL, NULL);
}

long long MainObject::checkAndRunFunctions()
{
	long long currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	if (injector.worker.isNull())
		return 0;

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);

	long long status = injector.worker->getUnloginStatus();

	if (status == util::kStatusDisappear)
	{
		return 0;
	}
	else
	{
		switch (status)
		{
		default:
		{
			if (injector.worker->getOnlineFlag())
				break;

			Q_FALLTHROUGH();
		}
		case util::kStatusInputUser:
		case util::kStatusSelectServer:
		case util::kStatusSelectSubServer:
		case util::kStatusSelectCharacter:
		case util::kStatusTimeout:
		case util::kStatusBusy:
		case util::kStatusConnecting:
		case util::kNoUserNameOrPassword:
		{
			//每次登出後只會執行一次
			if (!login_run_once_flag_)
			{
				login_run_once_flag_ = true;

				injector.worker->clear();
				injector.chatLogModel.clear();
			}

			injector.worker->loginTimer.restart();
			//自動登入 或 斷線重連
			if (injector.getEnableHash(util::kAutoLoginEnable) || injector.worker->IS_DISCONNECTED.load(std::memory_order_acquire))
				injector.worker->login(status);
			return 1;
		}
		case util::kStatusDisconnect:
		{
			if (injector.getEnableHash(util::kAutoReconnectEnable))
				injector.worker->login(status);
			return 1;
		}
		}
	}

	//每次登入後只會執行一次
	if (login_run_once_flag_)
	{
		login_run_once_flag_ = false;
		inGameInitialize();

		//首次登入標誌
		if (isFirstLogin_)
			isFirstLogin_ = false;
	}

	emit signalDispatcher.updateLoginTimeLabelTextChanged(util::formatMilliseconds(injector.worker->loginTimer.elapsed(), true));

	//更新掛機數據到UI
	updateAfkInfos();

	//批次開啟子任務線程
	for (long long i = 0; i < MissionThread::kMaxAutoMission; ++i)
	{
		MissionThread* p = autoThreads_.value(i);
		if (p == nullptr)
		{
			p = new MissionThread(currentIndex, i);
			if (p == nullptr)
				continue;

			autoThreads_[i] = p;
			continue;
		}

		if (p != nullptr)
			p->start();
	}

	//平時
	if (!injector.worker->getBattleFlag())
	{
		//每次進入平時只會執行一次
		if (!battle_run_once_flag_)
		{
			battle_run_once_flag_ = true;
		}

		return 2;
	}
	else //戰鬥中
	{
		//每次進入戰鬥只會執行一次
		if (battle_run_once_flag_)
		{
			battle_run_once_flag_ = false;
		}
		return 3;
	}
}

void MainObject::updateAfkInfos()
{
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(getIndex());
	Injector& injector = Injector::getInstance(getIndex());
	if (injector.worker.isNull())
		return;

	long long duration = injector.worker->loginTimer.elapsed();
	emit signalDispatcher.updateAfkInfoTable(0, util::formatMilliseconds(duration));

	util::AfkRecorder recorder = injector.worker->recorder[0];

	long long avgLevelPerHour = 0;
	if (duration > 0 && recorder.leveldifference > 0)
		avgLevelPerHour = recorder.leveldifference * 3600000.0 / duration;
	emit signalDispatcher.updateAfkInfoTable(2, QString(tr("%1→%2 (avg level: %3)"))
		.arg(recorder.levelrecord).arg(recorder.levelrecord + recorder.leveldifference).arg(avgLevelPerHour));

	long long avgExpPerHour = 0;
	if (duration > 0 && recorder.expdifference > 0)
		avgExpPerHour = recorder.expdifference * 3600000.0 / duration;

	emit signalDispatcher.updateAfkInfoTable(3, tr("%1 (avg exp: %2)").arg(recorder.expdifference).arg(avgExpPerHour));

	emit signalDispatcher.updateAfkInfoTable(4, util::toQString(recorder.deadthcount));

	long long avgGoldPerHour = 0;
	if (duration > 0 && recorder.goldearn > 0)
		avgGoldPerHour = recorder.goldearn * 3600000.0 / duration;
	emit signalDispatcher.updateAfkInfoTable(5, tr("%1 (avg gold: %2)").arg(recorder.goldearn).arg(avgGoldPerHour));

	constexpr long long n = 7;
	long long j = 0;
	for (long long i = 0; i < MAX_PET; ++i)
	{
		recorder = injector.worker->recorder[i + 1];

		avgExpPerHour = 0;
		if (duration > 0 && recorder.leveldifference > 0)
			avgExpPerHour = recorder.leveldifference * 3600000.0 / duration;

		emit signalDispatcher.updateAfkInfoTable(i + n + j, QString(tr("%1→%2 (avg level: %3)"))
			.arg(recorder.levelrecord).arg(recorder.levelrecord + recorder.leveldifference).arg(avgExpPerHour));

		avgExpPerHour = 0;
		if (duration > 0 && recorder.expdifference > 0)
			avgExpPerHour = recorder.expdifference * 3600000.0 / duration;
		emit signalDispatcher.updateAfkInfoTable(i + n + 1 + j, tr("%1 (avg exp: %2)").arg(recorder.expdifference).arg(avgExpPerHour));

		emit signalDispatcher.updateAfkInfoTable(i + n + 2 + j, util::toQString(recorder.deadthcount));

		emit signalDispatcher.updateAfkInfoTable(i + n + 3 + j, "");

		j += 3;
	}
}

//根據UI的選擇項變動做的一些操作
void MainObject::checkControl()
{
	Injector& injector = Injector::getInstance(getIndex());
	if (injector.worker.isNull())
		return;

	if (!injector.worker->getOnlineFlag())
		return;

	if (injector.worker->loginTimer.elapsed() < 10000)
		return;

	//異步關閉聲音
	bool bChecked = injector.getEnableHash(util::kMuteEnable);
	if (flagMuteEnable_ != bChecked)
	{
		flagMuteEnable_ = bChecked;
		injector.sendMessage(kEnableSound, !bChecked, NULL);
	}
}

//檢查開關 (隊伍、交易、名片...等等)
void MainObject::checkEtcFlag()
{
	Injector& injector = Injector::getInstance(getIndex());
	if (injector.worker.isNull())
		return;

	long long flg = injector.worker->getPC().etcFlag;
	bool hasChange = false;
	auto toBool = [flg](long long f)->bool
		{
			return ((flg & f) != 0);
		};

	bool bCurrent = injector.getEnableHash(util::kSwitcherTeamEnable);
	if (toBool(PC_ETCFLAG_GROUP) != bCurrent)
	{
		if (bCurrent)
			flg |= PC_ETCFLAG_GROUP;
		else
			flg &= ~PC_ETCFLAG_GROUP;

		hasChange = true;
	}

	bCurrent = injector.getEnableHash(util::kSwitcherPKEnable);
	if (toBool(PC_ETCFLAG_PK) != bCurrent)
	{
		if (bCurrent)
			flg |= PC_ETCFLAG_PK;
		else
			flg &= ~PC_ETCFLAG_PK;

		hasChange = true;
	}

	bCurrent = injector.getEnableHash(util::kSwitcherCardEnable);
	if (toBool(PC_ETCFLAG_CARD) != bCurrent)
	{
		if (bCurrent)
			flg |= PC_ETCFLAG_CARD;
		else
			flg &= ~PC_ETCFLAG_CARD;

		hasChange = true;
	}

	bCurrent = injector.getEnableHash(util::kSwitcherTradeEnable);
	if (toBool(PC_ETCFLAG_TRADE) != bCurrent)
	{
		if (bCurrent)
			flg |= PC_ETCFLAG_TRADE;
		else
			flg &= ~PC_ETCFLAG_TRADE;

		hasChange = true;
	}

	bCurrent = injector.getEnableHash(util::kSwitcherGroupEnable);
	if (toBool(PC_ETCFLAG_PARTY_CHAT) != bCurrent)
	{
		if (bCurrent)
			flg |= PC_ETCFLAG_PARTY_CHAT;
		else
			flg &= ~PC_ETCFLAG_PARTY_CHAT;

		hasChange = true;
	}

	bCurrent = injector.getEnableHash(util::kSwitcherFamilyEnable);
	if (toBool(PC_ETCFLAG_FM) != bCurrent)
	{
		if (bCurrent)
			flg |= PC_ETCFLAG_FM;
		else
			flg &= ~PC_ETCFLAG_FM;

		hasChange = true;
	}

	bCurrent = injector.getEnableHash(util::kSwitcherJobEnable);
	if (toBool(PC_ETCFLAG_JOB) != bCurrent)
	{
		if (bCurrent)
			flg |= PC_ETCFLAG_JOB;
		else
			flg &= ~PC_ETCFLAG_JOB;

		hasChange = true;
	}

	bCurrent = injector.getEnableHash(util::kSwitcherWorldEnable);
	if (toBool(PC_ETCFLAG_WORLD) != bCurrent)
	{
		if (bCurrent)
			flg |= PC_ETCFLAG_WORLD;
		else
			flg &= ~PC_ETCFLAG_WORLD;

		hasChange = true;
	}

	if (hasChange)
		injector.worker->setSwitcher(flg);
}

MissionThread::MissionThread(long long index, long long type, QObject* parent)
	: ThreadPlugin(index, parent)
{
	switch (type)
	{
	case kAutoJoin:
		connect(this, &MissionThread::start, this, &MissionThread::autoJoin, Qt::QueuedConnection);
		break;
	case kAutoWalk:
		connect(this, &MissionThread::start, this, &MissionThread::autoWalk, Qt::QueuedConnection);
		break;
	case kAutoSortItem:
		connect(this, &MissionThread::start, this, &MissionThread::autoSortItem, Qt::QueuedConnection);
		break;
	case kAutoRecordNPC:
		connect(this, &MissionThread::start, this, &MissionThread::autoRecordNPC, Qt::QueuedConnection);
		break;
	case kAsyncFindPath:
		connect(this, &MissionThread::start, this, &MissionThread::asyncFindPath, Qt::QueuedConnection);
		break;
	}

	connect(&thread_, &QThread::finished, this, &MissionThread::reset, Qt::QueuedConnection);
	connect(&thread_, &QThread::finished, &thread_, &QThread::quit, Qt::QueuedConnection);
	connect(&thread_, &QThread::finished, this, &MissionThread::onFinished, Qt::QueuedConnection);

	moveToThread(&thread_);
	thread_.start();
}

MissionThread::~MissionThread()
{
	qDebug() << "MissionThread::~MissionThread()";
}

void MissionThread::onFinished()
{
	qDebug() << "finished!";
}

void MissionThread::wait()
{
	if (thread_.isRunning())
	{
		requestInterruption();
		thread_.quit();
		thread_.wait();
	}
}

void MissionThread::autoJoin()
{
	long long index = getIndex();
	QSet<QPoint> blockList;
	Injector& injector = Injector::getInstance(index);

	constexpr long long MAX_SINGLE_STEP = 3;
	map_t map;
	std::vector<QPoint> path;
	QPoint current_point;
	QPoint newpoint;
	mapunit_t unit = {};
	long long dir = -1;
	long long floor = injector.worker->getFloor();
	long long len = MAX_SINGLE_STEP;
	long long size = 0;
	CAStar astar;
	PC ch = {};
	long long actionType = 0;
	QString leader;

	for (;;)
	{
		//如果主線程關閉則自動退出
		if (isInterruptionRequested())
			return;

		if (injector.worker.isNull())
			return;

		if (injector.getEnableHash(util::kAutoWalkEnable) || injector.getEnableHash(util::kFastAutoWalkEnable))
			return;

		if (!injector.getEnableHash(util::kAutoJoinEnable))
			return;

		leader = injector.getStringHash(util::kAutoFunNameString);
		if (leader.isEmpty())
			return;

		if (!injector.worker->mapAnalyzer.getMapDataByFloor(floor, &map))
		{
			injector.worker->mapAnalyzer.readFromBinary(index, floor, injector.worker->getFloorName(), false);
		}

		ch = injector.worker->getPC();
		actionType = injector.getValueHash(util::kAutoFunTypeValue);
		if (actionType == 0)
		{
			//檢查隊長是否正確
			if (ch.status & CHR_STATUS_LEADER)
				return;

			if (ch.status & CHR_STATUS_PARTY)
			{
				QString name = injector.worker->getParty(0).name;
				if ((!name.isEmpty() && leader == name)
					|| (!name.isEmpty() && leader.count("|") > 0 && leader.contains(name)))//隊長正確
					return;

				injector.worker->setTeamState(false);
				QThread::msleep(200);
			}
		}

		leader = injector.getStringHash(util::kAutoFunNameString);
		if (leader.isEmpty())
			break;

		if (isInterruptionRequested())
			return;

		//如果人物不在線上則自動退出
		if (!injector.worker->getOnlineFlag())
		{
			QThread::msleep(500);
			continue;
		}

		if (injector.worker->getBattleFlag())
		{
			QThread::msleep(500);
			continue;
		}

		ch = injector.worker->getPC();

		if (floor != injector.worker->getFloor())
			break;

		QString freeName = "";
		if (leader.count("|") == 1)
		{
			QStringList list = leader.split(util::rexOR);
			if (list.size() == 2)
			{
				leader = list.value(0);
				freeName = list.value(1);
			}
		}

		//查找目標人物所在坐標
		if (!injector.worker->findUnit(leader, util::OBJ_HUMAN, &unit, freeName))
			break;

		//如果和目標人物處於同一個坐標則向隨機方向移動一格
		current_point = injector.worker->getPoint();
		if (current_point == unit.p)
		{
			injector.worker->move(current_point + util::fix_point.value(QRandomGenerator::global()->bounded(0, 7)));
			QThread::msleep(200);
			continue;
		}

		//計算最短離靠近目標人物的坐標和面相的方向
		dir = injector.worker->mapAnalyzer.calcBestFollowPointByDstPoint(index, astar, floor, current_point, unit.p, &newpoint, false, -1);
		if (-1 == dir)
			return;

		if (current_point == newpoint)
		{
			actionType = injector.getValueHash(util::kAutoFunTypeValue);
			if (actionType == 0)
			{
				injector.worker->setCharFaceDirection(dir, true);
				injector.worker->setTeamState(true);
				continue;
			}
		}

		if (!injector.worker->mapAnalyzer.calcNewRoute(index, astar, floor, current_point, newpoint, blockList, &path))
			return;

		len = MAX_SINGLE_STEP;
		size = static_cast<long long>(path.size()) - 1;

		//步長 如果path大小 小於步長 就遞減步長
		for (;;)
		{
			if (!(size < len))
				break;
			--len;
		}

		//如果步長小於1 就不動
		if (len < 0)
			break;

		if (len >= static_cast<long long>(path.size()))
			break;

		injector.worker->move(path.at(len));
	}
}

void MissionThread::autoWalk()
{
	Injector& injector = Injector::getInstance(getIndex());
	QPoint current_pos;
	bool current_side = false;
	QPoint posCache = current_pos;

	for (;;)
	{
		//如果主線程關閉則自動退出
		if (isInterruptionRequested())
			return;

		//取設置
		bool enableAutoWalk = injector.getEnableHash(util::kAutoWalkEnable);//走路遇敵開關
		bool enableFastAutoWalk = injector.getEnableHash(util::kFastAutoWalkEnable);//快速遇敵開關
		if (!enableAutoWalk && !enableFastAutoWalk)
		{
			current_pos = QPoint();
			return;
		}
		else if (current_pos.isNull())
		{
			current_pos = injector.worker->getPoint();
		}

		//如果人物不在線上則自動退出
		if (!injector.worker->getOnlineFlag())
		{
			QThread::msleep(500);
			continue;
		}

		//如果人物在戰鬥中則進入循環等待
		if (injector.worker->getBattleFlag())
		{
			//先等一小段時間
			QThread::msleep(500);

			//如果已經退出戰鬥就等待1.5秒避免太快開始移動不夠時間吃肉補血丟東西...等
			if (!injector.worker->getBattleFlag())
			{
				for (long long i = 0; i < 5; ++i)
				{
					//如果主線程關閉則自動退出
					if (isInterruptionRequested())
						return;
					QThread::msleep(100);
				}
			}
			else
				continue;
		}

		long long walk_speed = injector.getValueHash(util::kAutoWalkDelayValue);//走路速度

		//走路遇敵
		if (enableAutoWalk)
		{
			long long walk_len = injector.getValueHash(util::kAutoWalkDistanceValue);//走路距離
			long long walk_dir = injector.getValueHash(util::kAutoWalkDirectionValue);//走路方向

			//如果direction是0，則current_pos +- x，否則 +- y 如果是2 則隨機加減
			//一次性移動walk_len格

			long long x = 0, y = 0;
			QString dirStr;
			QString steps;
			for (long long i = 0; i < 4; ++i)//4個字母為一組
			{
				if (walk_dir == 0)
				{
					if (current_side)//東
					{
						x = current_pos.x() + walk_len + 1;
						dirStr = "b";
					}
					else//西
					{
						x = current_pos.x() - walk_len - 1;
						dirStr = "f";
					}

					y = current_pos.y();
				}
				else if (walk_dir == 1)
				{
					x = current_pos.x();

					if (current_side)//南
					{
						y = current_pos.y() + walk_len + 1;
						dirStr = "e";
					}
					else//北
					{
						y = current_pos.y() - walk_len - 1;
						dirStr = "a";
					}
				}
				else
				{
					//取隨機數
					std::random_device rd;
					std::mt19937_64 gen(rd());
					std::uniform_int_distribution<long long> distributionX(current_pos.x() - walk_len, current_pos.x() + walk_len);
					std::uniform_int_distribution<long long> distributionY(current_pos.y() - walk_len, current_pos.y() + walk_len);
					x = distributionX(gen);
					y = distributionY(gen);
				}

				//每次循環切換方向
				if (current_side)
					current_side = false;
				else
					current_side = true;

				for (long long j = 0; j < walk_len; ++j)
					steps += dirStr;
			}

			if (walk_len <= 6 && walk_dir != 2)
				injector.worker->move(current_pos, steps);
			else
				injector.worker->move(QPoint(x, y));
			steps.clear();
		}
		else if (enableFastAutoWalk) //快速遇敵 (封包)
		{
			injector.worker->move(QPoint(0, 0), "gcgc");
		}
		QThread::msleep(walk_speed + 1);//避免太快無論如何都+15ms (太快並不會遇比較快)
	}
}

void MissionThread::autoSortItem()
{
	long long i = 0;
	constexpr long long duration = 30;
	Injector& injector = Injector::getInstance(getIndex());

	for (;;)
	{
		for (i = 0; i < duration; ++i)
		{
			if (isInterruptionRequested())
				return;

			QThread::msleep(500);
		}

		if (!injector.getEnableHash(util::kAutoStackEnable))
			return;

		if (injector.worker.isNull())
			return;

		injector.worker->sortItem();
	}
}

void MissionThread::autoRecordNPC()
{
	long long currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	for (;;)
	{
		for (long long i = 0; i < 5; ++i)
		{
			QThread::msleep(1000);

			if (isInterruptionRequested())
				return;
		}

		if (injector.worker.isNull())
			continue;

		if (isInterruptionRequested())
			return;

		if (!injector.worker->getOnlineFlag())
			continue;

		if (injector.worker->getBattleFlag())
			continue;

		CAStar astar;

		QHash<long long, mapunit_t> units = injector.worker->mapUnitHash.toHash();
		util::Config config(injector.getPointFileName(), QString("%1|%2").arg(__FUNCTION__).arg(__LINE__));

		for (const mapunit_t& unit : units)
		{
			if (isInterruptionRequested())
				return;

			if (injector.worker.isNull())
				break;

			if (!injector.worker->getOnlineFlag())
				break;

			if (injector.worker->getBattleFlag())
				break;

			if ((unit.objType != util::OBJ_NPC)
				|| unit.name.isEmpty()
				|| (injector.worker->getWorldStatus() != 9)
				|| (injector.worker->getGameStatus() != 3)
				|| injector.worker->npcUnitPointHash.contains(QPoint(unit.x, unit.y)))
			{
				continue;
			}

			injector.worker->npcUnitPointHash.insert(QPoint(unit.x, unit.y), unit);

			util::MapData d;
			long long nowFloor = injector.worker->nowFloor_.load(std::memory_order_acquire);
			QPoint nowPoint = injector.worker->nowPoint_.get();

			d.floor = nowFloor;
			d.name = unit.name;

			//npc前方一格
			QPoint newPoint = util::fix_point.value(unit.dir) + unit.p;
			//檢查是否可走
			if (injector.worker->mapAnalyzer.isPassable(currentIndex, astar, nowFloor, nowPoint, newPoint))
			{
				d.x = newPoint.x();
				d.y = newPoint.y();
			}
			else
			{
				//再往前一格
				QPoint additionPoint = util::fix_point.value(unit.dir) + newPoint;
				//檢查是否可走
				if (injector.worker->mapAnalyzer.isPassable(currentIndex, astar, nowFloor, nowPoint, additionPoint))
				{
					d.x = additionPoint.x();
					d.y = additionPoint.y();
				}
				else
				{
					//檢查NPC周圍8格
					bool flag = false;
					for (long long i = 0; i < 8; ++i)
					{
						newPoint = util::fix_point.value(i) + unit.p;
						if (injector.worker->mapAnalyzer.isPassable(currentIndex, astar, nowFloor, nowPoint, newPoint))
						{
							d.x = newPoint.x();
							d.y = newPoint.y();
							flag = true;
							break;
						}
					}

					if (!flag)
					{
						continue;
					}
				}
			}
			config.writeMapData(unit.name, d);
		}

		static bool constDataInit = false;

		if (constDataInit)
			continue;

		constDataInit = true;

		QString content;
		QStringList paths;
		util::searchFiles(util::applicationDirPath(), "point", ".txt", &paths, false);
		if (paths.isEmpty())
			continue;

		//這裡是讀取預製傳點坐標
		if (!util::readFile(paths.front(), &content))
		{
			qDebug() << "Failed to open point.dat";
			continue;
		}

		QStringList entrances = content.simplified().split(" ");

		for (const QString& entrance : entrances)
		{
			const QStringList entranceData(entrance.split(util::rexOR));
			if (entranceData.size() != 3)
				continue;

			bool ok = false;
			const long long floor = entranceData.value(0).toLongLong(&ok);
			if (!ok)
				continue;

			const QString pointStr(entranceData.value(1));
			const QStringList pointData(pointStr.split(util::rexComma));
			if (pointData.size() != 2)
				continue;

			long long x = pointData.value(0).toLongLong(&ok);
			if (!ok)
				continue;

			long long y = pointData.value(1).toLongLong(&ok);
			if (!ok)
				continue;

			const QPoint pos(x, y);

			const QString name(entranceData.value(2));

			util::MapData d;
			d.floor = floor;
			d.name = name;
			d.x = x;
			d.y = y;

			mapunit_t unit;
			unit.x = x;
			unit.y = y;
			unit.p = pos;
			unit.name = name;

			injector.worker->npcUnitPointHash.insert(pos, unit);

			config.writeMapData(name, d);
		}
	}
}

void MissionThread::asyncFindPath()
{
	Injector& injector = Injector::getInstance(getIndex());
	if (injector.worker.isNull())
		return;

	QPoint dst = args_.value(0).toPoint();
	injector.worker->findPathAsync(dst);
}

#if 0
//自動鎖寵排程
void MainObject::checkAutoLockSchedule()
{
	Injector& injector = Injector::getInstance(getIndex());
	if (injector.worker.isNull())
		return;

	auto checkSchedule = [this](util::UserSetting set)->bool
		{
			static const QHash <QString, PetState> hashType = {
				{ "戰", kBattle },
				{ "騎", kRide },

				{ "战", kBattle },
				{ "骑", kRide },

				{ "B", kBattle },
				{ "R", kRide },
			};

			Injector& injector = Injector::getInstance(getIndex());
			QString lockPetSchedule = injector.getStringHash(set);
			long long rindex = -1;
			long long bindex = -1;
			do
			{
				if (lockPetSchedule.isEmpty())
					break;

				const QStringList scheduleList = lockPetSchedule.split(util::rexOR, Qt::SkipEmptyParts);
				if (scheduleList.isEmpty())
					break;

				for (const QString& schedule : scheduleList)
				{
					if (schedule.isEmpty())
						continue;

					const QStringList schedules = schedule.split(util::rexSemicolon, Qt::SkipEmptyParts);
					if (schedules.isEmpty())
						continue;

					for (const QString& it : schedules)
					{
						const QStringList args = it.split(util::rexComma, Qt::SkipEmptyParts);
						if (args.isEmpty() || args.size() != 3)
							continue;

						QString name = args.value(0).simplified();
						if (name.isEmpty())
							continue;

						QString nameStr = name.left(1);
						long long petIndex = -1;
						bool ok = false;
						petIndex = nameStr.toLongLong(&ok);
						if (!ok)
							continue;
						--petIndex;

						if (petIndex < 0 || petIndex >= MAX_PET)
							continue;

						QString levelStr = args.value(1).simplified();
						if (levelStr.isEmpty())
							continue;

						ok = false;
						long long level = levelStr.toLongLong(&ok);
						if (!ok)
							continue;

						if (level < 0 || level > 255)
							continue;

						QString typeStr = args.value(2).simplified();
						if (typeStr.isEmpty())
							continue;

						PetState type = hashType.value(typeStr, kRest);

						PET pet = injector.worker->getPet(petIndex);

						if (pet.level >= level)
							continue;

						if (type == kBattle)
						{
							if (bindex != -1)
								continue;

							if (rindex != -1 && rindex == petIndex)
								continue;

							bindex = petIndex;
						}
						else if (type == kRide)
						{
							if (rindex != -1)
								continue;

							if (bindex != -1 && bindex == petIndex)
							{
								bindex = -1;
							}

							rindex = petIndex;
						}

						if (rindex != -1 && bindex != -1)
							break;
					}

					if (rindex != -1 || bindex != -1)
						break;
				}
			} while (false);

			if (rindex != -1)
			{
				PET pet = injector.worker->getPet(rindex);
				if (pet.hp <= 1)
				{
					injector.worker->setPetState(rindex, kRest);
					QThread::msleep(100);
				}

				if (pet.state != kRide)
					injector.worker->setPetState(rindex, kRide);
			}

			if (bindex != -1)
			{
				PET pet = injector.worker->getPet(bindex);
				if (pet.hp <= 1)
				{
					injector.worker->setPetState(bindex, kRest);
					QThread::msleep(100);
				}

				if (pet.state != kBattle)
					injector.worker->setPetState(bindex, kBattle);
			}

			for (long long i = 0; i < MAX_PET; ++i)
			{
				if (bindex == i || rindex == i)
					continue;

				PET pet = injector.worker->getPet(i);
				if ((pet.state != kRest && pet.state != kStandby) && set == util::kLockPetScheduleString)
					injector.worker->setPetState(i, kRest);
			}
			return false;
		};

	if (injector.getEnableHash(util::kLockPetScheduleEnable) && !injector.getEnableHash(util::kLockPetEnable) && !injector.getEnableHash(util::kLockRideEnable))
		checkSchedule(util::kLockPetScheduleString);

}
#endif
