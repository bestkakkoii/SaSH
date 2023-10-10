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

bool ThreadManager::createThread(qint64 index, MainObject** ppObj, QObject* parent)
{
	QMutexLocker locker(&mutex_);
	if (threads_.contains(index))
		return false;

	if (objects_.contains(index))
		return false;

	do
	{
		MainObject* object = new MainObject(index, nullptr);
		if (nullptr == object)
			break;

		QThread* thread = new QThread(this);
		if (nullptr == thread)
			break;

		thread->setObjectName(QString("thread_%1").arg(index));

		object->moveToThread(thread);

		threads_.insert(index, thread);
		objects_.insert(index, object);

		connect(thread, &QThread::started, object, &MainObject::run, Qt::UniqueConnection);
		//after delete must set nullptr
		connect(object, &MainObject::finished, this, [this]()
			{
				MainObject* object = qobject_cast<MainObject*>(sender());
				if (nullptr == object)
					return;

				QThread* thread_ = object->thread();
				if (nullptr == thread_)
					return;

				qint64 index = object->getIndex();
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

			}, Qt::UniqueConnection);
		thread->start();

		if (ppObj != nullptr)
			*ppObj = object;
		return true;
	} while (false);

	return false;
}

void ThreadManager::close(qint64 index)
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

MainObject::MainObject(qint64 index, QObject* parent)
	: ThreadPlugin(index, parent)
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
	Injector& injector = Injector::getInstance(getIndex());
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(getIndex());

	Injector::process_information_t process_info;
	util::REMOVE_THREAD_REASON remove_thread_reason = util::REASON_NO_ERROR;
	QElapsedTimer timer; timer.start();
	do
	{
		//檢查服務端是否實例化
		if (injector.server.isNull())
			break;

		//檢查服務端是否正在運行並且正在監聽
		if (!injector.server->isListening())
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
			if (!injector.injectLibrary(process_info, injector.server->getPort(), &remove_thread_reason)
				|| (remove_thread_reason != util::REASON_NO_ERROR))
			{
				qDebug() << "injectLibrary failed. reason:" << remove_thread_reason;
				break;
			}
		}

		//等待客戶端連入
		for (;;)
		{
			if (injector.server->hasClientExist())
				break;

			if (isInterruptionRequested())
			{
				remove_thread_reason = util::REASON_REQUEST_STOP;
				break;
			}

			if (timer.hasExpired(15000))
			{
				remove_thread_reason = util::REASON_TCP_CONNECTION_TIMEOUT;
				break;
			}
			QThread::msleep(100);
		}

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

	if (pointerwriter_future_.isRunning())
	{
		pointerwriter_future_cancel_flag_.store(true, std::memory_order_release);
		pointerwriter_future_.cancel();
		pointerwriter_future_.waitForFinished();
	}

	//關閉走路遇敵線程
	if (autowalk_future_.isRunning())
	{
		autowalk_future_cancel_flag_.store(true, std::memory_order_release);
		autowalk_future_.cancel();
		autowalk_future_.waitForFinished();
	}

	//關閉自動組隊線程
	if (autojoin_future_.isRunning())
	{
		autojoin_future_cancel_flag_.store(true, std::memory_order_release);
		autojoin_future_.cancel();
		autojoin_future_.waitForFinished();
	}

	//關閉自動補血線程
	if (autoheal_future_.isRunning())
	{
		autoheal_future_cancel_flag_.store(true, std::memory_order_release);
		autoheal_future_.cancel();
		autoheal_future_.waitForFinished();
	}

	//關閉自動丟寵
	if (autodroppet_future_.isRunning())
	{
		autodroppet_future_cancel_flag_.store(true, std::memory_order_release);
		autodroppet_future_.cancel();
		autodroppet_future_.waitForFinished();
	}

	//關閉自動疊加
	if (autosortitem_future_.isRunning())
	{
		autosortitem_future_cancel_flag_.store(true, std::memory_order_release);
		autosortitem_future_.cancel();
		autosortitem_future_.waitForFinished();
	}

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
	QElapsedTimer freeSelfMemTimer; freeSelfMemTimer.start();

	mem::freeUnuseMemory(injector.getProcess());

	bool nodelay = false;
	for (;;)
	{
		if (nodelay)
		{
			nodelay = false;
		}
		else
			QThread::msleep(1);

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

		//檢查TCP是否握手成功
		if (!injector.server->IS_TCP_CONNECTION_OK_TO_USE.load(std::memory_order_acquire))
		{
			QThread::msleep(100);
			nodelay = true;
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

		if (injector.getEnableHash(util::kAutoFreeMemoryEnable) && freeSelfMemTimer.hasExpired(60ll * 60ll * 1000ll))
		{
			freeSelfMemTimer.restart();
			injector.server->mapAnalyzer->clear();
		}
		else
			freeSelfMemTimer.restart();

		//有些數據需要和客戶端內存同步
		injector.server->updateDatasFromMemory();

		//其他所有功能
		qint64 status = checkAndRunFunctions();

		//這裡是預留的暫時沒作用
		if (status == 1)//非登入狀態
		{
			if (!isFirstLogin_)
				QThread::msleep(800);
			nodelay = true;
			continue;
		}
		else if (status == 2)//平時
		{

		}
		else if (status == 3)//戰鬥中不延時
		{
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
	if (injector.server.isNull())
		return;

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(getIndex());

	//等待完全進入登入後的遊戲畫面
	for (qint64 i = 0; i < 50; ++i)
	{
		if (isInterruptionRequested())
			return;

		if (injector.server.isNull())
			return;

		if (injector.server->getWorldStatus() == 9 && injector.server->getGameStatus() == 3)
			break;

		QThread::msleep(100);
	}

	if (!injector.server->getBattleFlag())
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
	injector.server->announce(tr("Welcome to use SaSH，For more information please visit %1").arg(url));
	injector.server->announce(tr("You are using %1 account, due date is:%2").arg(isbeta ? tr("trial") : tr("subscribed")).arg(dueStr));
	injector.server->announce(tr("StoneAge SaSH forum url:%1, newest version is %2").arg(url).arg(version));
}

qint64 MainObject::checkAndRunFunctions()
{
	Injector& injector = Injector::getInstance(getIndex());
	if (injector.server.isNull())
		return 0;

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(getIndex());

	qint64 status = injector.server->getUnloginStatus();

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
			if (injector.server->getOnlineFlag())
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

				injector.server->clear();
				if (!injector.chatLogModel.isNull())
					injector.chatLogModel->clear();
			}

			injector.server->loginTimer.restart();
			//自動登入 或 斷線重連
			if (injector.getEnableHash(util::kAutoLoginEnable) || injector.server->IS_DISCONNECTED.load(std::memory_order_acquire))
				injector.server->login(status);
			return 1;
		}
		case util::kStatusDisconnect:
		{
			if (injector.getEnableHash(util::kAutoReconnectEnable))
				injector.server->login(status);
			return 1;
		}
		}
	}

	//每次登入後只會執行一次
	if (login_run_once_flag_)
	{
		login_run_once_flag_ = false;

		QtConcurrent::run(this, &MainObject::inGameInitialize);

		if (isFirstLogin_)
			isFirstLogin_ = false;
	}

	emit signalDispatcher.updateLoginTimeLabelTextChanged(util::formatMilliseconds(injector.server->loginTimer.elapsed(), true));

	//更新掛機資訊
	//updateAfkInfos();

	//更新數據緩存(跨線程安全容器)
	setUserDatas();

	//檢查UI的設定是否有變化
	checkControl();

	//走路遇敵 或 快速遇敵 (封包)
	checkAutoWalk();

	//自動組隊、跟隨
	checkAutoJoin();

	//自動疊加
	checkAutoSortItem();

	//自動補血、氣
	checkAutoHeal();

	//自動丟寵
	checkAutoDropPet();

	//紀錄NPC
	checkRecordableNpcInfo();

	//平時
	if (!injector.server->getBattleFlag())
	{
		//每次進入平時只會執行一次
		if (!battle_run_once_flag_)
		{
			battle_run_once_flag_ = true;
		}

		//檢查開關 (隊伍、交易、名片...等等)
		checkEtcFlag();

		//檢查自動丟棄道具
		checkAutoDropItems();

		//檢查自動吃道具
		checkAutoEatBoostExpItem();

		return 2;
	}
	else //戰鬥中
	{
		//每次進入戰鬥只會執行一次
		if (battle_run_once_flag_)
		{
			battle_run_once_flag_ = false;
		}

		//異步處理戰鬥時間刷新
		if (!battleTime_future_.isRunning())
		{
			battleTime_future_cancel_flag_.store(false, std::memory_order_release);
			battleTime_future_ = QtConcurrent::run(this, &MainObject::battleTimeThread);
		}

		return 3;
	}

	return 1;
}

void MainObject::updateAfkInfos()
{
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(getIndex());
	Injector& injector = Injector::getInstance(getIndex());
	if (injector.server.isNull())
		return;

	qint64 duration = injector.server->loginTimer.elapsed();
	signalDispatcher.updateAfkInfoTable(0, util::formatMilliseconds(duration));

	util::AfkRecorder recorder = injector.server->recorder[0];

	qint64 avgLevelPerHour = 0;
	if (duration > 0 && recorder.leveldifference > 0)
		avgLevelPerHour = recorder.leveldifference * 3600000.0 / duration;
	signalDispatcher.updateAfkInfoTable(2, QString(tr("%1→%2 (avg level: %3)"))
		.arg(recorder.levelrecord).arg(recorder.levelrecord + recorder.leveldifference).arg(avgLevelPerHour));

	qint64 avgExpPerHour = 0;
	if (duration > 0 && recorder.expdifference > 0)
		avgExpPerHour = recorder.expdifference * 3600000.0 / duration;

	signalDispatcher.updateAfkInfoTable(3, tr("%1 (avg exp: %2)").arg(recorder.expdifference).arg(avgExpPerHour));

	signalDispatcher.updateAfkInfoTable(4, util::toQString(recorder.deadthcount));


	qint64 avgGoldPerHour = 0;
	if (duration > 0 && recorder.goldearn > 0)
		avgGoldPerHour = recorder.goldearn * 3600000.0 / duration;
	signalDispatcher.updateAfkInfoTable(5, tr("%1 (avg gold: %2)").arg(recorder.goldearn).arg(avgGoldPerHour));

	constexpr qint64 n = 7;
	qint64 j = 0;
	for (qint64 i = 0; i < MAX_PET; ++i)
	{
		recorder = injector.server->recorder[i + 1];

		avgExpPerHour = 0;
		if (duration > 0 && recorder.leveldifference > 0)
			avgExpPerHour = recorder.leveldifference * 3600000.0 / duration;

		signalDispatcher.updateAfkInfoTable(i + n + j, QString(tr("%1→%2 (avg level: %3)"))
			.arg(recorder.levelrecord).arg(recorder.levelrecord + recorder.leveldifference).arg(avgExpPerHour));

		avgExpPerHour = 0;
		if (duration > 0 && recorder.expdifference > 0)
			avgExpPerHour = recorder.expdifference * 3600000.0 / duration;
		signalDispatcher.updateAfkInfoTable(i + n + 1 + j, tr("%1 (avg exp: %2)").arg(recorder.expdifference).arg(avgExpPerHour));

		signalDispatcher.updateAfkInfoTable(i + n + 2 + j, util::toQString(recorder.deadthcount));


		signalDispatcher.updateAfkInfoTable(i + n + 3 + j, "");

		j += 3;

	}
}

void MainObject::battleTimeThread()
{
	Injector& injector = Injector::getInstance(getIndex());

	for (;;)
	{
		if (isInterruptionRequested())
			break;

		if (battleTime_future_cancel_flag_.load(std::memory_order_acquire))
			break;

		if (injector.server.isNull())
			break;

		if (!injector.server->getOnlineFlag())
			break;

		if (!injector.server->getBattleFlag())
			break;

		injector.server->updateBattleTimeInfo();
		QThread::msleep(1);
	}
	battleTime_future_cancel_flag_.store(false, std::memory_order_release);
}

//將一些數據保存到多線程安全容器
void MainObject::setUserDatas()
{
	Injector& injector = Injector::getInstance(getIndex());
	if (injector.server.isNull())
		return;

	QStringList itemNames;
	qint64 index = 0;

	QHash<qint64, ITEM> items = injector.server->getItems();
	for (qint64 index = CHAR_EQUIPPLACENUM; index < MAX_ITEM; ++index)
	{
		if (items.value(index).name.isEmpty() || !items.value(index).valid)
			continue;

		itemNames.append(items.value(index).name);
	}

	injector.setUserData(util::kUserItemNames, itemNames);

	QStringList petNames;

	for (qint64 i = 0; i < MAX_PET; ++i)
	{
		PET pet = injector.server->getPet(i);
		if (pet.name.isEmpty() || !pet.valid)
			continue;

		petNames.append(pet.name);
	}
	injector.setUserData(util::kUserPetNames, petNames);

	injector.setUserData(util::kUserEnemyNames, injector.server->enemyNameListCache.get());

}

//根據UI的選擇項變動做的一些操作
void MainObject::checkControl()
{
	Injector& injector = Injector::getInstance(getIndex());
	if (injector.server.isNull())
		return;

	//隱藏人物按下，異步隱藏
	bool bChecked = injector.getEnableHash(util::kHideCharacterEnable);
	if (flagHideCharacterEnable_ != bChecked)
	{
		flagHideCharacterEnable_ = bChecked;
		injector.postMessage(kEnableCharShow, !bChecked, NULL);
	}

	if (!injector.server->getOnlineFlag())
		return;

	//登出按下，異步登出
	if (injector.getEnableHash(util::kLogOutEnable))
	{
		injector.setEnableHash(util::kLogOutEnable, false);
		if (!injector.server.isNull())
			injector.server->logOut();
	}

	//回點按下，異步回點
	if (injector.getEnableHash(util::kLogBackEnable))
	{
		injector.setEnableHash(util::kLogBackEnable, false);
		if (!injector.server.isNull())
			injector.server->logBack();
	}

	//EO按下，異步發送EO
	if (injector.getEnableHash(util::kEchoEnable))
	{
		injector.setEnableHash(util::kEchoEnable, false);
		if (!injector.server.isNull())
			injector.server->EO();
	}

	//////////////////////////////

	//異步關閉特效
	bChecked = injector.getEnableHash(util::kCloseEffectEnable);
	if (flagCloseEffectEnable_ != bChecked)
	{
		flagCloseEffectEnable_ = bChecked;
		injector.postMessage(kEnableEffect, !bChecked, NULL);
	}

	//異步關閉聲音
	bChecked = injector.getEnableHash(util::kMuteEnable);
	if (flagMuteEnable_ != bChecked)
	{
		flagMuteEnable_ = bChecked;
		injector.postMessage(kEnableSound, !bChecked, NULL);
	}

	//異步鎖定時間
	bChecked = injector.getEnableHash(util::kLockTimeEnable);
	qint64 value = injector.getValueHash(util::kLockTimeValue);
	if (flagLockTimeEnable_ != bChecked || flagLockTimeValue_ != value)
	{
		flagLockTimeEnable_ = bChecked;
		flagLockTimeValue_ = value;
		injector.postMessage(kSetTimeLock, bChecked, flagLockTimeValue_);
	}

	//異步加速
	value = injector.getValueHash(util::kSpeedBoostValue);
	if (flagSetBoostValue_ != value)
	{
		flagSetBoostValue_ = value;
		injector.postMessage(kSetBoostSpeed, true, flagSetBoostValue_);
	}

	//異步快速走路
	bChecked = injector.getEnableHash(util::kFastWalkEnable);
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

	//異步戰鬥99秒
	bChecked = injector.getEnableHash(util::kBattleTimeExtendEnable);
	if (flagBattleTimeExtendEnable_ != bChecked)
	{
		flagBattleTimeExtendEnable_ = bChecked;
		injector.postMessage(kBattleTimeExtend, bChecked, NULL);
	}

	//異步資源優化
	bChecked = injector.getEnableHash(util::kOptimizeEnable);
	if (flagOptimizeEnable_ != bChecked)
	{
		flagOptimizeEnable_ = bChecked;
		injector.postMessage(kEnableOptimize, bChecked, NULL);
	}

	//自動戰鬥，異步戰鬥面板開關
	bool bCheckedFastBattle = injector.getEnableHash(util::kFastBattleEnable);
	bChecked = injector.getEnableHash(util::kAutoBattleEnable) || bCheckedFastBattle;
	if (bChecked)
	{
		flagBattleDialogEnable_ = false;
		injector.postMessage(kEnableBattleDialog, false, NULL);
	}
	else if (!bChecked && !flagBattleDialogEnable_)
	{
		flagBattleDialogEnable_ = true;
		injector.postMessage(kEnableBattleDialog, true, NULL);
	}

	//快速戰鬥，異步阻止戰鬥封包
	qint64 W = injector.server->getWorldStatus();
	if (bCheckedFastBattle && W == 9) //如果有開啟快速戰鬥，且畫面不在戰鬥中
	{
		injector.postMessage(kSetBlockPacket, true, NULL);
	}
	else
	{
		injector.postMessage(kSetBlockPacket, false, NULL);
	}
}

//檢查開關 (隊伍、交易、名片...等等)
void MainObject::checkEtcFlag()
{
	Injector& injector = Injector::getInstance(getIndex());
	if (injector.server.isNull())
		return;

	qint64 flg = injector.server->getPC().etcFlag;
	bool hasChange = false;
	auto toBool = [flg](qint64 f)->bool
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
		injector.server->setSwitcher(flg);
}

//自動疊加
void MainObject::checkAutoSortItem()
{
	Injector& injector = Injector::getInstance(getIndex());
	if (injector.server.isNull())
		return;

	if (injector.getEnableHash(util::kAutoStackEnable))
	{
		if (autosortitem_future_.isRunning())
			return;

		autosortitem_future_cancel_flag_.store(false, std::memory_order_release);

		autosortitem_future_ = QtConcurrent::run([&injector, this]()
			{
				qint64	i = 0;
				constexpr qint64 duration = 30;

				for (;;)
				{
					for (i = 0; i < duration; ++i)
					{
						if (isInterruptionRequested())
							return;

						if (injector.server.isNull())
							return;

						if (autosortitem_future_cancel_flag_.load(std::memory_order_acquire))
							return;

						QThread::msleep(100);
					}

					injector.server->sortItem();
				}
			});
	}
	else
	{
		if (autosortitem_future_.isRunning())
		{
			autosortitem_future_cancel_flag_.store(true, std::memory_order_release);
			autosortitem_future_.cancel();
			autosortitem_future_.waitForFinished();
		}
	}
}

//走路遇敵
void MainObject::checkAutoWalk()
{
	Injector& injector = Injector::getInstance(getIndex());
	if (injector.server.isNull())
		return;

	if (injector.getEnableHash(util::kAutoWalkEnable) || injector.getEnableHash(util::kFastAutoWalkEnable))
	{
		//如果線程已經在執行就返回
		if (autowalk_future_.isRunning())
			return;

		//重置停止標誌
		autowalk_future_cancel_flag_.store(false, std::memory_order_release);
		//紀錄當前人物座標
		QPoint current_pos = injector.server->getPoint();

		autowalk_future_ = QtConcurrent::run([&injector, current_pos, this]()
			{
				bool current_side = false;

				for (;;)
				{
					//如果主線程關閉則自動退出
					if (isInterruptionRequested())
						return;

					//如果停止標誌為真則自動退出
					if (autowalk_future_cancel_flag_.load(std::memory_order_acquire))
						return;

					//取設置
					bool enableAutoWalk = injector.getEnableHash(util::kAutoWalkEnable);//走路遇敵開關
					bool enableFastAutoWalk = injector.getEnableHash(util::kFastAutoWalkEnable);//快速遇敵開關
					if (!enableAutoWalk && !enableFastAutoWalk)
						return;

					//如果人物不在線上則自動退出
					if (!injector.server->getOnlineFlag())
						return;

					//如果人物在戰鬥中則進入循環等待
					if (injector.server->getBattleFlag())
					{
						//先等一小段時間
						QThread::msleep(100);

						//如果已經退出戰鬥就等待1.5秒避免太快開始移動不夠時間吃肉補血丟東西...等
						if (!injector.server->getBattleFlag())
						{
							for (qint64 i = 0; i < 5; ++i)
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

					qint64 walk_speed = injector.getValueHash(util::kAutoWalkDelayValue);//走路速度

					//走路遇敵
					if (enableAutoWalk)
					{
						qint64 walk_len = injector.getValueHash(util::kAutoWalkDistanceValue);//走路距離
						qint64 walk_dir = injector.getValueHash(util::kAutoWalkDirectionValue);//走路方向

						//如果direction是0，則current_pos +- x，否則 +- y 如果是2 則隨機加減
						//一次性移動walk_len格

						qint64 x = 0, y = 0;
						if (walk_dir == 0)
						{
							if (current_side)
								x = current_pos.x() + walk_len + 1;
							else
								x = current_pos.x() - walk_len - 1;

							y = current_pos.y();
						}
						else if (walk_dir == 1)
						{
							x = current_pos.x();

							if (current_side)
								y = current_pos.y() + walk_len + 1;
							else
								y = current_pos.y() - walk_len - 1;
						}
						else
						{
							//取隨機數
							std::random_device rd;
							std::mt19937_64 gen(rd());
							std::uniform_int_distribution<qint64> distributionX(current_pos.x() - walk_len, current_pos.x() + walk_len);
							std::uniform_int_distribution<qint64> distributionY(current_pos.x() - walk_len, current_pos.x() + walk_len);
							x = distributionX(gen);
							y = distributionY(gen);
						}

						//每次循環切換方向
						if (current_side)
							current_side = false;
						else
							current_side = true;

						//移動
						injector.server->move(QPoint(x, y));
					}
					else if (enableFastAutoWalk) //快速遇敵 (封包)
						injector.server->move(QPoint(0, 0), "gcgc");
					QThread::msleep(walk_speed + 1);//避免太快無論如何都+15ms (太快並不會遇比較快)
				}
			});
	}
	else
	{
		//如果線程正在執行就取消
		if (autowalk_future_.isRunning())
		{
			autowalk_future_cancel_flag_.store(true, std::memory_order_release);
			autowalk_future_.cancel();
			autowalk_future_.waitForFinished();
		}
	}
}

//自動組隊
void MainObject::checkAutoJoin()
{
	Injector& injector = Injector::getInstance(getIndex());
	if (injector.server.isNull())
		return;

	if (injector.getEnableHash(util::kAutoJoinEnable) &&
		(!injector.getEnableHash(util::kAutoWalkEnable) && !injector.getEnableHash(util::kFastAutoWalkEnable)))
	{
		//如果線程已經在執行就返回
		if (autojoin_future_.isRunning())
			return;

		autojoin_future_cancel_flag_.store(false, std::memory_order_release);

		autojoin_future_ = QtConcurrent::run([this]()
			{
				QSet<QPoint> blockList;
				for (;;)
				{
					Injector& injector = Injector::getInstance(getIndex());
					if (injector.server.isNull()) return;
					if (!injector.server->getOnlineFlag()) return;
					if (injector.server->getBattleFlag()) return;
					if (injector.server->getWorldStatus() != 9 || injector.server->getGameStatus() != 3) return;

					PC ch = injector.server->getPC();

					if (injector.getEnableHash(util::kAutoWalkEnable) || injector.getEnableHash(util::kFastAutoWalkEnable))
						return;

					if (!injector.getEnableHash(util::kAutoJoinEnable))
						return;

					QString leader = injector.getStringHash(util::kAutoFunNameString);

					if (leader.isEmpty())
						return;

					qint64 actionType = injector.getValueHash(util::kAutoFunTypeValue);
					if (actionType == 0)
					{
						if ((ch.status & CHR_STATUS_LEADER) || (ch.status & CHR_STATUS_PARTY))
						{
							QThread::msleep(500);
							bool ok = false;

							QString name = injector.server->getParty(0).name;
							if (!name.isEmpty() && leader.contains(name))
							{
								return;
							}

							injector.server->setTeamState(false);
							QThread::msleep(100);
						}
					}

					constexpr qint64 MAX_SINGLE_STEP = 3;
					map_t map;
					std::vector<QPoint> path;
					QPoint current_point;
					QPoint newpoint;
					mapunit_t unit = {};
					qint64 dir = -1;
					qint64 floor = injector.server->getFloor();
					qint64 len = MAX_SINGLE_STEP;
					qint64 size = 0;
					CAStar astar;

					for (;;)
					{
						//如果主線程關閉則自動退出
						if (isInterruptionRequested())
							return;

						//如果停止標誌為真則自動退出
						if (autojoin_future_cancel_flag_.load(std::memory_order_acquire))
							return;

						if (injector.server.isNull())
							return;

						//如果人物不在線上則自動退出
						if (!injector.server->getOnlineFlag())
							return;

						if (injector.server->getBattleFlag())
							return;

						leader = injector.getStringHash(util::kAutoFunNameString);

						if (leader.isEmpty())
							return;

						ch = injector.server->getPC();
						if (leader == ch.name)
							return;

						if (injector.server->getWorldStatus() != 9 || injector.server->getGameStatus() != 3)
							return;

						if (!floor)
							return;

						if (floor != injector.server->getFloor())
							return;

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
						if (!injector.server->findUnit(leader, util::OBJ_HUMAN, &unit, freeName))
							return;

						//如果和目標人物處於同一個坐標則向隨機方向移動一格
						current_point = injector.server->getPoint();
						if (current_point == unit.p)
						{
							injector.server->move(current_point + util::fix_point.value(QRandomGenerator::global()->bounded(0, 7)));
							continue;
						}

						//計算最短離靠近目標人物的坐標和面相的方向
						dir = injector.server->mapAnalyzer->calcBestFollowPointByDstPoint(astar, floor, current_point, unit.p, &newpoint, false, -1);
						if (-1 == dir)
							break;

						if (current_point == newpoint)
							break;

						if (current_point != newpoint)
						{
							if (!injector.server->mapAnalyzer->getMapDataByFloor(floor, &map))
							{
								injector.server->mapAnalyzer->readFromBinary(floor, injector.server->getFloorName(), false);
								continue;
							}

							if (!injector.server->mapAnalyzer->calcNewRoute(astar, floor, current_point, newpoint, blockList, &path))
								return;

							len = MAX_SINGLE_STEP;
							size = static_cast<qint64>(path.size()) - 1;

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

							if (len >= static_cast<qint64>(path.size()))
								break;

							injector.server->move(path.at(len));
						}
						else
							break;
					}

					if (leader.isEmpty())
						continue;

					actionType = injector.getValueHash(util::kAutoFunTypeValue);
					if (actionType == 0)
					{
						injector.server->setCharFaceDirection(dir);
						injector.server->setTeamState(true);
						continue;
					}
				}
			}
		);
	}
	else
	{
		//如果線程正在執行就取消
		if (autojoin_future_.isRunning())
		{
			autojoin_future_cancel_flag_.store(true, std::memory_order_release);
			autojoin_future_.cancel();
			autojoin_future_.waitForFinished();
		}
	}
}

//自動補血
void MainObject::checkAutoHeal()
{
	Injector& injector = Injector::getInstance(getIndex());
	if (injector.server.isNull())
		return;

	if (autoheal_future_.isRunning())
		return;

	autoheal_future_cancel_flag_.store(false, std::memory_order_release);

	if (injector.getEnableHash(util::kNormalItemHealMpEnable)
		|| injector.getEnableHash(util::kNormalItemHealEnable)
		|| injector.getEnableHash(util::kNormalMagicHealEnable))
	{
		autoheal_future_ = QtConcurrent::run([this]()->void
			{
				Injector& injector = Injector::getInstance(getIndex());

				QStringList items;
				qint64 itemIndex = -1;

				auto checkStatus = [this, &injector]()->bool
				{
					//如果主線程關閉則自動退出
					if (isInterruptionRequested())
						return false;

					//如果停止標誌為真則自動退出
					if (autoheal_future_cancel_flag_.load(std::memory_order_acquire))
						return false;

					if (injector.server.isNull())
						return false;

					if (!injector.server->getOnlineFlag())
						return false;

					if (injector.server->getBattleFlag())
						return false;

					return true;
				};

				//平時道具補氣
				for (;;)
				{
					if (!checkStatus())
						return;

					bool itemHealMpEnable = injector.getEnableHash(util::kNormalItemHealMpEnable);
					if (!itemHealMpEnable)
						break;

					qint64 cmpvalue = injector.getValueHash(util::kNormalItemHealMpValue);
					if (!injector.server->checkCharMp(cmpvalue))
						break;

					QString text = injector.getStringHash(util::kNormalItemHealMpItemString).simplified();
					if (text.isEmpty())
						break;

					items = text.split(util::rexOR, Qt::SkipEmptyParts);
					if (items.isEmpty())
						break;

					itemIndex = -1;
					for (const QString& str : items)
					{
						itemIndex = injector.server->getItemIndexByName(str);
						if (itemIndex != -1)
							break;
					}

					if (itemIndex == -1)
						break;

					injector.server->useItem(itemIndex, 0);
					QThread::msleep(300);
				}

				//平時道具補血
				for (;;)
				{
					if (!checkStatus())
						return;

					bool itemHealHpEnable = injector.getEnableHash(util::kNormalItemHealEnable);
					if (!itemHealHpEnable)
						break;

					qint64 charPercent = injector.getValueHash(util::kNormalItemHealCharValue);
					qint64 petPercent = injector.getValueHash(util::kNormalItemHealPetValue);
					qint64 alliePercent = injector.getValueHash(util::kNormalItemHealAllieValue);

					if (charPercent == 0 && petPercent == 0 && alliePercent == 0)
						break;

					qint64 target = -1;
					bool ok = false;
					if ((charPercent > 0) && injector.server->checkCharHp(charPercent))
					{
						ok = true;
						target = 0;
					}
					else if (!ok && (petPercent > 0) && injector.server->checkPetHp(petPercent))
					{
						ok = true;
						target = injector.server->getPC().battlePetNo + 1;
					}
					else if (!ok && (petPercent > 0) && injector.server->checkRideHp(petPercent))
					{
						ok = true;
						target = injector.server->getPC().ridePetNo + 1;
					}
					else if (!ok && (alliePercent > 0) && injector.server->checkPartyHp(alliePercent, &target))
					{
						ok = true;
						target += MAX_PET;
					}

					if (!ok || target == -1)
						break;

					itemIndex = -1;
					bool meatProiory = injector.getEnableHash(util::kNormalItemHealMeatPriorityEnable);
					if (meatProiory)
					{
						itemIndex = injector.server->getItemIndexByName("?肉", false, "耐久力");
					}

					if (itemIndex == -1)
					{
						QString text = injector.getStringHash(util::kNormalItemHealItemString).simplified();

						items = text.split(util::rexOR, Qt::SkipEmptyParts);
						for (const QString& str : items)
						{
							itemIndex = injector.server->getItemIndexByName(str);
							if (itemIndex != -1)
								break;
						}
					}

					if (itemIndex == -1)
						break;

					qint64 targetType = injector.server->getItem(itemIndex).target;
					if ((targetType != ITEM_TARGET_MYSELF) && (targetType != ITEM_TARGET_OTHER))
						break;

					if ((targetType == ITEM_TARGET_MYSELF) && (target != 0))
						break;

					injector.server->useItem(itemIndex, target);
					QThread::msleep(300);
				}

				//平時精靈補血
				for (;;)
				{
					if (!checkStatus())
						return;

					bool magicHealHpEnable = injector.getEnableHash(util::kNormalMagicHealEnable);
					if (!magicHealHpEnable)
						break;

					qint64 charPercent = injector.getValueHash(util::kNormalMagicHealCharValue);
					qint64 petPercent = injector.getValueHash(util::kNormalMagicHealPetValue);
					qint64 alliePercent = injector.getValueHash(util::kNormalMagicHealAllieValue);

					if (charPercent == 0 && petPercent == 0 && alliePercent == 0)
						break;


					qint64 target = -1;
					bool ok = false;
					if ((charPercent > 0) && injector.server->checkCharHp(charPercent))
					{
						ok = true;
						target = 0;
					}
					else if (!ok && (petPercent > 0) && injector.server->checkPetHp(petPercent))
					{
						ok = true;
						target = injector.server->getPC().battlePetNo + 1;
					}
					else if (!ok && (petPercent > 0) && injector.server->checkRideHp(petPercent))
					{
						ok = true;
						target = injector.server->getPC().ridePetNo + 1;
					}
					else if (!ok && (alliePercent > 0) && injector.server->checkPartyHp(alliePercent, &target))
					{
						ok = true;
						target += MAX_PET;
					}

					if (!ok || target == -1)
						break;

					qint64 magicIndex = injector.getValueHash(util::kNormalMagicHealMagicValue);
					if (magicIndex < 0 || magicIndex >= MAX_ITEM)
						break;

					if (magicIndex < CHAR_EQUIPPLACENUM)
					{
						qint64 targetType = injector.server->getMagic(magicIndex).target;
						if ((targetType != MAGIC_TARGET_MYSELF) && (targetType != MAGIC_TARGET_OTHER))
							break;

						if ((targetType == MAGIC_TARGET_MYSELF) && (target != 0))
							break;
					}

					injector.server->useMagic(magicIndex, target);
					QThread::msleep(300);
				}
			}
		);
	}
	else
	{
		//如果線程正在執行就取消
		if (autoheal_future_.isRunning())
		{
			autoheal_future_cancel_flag_.store(true, std::memory_order_release);
			autoheal_future_.cancel();
			autoheal_future_.waitForFinished();
		}
	}
}

//自動丟寵
void MainObject::checkAutoDropPet()
{
	Injector& injector = Injector::getInstance(getIndex());
	if (injector.server.isNull())
		return;

	if (autodroppet_future_.isRunning())
		return;

	autodroppet_future_cancel_flag_.store(false, std::memory_order_release);

	if (injector.getEnableHash(util::kDropPetEnable))
	{
		autodroppet_future_ = QtConcurrent::run([this]()->void
			{
				Injector& injector = Injector::getInstance(getIndex());
				auto checkStatus = [this, &injector]()->bool
				{
					//如果主線程關閉則自動退出
					if (isInterruptionRequested())
						return false;

					//如果停止標誌為真則自動退出
					if (autoheal_future_cancel_flag_.load(std::memory_order_acquire))
						return false;

					if (injector.server.isNull())
						return false;

					if (!injector.server->getOnlineFlag())
						return false;

					if (injector.server->getBattleFlag())
						return false;

					return true;
				};

				bool strLowAtEnable = injector.getEnableHash(util::kDropPetStrEnable);
				bool defLowAtEnable = injector.getEnableHash(util::kDropPetDefEnable);
				bool agiLowAtEnable = injector.getEnableHash(util::kDropPetAgiEnable);
				bool aggregateLowAtEnable = injector.getEnableHash(util::kDropPetAggregateEnable);
				double strLowAtValue = injector.getValueHash(util::kDropPetStrValue);
				double defLowAtValue = injector.getValueHash(util::kDropPetDefValue);
				double agiLowAtValue = injector.getValueHash(util::kDropPetAgiValue);
				double aggregateLowAtValue = injector.getValueHash(util::kDropPetAggregateValue);
				QString text = injector.getStringHash(util::kDropPetNameString);
				QStringList nameList;
				if (!text.isEmpty())
					nameList = text.split(util::rexOR, Qt::SkipEmptyParts);

				for (qint64 i = 0; i < MAX_PET; ++i)
				{
					if (!checkStatus())
						return;

					PET pet = injector.server->getPet(i);
					if (!pet.valid || pet.maxHp <= 0 || pet.level <= 0)
						continue;

					double str = pet.atk;
					double def = pet.def;
					double agi = pet.agi;
					double aggregate = ((str + def + agi + (static_cast<double>(pet.maxHp) / 4.0)) / static_cast<double>(pet.level)) * 100.0;

					bool okDrop = false;
					if (strLowAtEnable && (str < strLowAtValue))
					{
						okDrop = true;
					}
					else if (defLowAtEnable && (def < defLowAtValue))
					{
						okDrop = true;
					}
					else if (agiLowAtEnable && (agi < agiLowAtValue))
					{
						okDrop = true;
					}
					else if (aggregateLowAtEnable && (aggregate < aggregateLowAtValue))
					{
						okDrop = true;
					}
					else
					{
						okDrop = false;
					}


					if (okDrop)
					{
						if (!nameList.isEmpty())
						{
							bool isExact = true;
							okDrop = false;
							for (const QString& it : nameList)
							{
								QString newName = it.simplified();
								if (newName.startsWith("?"))
								{
									isExact = false;
									newName = newName.mid(1);
								}

								if (isExact && pet.name == newName)
								{
									okDrop = true;
									break;
								}
								else if (isExact && pet.name.contains(newName))
								{
									okDrop = true;
									break;
								}
							}
						}

						if (okDrop)
							injector.server->dropPet(i);
					}
				}
			}
		);
	}
	else
	{
		//如果線程正在執行就取消
		if (autodroppet_future_.isRunning())
		{
			autodroppet_future_cancel_flag_.store(true, std::memory_order_release);
			autodroppet_future_.cancel();
			autodroppet_future_.waitForFinished();
		}
	}
}

//自動丟棄道具
void MainObject::checkAutoDropItems()
{
	Injector& injector = Injector::getInstance(getIndex());
	auto checkEnable = [this, &injector]()->bool
	{
		if (isInterruptionRequested())
			return false;

		if (!injector.getEnableHash(util::kAutoDropEnable))
			return false;

		if (injector.server.isNull())
			return false;

		if (!injector.server->getOnlineFlag())
			return false;

		if (injector.server->getBattleFlag())
			return false;

		return true;
	};

	if (!checkEnable())
		return;

	QString strDropItems = injector.getStringHash(util::kAutoDropItemString);
	if (strDropItems.isEmpty())
		return;

	QStringList dropItems = strDropItems.split(util::rexOR, Qt::SkipEmptyParts);
	if (dropItems.isEmpty())
		return;

	QHash<qint64, ITEM> items = injector.server->getItems();
	for (qint64 i = 0; i < MAX_ITEM; ++i)
	{
		ITEM item = items.value(i);
		if (item.name.isEmpty())
			continue;

		for (const QString& cmpItem : dropItems)
		{
			if (!checkEnable())
				return;

			if (cmpItem.isEmpty())
				continue;

			if (cmpItem.startsWith("?"))//如果丟棄列表名稱前面有?則表示要模糊匹配
			{
				QString newCmpItem = cmpItem.mid(1);//去除問號
				if (item.name.contains(newCmpItem))
				{
					injector.server->dropItem(i);
					continue;
				}
			}
			else if ((item.name == cmpItem))//精確匹配
			{
				injector.server->dropItem(i);
			}
		}
	}
	injector.server->refreshItemInfo();
}

//自動吃經驗加乘道具
void MainObject::checkAutoEatBoostExpItem()
{
	Injector& injector = Injector::getInstance(getIndex());
	auto checkEnable = [this, &injector]()->bool
	{
		if (isInterruptionRequested())
			return false;

		if (!injector.getEnableHash(util::kAutoEatBeanEnable))
			return false;

		if (injector.server.isNull())
			return false;

		if (!injector.server->getOnlineFlag())
			return false;

		if (injector.server->getBattleFlag())
			return false;

		return true;
	};

	if (!checkEnable())
		return;


	QHash<qint64, ITEM> items = injector.server->getItems();
	for (qint64 i = 0; i < MAX_ITEM; ++i)
	{
		if (!checkEnable())
			return;

		ITEM item = items.value(i);
		if (item.name.isEmpty() || item.memo.isEmpty() || !item.valid)
			continue;

		if (item.memo.contains("經驗值上升") || item.memo.contains("经验值上升"))
		{
			if (injector.server.isNull())
				return;
			injector.server->useItem(i, 0);
		}
	}
}

//檢查可記錄的NPC坐標訊息
void MainObject::checkRecordableNpcInfo()
{
	Injector& injector = Injector::getInstance(getIndex());
	if (injector.server.isNull())
		return;

	if (pointerwriter_future_.isRunning())
		return;

	pointerwriter_future_cancel_flag_ = false;

	pointerwriter_future_ = QtConcurrent::run([this]()
		{
			for (;;)
			{
				QThread::msleep(1000);
				Injector& injector = Injector::getInstance(getIndex());
				if (injector.server.isNull())
					return;

				if (isInterruptionRequested())
					return;

				if (pointerwriter_future_cancel_flag_.load(std::memory_order_acquire))
					return;

				if (injector.server.isNull())
					return;

				CAStar astar;

				QHash<qint64, mapunit_t> units = injector.server->mapUnitHash.toHash();
				util::Config config(injector.getPointFileName());

				for (const mapunit_t& unit : units)
				{
					if (isInterruptionRequested())
						return;

					if (pointerwriter_future_cancel_flag_.load(std::memory_order_acquire))
						return;

					if (injector.server.isNull())
						return;

					if (!injector.server->getOnlineFlag())
						break;

					if ((unit.objType != util::OBJ_NPC)
						|| unit.name.isEmpty()
						|| (injector.server->getWorldStatus() != 9)
						|| (injector.server->getGameStatus() != 3)
						|| injector.server->npcUnitPointHash.contains(QPoint(unit.x, unit.y)))
					{
						continue;
					}

					injector.server->npcUnitPointHash.insert(QPoint(unit.x, unit.y), unit);

					util::MapData d;
					qint64 nowFloor = injector.server->nowFloor_.load(std::memory_order_acquire);
					QPoint nowPoint = injector.server->nowPoint_.get();

					d.floor = nowFloor;
					d.name = unit.name;

					//npc前方一格
					QPoint newPoint = util::fix_point.value(unit.dir) + unit.p;
					//檢查是否可走
					if (injector.server->mapAnalyzer->isPassable(astar, nowFloor, nowPoint, newPoint))
					{
						d.x = newPoint.x();
						d.y = newPoint.y();
					}
					else
					{
						//再往前一格
						QPoint additionPoint = util::fix_point.value(unit.dir) + newPoint;
						//檢查是否可走
						if (injector.server->mapAnalyzer->isPassable(astar, nowFloor, nowPoint, additionPoint))
						{
							d.x = additionPoint.x();
							d.y = additionPoint.y();
						}
						else
						{
							//檢查NPC周圍8格
							bool flag = false;
							for (qint64 i = 0; i < 8; ++i)
							{
								newPoint = util::fix_point.value(i) + unit.p;
								if (injector.server->mapAnalyzer->isPassable(astar, nowFloor, nowPoint, newPoint))
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
					const qint64 floor = entranceData.value(0).toLongLong(&ok);
					if (!ok)
						continue;

					const QString pointStr(entranceData.value(1));
					const QStringList pointData(pointStr.split(util::rexComma));
					if (pointData.size() != 2)
						continue;

					qint64 x = pointData.value(0).toLongLong(&ok);
					if (!ok)
						continue;

					qint64 y = pointData.value(1).toLongLong(&ok);
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

					injector.server->npcUnitPointHash.insert(pos, unit);

					config.writeMapData(name, d);
				}
			}
		}
	);
}

#if 0
//自動鎖寵排程
void MainObject::checkAutoLockSchedule()
{
	Injector& injector = Injector::getInstance(getIndex());
	if (injector.server.isNull())
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
		qint64 rindex = -1;
		qint64 bindex = -1;
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
					qint64 petIndex = -1;
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
					qint64 level = levelStr.toLongLong(&ok);
					if (!ok)
						continue;

					if (level < 0 || level > 255)
						continue;

					QString typeStr = args.value(2).simplified();
					if (typeStr.isEmpty())
						continue;

					PetState type = hashType.value(typeStr, kRest);

					PET pet = injector.server->getPet(petIndex);

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
			PET pet = injector.server->getPet(rindex);
			if (pet.hp <= 1)
			{
				injector.server->setPetState(rindex, kRest);
				QThread::msleep(100);
			}

			if (pet.state != kRide)
				injector.server->setPetState(rindex, kRide);
		}

		if (bindex != -1)
		{
			PET pet = injector.server->getPet(bindex);
			if (pet.hp <= 1)
			{
				injector.server->setPetState(bindex, kRest);
				QThread::msleep(100);
			}

			if (pet.state != kBattle)
				injector.server->setPetState(bindex, kBattle);
		}

		for (qint64 i = 0; i < MAX_PET; ++i)
		{
			if (bindex == i || rindex == i)
				continue;

			PET pet = injector.server->getPet(i);
			if ((pet.state != kRest && pet.state != kStandby) && set == util::kLockPetScheduleString)
				injector.server->setPetState(i, kRest);
		}
		return false;
	};

	if (injector.getEnableHash(util::kLockPetScheduleEnable) && !injector.getEnableHash(util::kLockPetEnable) && !injector.getEnableHash(util::kLockRideEnable))
		checkSchedule(util::kLockPetScheduleString);

}
#endif
