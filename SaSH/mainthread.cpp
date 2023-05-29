#include "stdafx.h"
#include "mainthread.h"
#include "signaldispatcher.h"
#include "net/tcpserver.h"
#include "net/autil.h"
#include "injector.h"

bool ThreadManager::createThread(QObject* parent)
{
	if (thread_)
		return false;

	if (object_)
		return false;

	do
	{
		object_ = new MainObject();
		if (!object_)
			break;

		thread_ = new QThread();
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
	: QObject(parent)
{


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

	do
	{
		if (!injector.server)
			break;

		injector.server->run();

		if (!injector.server->isRunning())
			break;

		emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusOpening);
		if (!injector.createProcess(process_info))
		{
			break;
		}
		emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusOpened);

		if (!injector.injectLibrary(process_info, injector.server->getPort(), &remove_thread_reason))
		{
			qDebug() << "injectLibrary failed. reason:" << remove_thread_reason;
			break;
		}

		mainProc();
	} while (false);

	if (autowalk_future_.isRunning())
	{
		autowalk_future_cancel_flag_.store(true, std::memory_order_release);
		autowalk_future_.cancel();
		autowalk_future_.waitForFinished();
	}

	emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusNotOpen);
	emit signalDispatcher.setStartButtonEnabled(true);
	MINT::NtTerminateProcess(injector.getProcess(), 0);

	emit finished();
	qDebug() << "emit finished()";
}

void MainObject::mainProc()
{
	Injector& injector = Injector::getInstance();
	for (;;)
	{
		QThread::msleep(200);
		if (isInterruptionRequested())
			break;

		if (!injector.isWindowAlive())
		{
			qDebug() << "window is disappear!";
			break;
		}

		injector.server->updateDatasFromMemory();

		int status = checkAndRunFunctions();
		if (status == 1)
		{
			QThread::msleep(300);
		}
		else if (status == 2)
		{
		}
		else
		{
		}
		QThread::yieldCurrentThread();
	}
}


int MainObject::checkAndRunFunctions()
{
	Injector& injector = Injector::getInstance();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();

	int status = injector.server->getUnloginStatus();

	if (status == util::kStatusInputUser ||
		status == util::kStatusSelectServer ||
		status == util::kStatusSelectSubServer ||
		status == util::kStatusSelectCharacter ||
		status == util::kStatusDisconnect ||
		status == util::kStatusTimeout ||
		status == util::kStatusBusy ||
		status == util::kNoUserNameOrPassword ||
		!injector.server->IS_ONLINE_FLAG)
	{
		//每次登出後只會執行一次
		if (!login_run_once_flag_)
		{
			login_run_once_flag_ = true;
			Sleep(1000);
		}

		if (injector.getEnableHash(util::kAutoLoginEnable) || injector.getEnableHash(util::kAutoReconnectEnable))
			injector.server->login(status);
		return 0;
	}

	//每次登入後只會執行一次
	if (login_run_once_flag_)
	{
		login_run_once_flag_ = false;
		emit signalDispatcher.applyHashSettingsToUI();
		for (int i = 0; i < 30; ++i)
		{
			if (isInterruptionRequested())
				return 0;
			QThread::msleep(100);
		}
		injector.server->isPacketAutoClear.store(true, std::memory_order_release);

		emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusLoginSuccess);

		constexpr bool isbeta = true;
		QDateTime current = QDateTime::currentDateTime();
		QDateTime due = current.addYears(1);
		const QString dueStr(due.toString("yyyy-MM-dd hh:mm:ss"));

		const QString url("https://bbs.shiqi.so");

		const QString version = QString("%1.%2%3")
			.arg(SASH_VERSION_MAJOR) \
			.arg(SASH_VERSION_MINOR) \
			.arg(SASH_VERSION_PATCH);
		injector.server->announce(tr("Welcome to use SaSH，For more information please visit %1").arg(url));
		injector.server->announce(tr("You are using %1 account, due date is:%2").arg(isbeta ? tr("trial") : tr("subscribed")).arg(dueStr));
		injector.server->announce(tr("StoneAge SaSH forum url:%1, newest version is %2").arg(url).arg(version));

		QString securityCode = injector.getStringHash(util::kGameSecurityCodeString);
		if (!securityCode.isEmpty())
		{
			injector.server->unlockSecurityCode(securityCode);
		}
		return 1;
	}

	setUserDatas();

	checkControl();

	checkAutoWalk();

	if (!injector.server->IS_BATTLE_FLAG)
	{
		if (!battle_run_once_flag_)
		{
			battle_run_once_flag_ = true;
			emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusInNormal);
			injector.server->normalDurationTimer.restart();
		}

		checkEtcFlag();
		checkAutoDropItems();
		return 1;
	}
	else
	{
		if (battle_run_once_flag_)
		{
			battle_run_once_flag_ = false;
			emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusInBattle);
			++injector.server->battle_totol;

			injector.server->battleDurationTimer.restart();

		}

		double time = injector.server->battleDurationTimer.elapsed() / 1000.0;
		QString battle_time_text = QString(tr("%1 count    no %2 round    duration: %3 sec    total time: %4 minues"))
			.arg(injector.server->battle_totol)
			.arg(injector.server->BattleCliTurnNo + 1)
			.arg(QString::number(time, 'f', 3))
			.arg(injector.server->battle_total_time / 1000 / 60);
		emit signalDispatcher.updateTimeLabelContents(battle_time_text);
		return 2;
	}

	return 1;
}

void MainObject::checkControl()
{
	Injector& injector = Injector::getInstance();


	if (injector.getEnableHash(util::kLogOutEnable))
	{
		injector.server->logOut();
		injector.setEnableHash(util::kLogOutEnable, false);
	}

	if (injector.getEnableHash(util::kLogBackEnable))
	{
		injector.server->logBack();
		injector.setEnableHash(util::kLogBackEnable, false);
	}

	if (injector.getEnableHash(util::kEchoEnable))
	{
		injector.server->EO();
		injector.setEnableHash(util::kEchoEnable, false);
	}

	bool bChecked = injector.getEnableHash(util::kHideCharacterEnable);
	if (flagHideCharacterEnable_ != bChecked)
	{
		flagHideCharacterEnable_ = bChecked;
		injector.postMessage(Injector::kEnablePlayerShow, !bChecked, NULL);
	}

	bChecked = injector.getEnableHash(util::kCloseEffectEnable);
	if (flagCloseEffectEnable_ != bChecked)
	{
		flagCloseEffectEnable_ = bChecked;
		injector.postMessage(Injector::kEnableEffect, !bChecked, NULL);
	}

	bChecked = injector.getEnableHash(util::kMuteEnable);
	if (flagMuteEnable_ != bChecked)
	{
		flagMuteEnable_ = bChecked;
		injector.postMessage(Injector::kEnableSound, !bChecked, NULL);
	}

	bChecked = injector.getEnableHash(util::kLockTimeEnable);
	int value = injector.getValueHash(util::kLockTimeValue);
	if (flagLockTimeEnable_ != bChecked || flagLockTimeValue_ != value)
	{
		flagLockTimeEnable_ = bChecked;
		flagLockTimeValue_ = value;
		injector.postMessage(Injector::kSetTimeLock, bChecked, flagLockTimeValue_);
	}

	value = injector.getValueHash(util::kSpeedBoostValue);
	if (flagSetBoostValue_ != value)
	{
		flagSetBoostValue_ = value;
		injector.postMessage(Injector::kSetBoostSpeed, true, flagSetBoostValue_);
	}

	bChecked = injector.getEnableHash(util::kFastWalkEnable);
	if (flagFastWalkEnable_ != bChecked)
	{
		flagFastWalkEnable_ = bChecked;
		injector.postMessage(Injector::kEnableFastWalk, bChecked, NULL);
	}

	bChecked = injector.getEnableHash(util::kPassWallEnable);
	if (flagPassWallEnable_ != bChecked)
	{
		flagPassWallEnable_ = bChecked;
		injector.postMessage(Injector::kEnablePassWall, bChecked, NULL);
	}

	bChecked = injector.getEnableHash(util::kLockImageEnable);
	if (flagLockImageEnable_ != bChecked)
	{
		flagLockImageEnable_ = bChecked;
		injector.postMessage(Injector::kEnableImageLock, bChecked, NULL);
	}

	bChecked = injector.getEnableHash(util::kLockMoveEnable);
	if (flagLockMoveEnable_ != bChecked)
	{
		flagLockMoveEnable_ = bChecked;
		injector.postMessage(Injector::kEnableMoveLock, bChecked, NULL);
	}

	bool bCheckedFastBattle = injector.getEnableHash(util::kFastBattleEnable);
	bChecked = injector.getEnableHash(util::kAutoBattleEnable) || bCheckedFastBattle;
	if (bChecked)
	{
		injector.postMessage(Injector::kEnableBattleDialog, false, NULL);
	}
	else if (!bChecked)
	{
		injector.postMessage(Injector::kEnableBattleDialog, true, NULL);
	}

	int W = injector.server->getWorldStatus();
	if (bCheckedFastBattle && W == 9)
	{
		injector.postMessage(Injector::kSetBLockPacket, true, NULL);
	}
	else
	{
		injector.postMessage(Injector::kSetBLockPacket, false, NULL);
	}
}

void MainObject::checkEtcFlag()
{
	Injector& injector = Injector::getInstance();
	int flg = injector.server->pc.etcFlag;
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

void MainObject::checkAutoWalk()
{
	Injector& injector = Injector::getInstance();
	if (injector.getEnableHash(util::kAutoWalkEnable) || injector.getEnableHash(util::kFastAutoWalkEnable))
	{
		if (autowalk_future_.isRunning())
			return;

		autowalk_future_cancel_flag_.store(false, std::memory_order_release);
		QPoint current_pos = QPoint{ injector.server->nowGx, injector.server->nowGy };
		autowalk_future_ = QtConcurrent::run([&injector, current_pos, this]()
			{
				bool current_side = false;

				for (;;)
				{
					if (isInterruptionRequested())
						break;

					if (autowalk_future_cancel_flag_.load(std::memory_order_acquire))
						break;

					if (!injector.server->IS_ONLINE_FLAG)
						break;

					if (injector.server->IS_BATTLE_FLAG)
					{
						QThread::msleep(1000);
						if (!injector.server->IS_BATTLE_FLAG)
						{
							for (int i = 0; i < 10; ++i)
							{
								if (isInterruptionRequested())
									return;
								QThread::msleep(100);
							}
						}
						else
							continue;
					}


					bool enableAutoWalk = injector.getEnableHash(util::kAutoWalkEnable);
					bool enableFastAutoWalk = injector.getEnableHash(util::kFastAutoWalkEnable);
					int walk_speed = injector.getValueHash(util::kAutoWalkDelayValue);

					if (enableAutoWalk)
					{
						int walk_len = injector.getValueHash(util::kAutoWalkDistanceValue);
						int walk_dir = injector.getValueHash(util::kAutoWalkDirectionValue);

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
							//Qt取亂數
							QRandomGenerator* random = QRandomGenerator::global();
							x = random->bounded(current_pos.x() - walk_len, current_pos.x() + walk_len);
							y = random->bounded(current_pos.x() - walk_len, current_pos.x() + walk_len);
						}

						if (current_side)
							current_side = false;
						else
							current_side = true;

						injector.server->move(QPoint(x, y));
					}
					else if (enableFastAutoWalk)
						injector.server->move(QPoint(0, 0), "gcgc");
					QThread::msleep(walk_speed + 10);
				}
			});
	}
	else
	{
		if (autowalk_future_.isRunning())
		{
			autowalk_future_cancel_flag_.store(true, std::memory_order_release);
			autowalk_future_.cancel();
			autowalk_future_.waitForFinished();
		}
	}
}

void MainObject::checkAutoDropItems()
{
	Injector& injector = Injector::getInstance();
	if (!injector.getEnableHash(util::kAutoDropEnable))
		return;

	QString strDropItems = injector.getStringHash(util::kAutoDropItemString);
	if (strDropItems.isEmpty())
		return;

	QStringList dropItems = strDropItems.split("|", Qt::SkipEmptyParts);
	if (dropItems.isEmpty())
		return;

	for (int i = 0; i < MAX_ITEM; ++i)
	{
		ITEM item = injector.server->pc.item[i];
		if (item.name.isEmpty())
			continue;

		for (const QString& cmpItem : dropItems)
		{
			if (isInterruptionRequested())
				break;

			if (cmpItem.isEmpty())
				continue;

			if (cmpItem.startsWith("?"))
			{
				QString newCmpItem = cmpItem.mid(1);
				if (item.name.contains(newCmpItem))
				{
					injector.server->dropItem(i);
					continue;
				}
			}
			else if ((item.name == cmpItem))
			{
				injector.server->dropItem(i);
			}
		}
	}
}

void MainObject::setUserDatas()
{
	Injector& injector = Injector::getInstance();

	QStringList itemNames;
	for (const ITEM& item : injector.server->pc.item)
	{
		if (item.name.isEmpty())
			continue;

		itemNames.append(item.name);
	}

	injector.setUserData(util::kUserItemNames, itemNames);

}