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
#include <gamedevice.h>
#include "map/mapdevice.h"
#include "update/downloader.h"

QSharedMemory g_sharedMemory;

static void DNSInitialize()
{
	using DnsFlushResolverCacheFuncPtr = BOOL(WINAPI*)();
	// Flush the DNS cache
	HMODULE dnsapi = LoadLibrary(L"dnsapi.dll");
	if (dnsapi)
	{
		do
		{
			DnsFlushResolverCacheFuncPtr DnsFlushResolverCache = (DnsFlushResolverCacheFuncPtr)GetProcAddress(dnsapi, "DnsFlushResolverCache");
			if (!DnsFlushResolverCache)
			{
				break;
			}
			DnsFlushResolverCache();
		} while (false);
		FreeLibrary(dnsapi);
	}
}

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

	if (allocatedId == 0)
		DNSInitialize();

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
	if (objects_.contains(index))
		return false;

	do
	{
		MainObject* object = q_check_ptr(new MainObject(index, nullptr));
		sash_assume(object != nullptr);
		if (nullptr == object)
			break;

		objects_.insert(index, object);

		//after delete must set nullptr
		connect(object, &MainObject::finished, this, [this]()
			{
				MainObject* object = qobject_cast<MainObject*>(sender());
				if (nullptr == object)
					return;

				long long index = object->getIndex();
				objects_.remove(index);

				qDebug() << "recv MainObject::finished, start cleanning";
				object->thread.quit();
				object->thread.wait();
				object->deleteLater();
				object = nullptr;
				GameDevice::reset(index);

			}, Qt::QueuedConnection);

		object->thread.start();

		if (ppObj != nullptr)
			*ppObj = object;
		return true;
	} while (false);

	return false;
}

void ThreadManager::close(long long index)
{
	QMutexLocker locker(&mutex_);
	if (objects_.contains(index))
	{
		MainObject* object = objects_.take(index);
		GameDevice& gamedevice = GameDevice::getInstance(index);
		gamedevice.gameRequestInterruption();
		object->thread.quit();
		object->thread.wait();
		object->deleteLater();
		object = nullptr;
		GameDevice::reset(index);
	}
}

MainObject::MainObject(long long index, QObject* parent)
	: Indexer(index)
	, autoThreads_(MissionThread::kMaxAutoMission, nullptr)
{
	std::ignore = parent;
	moveToThread(&thread);
	connect(this, &MainObject::finished, &thread, &QThread::quit);
	connect(&thread, &QThread::started, this, &MainObject::run);
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
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);

	GameDevice::process_information_t process_info;
	remove_thread_reason = util::REASON_NO_ERROR;
	util::timer timer;
	do
	{
		//檢查服務端是否實例化
		//檢查服務端是否正在運行並且正在監聽
		if (!gamedevice.server.isListening())
			break;

		emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusOpening);

		//創建遊戲進程
		GameDevice::CreateProcessResult createResult = gamedevice.createProcess(process_info);
		if (createResult == GameDevice::CreateAboveWindow8Failed || createResult == GameDevice::CreateBelowWindow8Failed)
			break;

		if (remove_thread_reason != util::REASON_NO_ERROR)
			break;

		emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusOpened);

		if (createResult == GameDevice::CreateAboveWindow8Success
			|| createResult == GameDevice::CreateWithExistingProcess
			|| createResult == GameDevice::CreateWithExistingProcessNoDll)
		{
			//注入dll 並通知客戶端要連入的port
			if (!gamedevice.injectLibrary(process_info, gamedevice.server.serverPort(), &remove_thread_reason, createResult == GameDevice::CreateWithExistingProcess)
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
			if (gamedevice.IS_TCP_CONNECTION_OK_TO_USE.get())
				break;

			if (gamedevice.isGameInterruptionRequested())
			{
				remove_thread_reason = util::REASON_REQUEST_STOP;
				break;
			}

			if (!gamedevice.isWindowAlive())
			{
				remove_thread_reason = util::REASON_TARGET_WINDOW_DISAPPEAR;
				break;
			}

			if (timer.hasExpired(sa::MAX_TIMEOUT))
			{
				remove_thread_reason = util::REASON_TCP_CONNECTION_TIMEOUT;
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(100));;
		}

		if (remove_thread_reason != util::REASON_NO_ERROR)
			break;

		if (gamedevice.IS_SCRIPT_FLAG.get())
			emit signalDispatcher.scriptResumed();

		//進入主循環
		mainProc();
	} while (false);

	//開始逐步停止所有功能
	gamedevice.gameRequestInterruption();

	if (SignalDispatcher::contains(getIndex()))
	{
		emit signalDispatcher.scriptPaused();
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

	//通知線程結束
	emit finished();
	qDebug() << "emit finished()";
}

void MainObject::mainProc()
{
	GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	util::timer freeMemTimer;

	mem::freeUnuseMemory();
	mem::freeUnuseMemory(gamedevice.getProcess());

	bool nodelay = false;
	bool bCheckedFastBattle = false;
	bool bCheckedAutoBattle = false;
	bool bChecked = false;
	long long value = 0;
	long long status = 0;
	long long W = 0;

	for (;;)
	{
		if (!nodelay)
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		else
			nodelay = false;

		//檢查是否接收到停止執行的訊號
		if (gamedevice.isGameInterruptionRequested())
		{
			remove_thread_reason = util::REASON_REQUEST_STOP;
			break;
		}

		//檢查遊戲是否消失
		if (!gamedevice.isWindowAlive())
		{
			remove_thread_reason = util::REASON_TARGET_WINDOW_DISAPPEAR;
			qDebug() << "window is disappear!";
			break;
		}

		if (gamedevice.worker.isNull())
		{
			std::this_thread::yield();
			continue;
		}

		//自動釋放記憶體
		if (freeMemTimer.hasExpired(5ll * 60ll * 1000ll))
		{
			freeMemTimer.restart();
			mem::freeUnuseMemory();
			mem::freeUnuseMemory(gamedevice.getProcess());
		}
		else
			freeMemTimer.restart();

		//有些數據需要和客戶端內存同步
		gamedevice.worker->updateDatasFromMemory();

		gamedevice.worker->setWindowTitle(gamedevice.getStringHash(util::kTitleFormatString));

		checkControl();

		//其他所有功能
		status = checkAndRunFunctions();

		//這裡是預留的暫時沒作用
		if (status == 1)//非登入狀態
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			nodelay = true;
		}
		else if (status == 2)//平時
		{
			//檢查開關 (隊伍、交易、名片...等等)
			checkEtcFlag();

			bCheckedFastBattle = gamedevice.getEnableHash(util::kFastBattleEnable);
			bCheckedAutoBattle = gamedevice.getEnableHash(util::kAutoBattleEnable);
			W = gamedevice.worker->getWorldStatus();
			// 允許 自動戰鬥
			if (bCheckedAutoBattle)
			{
				gamedevice.sendMessage(kSetBlockPacket, false, NULL); // 禁止阻擋戰鬥封包
			}
			// 允許 快速戰鬥
			else if (bCheckedFastBattle)
			{
				if (W == 10)// 強退戰鬥畫面
					gamedevice.worker->setGameStatus(7);
				gamedevice.sendMessage(kSetBlockPacket, true, NULL); // 允許阻擋戰鬥封包
			}
			else // 不允許 快速戰鬥 和 自動戰鬥
				gamedevice.sendMessage(kSetBlockPacket, false, NULL); // 禁止阻擋戰鬥封包
		}
		else if (status == 3)//戰鬥中
		{
			bCheckedFastBattle = gamedevice.getEnableHash(util::kFastBattleEnable);
			bCheckedAutoBattle = gamedevice.getEnableHash(util::kAutoBattleEnable);
			W = gamedevice.worker->getWorldStatus();
			if (bCheckedFastBattle)
			{
				if (W == 10)// 強退戰鬥畫面
					gamedevice.worker->setGameStatus(7);
			}

			if (bCheckedFastBattle || bCheckedAutoBattle)
				gamedevice.postMessage(kEnableBattleDialog, false, NULL);//禁止戰鬥面板出現
			else
				gamedevice.postMessage(kEnableBattleDialog, true, NULL);//允許戰鬥面板出現
		}
		else//錯誤
		{
			break;
		}

		std::this_thread::yield();
	}
}

long long MainObject::inGameInitialize() const
{
	GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	if (gamedevice.isGameInterruptionRequested())
	{
		qDebug() << "gamedevice.isGameInterruptionRequested() == true";
		return -1;
	}

	if (gamedevice.worker.isNull())
	{
		qDebug() << "gamedevice.worker.isNull() == true";
		return -1;
	}

	if (!gamedevice.worker->getOnlineFlag())
	{
		qDebug() << "gamedevice.worker->getOnlineFlag() == false";
		return 0;
	}

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(getIndex());

	//等待完全進入登入後的遊戲畫面
	util::timer timer;
	for (;;)
	{
		if (gamedevice.isGameInterruptionRequested())
		{
			qDebug() << "gamedevice.isGameInterruptionRequested() == true";
			return -1;
		}

		if (!gamedevice.isWindowAlive())
		{
			qDebug() << "gamedevice.isWindowAlive() == false";
			return -1;
		}

		if (gamedevice.worker.isNull())
		{
			qDebug() << "gamedevice.worker.isNull() == true";
			return -1;
		}

		if (!gamedevice.worker->getOnlineFlag())
		{
			qDebug() << "gamedevice.worker->getOnlineFlag() == false";
			return 0;
		}

		if (timer.hasExpired(sa::MAX_TIMEOUT))
		{
			qDebug() << "timer.hasExpired(sa::MAX_TIMEOUT) == true";
			return 0;
		}

		if (gamedevice.worker->checkWG(9, 3))
			break;

		std::this_thread::sleep_for(std::chrono::milliseconds(100));;
	}

	if (!gamedevice.worker->getBattleFlag())
		emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusInNormal);

	//QDateTime current = QDateTime::currentDateTime();
	//QDateTime due = current.addYears(99);
	//const QString dueStr(due.toString("yyyy-MM-dd hh:mm:ss"));
	const QString url("https://www.lovesa.cc");
	//QString currentVerStr;
	//QString newestVerStr;

	//if (!Downloader::checkUpdate(&currentVerStr, &newestVerStr, nullptr))
	//{
	//	newestVerStr = "nil";
	//}

	//登入後的廣告公告
	constexpr bool isbeta = true;
	const QString version = QString("%1.%2.%3")
		.arg(SASH_VERSION_MAJOR) \
		.arg(SASH_VERSION_MINOR) \
		.arg(0);
	gamedevice.worker->announce(tr("Welcome to use SaSH，For more information please visit %1").arg(url));
	gamedevice.worker->announce(tr("You are using %1 account, due date is:%2").arg(isbeta ? tr("trial") : tr("subscribed")).arg(0));
	gamedevice.worker->announce(tr("StoneAge SaSH forum url:%1, newest version is %2").arg(url).arg(version));
	gamedevice.sendMessage(kDistoryDialog, NULL, NULL);
	gamedevice.worker->echo();
	gamedevice.worker->updateComboBoxList();
	gamedevice.worker->updateDatasFromMemory();
	gamedevice.worker->updateItemByMemory();
	gamedevice.worker->refreshItemInfo();

	emit signalDispatcher.applyHashSettingsToUI();

	mem::freeUnuseMemory(gamedevice.getProcess());
	return 1;
}

long long MainObject::checkAndRunFunctions()
{
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return 0;

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);

	long long status = gamedevice.worker->getUnloginStatus();

	if (status == util::kStatusDisappear)
	{
		qDebug() << "status == util::kStatusDisappear";
		return 0;
	}
	else
	{
		switch (status)
		{
		case util::kStatusLogined:
		{
			break;
		}
		default:
		{
			if (gamedevice.worker->getOnlineFlag())
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

				gamedevice.worker->clear();
				gamedevice.chatLogModel.clear();
			}

			gamedevice.worker->loginTimer.restart();
			//自動登入 或 斷線重連
			if (gamedevice.getEnableHash(util::kAutoLoginEnable) || gamedevice.worker->IS_DISCONNECTED.get())
				gamedevice.worker->login(status);
			return 1;
		}
		case util::kStatusDisconnect:
		{
			if (gamedevice.getEnableHash(util::kAutoReconnectEnable))
				gamedevice.worker->login(status);
			return 1;
		}
		}
	}

	//每次登入後只會執行一次
	if (login_run_once_flag_)
	{
		long long value = inGameInitialize();
		if (-1 == value)
			return 0;
		else if (0 == value)
			return 1;

		login_run_once_flag_ = false;
	}

	emit signalDispatcher.updateLoginTimeLabelTextChanged(util::formatMilliseconds(gamedevice.worker->loginTimer.cost(), true));

	//更新掛機數據到UI
	updateAfkInfos();

	//批次開啟子任務線程
	for (long long i = 0; i < MissionThread::kMaxAutoMission; ++i)
	{
		MissionThread* p = autoThreads_.value(i);
		if (p != nullptr && !p->isFinished())
		{
			if (!p->isRunning())
			{
				p->start();
			}

			continue;
		}

		if (p != nullptr)
		{
			p->wait();
			delete p;
			p = nullptr;
		}

		p = q_check_ptr(new MissionThread(currentIndex, i));
		sash_assume(p != nullptr);
		if (p == nullptr)
			continue;

		autoThreads_[i] = p;
		p->start();
	}

	//平時
	if (!gamedevice.worker->getBattleFlag())
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

		if (gamedevice.battleActionFuture.isRunning())
			return 3;

		gamedevice.battleActionFuture = QtConcurrent::run([this]()->void
			{
				GameDevice& gamedevice = GameDevice::getInstance(getIndex());
				if (gamedevice.worker.isNull())
					return;

				if (gamedevice.isGameInterruptionRequested())
					return;

				for (;;)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(100));

					if (gamedevice.isGameInterruptionRequested())
						return;

					if (!gamedevice.worker->getBattleFlag())
						continue;

					if (!gamedevice.worker->getOnlineFlag())
						continue;

					gamedevice.worker->asyncBattleAction(true);
				}

			});
		return 3;
	}
}

void MainObject::updateAfkInfos() const
{
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(getIndex());
	GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	if (gamedevice.worker.isNull())
		return;

	long long duration = gamedevice.worker->loginTimer.cost();
	emit signalDispatcher.updateAfkInfoTable(0, util::formatMilliseconds(duration));

	sa::afk_record_data_t records = gamedevice.worker->afkRecords[0];

	long long avgLevelPerHour = 0;
	if (duration > 0 && records.leveldifference > 0)
		avgLevelPerHour = records.leveldifference * 3600000.0 / duration;
	emit signalDispatcher.updateAfkInfoTable(2, QString(tr("%1→%2 (avg level: %3)"))
		.arg(records.levelrecord).arg(records.levelrecord + records.leveldifference).arg(avgLevelPerHour));

	long long avgExpPerHour = 0;
	if (duration > 0 && records.expdifference > 0)
		avgExpPerHour = records.expdifference * 3600000.0 / duration;

	emit signalDispatcher.updateAfkInfoTable(3, tr("%1 (avg exp: %2)").arg(records.expdifference).arg(avgExpPerHour));

	emit signalDispatcher.updateAfkInfoTable(4, util::toQString(records.deadthcount));

	long long avgGoldPerHour = 0;
	if (duration > 0 && records.goldearn > 0)
		avgGoldPerHour = records.goldearn * 3600000.0 / duration;
	emit signalDispatcher.updateAfkInfoTable(5, tr("%1 (avg gold: %2)").arg(records.goldearn).arg(avgGoldPerHour));

	constexpr long long n = 7;
	long long j = 0;
	for (long long i = 0; i < sa::MAX_PET; ++i)
	{
		records = gamedevice.worker->afkRecords[i + 1];

		avgExpPerHour = 0;
		if (duration > 0 && records.leveldifference > 0)
			avgExpPerHour = records.leveldifference * 3600000.0 / duration;

		emit signalDispatcher.updateAfkInfoTable(i + n + j, QString(tr("%1→%2 (avg level: %3)"))
			.arg(records.levelrecord).arg(records.levelrecord + records.leveldifference).arg(avgExpPerHour));

		avgExpPerHour = 0;
		if (duration > 0 && records.expdifference > 0)
			avgExpPerHour = records.expdifference * 3600000.0 / duration;
		emit signalDispatcher.updateAfkInfoTable(i + n + 1 + j, tr("%1 (avg exp: %2)").arg(records.expdifference).arg(avgExpPerHour));

		emit signalDispatcher.updateAfkInfoTable(i + n + 2 + j, util::toQString(records.deadthcount));

		emit signalDispatcher.updateAfkInfoTable(i + n + 3 + j, "");

		j += 3;
	}
}

//根據UI的選擇項變動做的一些操作
void MainObject::checkControl()
{
	GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	if (gamedevice.worker.isNull())
		return;

	//異步加速
	long long value = gamedevice.getValueHash(util::kSpeedBoostValue);
	if (flagSetBoostValue_ != value)
	{
		flagSetBoostValue_ = value;
		gamedevice.postMessage(kSetBoostSpeed, true, flagSetBoostValue_);
	}

	//登出按下，異步登出
	if (gamedevice.getEnableHash(util::kLogOutEnable))
	{
		gamedevice.setEnableHash(util::kLogOutEnable, false);
		if (!gamedevice.worker.isNull())
			gamedevice.worker->logOut();
	}

	//EO按下，異步發送EO
	if (gamedevice.getEnableHash(util::kEchoEnable))
	{
		gamedevice.setEnableHash(util::kEchoEnable, false);
		if (!gamedevice.worker.isNull())
			gamedevice.worker->EO();
	}

	//回點按下，異步回點
	if (gamedevice.getEnableHash(util::kLogBackEnable))
	{
		gamedevice.setEnableHash(util::kLogBackEnable, false);
		if (!gamedevice.worker.isNull())
			gamedevice.worker->logBack();
	}

	//異步快速走路
	bool bChecked = gamedevice.getEnableHash(util::kFastWalkEnable);
	if (flagFastWalkEnable_ != bChecked)
	{
		flagFastWalkEnable_ = bChecked;
		gamedevice.postMessage(kEnableFastWalk, bChecked, NULL);
	}

	//異步橫衝直撞 (穿牆)
	bChecked = gamedevice.getEnableHash(util::kPassWallEnable);
	if (flagPassWallEnable_ != bChecked)
	{
		flagPassWallEnable_ = bChecked;
		gamedevice.postMessage(kEnablePassWall, bChecked, NULL);
	}

	//異步鎖定畫面
	bChecked = gamedevice.getEnableHash(util::kLockImageEnable);
	if (flagLockImageEnable_ != bChecked)
	{
		flagLockImageEnable_ = bChecked;
		gamedevice.postMessage(kEnableImageLock, bChecked, NULL);
	}

	//異步資源優化
	bChecked = true;
	if (flagOptimizeEnable_ != bChecked)
	{
		flagOptimizeEnable_ = bChecked;
		gamedevice.postMessage(kEnableOptimize, bChecked, NULL);
	}

	//異步關閉特效
	bChecked = gamedevice.getEnableHash(util::kCloseEffectEnable);
	if (flagCloseEffectEnable_ != bChecked)
	{
		flagCloseEffectEnable_ = bChecked;
		gamedevice.postMessage(kEnableEffect, !bChecked, NULL);
	}

	//異步鎖定時間
	bChecked = gamedevice.getEnableHash(util::kLockTimeEnable);
	value = gamedevice.getValueHash(util::kLockTimeValue);
	if (flagLockTimeEnable_ != bChecked || flagLockTimeValue_ != value)
	{
		flagLockTimeEnable_ = bChecked;
		flagLockTimeValue_ = value;
		gamedevice.postMessage(kSetTimeLock, bChecked, flagLockTimeValue_);
	}

	//隱藏人物按下，異步隱藏
	bChecked = gamedevice.getEnableHash(util::kHideCharacterEnable);
	if (flagHideCharacterEnable_ != bChecked)
	{
		flagHideCharacterEnable_ = bChecked;
		gamedevice.postMessage(kEnableCharShow, !bChecked, NULL);
	}

	//異步戰鬥99秒
	bChecked = gamedevice.getEnableHash(util::kBattleTimeExtendEnable);
	if (flagBattleTimeExtendEnable_ != bChecked)
	{
		flagBattleTimeExtendEnable_ = bChecked;
		gamedevice.postMessage(kBattleTimeExtend, bChecked, NULL);
	}

	if (!gamedevice.worker->getOnlineFlag())
		return;

	//超過10秒才能執行否則會崩潰
	if (!gamedevice.worker->loginTimer.hasExpired(10000))
		return;

	//異步關閉聲音
	bChecked = gamedevice.getEnableHash(util::kMuteEnable);
	if (flagMuteEnable_ != bChecked)
	{
		flagMuteEnable_ = bChecked;
		gamedevice.sendMessage(kEnableSound, !bChecked, NULL);
	}
}

//檢查開關 (隊伍、交易、名片...等等)
void MainObject::checkEtcFlag() const
{
	GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	if (gamedevice.worker.isNull())
		return;

	long long flg = gamedevice.worker->getCharacter().etcFlag;
	bool hasChange = false;
	auto toBool = [flg](long long f)->bool
		{
			return ((flg & f) != 0);
		};

	bool bCurrent = gamedevice.getEnableHash(util::kSwitcherTeamEnable);
	if (toBool(sa::PC_ETCFLAG_GROUP) != bCurrent)
	{
		if (bCurrent)
			flg |= sa::PC_ETCFLAG_GROUP;
		else
			flg &= ~sa::PC_ETCFLAG_GROUP;

		hasChange = true;
	}

	bCurrent = gamedevice.getEnableHash(util::kSwitcherPKEnable);
	if (toBool(sa::PC_ETCFLAG_PK) != bCurrent)
	{
		if (bCurrent)
			flg |= sa::PC_ETCFLAG_PK;
		else
			flg &= ~sa::PC_ETCFLAG_PK;

		hasChange = true;
	}

	bCurrent = gamedevice.getEnableHash(util::kSwitcherCardEnable);
	if (toBool(sa::PC_ETCFLAG_CARD) != bCurrent)
	{
		if (bCurrent)
			flg |= sa::PC_ETCFLAG_CARD;
		else
			flg &= ~sa::PC_ETCFLAG_CARD;

		hasChange = true;
	}

	bCurrent = gamedevice.getEnableHash(util::kSwitcherTradeEnable);
	if (toBool(sa::PC_ETCFLAG_TRADE) != bCurrent)
	{
		if (bCurrent)
			flg |= sa::PC_ETCFLAG_TRADE;
		else
			flg &= ~sa::PC_ETCFLAG_TRADE;

		hasChange = true;
	}

	bCurrent = gamedevice.getEnableHash(util::kSwitcherGroupEnable);
	if (toBool(sa::PC_ETCFLAG_TEAM_CHAT) != bCurrent)
	{
		if (bCurrent)
			flg |= sa::PC_ETCFLAG_TEAM_CHAT;
		else
			flg &= ~sa::PC_ETCFLAG_TEAM_CHAT;

		hasChange = true;
	}

	bCurrent = gamedevice.getEnableHash(util::kSwitcherFamilyEnable);
	if (toBool(sa::PC_ETCFLAG_FM) != bCurrent)
	{
		if (bCurrent)
			flg |= sa::PC_ETCFLAG_FM;
		else
			flg &= ~sa::PC_ETCFLAG_FM;

		hasChange = true;
	}

	bCurrent = gamedevice.getEnableHash(util::kSwitcherJobEnable);
	if (toBool(sa::PC_ETCFLAG_JOB) != bCurrent)
	{
		if (bCurrent)
			flg |= sa::PC_ETCFLAG_JOB;
		else
			flg &= ~sa::PC_ETCFLAG_JOB;

		hasChange = true;
	}

	bCurrent = gamedevice.getEnableHash(util::kSwitcherWorldEnable);
	if (toBool(sa::PC_ETCFLAG_WORLD) != bCurrent)
	{
		if (bCurrent)
			flg |= sa::PC_ETCFLAG_WORLD;
		else
			flg &= ~sa::PC_ETCFLAG_WORLD;

		hasChange = true;
	}

	if (hasChange)
		gamedevice.worker->setSwitchers(flg);
}

MissionThread::MissionThread(long long index, long long type, QObject* parent)
	: Indexer(index)
	, type_(type)
{
	std::ignore = parent;

	switch (type)
	{
	case kAutoJoin:
		func_ = std::bind(&MissionThread::autoJoin, this);
		break;
	case kAutoWalk:
		func_ = std::bind(&MissionThread::autoWalk, this);
		break;
	case kAutoSortItem:
		func_ = std::bind(&MissionThread::autoSortItem, this);
		break;
	case kAutoHeal:
		func_ = std::bind(&MissionThread::autoHeal, this);
		break;
	case kAutoRecordNPC:
		func_ = std::bind(&MissionThread::autoRecordNPC, this);
		break;
	case kAsyncFindPath:
		func_ = std::bind(&MissionThread::asyncFindPath, this);
		break;
	}

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(index);
	connect(&signalDispatcher, &SignalDispatcher::nodifyAllStop, this, &MissionThread::requestMissionInterruption);
}

MissionThread::~MissionThread()
{
	qDebug() << "MissionThread::~MissionThread()";
}

void MissionThread::wait()
{
	if (future_.isRunning())
	{
		requestMissionInterruption();
		future_.cancel();
		future_.waitForFinished();
	}
}

void MissionThread::start()
{
	//GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	//switch (type_)
	//{
	//case kAutoJoin:
	//{
	//	if (gamedevice.getEnableHash(util::kAutoWalkEnable) || gamedevice.getEnableHash(util::kFastAutoWalkEnable))
	//		return;

	//	if (!gamedevice.getEnableHash(util::kAutoJoinEnable))
	//		return;

	//	break;
	//}
	//case kAutoWalk:
	//{
	//	if (!gamedevice.getEnableHash(util::kAutoWalkEnable) && !gamedevice.getEnableHash(util::kFastAutoWalkEnable))
	//		return;

	//	break;
	//}
	//case kAutoHeal:
	//{
	//	if (!gamedevice.getEnableHash(util::kNormalItemHealMpEnable)
	//		&& !gamedevice.getEnableHash(util::kNormalItemHealEnable)
	//		&& !gamedevice.getEnableHash(util::kNormalMagicHealEnable))
	//	{
	//		return;
	//	}

	//	break;
	//}
	//case kAutoSortItem:
	//{
	//	if (!gamedevice.getEnableHash(util::kAutoStackEnable))
	//		return;

	//	break;
	//}
	//case kAutoRecordNPC:
	//	break;
	//case kAsyncFindPath:
	//	break;
	//}

	if (future_.isRunning())
		return;

	future_ = QtConcurrent::run(func_);
}

void MissionThread::autoJoin()
{
	long long index = getIndex();
	QSet<QPoint> blockList;
	GameDevice& gamedevice = GameDevice::getInstance(index);
	constexpr long long MAX_SINGLE_STEP = 3;
	sa::map_t map = {};
	std::vector<QPoint> path;
	QPoint current_point;
	QPoint newpoint;
	sa::map_unit_t unit = {};
	long long dir = -1;
	long long floor = 0;
	long long len = MAX_SINGLE_STEP;
	long long size = 0;
	AStarDevice astar;
	sa::character_t ch = {};
	long long actionType = 0;
	QString leader;

	for (;;)
	{
		if (gamedevice.isGameInterruptionRequested())
			break;

		//如果主線程關閉則自動退出
		if (isMissionInterruptionRequested())
			break;

		if (gamedevice.worker.isNull())
			break;

		if (gamedevice.getEnableHash(util::kAutoWalkEnable) || gamedevice.getEnableHash(util::kFastAutoWalkEnable))
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			continue;
		}

		if (!gamedevice.getEnableHash(util::kAutoJoinEnable))
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			continue;
		}

		leader = gamedevice.getStringHash(util::kAutoFunNameString);
		if (leader.isEmpty())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			continue;
		}

		floor = gamedevice.worker->getFloor();

		for (;;)
		{
			if (gamedevice.isGameInterruptionRequested())
				break;

			//如果主線程關閉則自動退出
			if (isMissionInterruptionRequested())
				break;

			if (gamedevice.worker.isNull())
				break;

			if (gamedevice.getEnableHash(util::kAutoWalkEnable) || gamedevice.getEnableHash(util::kFastAutoWalkEnable))
				break;

			if (!gamedevice.getEnableHash(util::kAutoJoinEnable))
				break;

			leader = gamedevice.getStringHash(util::kAutoFunNameString);
			if (leader.isEmpty())
				break;

			if (!gamedevice.worker->mapDevice.getMapDataByFloor(floor, &map))
			{
				gamedevice.worker->mapDevice.readFromBinary(index, floor, gamedevice.worker->getFloorName(), false);
			}

			ch = gamedevice.worker->getCharacter();
			actionType = gamedevice.getValueHash(util::kAutoFunTypeValue);
			if (actionType == 0)
			{
				//檢查隊長是否正確
				if (util::checkAND(ch.status, sa::kCharacterStatus_IsLeader))
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(500));
					continue;
				}

				if (util::checkAND(ch.status, sa::kCharacterStatus_HasTeam))
				{
					QString name = gamedevice.worker->getTeam(0).name;
					if ((!name.isEmpty() && leader == name)
						|| (!name.isEmpty() && leader.count("|") > 0 && leader.contains(name)))//隊長正確
					{
						std::this_thread::sleep_for(std::chrono::milliseconds(500));
						continue;
					}

					gamedevice.worker->setTeamState(false);
					std::this_thread::sleep_for(std::chrono::milliseconds(200));
				}
			}


			if (gamedevice.isGameInterruptionRequested())
				break;

			if (gamedevice.worker.isNull())
				break;

			if (isMissionInterruptionRequested())
				break;

			//如果人物不在線上則自動退出
			if (!gamedevice.worker->getOnlineFlag())
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100));;
				continue;
			}

			if (gamedevice.worker->getBattleFlag())
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100));;
				continue;
			}

			ch = gamedevice.worker->getCharacter();

			if (floor != gamedevice.worker->getFloor())
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
				continue;
			}

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
			if (!gamedevice.worker->findUnit(leader, sa::kObjectHuman, &unit, freeName))
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
				continue;
			}

			//如果和目標人物處於同一個坐標則向隨機方向移動一格
			current_point = gamedevice.worker->getPoint();
			if (current_point == unit.p)
			{
				gamedevice.worker->move(current_point + util::fix_point.value(util::rnd::get(0, 7)));
				std::this_thread::sleep_for(std::chrono::milliseconds(100));;
				continue;
			}

			//計算最短離靠近目標人物的坐標和面相的方向
			dir = gamedevice.worker->mapDevice.calcBestFollowPointByDstPoint(index, &astar, floor, current_point, unit.p, &newpoint, false, -1);
			if (-1 == dir)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
				continue;
			}

			if (current_point == newpoint)
			{
				actionType = gamedevice.getValueHash(util::kAutoFunTypeValue);
				if (actionType == 0)
				{
					gamedevice.worker->setCharFaceDirection(dir, true);
					gamedevice.worker->setTeamState(true);
					continue;
				}
			}

			if (!gamedevice.worker->mapDevice.calcNewRoute(index, &astar, floor, current_point, newpoint, blockList, &path))
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
				continue;
			}

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
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
				continue;
			}

			if (len >= static_cast<long long>(path.size()))
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
				continue;
			}

			gamedevice.worker->move(path.at(len));
		}
	}
}

void MissionThread::autoWalk()
{
	GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	QPoint current_pos;
	bool current_side = false;
	QPoint posCache = current_pos;

	for (;;)
	{
		if (gamedevice.isGameInterruptionRequested())
			break;

		if (gamedevice.worker.isNull())
			break;

		//如果主線程關閉則自動退出
		if (isMissionInterruptionRequested())
			break;

		//取設置
		bool enableAutoWalk = gamedevice.getEnableHash(util::kAutoWalkEnable);//走路遇敵開關
		bool enableFastAutoWalk = gamedevice.getEnableHash(util::kFastAutoWalkEnable);//快速遇敵開關
		if (!enableAutoWalk && !enableFastAutoWalk)
		{
			current_pos = QPoint();
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}
		else if (current_pos.isNull())
		{
			current_pos = gamedevice.worker->getPoint();
		}

		//如果人物不在線上則自動退出
		if (!gamedevice.worker->getOnlineFlag())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			continue;
		}

		//如果人物在戰鬥中則進入循環等待
		if (gamedevice.worker->getBattleFlag())
		{
			//先等一小段時間
			std::this_thread::sleep_for(std::chrono::milliseconds(500));

			//如果已經退出戰鬥就等待1.5秒避免太快開始移動不夠時間吃肉補血丟東西...等
			if (!gamedevice.worker->getBattleFlag())
			{
				for (long long i = 0; i < 5; ++i)
				{
					if (gamedevice.isGameInterruptionRequested())
						break;

					if (gamedevice.worker.isNull())
						break;

					//如果主線程關閉則自動退出
					if (isMissionInterruptionRequested())
						break;

					std::this_thread::sleep_for(std::chrono::milliseconds(100));;
				}
			}
			else
				continue;
		}

		long long walk_speed = gamedevice.getValueHash(util::kAutoWalkDelayValue);//走路速度

		//走路遇敵
		if (enableAutoWalk)
		{
			long long walk_len = gamedevice.getValueHash(util::kAutoWalkDistanceValue);//走路距離
			long long walk_dir = gamedevice.getValueHash(util::kAutoWalkDirectionValue);//走路方向

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
					x = util::rnd::get(current_pos.x() - walk_len, current_pos.x() + walk_len);
					y = util::rnd::get(current_pos.y() - walk_len, current_pos.y() + walk_len);
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
				gamedevice.worker->move(current_pos, steps);
			else
				gamedevice.worker->move(QPoint(x, y));
			steps.clear();
		}
		else if (enableFastAutoWalk) //快速遇敵 (封包)
		{
			gamedevice.worker->move(QPoint(0, 0), "gcgc");
		}
		//避免太快無論如何都+15ms (太快並不會遇比較快)
		std::this_thread::sleep_for(std::chrono::milliseconds(walk_speed + 1));
	}
}

void MissionThread::autoSortItem()
{
	long long i = 0;
	constexpr long long duration = 50;
	GameDevice& gamedevice = GameDevice::getInstance(getIndex());

	for (;;)
	{
		for (i = 0; i < duration; ++i)
		{
			if (gamedevice.isGameInterruptionRequested())
				break;

			if (gamedevice.worker.isNull())
				break;

			if (isMissionInterruptionRequested())
				break;

			if (!gamedevice.getEnableHash(util::kAutoStackEnable))
				break;

			std::this_thread::sleep_for(std::chrono::milliseconds(100));;
		}

		if (!gamedevice.getEnableHash(util::kAutoStackEnable))
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			continue;
		}

		if (gamedevice.isGameInterruptionRequested())
			break;

		if (gamedevice.worker.isNull())
			break;

		gamedevice.worker->sortItem();
	}
}

//自動補血
void MissionThread::autoHeal()
{
	for (;;)
	{
		GameDevice& gamedevice = GameDevice::getInstance(getIndex());

		auto checkStatus = [this, &gamedevice]()->long long
			{
				//如果主線程關閉則自動退出
				if (gamedevice.isGameInterruptionRequested())
					return -1;

				if (gamedevice.worker.isNull())
					return -1;

				if (!gamedevice.worker->getOnlineFlag())
					return 0;

				if (gamedevice.worker->getBattleFlag())
					return 0;

				if (!gamedevice.getEnableHash(util::kNormalItemHealMpEnable)
					&& !gamedevice.getEnableHash(util::kNormalItemHealEnable)
					&& !gamedevice.getEnableHash(util::kNormalMagicHealEnable))
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
					return 0;
				}

				return 1;
			};

		long long state = checkStatus();
		if (-1 == state)
			break;
		else if (0 == state)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		QStringList items;
		long long itemIndex = -1;

		//平時道具補氣
		for (;;)
		{
			if (checkStatus() != 1)
				break;

			bool itemHealMpEnable = gamedevice.getEnableHash(util::kNormalItemHealMpEnable);
			if (!itemHealMpEnable)
				break;

			long long cmpvalue = gamedevice.getValueHash(util::kNormalItemHealMpValue);
			if (!gamedevice.worker->checkCharMp(cmpvalue))
				break;

			QString text = gamedevice.getStringHash(util::kNormalItemHealMpItemString).simplified();
			if (text.isEmpty())
				break;

			items = text.split(util::rexOR, Qt::SkipEmptyParts);
			if (items.isEmpty())
				break;

			itemIndex = -1;
			for (const QString& str : items)
			{
				itemIndex = gamedevice.worker->getItemIndexByName(str);
				if (itemIndex != -1)
					break;
			}

			if (itemIndex == -1)
				break;

			gamedevice.worker->useItem(itemIndex, 0);
			std::this_thread::sleep_for(std::chrono::milliseconds(150));
		}

		//平時道具補血
		for (;;)
		{
			if (checkStatus() != 1)
				break;

			bool itemHealHpEnable = gamedevice.getEnableHash(util::kNormalItemHealEnable);
			if (!itemHealHpEnable)
				break;

			long long charPercent = gamedevice.getValueHash(util::kNormalItemHealCharValue);
			long long petPercent = gamedevice.getValueHash(util::kNormalItemHealPetValue);
			long long alliePercent = gamedevice.getValueHash(util::kNormalItemHealAllieValue);

			if (charPercent == 0 && petPercent == 0 && alliePercent == 0)
				break;

			long long target = -1;
			bool ok = false;
			if ((charPercent > 0) && gamedevice.worker->checkCharHp(charPercent))
			{
				ok = true;
				target = 0;
			}

			if (!ok && (petPercent > 0) && gamedevice.worker->checkPetHp(petPercent))
			{
				ok = true;
				target = gamedevice.worker->getCharacter().battlePetNo + 1;
			}

			if (!ok && (petPercent > 0) && gamedevice.worker->checkRideHp(petPercent))
			{
				ok = true;
				target = gamedevice.worker->getCharacter().ridePetNo + 1;
			}

			if (!ok && (alliePercent > 0) && gamedevice.worker->checkTeammateHp(alliePercent, &target))
			{
				ok = true;
				target += sa::MAX_PET;
			}

			if (!ok || target == -1)
				break;

			itemIndex = -1;
			bool meatProiory = gamedevice.getEnableHash(util::kNormalItemHealMeatPriorityEnable);
			if (meatProiory)
			{
				itemIndex = gamedevice.worker->getItemIndexByName("?肉", false, "耐久力");
			}

			if (itemIndex == -1)
			{
				QString text = gamedevice.getStringHash(util::kNormalItemHealItemString).simplified();

				items = text.split(util::rexOR, Qt::SkipEmptyParts);
				for (const QString& str : items)
				{
					itemIndex = gamedevice.worker->getItemIndexByName(str);
					if (itemIndex != -1)
						break;
				}
			}

			if (itemIndex == -1)
				break;

			long long targetType = gamedevice.worker->getItem(itemIndex).target;
			if ((targetType != sa::ITEM_TARGET_MYSELF) && (targetType != sa::ITEM_TARGET_OTHER))
				break;

			if ((targetType == sa::ITEM_TARGET_MYSELF) && (target != 0))
				break;

			gamedevice.worker->useItem(itemIndex, target);
			std::this_thread::sleep_for(std::chrono::milliseconds(150));
		}

		//平時精靈補血
		for (;;)
		{
			if (checkStatus() != 1)
				break;

			bool magicHealHpEnable = gamedevice.getEnableHash(util::kNormalMagicHealEnable);
			if (!magicHealHpEnable)
				break;

			long long charPercent = gamedevice.getValueHash(util::kNormalMagicHealCharValue);
			long long petPercent = gamedevice.getValueHash(util::kNormalMagicHealPetValue);
			long long alliePercent = gamedevice.getValueHash(util::kNormalMagicHealAllieValue);

			if (charPercent == 0 && petPercent == 0 && alliePercent == 0)
				break;


			long long target = -1;
			bool ok = false;
			if ((charPercent > 0) && gamedevice.worker->checkCharHp(charPercent))
			{
				ok = true;
				target = 0;
			}
			else if (!ok && (petPercent > 0) && gamedevice.worker->checkPetHp(petPercent))
			{
				ok = true;
				target = gamedevice.worker->getCharacter().battlePetNo + 1;
			}
			else if (!ok && (petPercent > 0) && gamedevice.worker->checkRideHp(petPercent))
			{
				ok = true;
				target = gamedevice.worker->getCharacter().ridePetNo + 1;
			}
			else if (!ok && (alliePercent > 0) && gamedevice.worker->checkTeammateHp(alliePercent, &target))
			{
				ok = true;
				target += sa::MAX_PET;
			}

			if (!ok || target == -1)
				break;

			long long magicIndex = -1;
			{
				QString itemNames = gamedevice.getStringHash(util::kNormalMagicHealItemString).simplified();
				QVector<long long> nitems;
				if (gamedevice.worker->getItemIndexsByName(itemNames, "", &nitems) && !nitems.isEmpty())
					magicIndex = nitems.front();
			}

			if (magicIndex == -1)
			{
				magicIndex = gamedevice.getValueHash(util::kNormalMagicHealMagicValue);
				if (magicIndex >= 0 && magicIndex < sa::CHAR_EQUIPSLOT_COUNT)
				{
					long long targetType = gamedevice.worker->getMagic(magicIndex).target;
					if ((targetType != sa::MAGIC_TARGET_MYSELF) && (targetType != sa::MAGIC_TARGET_OTHER))
						break;

					if ((targetType == sa::MAGIC_TARGET_MYSELF) && (target != 0))
						break;
				}
			}

			gamedevice.worker->useMagic(magicIndex, target);
			std::this_thread::sleep_for(std::chrono::milliseconds(150));
		}
	}
}


void MissionThread::autoRecordNPC()
{
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	for (;;)
	{
		for (long long i = 0; i < 50; ++i)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));;

			if (gamedevice.isGameInterruptionRequested())
				break;

			if (gamedevice.worker.isNull())
				break;

			if (isMissionInterruptionRequested())
				break;
		}

		if (gamedevice.isGameInterruptionRequested())
			return;

		if (gamedevice.worker.isNull())
			return;

		if (isMissionInterruptionRequested())
			return;

		if (!gamedevice.worker->getOnlineFlag())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			continue;
		}

		if (gamedevice.worker->getBattleFlag())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			continue;
		}

		AStarDevice astar;

		QHash<long long, sa::map_unit_t> units = gamedevice.worker->mapUnitHash.toHash();
		util::Config config(gamedevice.getPointFileName(), QString("%1|%2").arg(__FUNCTION__).arg(__LINE__));

		for (const sa::map_unit_t& unit : units)
		{
			if (gamedevice.isGameInterruptionRequested())
				break;

			if (isMissionInterruptionRequested())
				break;

			if (gamedevice.worker.isNull())
				break;

			if (!gamedevice.worker->getOnlineFlag())
				break;

			if (gamedevice.worker->getBattleFlag())
				break;

			if ((unit.objType != sa::kObjectNPC)
				|| unit.name.isEmpty()
				|| (gamedevice.worker->getWorldStatus() != 9)
				|| (gamedevice.worker->getGameStatus() != 3)
				|| gamedevice.worker->npcUnitPointHash.contains(QPoint(unit.x, unit.y)))
			{
				continue;
			}

			gamedevice.worker->npcUnitPointHash.insert(QPoint(unit.x, unit.y), unit);

			util::Config::MapData d;
			long long nowFloor = gamedevice.worker->getFloor();
			QPoint nowPoint = gamedevice.worker->getPoint();

			d.floor = nowFloor;
			d.name = unit.name;

			//npc前方一格
			QPoint newPoint = util::fix_point.value(unit.dir) + unit.p;
			//檢查是否可走
			if (gamedevice.worker->mapDevice.isPassable(currentIndex, &astar, nowFloor, nowPoint, newPoint))
			{
				d.x = newPoint.x();
				d.y = newPoint.y();
			}
			else
			{
				//再往前一格
				QPoint additionPoint = util::fix_point.value(unit.dir) + newPoint;
				//檢查是否可走
				if (gamedevice.worker->mapDevice.isPassable(currentIndex, &astar, nowFloor, nowPoint, additionPoint))
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
						if (gamedevice.worker->mapDevice.isPassable(currentIndex, &astar, nowFloor, nowPoint, newPoint))
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

			util::Config::MapData d;
			d.floor = floor;
			d.name = name;
			d.x = x;
			d.y = y;

			sa::map_unit_t unit;
			unit.x = x;
			unit.y = y;
			unit.p = pos;
			unit.name = name;

			gamedevice.worker->npcUnitPointHash.insert(pos, unit);

			config.writeMapData(name, d);
		}
	}
}

void MissionThread::asyncFindPath()
{
	GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	if (gamedevice.worker.isNull())
		return;

	QPoint dst = args_.value(0).toPoint();
	gamedevice.worker->findPathAsync(dst);
}

#if 0
//自動鎖寵排程
void MainObject::checkAutoLockSchedule()
{
	GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	if (gamedevice.worker.isNull())
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

			GameDevice& gamedevice = GameDevice::getInstance(getIndex());
			QString lockPetSchedule = gamedevice.getStringHash(set);
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

						if (petIndex < 0 || petIndex >= sa::MAX_PET)
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

						PET pet = gamedevice.worker->getPet(petIndex);

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
				PET pet = gamedevice.worker->getPet(rindex);
				if (pet.hp <= 1)
				{
					gamedevice.worker->setPetState(rindex, kRest);
					std::this_thread::sleep_for(std::chrono::milliseconds(100));;
				}

				if (pet.state != kRide)
					gamedevice.worker->setPetState(rindex, kRide);
			}

			if (bindex != -1)
			{
				PET pet = gamedevice.worker->getPet(bindex);
				if (pet.hp <= 1)
				{
					gamedevice.worker->setPetState(bindex, kRest);
					std::this_thread::sleep_for(std::chrono::milliseconds(100));;
				}

				if (pet.state != kBattle)
					gamedevice.worker->setPetState(bindex, kBattle);
			}

			for (long long i = 0; i < sa::MAX_PET; ++i)
			{
				if (bindex == i || rindex == i)
					continue;

				PET pet = gamedevice.worker->getPet(i);
				if ((pet.state != kRest && pet.state != kStandby) && set == util::kLockPetScheduleString)
					gamedevice.worker->setPetState(i, kRest);
			}
			return false;
};

	if (gamedevice.getEnableHash(util::kLockPetScheduleEnable) && !gamedevice.getEnableHash(util::kLockPetEnable) && !gamedevice.getEnableHash(util::kLockRideEnable))
		checkSchedule(util::kLockPetScheduleString);

	}
#endif
