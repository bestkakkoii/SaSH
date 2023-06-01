#include "stdafx.h"
#include "generalform.h"
#include "selectobjectform.h"

#include "mainthread.h"
#include <util.h>
#include <injector.h>
#include "signaldispatcher.h"

GeneralForm::GeneralForm(QWidget* parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	connect(this, &GeneralForm::resetControlTextLanguage, this, &GeneralForm::onResetControlTextLanguage, Qt::UniqueConnection);

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	connect(&signalDispatcher, &SignalDispatcher::setStartButtonEnabled, ui.pushButton_start, &QPushButton::setEnabled, Qt::UniqueConnection);
	connect(&signalDispatcher, &SignalDispatcher::applyHashSettingsToUI, this, &GeneralForm::onApplyHashSettingsToUI, Qt::UniqueConnection);


	QList<QPushButton*> buttonList = util::findWidgets<QPushButton>(this);
	for (auto& button : buttonList)
	{
		if (button)
			connect(button, &QPushButton::clicked, this, &GeneralForm::onButtonClicked, Qt::UniqueConnection);
	}

	QList <QCheckBox*> checkBoxList = util::findWidgets<QCheckBox>(this);
	for (auto& checkBox : checkBoxList)
	{
		if (checkBox)
			connect(checkBox, &QCheckBox::stateChanged, this, &GeneralForm::onCheckBoxStateChanged, Qt::UniqueConnection);
	}

	QList <QSpinBox*> spinBoxList = util::findWidgets<QSpinBox>(this);
	for (auto& spinBox : spinBoxList)
	{
		if (spinBox)
			connect(spinBox, SIGNAL(valueChanged(int)), this, SLOT(onSpinBoxValueChanged(int)), Qt::UniqueConnection);
	}

	QList <ComboBox*> comboBoxList = util::findWidgets<ComboBox>(this);
	for (auto& comboBox : comboBoxList)
	{
		if (comboBox)
			connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onComboBoxCurrentIndexChanged(int)), Qt::UniqueConnection);
	}

	emit signalDispatcher.applyHashSettingsToUI();
}

GeneralForm::~GeneralForm()
{
	ThreadManager& thread_manager = ThreadManager::getInstance();
	thread_manager.close();
	qDebug() << "~GeneralForm()";
}

void GeneralForm::onResetControlTextLanguage()
{
	const QString fileName(qgetenv("JSON_PATH"));
	util::Config config(fileName);
	QStringList serverList = config.readStringList("System", "ServerList");
	if (serverList.isEmpty())
	{
		serverList = QStringList{
			tr("1st//Acticity"), tr("2nd///Market"), tr("3rd//Family"), tr("4th//Away"),
			tr("5th//Away"), tr("6th//Away"), tr("7th//Away"), tr("8th//Away"),
			tr("9th//Away"), tr("15th//Company"), tr("21th//Member"), tr("22th//Member"),
		};

		config.write("System", "ServerList", serverList.join("|"));
	}

	QStringList subServerList = config.readStringList("System", "SubServerList");
	if (subServerList.isEmpty())
	{
		subServerList = QStringList{
			tr("Telecom"), tr("UnitedNetwork"), tr("Easyown"), tr("Oversea"), tr("Backup"),
		};

		config.write("System", "SubServerList", subServerList.join("|"));
	}

	const QStringList positionList = { tr("Left"), tr("Right") };

	//下午 黃昏 午夜 早晨 中午
	const QStringList timeList = { tr("Afternoon"), tr("Dusk"), tr("Midnight"), tr("Morning"), tr("Noon") };

	ui.comboBox_server->clear();
	ui.comboBox_subserver->clear();
	ui.comboBox_position->clear();
	ui.comboBox_locktime->clear();

	ui.comboBox_server->addItems(serverList);
	ui.comboBox_subserver->addItems(subServerList);
	ui.comboBox_position->addItems(positionList);
	ui.comboBox_locktime->addItems(timeList);
}

void GeneralForm::onButtonClicked()
{
	QPushButton* pPushButton = qobject_cast<QPushButton*>(sender());
	if (!pPushButton)
		return;

	QString name = pPushButton->objectName();
	if (name.isEmpty())
		return;

	Injector& injector = Injector::getInstance();

	if (name == "pushButton_logout")
	{
		bool flag = injector.getEnableHash(util::kLogOutEnable);
		if (injector.isValid())
		{
			QMessageBox::StandardButton button = QMessageBox::warning(this, tr("logout"), tr("Are you sure you want to logout now?"),
				QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
			if (button == QMessageBox::Yes)
			{
				injector.setEnableHash(util::kLogOutEnable, true);
			}
		}
		return;
	}

	else if (name == "pushButton_logback")
	{
		if (injector.isValid())
		{
			QMessageBox::StandardButton button = QMessageBox::warning(this, tr("logback"), tr("Are you sure you want to logback now?"),
				QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
			if (button == QMessageBox::Yes)
			{
				injector.setEnableHash(util::kLogBackEnable, true);
			}
		}

		return;
	}

	else if (name == "pushButton_start")
	{
		setFocus();
		pPushButton->setEnabled(false);
		const QString fileName(qgetenv("JSON_PATH"));
		if (fileName.isEmpty())
			return;

		util::Config config(fileName);
		QString path = config.readString("System", "Command", "DirPath");
		if (path.isEmpty() || !path.contains(util::SA_NAME) || !QFile::exists(fileName))
		{
			if (!util::createFileDialog(util::SA_NAME, &path, this))
				return;
		}

		if (!QFile::exists(path))
		{
			if (!util::createFileDialog(util::SA_NAME, &path, this))
				return;
		}

		config.write("System", "Command", "DirPath", path);
		QFileInfo fileInfo(path);
		QString dirPath = fileInfo.absolutePath();
		QByteArray dirPathUtf8 = dirPath.toUtf8();
		qputenv("GAME_DIR_PATH", dirPathUtf8);

		Injector& injector = Injector::getInstance();
		injector.server.reset(new Server(this));
		if (injector.server.isNull())
			return;

		if (!injector.server->start(this))
			return;

		ThreadManager& thread_manager = ThreadManager::getInstance();
		if (!thread_manager.createThread())
		{
			pPushButton->setEnabled(true);
		}
		return;
	}

	else if (name == "pushButton_joingroup")
	{

		return;
	}

	else if (name == "pushButton_leavegroup")
	{

		return;
	}

	else if (name == "pushButton_sell")
	{

		return;
	}

	else if (name == "pushButton_pick")
	{

		return;
	}

	else if (name == "pushButton_watch")
	{

		return;
	}

	else if (name == "pushButton_eo")
	{
		injector.setEnableHash(util::kEchoEnable, true);
		return;
	}

	else if (name == "pushButton_savesettings")
	{
		QString fileName;
		if (!injector.server.isNull())
			fileName = injector.server->pc.name;
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
		emit signalDispatcher.saveHashSettings(fileName);
		return;
	}

	else if (name == "pushButton_loadsettings")
	{
		QString fileName;
		if (!injector.server.isNull())
			fileName = injector.server->pc.name;
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
		emit signalDispatcher.loadHashSettings(fileName);
		return;
	}
}

void GeneralForm::onCheckBoxStateChanged(int state)
{
	QCheckBox* pCheckBox = qobject_cast<QCheckBox*>(sender());
	if (!pCheckBox)
		return;

	bool isChecked = (state == Qt::Checked);

	QString name = pCheckBox->objectName();
	if (name.isEmpty())
		return;

	Injector& injector = Injector::getInstance();

	//login
	if (name == "checkBox_autologin")
	{
		injector.setEnableHash(util::kAutoLoginEnable, isChecked);
		return;
	}

	else if (name == "checkBox_autoreconnect")
	{
		injector.setEnableHash(util::kAutoReconnectEnable, isChecked);
		return;
	}

	//support
	else if (name == "checkBox_hidechar")
	{
		injector.setEnableHash(util::kHideCharacterEnable, isChecked);
		return;
	}

	else if (name == "checkBox_closeeffect")
	{
		injector.setEnableHash(util::kCloseEffectEnable, isChecked);
		return;
	}

	else if (name == "checkBox_optimize")
	{
		injector.setEnableHash(util::kOptimizeEnable, isChecked);
		return;
	}

	else if (name == "checkBox_hidewindow")
	{
		injector.setEnableHash(util::kHideWindowEnable, isChecked);
		HWND hWnd = injector.getProcessWindow();
		if (hWnd)
		{
			if (isChecked)
			{
				ShowWindow(hWnd, SW_HIDE);
			}
			else
			{
				ShowWindow(hWnd, SW_SHOW);
			}
		}
		return;
	}

	else if (name == "checkBox_mute")
	{
		injector.setEnableHash(util::kMuteEnable, isChecked);
		return;
	}

	else if (name == "checkBox_autojoin")
	{
		injector.setEnableHash(util::kAutoJoinEnable, isChecked);
		int type = injector.getValueHash(util::kAutoFunTypeValue);
		if (isChecked && type != 0)
		{
			injector.setValueHash(util::kAutoFunTypeValue, 0);
		}

		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
		emit signalDispatcher.applyHashSettingsToUI();
		return;
	}

	else if (name == "checkBox_locktime")
	{
		injector.setEnableHash(util::kLockTimeEnable, isChecked);
		return;
	}

	else if (name == "checkBox_autofreememory")
	{
		injector.setEnableHash(util::kAutoFreeMemoryEnable, isChecked);
		return;
	}

	//support2
	else if (name == "checkBox_fastwalk")
	{
		injector.setEnableHash(util::kFastWalkEnable, isChecked);
		return;
	}

	else if (name == "checkBox_passwall")
	{
		injector.setEnableHash(util::kPassWallEnable, isChecked);
		return;
	}

	else if (name == "checkBox_lockmove")
	{
		injector.setEnableHash(util::kLockMoveEnable, isChecked);
		return;
	}

	else if (name == "checkBox_lockimage")
	{
		injector.setEnableHash(util::kLockImageEnable, isChecked);
		return;
	}

	else if (name == "checkBox_autodropmeat")
	{
		injector.setEnableHash(util::kAutoDropMeatEnable, isChecked);

		return;
	}

	else if (name == "checkBox_autodrop")
	{
		bool bOriginal = injector.getEnableHash(util::kAutoDropEnable);
		if (bOriginal == isChecked)
			return;

		injector.setEnableHash(util::kAutoDropEnable, isChecked);

		if (!isChecked)
			return;

		QStringList srcList;
		QStringList dstList;
		QStringList srcSelectList;

		QVariant data = injector.getUserData(util::kUserItemNames);
		if (data.isValid())
		{
			srcSelectList = data.toStringList();
		}

		QString src = injector.getStringHash(util::kAutoDropItemString);
		if (!src.isEmpty())
		{
			srcList = src.split(util::rexOR, Qt::SkipEmptyParts);
		}

		if (!createSelectObjectForm(SelectObjectForm::kAutoDropItem, srcSelectList, srcList, &dstList, this))
			return;

		QString dst = dstList.join("|");
		injector.setStringHash(util::kAutoDropItemString, dst);

		return;
	}

	else if (name == "checkBox_autostack")
	{
		injector.setEnableHash(util::kAutoStackEnable, isChecked);
		return;
	}

	else if (name == "checkBox_knpc")
	{
		injector.setEnableHash(util::kKNPCEnable, isChecked);
		return;
	}

	else if (name == "checkBox_autoanswer")
	{
		injector.setEnableHash(util::kAutoAnswerEnable, isChecked);
		return;
	}

	else if (name == "checkBox_forceleavebattle")
	{
		injector.setEnableHash(util::kForceLeaveBattleEnable, isChecked);
		return;
	}

	//battle
	else if (name == "checkBox_autowalk")
	{
		if (isChecked)
		{
			ui.checkBox_fastautowalk->setChecked(!isChecked);
		}
		injector.setEnableHash(util::kAutoWalkEnable, isChecked);
		return;
	}

	else if (name == "checkBox_fastautowalk")
	{
		if (isChecked)
		{
			ui.checkBox_autowalk->setChecked(!isChecked);
		}
		injector.setEnableHash(util::kFastAutoWalkEnable, isChecked);
		return;
	}

	else if (name == "checkBox_fastbattle")
	{
		if (isChecked)
		{
			ui.checkBox_autobattle->setChecked(!isChecked);
		}

		bool bOriginal = injector.getEnableHash(util::kFastBattleEnable);
		injector.setEnableHash(util::kFastBattleEnable, isChecked);
		if (!bOriginal && isChecked && !injector.server.isNull())
		{
			injector.server->asyncBattleWork(false);
		}
		return;
	}

	else if (name == "checkBox_autobattle")
	{
		if (isChecked)
		{
			ui.checkBox_fastbattle->setChecked(!isChecked);
		}

		bool bOriginal = injector.getEnableHash(util::kAutoBattleEnable);
		injector.setEnableHash(util::kAutoBattleEnable, isChecked);
		if (!bOriginal && isChecked && !injector.server.isNull())
		{
			injector.server->asyncBattleWork(false);
		}

		return;
	}

	else if (name == "checkBox_autocatch")
	{
		injector.setEnableHash(util::kAutoCatchEnable, isChecked);
		return;
	}

	else if (name == "checkBox_lockattck")
	{
		bool bOriginal = injector.getEnableHash(util::kLockAttackEnable);
		if (bOriginal == isChecked)
			return;

		injector.setEnableHash(util::kLockAttackEnable, isChecked);

		if (!isChecked)
			return;

		QStringList srcList;
		QStringList dstList;
		QStringList srcSelectList;

		QVariant data = injector.getUserData(util::kUserEnemyNames);
		if (data.isValid())
		{
			srcSelectList = data.toStringList();
		}
		srcSelectList.removeDuplicates();

		QString src = injector.getStringHash(util::kLockAttackString);
		if (!src.isEmpty())
		{
			srcList = src.split("|", Qt::SkipEmptyParts);
		}

		if (!createSelectObjectForm(SelectObjectForm::kLockAttack, srcSelectList, srcList, &dstList, this))
			return;

		QString dst = dstList.join("|");
		injector.setStringHash(util::kLockAttackString, dst);
		return;
	}

	else if (name == "checkBox_autoescape")
	{
		injector.setEnableHash(util::kAutoEscapeEnable, isChecked);
		return;
	}

	else if (name == "checkBox_lockescape")
	{
		bool bOriginal = injector.getEnableHash(util::kLockEscapeEnable);
		if (bOriginal == isChecked)
			return;

		injector.setEnableHash(util::kLockEscapeEnable, isChecked);

		if (!isChecked)
			return;

		QStringList srcList;
		QStringList dstList;
		QStringList srcSelectList;

		QVariant data = injector.getUserData(util::kUserEnemyNames);
		if (data.isValid())
		{
			srcSelectList = data.toStringList();
		}
		srcSelectList.removeDuplicates();

		QString src = injector.getStringHash(util::kLockEscapeString);
		if (!src.isEmpty())
		{
			srcList = src.split("|", Qt::SkipEmptyParts);
		}

		if (!createSelectObjectForm(SelectObjectForm::kLockAttack, srcSelectList, srcList, &dstList, this))
			return;

		QString dst = dstList.join("|");
		injector.setStringHash(util::kLockEscapeString, dst);
		return;
	}

	else if (name == "checkBox_battletimeextend")
	{
		injector.setEnableHash(util::kBattleTimeExtendEnable, isChecked);
		return;
	}

	else if (name == "checkBox_falldownescape")
	{
		injector.setEnableHash(util::kFallDownEscapeEnable, isChecked);
		return;
	}

	//shortcut switcher
	else if (name == "checkBox_switcher_team")
	{
		injector.setEnableHash(util::kSwitcherTeamEnable, isChecked);
		return;
	}

	else if (name == "checkBox_switcher_pk")
	{
		injector.setEnableHash(util::kSwitcherPKEnable, isChecked);
		return;
	}

	else if (name == "checkBox_switcher_card")
	{
		injector.setEnableHash(util::kSwitcherCardEnable, isChecked);
		return;
	}

	else if (name == "checkBox_switcher_trade")
	{
		injector.setEnableHash(util::kSwitcherTradeEnable, isChecked);
		return;
	}

	else if (name == "checkBox_switcher_family")
	{
		injector.setEnableHash(util::kSwitcherFamilyEnable, isChecked);
		return;
	}

	else if (name == "checkBox_switcher_job")
	{
		injector.setEnableHash(util::kSwitcherJobEnable, isChecked);
		return;
	}

	else if (name == "checkBox_switcher_world")
	{
		injector.setEnableHash(util::kSwitcherWorldEnable, isChecked);
		return;
	}

}

void GeneralForm::onSpinBoxValueChanged(int value)
{
	QSpinBox* pSpinBox = qobject_cast<QSpinBox*>(sender());
	if (!pSpinBox)
		return;

	QString name = pSpinBox->objectName();
	if (name.isEmpty())
		return;

	Injector& injector = Injector::getInstance();

	if (name == "spinBox_speedboost")
	{
		injector.setValueHash(util::kSpeedBoostValue, value);
		return;
	}
}

void GeneralForm::onComboBoxCurrentIndexChanged(int value)
{
	ComboBox* pComboBox = qobject_cast<ComboBox*>(sender());
	if (!pComboBox)
		return;

	QString name = pComboBox->objectName();
	if (name.isEmpty())
		return;

	Injector& injector = Injector::getInstance();

	if (name == "comboBox_server")
	{
		injector.setValueHash(util::kServerValue, value);
	}

	if (name == "comboBox_subserver")
	{
		injector.setValueHash(util::kSubServerValue, value);
	}

	if (name == "comboBox_position")
	{
		injector.setValueHash(util::kPositionValue, value);
	}

	if (name == "comboBox_locktime")
	{
		injector.setValueHash(util::kLockTimeValue, value);
		if (ui.checkBox_locktime->isChecked())
			injector.sendMessage(Injector::kSetTimeLock, true, value);
	}
}

void GeneralForm::onApplyHashSettingsToUI()
{
	Injector& injector = Injector::getInstance();
	util::SafeHash<util::UserSetting, bool> enableHash = injector.getEnableHash();
	util::SafeHash<util::UserSetting, int> valueHash = injector.getValueHash();
	util::SafeHash<util::UserSetting, QString> stringHash = injector.getStringHash();

	//login
	ui.comboBox_server->setCurrentIndex(valueHash.value(util::kServerValue));
	ui.comboBox_subserver->setCurrentIndex(valueHash.value(util::kSubServerValue));
	ui.comboBox_position->setCurrentIndex(valueHash.value(util::kPositionValue));
	ui.comboBox_locktime->setCurrentIndex(valueHash.value(util::kLockTimeValue));
	ui.checkBox_autologin->setChecked(enableHash.value(util::kAutoLoginEnable));
	ui.checkBox_autoreconnect->setChecked(enableHash.value(util::kAutoReconnectEnable));

	//support
	ui.checkBox_hidechar->setChecked(enableHash.value(util::kHideCharacterEnable));
	ui.checkBox_closeeffect->setChecked(enableHash.value(util::kCloseEffectEnable));
	ui.checkBox_optimize->setChecked(enableHash.value(util::kOptimizeEnable));
	ui.checkBox_hidewindow->setChecked(enableHash.value(util::kHideWindowEnable));
	ui.checkBox_mute->setChecked(enableHash.value(util::kMuteEnable));
	ui.checkBox_autojoin->setChecked(enableHash.value(util::kAutoJoinEnable));
	ui.checkBox_locktime->setChecked(enableHash.value(util::kLockTimeEnable));
	ui.checkBox_autofreememory->setChecked(enableHash.value(util::kAutoFreeMemoryEnable));

	//sp
	ui.spinBox_speedboost->setValue(valueHash.value(util::kSpeedBoostValue));


	//support2
	ui.checkBox_fastwalk->setChecked(enableHash.value(util::kFastWalkEnable));
	ui.checkBox_passwall->setChecked(enableHash.value(util::kPassWallEnable));
	ui.checkBox_lockmove->setChecked(enableHash.value(util::kLockMoveEnable));
	ui.checkBox_lockimage->setChecked(enableHash.value(util::kLockImageEnable));
	ui.checkBox_autodropmeat->setChecked(enableHash.value(util::kAutoDropMeatEnable));
	ui.checkBox_autodrop->setChecked(enableHash.value(util::kAutoDropEnable));
	ui.checkBox_autostack->setChecked(enableHash.value(util::kAutoStackEnable));
	ui.checkBox_knpc->setChecked(enableHash.value(util::kKNPCEnable));
	ui.checkBox_autoanswer->setChecked(enableHash.value(util::kAutoAnswerEnable));
	ui.checkBox_forceleavebattle->setChecked(enableHash.value(util::kForceLeaveBattleEnable));

	//battle
	ui.checkBox_autowalk->setChecked(enableHash.value(util::kAutoWalkEnable));
	ui.checkBox_fastautowalk->setChecked(enableHash.value(util::kFastAutoWalkEnable));
	ui.checkBox_fastbattle->setChecked(enableHash.value(util::kFastBattleEnable));
	ui.checkBox_autobattle->setChecked(enableHash.value(util::kAutoBattleEnable));
	ui.checkBox_autocatch->setChecked(enableHash.value(util::kAutoCatchEnable));
	ui.checkBox_lockattck->setChecked(enableHash.value(util::kLockAttackEnable));
	ui.checkBox_autoescape->setChecked(enableHash.value(util::kAutoEscapeEnable));
	ui.checkBox_lockescape->setChecked(enableHash.value(util::kLockEscapeEnable));
	ui.checkBox_battletimeextend->setChecked(enableHash.value(util::kBattleTimeExtendEnable));
	ui.checkBox_falldownescape->setChecked(enableHash.value(util::kFallDownEscapeEnable));

	//switcher
	ui.checkBox_switcher_team->setChecked(enableHash.value(util::kSwitcherTeamEnable));
	ui.checkBox_switcher_pk->setChecked(enableHash.value(util::kSwitcherPKEnable));
	ui.checkBox_switcher_card->setChecked(enableHash.value(util::kSwitcherCardEnable));
	ui.checkBox_switcher_trade->setChecked(enableHash.value(util::kSwitcherTradeEnable));
	ui.checkBox_switcher_pm->setChecked(enableHash.value(util::kSwitcherFamilyEnable));
	ui.checkBox_switcher_family->setChecked(enableHash.value(util::kSwitcherFamilyEnable));
	ui.checkBox_switcher_job->setChecked(enableHash.value(util::kSwitcherJobEnable));
	ui.checkBox_switcher_world->setChecked(enableHash.value(util::kSwitcherWorldEnable));
}