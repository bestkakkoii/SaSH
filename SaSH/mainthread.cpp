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

#include <spdloger.hpp>
extern QString g_logger_name;//parser.cpp

bool ThreadManager::createThread(QObject* parent)
{
	if (thread_)
		return false;

	if (object_)
		return false;

	do
	{
		object_ = new MainObject(nullptr);
		if (!object_)
			break;

		thread_ = new QThread(this);
		if (!thread_)
			break;

		object_->moveToThread(thread_);

		connect(thread_, &QThread::started, object_, &MainObject::run, Qt::UniqueConnection);
		//after delete must set nullptr
		connect(object_, &MainObject::finished, this, [this]()
			{
				qDebug() << "recv MainObject::finished, start cleanning";
				thread_->quit();
				thread_->wait();
				delete thread_;
				thread_ = nullptr;
				delete object_;
				object_ = nullptr;
				Injector::clear();
			}, Qt::UniqueConnection);
		thread_->start();
		return true;
	} while (false);

	return false;
}

MainObject::MainObject(QObject* parent)
	: ThreadPlugin(parent)
{
	pointerWriterSync_.setCancelOnWait(true);
}

MainObject::~MainObject()
{
	qDebug() << "MainObject is destroyed!";
}

void MainObject::run()
{

	Injector& injector = Injector::getInstance();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();

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
		//有些時候太快創建遊戲進程會閃退，原因不明，所以延遲一下
		//QThread::msleep(1000);
		//創建遊戲進程
		if (!injector.createProcess(process_info))
		{
			break;
		}

		if (remove_thread_reason != util::REASON_NO_ERROR)
			break;

		//QThread::msleep(1000);

		emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusOpened);

		//注入dll 並通知客戶端要連入的port
		if (!injector.injectLibrary(process_info, injector.server->getPort(), &remove_thread_reason))
		{
			qDebug() << "injectLibrary failed. reason:" << remove_thread_reason;
			break;
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
	emit signalDispatcher.scriptStoped();
	emit signalDispatcher.nodifyAllStop();

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

	pointerWriterSync_.waitForFinished();

	//強制關閉遊戲進程
	injector.close();

	while (injector.IS_SCRIPT_FLAG.load(std::memory_order_acquire))
	{
		QThread::msleep(100);
	}

	emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusNotOpen);
	emit signalDispatcher.setStartButtonEnabled(true);

	//通知線程結束
	emit finished();
	qDebug() << "emit finished()";
}

void MainObject::mainProc()
{
	Injector& injector = Injector::getInstance();
	QElapsedTimer freeMemTimer; freeMemTimer.start();
	//首次先釋放一次記憶體，並且開始計時
	if (injector.getEnableHash(util::kAutoFreeMemoryEnable))
	{
		freeMemTimer.restart();
		mem::freeUnuseMemory(injector.getProcess());
	}

	for (;;)
	{
#ifdef _DEBUG
		QThread::msleep(300);
#else
		QThread::msleep(300);
#endif
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
		if (!injector.server->IS_TCP_CONNECTION_OK_TO_USE)
		{
			continue;
		}

		//自動釋放記憶體
		if (injector.getEnableHash(util::kAutoFreeMemoryEnable) && freeMemTimer.hasExpired(10ll * 60ll * 1000ll))
		{
			freeMemTimer.restart();
			mem::freeUnuseMemory(injector.getProcess());
		}
		else
			freeMemTimer.restart();

		//有些數據需要和客戶端內存同步
		injector.server->updateDatasFromMemory();

		//其他所有功能
		int status = checkAndRunFunctions();

		//這裡是預留的暫時沒作用
		if (status == 1)//非登入狀態 或 平時
		{

		}
		else if (status == 2)//戰鬥中不延時
		{
		}
		else//錯誤
		{
			break;
		}
		QThread::yieldCurrentThread();
	}
}

int MainObject::checkAndRunFunctions()
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return 0;

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();

	int status = injector.server->getUnloginStatus();

	if (status == util::kStatusDisappear)
	{
		return 0;
	}
	else if (status == util::kStatusInputUser ||
		status == util::kStatusSelectServer ||
		status == util::kStatusSelectSubServer ||
		status == util::kStatusSelectCharacter ||
		status == util::kStatusDisconnect ||
		status == util::kStatusTimeout ||
		status == util::kStatusBusy ||
		status == util::kStatusConnecting ||
		status == util::kNoUserNameOrPassword ||
		!injector.server->getOnlineFlag())
	{
		//每次登出後只會執行一次
		if (!login_run_once_flag_)
		{
			login_run_once_flag_ = true;

			SPD_CLOSE(injector.server->protoBattleLogName.toStdString());


			for (int i = 0; i < 15; ++i)
			{
				if (isInterruptionRequested())
					return 0;
				QThread::msleep(100);
			}
			injector.server->clear();
			if (!injector.chatLogModel.isNull())
				injector.chatLogModel->clear();
		}

		injector.server->loginTimer.restart();
		//自動登入 或 斷線重連
		if (injector.getEnableHash(util::kAutoLoginEnable) || injector.getEnableHash(util::kAutoReconnectEnable))
		{
			if (injector.server->login(status))
				QThread::msleep(200);
		}
		return 1;
	}

	//每次登入後只會執行一次
	if (login_run_once_flag_)
	{
		login_run_once_flag_ = false;
		for (int i = 0; i < 50; ++i)
		{
			if (isInterruptionRequested())
				return 0;

			if (injector.server.isNull())
				return 0;

			if (injector.server->getWorldStatus() == 9 && injector.server->getGameStatus() == 3)
				break;

			QThread::msleep(100);
		}

		injector.server->EO();
		emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusLoginSuccess);
		emit signalDispatcher.updateMainFormTitle(injector.server->getPC().name);

		//登入後的廣告公告
		constexpr bool isbeta = true;
		QDateTime current = QDateTime::currentDateTime();
		QDateTime due = current.addYears(99);
		const QString dueStr(due.toString("yyyy-MM-dd hh:mm:ss"));

		const QString url("");//https://bbs.shiqi.so https://www.lovesa.cc

		const QString version = QString("%1.%2.%3")
			.arg(SASH_VERSION_MAJOR) \
			.arg(SASH_VERSION_MINOR) \
			.arg(util::buildDateTime(nullptr));
		injector.server->announce(tr("Welcome to use SaSH，For more information please visit %1").arg(url));
		injector.server->announce(tr("You are using %1 account, due date is:%2").arg(isbeta ? tr("trial") : tr("subscribed")).arg(dueStr));
		injector.server->announce(tr("StoneAge SaSH forum url:%1, newest version is %2").arg(url).arg(version));

		//自動解鎖安全碼
		QString securityCode = injector.getStringHash(util::kGameSecurityCodeString);
		if (!securityCode.isEmpty())
		{
			injector.server->unlockSecurityCode(securityCode);
		}

		injector.server->loginTimer.restart();
		PC pc = injector.server->getPC();
		util::AfkRecorder recorder;
		recorder.levelrecord = pc.level;
		recorder.exprecord = pc.exp;
		recorder.goldearn = 0;
		recorder.deadthcount = 0;
		injector.server->recorder[0] = recorder;

		for (int i = 0; i < MAX_PET; ++i)
		{
			PET pet = injector.server->getPet(i);
			recorder = {};
			recorder.levelrecord = pet.level;
			recorder.exprecord = pet.exp;
			recorder.deadthcount = 0;
			injector.server->recorder[i + 1] = recorder;
		}

		const QString fileName(qgetenv("JSON_PATH"));
		QStringList list;
		extern int g_CurrentListIndex;//generalform.cpp

		{
			util::Config config(fileName);
			list = config.readStringArray("System", "Server", QString("List_%1").arg(g_CurrentListIndex));
		}

		QStringList serverNameList;
		QStringList subServerNameList;
		for (const QString& it : list)
		{
			QStringList subList = it.split(util::rexOR, Qt::SkipEmptyParts);
			if (subList.isEmpty())
				continue;

			if (subList.size() != 2)
				continue;

			QString server = subList.takeFirst();

			subList = subList.first().split(util::rexComma, Qt::SkipEmptyParts);
			if (subList.isEmpty())
				continue;

			serverNameList.append(server);
			subServerNameList.append(subList);
		}
		Injector& injector = Injector::getInstance();
		injector.serverNameList = serverNameList;
		injector.subServerNameList = subServerNameList;

		emit signalDispatcher.updateNpcList(injector.server->nowFloor);
		emit signalDispatcher.applyHashSettingsToUI();

		if (injector.getEnableHash(util::kScriptDebugModeEnable))
		{
			QString logname = QString("battle_%1_%2_%3").arg(pc.name).arg(pc.freeName).arg(_getpid());
			injector.server->protoBattleLogName = SPD_INIT(logname);
		}

		return 1;
	}

	updateAfkInfos();

	//更新數據緩存(跨線程安全容器)
	setUserDatas();

	//檢查UI的設定是否有變化
	checkControl();

	//走路遇敵 或 快速遇敵 (封包)
	checkAutoWalk();

	//平時
	if (!injector.server->getBattleFlag())
	{
		//每次進入平時只會執行一次
		if (!battle_run_once_flag_)
		{
			battle_run_once_flag_ = true;
			emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusInNormal);

		}

		//紀錄NPC
		checkRecordableNpcInfo();

		//檢查開關 (隊伍、交易、名片...等等)
		checkEtcFlag();

		//自動組隊、跟隨
		checkAutoJoin();

		//自動補血、氣
		checkAutoHeal();

		//自動丟寵
		checkAutoDropPet();

		//檢查自動丟棄道具
		checkAutoDropItems();

		checkAutoDropMeat(QStringList());

		//檢查自動吃道具
		checkAutoEatBoostExpItem();

		//自動鎖寵
		checkAutoLockPet();

		//鎖寵排程
		checkAutoLockSchedule();

		injector.server->sortItem();
		return 1;
	}
	else //戰鬥中
	{
		//每次進入戰鬥只會執行一次
		if (battle_run_once_flag_)
		{
			battle_run_once_flag_ = false;
			emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusInBattle);
		}

		//異步處理戰鬥時間刷新
		if (!battleTime_future_.isRunning())
		{
			battleTime_future_cancel_flag_.store(false, std::memory_order_release);
			battleTime_future_ = QtConcurrent::run(this, &MainObject::battleTimeThread);
		}

		return 2;
	}

	return 1;
}

void MainObject::updateAfkInfos()
{
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return;

	int duration = injector.server->loginTimer.elapsed();
	signalDispatcher.updateAfkInfoTable(0, util::formatMilliseconds(duration));

	util::AfkRecorder recorder = injector.server->recorder[0];

	int avgLevelPerHour = 0;
	if (duration > 0 && recorder.leveldifference > 0)
		avgLevelPerHour = static_cast<int>(recorder.leveldifference * 3600000.0 / duration);
	signalDispatcher.updateAfkInfoTable(2, QString(tr("%1→%2 (avg level: %3)"))
		.arg(recorder.levelrecord).arg(recorder.levelrecord + recorder.leveldifference).arg(avgLevelPerHour));

	int avgExpPerHour = 0;
	if (duration > 0 && recorder.expdifference > 0)
		avgExpPerHour = static_cast<int>(recorder.expdifference * 3600000.0 / duration);

	signalDispatcher.updateAfkInfoTable(3, tr("%1 (avg exp: %2)").arg(recorder.expdifference).arg(avgExpPerHour));

	signalDispatcher.updateAfkInfoTable(4, QString::number(recorder.deadthcount));


	int avgGoldPerHour = 0;
	if (duration > 0 && recorder.goldearn > 0)
		avgGoldPerHour = static_cast<int>(recorder.goldearn * 3600000.0 / duration);
	signalDispatcher.updateAfkInfoTable(5, tr("%1 (avg gold: %2)").arg(recorder.goldearn).arg(avgGoldPerHour));

	constexpr int n = 7;
	int j = 0;
	for (int i = 0; i < MAX_PET; ++i)
	{
		recorder = injector.server->recorder[i + 1];

		avgExpPerHour = 0;
		if (duration > 0 && recorder.leveldifference > 0)
			avgExpPerHour = static_cast<int>(recorder.leveldifference * 3600000.0 / duration);

		signalDispatcher.updateAfkInfoTable(i + n + j, QString(tr("%1→%2 (avg level: %3)"))
			.arg(recorder.levelrecord).arg(recorder.levelrecord + recorder.leveldifference).arg(avgExpPerHour));

		avgExpPerHour = 0;
		if (duration > 0 && recorder.expdifference > 0)
			avgExpPerHour = static_cast<int>(recorder.expdifference * 3600000.0 / duration);
		signalDispatcher.updateAfkInfoTable(i + n + 1 + j, tr("%1 (avg exp: %2)").arg(recorder.expdifference).arg(avgExpPerHour));

		signalDispatcher.updateAfkInfoTable(i + n + 2 + j, QString::number(recorder.deadthcount));


		signalDispatcher.updateAfkInfoTable(i + n + 3 + j, "");

		j += 3;

	}
}

void MainObject::battleTimeThread()
{
	Injector& injector = Injector::getInstance();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	for (;;)
	{
		if (battleTime_future_cancel_flag_.load(std::memory_order_acquire))
			break;

		if (injector.server.isNull())
			break;

		if (!injector.server->getOnlineFlag())
			break;

		if (!injector.server->getBattleFlag())
			break;

		if (injector.server->battleDurationTimer.hasExpired(60ll * 1000ll * 5ll))
		{
			injector.server->setBattleEnd();
			injector.server->announce(tr("<error>battle time out"));
			break;
		}

		//刷新要顯示的戰鬥時間和相關數據
		double time = injector.server->battleDurationTimer.elapsed() / 1000.0;
		QString battle_time_text = QString(tr("%1 count    no %2 round    duration: %3 sec    total time: %4 minues"))
			.arg(injector.server->battle_totol)
			.arg(injector.server->BattleCliTurnNo + 1)
			.arg(QString::number(time, 'f', 3))
			.arg(injector.server->battle_total_time / 1000 / 60);

		if (battle_time_text.isEmpty() || injector.server->timeLabelContents != battle_time_text)
		{
			injector.server->timeLabelContents = battle_time_text;
			emit signalDispatcher.updateTimeLabelContents(battle_time_text);
		}
		QThread::msleep(50);
	}
	battleTime_future_cancel_flag_.store(false, std::memory_order_release);
}

//將一些數據保存到多線程安全容器
void MainObject::setUserDatas()
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return;

	QStringList itemNames;
	for (const ITEM& item : injector.server->getPC().item)
	{
		if (item.name.isEmpty())
			continue;

		itemNames.append(item.name);
	}

	injector.setUserData(util::kUserItemNames, itemNames);

	QStringList petNames;

	for (int i = 0; i < MAX_PET; ++i)
	{
		PET pet = injector.server->getPet(i);
		if (pet.name.isEmpty() || pet.useFlag == 0)
			continue;

		petNames.append(pet.name);
	}
	injector.setUserData(util::kUserPetNames, petNames);

	injector.setUserData(util::kUserEnemyNames, injector.server->enemyNameListCache);

}

//根據UI的選擇項變動做的一些操作
void MainObject::checkControl()
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return;

	//隱藏視窗按下，異步隱藏
	bool bChecked = injector.getEnableHash(util::kHideCharacterEnable);
	if (flagHideCharacterEnable_ != bChecked)
	{
		flagHideCharacterEnable_ = bChecked;
		injector.postMessage(Injector::kEnablePlayerShow, !bChecked, NULL);
	}

	if (!injector.server->getOnlineFlag())
		return;

	//登出按下，異步登出
	if (injector.getEnableHash(util::kLogOutEnable))
	{
		injector.server->logOut();
		injector.setEnableHash(util::kLogOutEnable, false);
	}

	//回點按下，異步回點
	if (injector.getEnableHash(util::kLogBackEnable))
	{
		injector.server->logBack();
		injector.setEnableHash(util::kLogBackEnable, false);
	}

	//EO按下，異步發送EO
	if (injector.getEnableHash(util::kEchoEnable))
	{
		injector.server->EO();
		injector.setEnableHash(util::kEchoEnable, false);
	}

	//////////////////////////////

	//異步關閉特效
	bChecked = injector.getEnableHash(util::kCloseEffectEnable);
	if (flagCloseEffectEnable_ != bChecked)
	{
		flagCloseEffectEnable_ = bChecked;
		injector.postMessage(Injector::kEnableEffect, !bChecked, NULL);
	}

	//異步關閉聲音
	bChecked = injector.getEnableHash(util::kMuteEnable);
	if (flagMuteEnable_ != bChecked)
	{
		flagMuteEnable_ = bChecked;
		injector.postMessage(Injector::kEnableSound, !bChecked, NULL);
	}

	//異步鎖定時間
	bChecked = injector.getEnableHash(util::kLockTimeEnable);
	int value = injector.getValueHash(util::kLockTimeValue);
	if (flagLockTimeEnable_ != bChecked || flagLockTimeValue_ != value)
	{
		flagLockTimeEnable_ = bChecked;
		flagLockTimeValue_ = value;
		injector.postMessage(Injector::kSetTimeLock, bChecked, flagLockTimeValue_);
	}

	//異步加速
	value = injector.getValueHash(util::kSpeedBoostValue);
	if (flagSetBoostValue_ != value)
	{
		flagSetBoostValue_ = value;
		injector.postMessage(Injector::kSetBoostSpeed, true, flagSetBoostValue_);
	}

	//異步快速走路
	bChecked = injector.getEnableHash(util::kFastWalkEnable);
	if (flagFastWalkEnable_ != bChecked)
	{
		flagFastWalkEnable_ = bChecked;
		injector.postMessage(Injector::kEnableFastWalk, bChecked, NULL);
	}

	//異步橫衝直撞 (穿牆)
	bChecked = injector.getEnableHash(util::kPassWallEnable);
	if (flagPassWallEnable_ != bChecked)
	{
		flagPassWallEnable_ = bChecked;
		injector.postMessage(Injector::kEnablePassWall, bChecked, NULL);
	}

	//異步鎖定畫面
	bChecked = injector.getEnableHash(util::kLockImageEnable);
	if (flagLockImageEnable_ != bChecked)
	{
		flagLockImageEnable_ = bChecked;
		injector.postMessage(Injector::kEnableImageLock, bChecked, NULL);
	}

	//異步戰鬥99秒
	bChecked = injector.getEnableHash(util::kBattleTimeExtendEnable);
	if (flagBattleTimeExtendEnable_ != bChecked)
	{
		flagBattleTimeExtendEnable_ = bChecked;
		injector.postMessage(Injector::kBattleTimeExtend, bChecked, NULL);
	}

	//異步資源優化
	bChecked = injector.getEnableHash(util::kOptimizeEnable);
	if (flagOptimizeEnable_ != bChecked)
	{
		flagOptimizeEnable_ = bChecked;
		injector.postMessage(Injector::kEnableOptimize, bChecked, NULL);
	}

	////同步鎖定移動
	//bChecked = injector.getEnableHash(util::kLockMoveEnable);
	//bool isFastBattle = injector.getEnableHash(util::kFastBattleEnable);
	//if (!bChecked && isFastBattle && injector.server->getBattleFlag())//如果有開啟快速戰鬥，那必須在戰鬥時鎖定移動
	//{
	//	flagLockMoveEnable_ = true;
	//	injector.sendMessage(Injector::kEnableMoveLock, true, NULL);
	//}
	//else if (!bChecked && isFastBattle && !injector.server->getBattleFlag()) //如果有開啟快速戰鬥，但是不在戰鬥時，那就不鎖定移動
	//{
	//	flagLockMoveEnable_ = false;
	//	injector.sendMessage(Injector::kEnableMoveLock, false, NULL);
	//}
	//else if (flagLockMoveEnable_ != bChecked) //如果沒有開啟快速戰鬥，那就照常
	//{
	//	flagLockMoveEnable_ = bChecked;
	//	injector.sendMessage(Injector::kEnableMoveLock, bChecked, NULL);
	//}

	//自動戰鬥，異步戰鬥面板開關
	bool bCheckedFastBattle = injector.getEnableHash(util::kFastBattleEnable);
	bChecked = injector.getEnableHash(util::kAutoBattleEnable) || bCheckedFastBattle;
	if (bChecked)
	{
		flagBattleDialogEnable_ = false;
		injector.postMessage(Injector::kEnableBattleDialog, false, NULL);
	}
	else if (!bChecked && !flagBattleDialogEnable_)
	{
		flagBattleDialogEnable_ = true;
		injector.postMessage(Injector::kEnableBattleDialog, true, NULL);
	}

	//快速戰鬥，異步阻止戰鬥封包
	int W = injector.server->getWorldStatus();
	if (bCheckedFastBattle && W == 9) //如果有開啟快速戰鬥，且畫面不在戰鬥中
	{
		injector.postMessage(Injector::kSetBlockPacket, true, NULL);
	}
	else
	{
		injector.postMessage(Injector::kSetBlockPacket, false, NULL);
	}
}

//檢查開關 (隊伍、交易、名片...等等)
void MainObject::checkEtcFlag()
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return;

	int flg = injector.server->getPC().etcFlag;
	bool hasChange = false;
	auto toBool = [flg](int f)->bool
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

//走路遇敵
void MainObject::checkAutoWalk()
{
	Injector& injector = Injector::getInstance();
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
						break;

					//如果停止標誌為真則自動退出
					if (autowalk_future_cancel_flag_.load(std::memory_order_acquire))
						break;

					//如果人物不在線上則自動退出
					if (!injector.server->getOnlineFlag())
						break;

					//如果人物在戰鬥中則進入循環等待
					if (injector.server->getBattleFlag())
					{
						//先等一小段時間
						QThread::msleep(100);

						//如果已經退出戰鬥就等待1.5秒避免太快開始移動不夠時間吃肉補血丟東西...等
						//if(!injector.server->getBattleFlag())
						//{
						//	for (int i = 0; i < 2; ++i)
						//	{
						//		//如果主線程關閉則自動退出
						//		if (isInterruptionRequested())
						//			return;
						//		QThread::msleep(100);
						//	}
						//}
						//else
						continue;
					}

					//取設置
					bool enableAutoWalk = injector.getEnableHash(util::kAutoWalkEnable);//走路遇敵開關
					bool enableFastAutoWalk = injector.getEnableHash(util::kFastAutoWalkEnable);//快速遇敵開關
					int walk_speed = injector.getValueHash(util::kAutoWalkDelayValue);//走路速度

					//走路遇敵
					if (enableAutoWalk)
					{
						int walk_len = injector.getValueHash(util::kAutoWalkDistanceValue);//走路距離
						int walk_dir = injector.getValueHash(util::kAutoWalkDirectionValue);//走路方向

						//如果direction是0，則current_pos +- x，否則 +- y 如果是2 則隨機加減
						//一次性移動walk_len格

						int x = 0, y = 0;
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
							QRandomGenerator* random = QRandomGenerator::global();
							x = random->bounded(current_pos.x() - walk_len, current_pos.x() + walk_len);
							y = random->bounded(current_pos.x() - walk_len, current_pos.x() + walk_len);
						}

						//每次循環切換方向
						if (current_side)
							current_side = false;
						else
							current_side = true;

						//移動
						injector.server->move(QPoint(x, y));
						injector.server->move(QPoint(x, y), "gcgc");
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

//自動丟棄道具
void MainObject::checkAutoDropItems()
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return;

	if (!injector.getEnableHash(util::kAutoDropEnable))
		return;

	QString strDropItems = injector.getStringHash(util::kAutoDropItemString);
	if (strDropItems.isEmpty())
		return;

	QStringList dropItems = strDropItems.split(util::rexOR, Qt::SkipEmptyParts);
	if (dropItems.isEmpty())
		return;

	for (int i = 0; i < MAX_ITEM; ++i)
	{
		ITEM item = injector.server->getPC().item[i];
		if (item.name.isEmpty())
			continue;

		for (const QString& cmpItem : dropItems)
		{
			if (isInterruptionRequested())
				break;

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

//檢查並自動吃肉、或丟肉
void MainObject::checkAutoDropMeat(const QStringList& item)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return;

	if (!injector.getEnableHash(util::kAutoDropMeatEnable))
		return;

	bool bret = false;
	constexpr const char* meat = u8"肉";
	constexpr const char* memo = u8"耐久力";

	if (!item.isEmpty())
	{
		for (const QString& it : item)
		{
			QString newItemNmae = it.simplified();
			if (newItemNmae.contains(meat))
			{
				bret = true;
				break;
			}
		}
	}
	else
	{
		PC pc = injector.server->getPC();
		for (const ITEM& it : pc.item)
		{
			QString newItemNmae = it.name.simplified();
			if (!newItemNmae.isEmpty() && newItemNmae.contains(meat))
			{
				bret = true;
				break;
			}
		}
	}

	if (!bret)
		return;

	int index = 0;
	PC pc = injector.server->getPC();
	for (const ITEM& item : pc.item)
	{
		QString newItemNmae = item.name.simplified();
		QString newItemMemo = item.memo.simplified();
		if (newItemNmae.contains(meat))
		{
			if (!newItemMemo.contains(memo) && (newItemNmae != QString(u8"味道爽口的肉湯")) && (newItemNmae != QString(u8"味道爽口的肉汤")))
				injector.server->dropItem(index);
			else
				injector.server->useItem(index, injector.server->findInjuriedAllie());
		}
		++index;
	}

	injector.server->refreshItemInfo();
}

//自動組隊
void MainObject::checkAutoJoin()
{
	Injector& injector = Injector::getInstance();
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

				Injector& injector = Injector::getInstance();
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

				int actionType = injector.getValueHash(util::kAutoFunTypeValue);
				if (actionType == 0)
				{
					if ((ch.status & CHR_STATUS_LEADER) || (ch.status & CHR_STATUS_PARTY))
					{
						QThread::msleep(1000);
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

				constexpr int MAX_SINGLE_STEP = 3;
				map_t map;
				QVector<QPoint> path;
				QPoint current_point;
				QPoint newpoint;
				mapunit_t unit = {};
				int dir = -1;
				int floor = injector.server->nowFloor;
				int len = MAX_SINGLE_STEP;
				int size = 0;

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

					if (floor != injector.server->nowFloor)
						return;

					QString freeName = "";
					if (leader.count("|") == 1)
					{
						QStringList list = leader.split(util::rexOR);
						if (list.size() == 2)
						{
							leader = list.at(0);
							freeName = list.at(1);
						}
					}

					//查找目標人物所在坐標
					if (!injector.server->findUnit(leader, util::OBJ_HUMAN, &unit, freeName))
						return;

					//如果和目標人物處於同一個坐標則向隨機方向移動一格
					current_point = injector.server->getPoint();
					if (current_point == unit.p)
					{
						injector.server->move(current_point + util::fix_point.at(QRandomGenerator::global()->bounded(0, 7)));
						continue;
					}

					//計算最短離靠近目標人物的坐標和面相的方向
					dir = injector.server->mapAnalyzer->calcBestFollowPointByDstPoint(floor, current_point, unit.p, &newpoint, false, -1);
					if (-1 == dir)
						break;

					if (current_point == newpoint)
						break;

					if (current_point != newpoint)
					{
						if (!injector.server->mapAnalyzer->getMapDataByFloor(floor, &map))
						{
							injector.server->mapAnalyzer->readFromBinary(floor, injector.server->nowFloorName, false);
							continue;
						}

						if (!injector.server->mapAnalyzer->calcNewRoute(map, current_point, newpoint, &path))
							return;

						len = MAX_SINGLE_STEP;
						size = path.size() - 1;

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

						if (len >= path.size())
							break;

						injector.server->move(path.at(len));
					}
					else
						break;
				}

				if (!leader.isEmpty())
				{
					actionType = injector.getValueHash(util::kAutoFunTypeValue);
					///_SYS_CloseDialog();
					//_CHAR_ClearTeamJoinableList();
					if (actionType == 0)
					{
						injector.server->setPlayerFaceDirection(dir);
						injector.server->setTeamState(true);
						return;
					}
					/*QThread::msleep(1000UL);
					_CHAR_RefreshData();
					ch = GetCharData();
					if (ch.teamsize < 1)
					{
						QStringList list = _CHAR_GetTeamJoinableList();
						int index = list.indexOf(leader);
						if (index != -1)
						{
							_MAP_SetPlayerFaceDirection(dir);
							_CHAR_JoinTeamByIndex(index);
						}

					}*/
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
	Injector& injector = Injector::getInstance();
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

				Injector& injector = Injector::getInstance();

				QStringList items;
				int itemIndex = -1;

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

					int cmpvalue = injector.getValueHash(util::kNormalItemHealMpValue);
					if (!injector.server->checkPlayerMp(cmpvalue))
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
					QThread::msleep(200);
				}

				//平時道具補血
				for (;;)
				{
					if (!checkStatus())
						return;

					bool itemHealHpEnable = injector.getEnableHash(util::kNormalItemHealEnable);
					if (!itemHealHpEnable)
						break;

					int charPercent = injector.getValueHash(util::kNormalItemHealCharValue);
					int petPercent = injector.getValueHash(util::kNormalItemHealPetValue);
					int alliePercent = injector.getValueHash(util::kNormalItemHealAllieValue);

					if (charPercent == 0 && petPercent == 0 && alliePercent == 0)
						break;

					int target = -1;
					bool ok = false;
					if ((charPercent > 0) && injector.server->checkPlayerHp(charPercent))
					{
						ok = true;
						target = 0;
					}
					if (!ok && (petPercent > 0) && injector.server->checkPetHp(charPercent))
					{
						ok = true;
						target = injector.server->getPC().battlePetNo + 1;
					}
					if (!ok && (alliePercent > 0) && injector.server->checkPartyHp(charPercent, &target))
					{
						ok = true;
						target += MAX_PET;
					}

					if (!ok || target == -1)
						break;

					QString text = injector.getStringHash(util::kNormalItemHealItemString).simplified();
					if (text.isEmpty())
						break;

					items = text.split(util::rexOR, Qt::SkipEmptyParts);
					if (items.isEmpty())
						break;

					itemIndex = -1;
					bool meatProiory = injector.getEnableHash(util::kNormalItemHealMeatPriorityEnable);
					if (meatProiory)
					{
						itemIndex = injector.server->getItemIndexByName(u8"肉", false, u8"耐久力");
					}

					for (const QString& str : items)
					{
						itemIndex = injector.server->getItemIndexByName(str);
						if (itemIndex != -1)
							break;
					}

					if (itemIndex == -1)
						break;

					int targetType = injector.server->getPC().item[itemIndex].target;
					if ((targetType != ITEM_TARGET_MYSELF) && (targetType != ITEM_TARGET_OTHER))
						break;

					if ((targetType == ITEM_TARGET_MYSELF) && (target != 0))
						break;

					injector.server->useItem(itemIndex, target);
					QThread::msleep(200);
				}

				//平時精靈補血
				for (;;)
				{
					if (!checkStatus())
						return;

					bool magicHealHpEnable = injector.getEnableHash(util::kNormalMagicHealEnable);
					if (!magicHealHpEnable)
						break;

					int charPercent = injector.getValueHash(util::kNormalMagicHealCharValue);
					int petPercent = injector.getValueHash(util::kNormalMagicHealPetValue);
					int alliePercent = injector.getValueHash(util::kNormalMagicHealAllieValue);

					if (charPercent == 0 && petPercent == 0 && alliePercent == 0)
						break;


					int target = -1;
					bool ok = false;
					if ((charPercent > 0) && injector.server->checkPlayerHp(charPercent))
					{
						ok = true;
						target = 0;
					}
					if (!ok && (petPercent > 0) && injector.server->checkPetHp(charPercent))
					{
						ok = true;
						target = injector.server->getPC().battlePetNo + 1;
					}
					if (!ok && (alliePercent > 0) && injector.server->checkPartyHp(charPercent, &target))
					{
						ok = true;
						target += MAX_PET;
					}

					if (!ok || target == -1)
						break;

					int magicIndex = injector.getValueHash(util::kNormalMagicHealMagicValue);
					if (magicIndex < 0 || magicIndex >= MAX_MAGIC)
						break;

					int targetType = injector.server->getMagic(magicIndex).target;
					if ((targetType != MAGIC_TARGET_MYSELF) && (targetType != MAGIC_TARGET_OTHER))
						break;

					if ((targetType == MAGIC_TARGET_MYSELF) && (target != 0))
						break;

					injector.server->useMagic(magicIndex, target);
					QThread::msleep(100);
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
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return;

	if (autodroppet_future_.isRunning())
		return;

	autodroppet_future_cancel_flag_.store(false, std::memory_order_release);

	if (injector.getEnableHash(util::kDropPetEnable))
	{
		autodroppet_future_ = QtConcurrent::run([this]()->void
			{
				Injector& injector = Injector::getInstance();
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

				for (int i = 0; i < MAX_PET; ++i)
				{
					if (!checkStatus())
						return;

					PET pet = injector.server->getPet(i);
					if (pet.useFlag == 0 || pet.maxHp <= 0 || pet.level <= 0)
						continue;

					double str = pet.atk;
					double def = pet.def;
					double agi = pet.quick;
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

//自動鎖寵
void MainObject::checkAutoLockPet()
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return;

	bool iswait = false;
	bool enableLockPet = injector.getEnableHash(util::kLockPetEnable) && !injector.getEnableHash(util::kLockPetScheduleEnable);
	if (enableLockPet)
	{
		int lockPetIndex = injector.getValueHash(util::kLockPetValue);
		if (lockPetIndex >= 0 && lockPetIndex < MAX_PET)
		{
			PET pet = injector.server->getPet(lockPetIndex);
			if (pet.state != PetState::kBattle)
			{
				injector.server->setPetState(lockPetIndex, kBattle);
				iswait = true;
			}
		}
	}

	bool enableLockRide = injector.getEnableHash(util::kLockRideEnable) && !injector.getEnableHash(util::kLockPetScheduleEnable);
	if (enableLockRide)
	{
		int lockRideIndex = injector.getValueHash(util::kLockRideValue);
		if (lockRideIndex >= 0 && lockRideIndex < MAX_PET)
		{
			PET pet = injector.server->getPet(lockRideIndex);
			if (pet.state != PetState::kRide)
			{
				injector.server->setPetState(lockRideIndex, kRide);
				iswait = true;
			}
		}
	}

	if (iswait)
	{
		QThread::msleep(1000);
	}
}

//自動鎖寵排程
void MainObject::checkAutoLockSchedule()
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return;



	auto checkSchedule = [](util::UserSetting set)->bool
	{
		static const QHash <QString, PetState> hashType = {
			{ "戰", kBattle },
			{ "騎", kRide },

			{ "战", kBattle },
			{ "骑", kRide },

			{ "B", kBattle },
			{ "R", kRide },
		};

		Injector& injector = Injector::getInstance();
		QString lockPetSchedule = injector.getStringHash(set);
		int rindex = -1;
		int bindex = -1;
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

					QString name = args.at(0).simplified();
					if (name.isEmpty())
						continue;

					QString nameStr = name.left(1);
					int petIndex = -1;
					bool ok = false;
					petIndex = nameStr.toInt(&ok);
					if (!ok)
						continue;
					--petIndex;

					if (petIndex < 0 || petIndex >= MAX_PET)
						continue;

					QString levelStr = args.at(1).simplified();
					if (levelStr.isEmpty())
						continue;

					ok = false;
					int level = levelStr.toInt(&ok);
					if (!ok)
						continue;

					if (level < 0 || level > 255)
						continue;

					QString typeStr = args.at(2).simplified();
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
				QThread::msleep(500);
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
				QThread::msleep(500);
			}

			if (pet.state != kBattle)
				injector.server->setPetState(bindex, kBattle);
		}

		for (int i = 0; i < MAX_PET; ++i)
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

//自動吃經驗加乘道具
void MainObject::checkAutoEatBoostExpItem()
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return;

	if (!injector.getEnableHash(util::kAutoEatBeanEnable))
		return;

	if (injector.server.isNull())
		return;

	if (!injector.server->getOnlineFlag())
		return;

	if (injector.server->getBattleFlag())
		return;

	for (int i = 0; i < MAX_ITEM; ++i)
	{
		ITEM item = injector.server->getPC().item[i];
		if (item.name.isEmpty() || item.memo.isEmpty() || item.useFlag == 0)
			continue;

		if (item.memo.contains(u8"經驗值上升") || item.memo.contains(u8"经验值上升"))
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
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return;

	pointerWriterSync_.addFuture(QtConcurrent::run([this]()
		{
			Injector& injector = Injector::getInstance();
			if (injector.server.isNull())
				return;

			util::SafeHash<int, mapunit_t> units = injector.server->mapUnitHash;
			for (const mapunit_t& unit : units)
			{
				if (isInterruptionRequested())
					return;

				if (injector.server.isNull())
					return;

				if ((unit.objType != util::OBJ_NPC)
					|| unit.name.isEmpty()
					|| (injector.server->getWorldStatus() != 9)
					|| (injector.server->getGameStatus() != 3)
					|| injector.server->npcUnitPointHash.contains(QPoint(unit.x, unit.y)))
				{
					continue;
				}

				injector.server->npcUnitPointHash.insert(QPoint(unit.x, unit.y), unit);

				util::Config config(util::getPointFileName());
				util::MapData d;
				int nowFloor = injector.server->nowFloor;
				QPoint nowPoint = injector.server->getPoint();
				d.floor = nowFloor;
				d.name = unit.name;

				//npc前方一格
				QPoint newPoint = util::fix_point.at(unit.dir) + unit.p;
				//檢查是否可走
				if (injector.server->mapAnalyzer->isPassable(nowFloor, nowPoint, newPoint))
				{
					d.x = newPoint.x();
					d.y = newPoint.y();
				}
				else
				{
					//再往前一格
					QPoint additionPoint = util::fix_point.at(unit.dir) + newPoint;
					//檢查是否可走
					if (injector.server->mapAnalyzer->isPassable(nowFloor, nowPoint, additionPoint))
					{
						d.x = additionPoint.x();
						d.y = additionPoint.y();
					}
					else
					{
						//檢查NPC周圍8格
						bool flag = false;
						for (int i = 0; i < 8; ++i)
						{
							newPoint = util::fix_point.at(i) + unit.p;
							if (injector.server->mapAnalyzer->isPassable(nowFloor, nowPoint, newPoint))
							{
								d.x = newPoint.x();
								d.y = newPoint.y();
								flag = true;
								break;
							}
						}
						if (!flag)
						{
							return;
						}
					}
				}
				config.writeMapData(unit.name, d);
				SPD_LOG(g_logger_name, "[threadpool] recorded new npc infos");
			}
		}
	));
}