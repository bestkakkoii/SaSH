#include "stdafx.h"
#include "generalform.h"
#include "selectobjectform.h"

#include "mainthread.h"
#include <util.h>
#include <gamedevice.h>
#include "signaldispatcher.h"

//#ifndef _DEBUG
//#include "net/webauthenticator.h"
//#endif

GeneralForm::GeneralForm(long long index, QWidget* parent)
	: QWidget(parent)
	, Indexer(index)
	, pAfkForm_(index, this)
{
	ui.setupUi(this);
	ui.checkBox_autoanswer->hide();
	ui.checkBox_battletimeextend->hide();
	//ui.checkBox_lockimage->hide();

	setFont(util::getFont());

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(index);
	connect(&signalDispatcher, &SignalDispatcher::applyHashSettingsToUI, this, &GeneralForm::onApplyHashSettingsToUI, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::gameStarted, this, &GeneralForm::onGameStart, Qt::QueuedConnection);

	QStringList nameCheckList;
	QList<PushButton*> buttonList = util::findWidgets<PushButton>(this);
	for (auto& button : buttonList)
	{
		if (button && !nameCheckList.contains(button->objectName()))
		{
			util::setPushButton(button);
			nameCheckList.append(button->objectName());
			connect(button, &PushButton::clicked, this, &GeneralForm::onButtonClicked, Qt::UniqueConnection);
		}
	}

	QList <QCheckBox*> checkBoxList = util::findWidgets<QCheckBox>(this);
	for (auto& checkBox : checkBoxList)
	{
		if (checkBox && !nameCheckList.contains(checkBox->objectName()))
		{
			util::setCheckBox(checkBox);
			nameCheckList.append(checkBox->objectName());
			connect(checkBox, &QCheckBox::stateChanged, this, &GeneralForm::onCheckBoxStateChanged, Qt::UniqueConnection);
		}
	}

	QList <QSpinBox*> spinBoxList = util::findWidgets<QSpinBox>(this);
	for (auto& spinBox : spinBoxList)
	{
		if (spinBox && !nameCheckList.contains(spinBox->objectName()))
		{
			util::setSpinBox(spinBox);
			nameCheckList.append(spinBox->objectName());
			connect(spinBox, SIGNAL(valueChanged(int)), this, SLOT(onSpinBoxValueChanged(int)), Qt::UniqueConnection);
		}
	}

	QList <ComboBox*> comboBoxList = util::findWidgets<ComboBox>(this);
	for (auto& comboBox : comboBoxList)
	{
		if (comboBox && !nameCheckList.contains(comboBox->objectName()))
		{
			util::setComboBox(comboBox);
			nameCheckList.append(comboBox->objectName());
			connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onComboBoxCurrentIndexChanged(int)), Qt::UniqueConnection);
			connect(comboBox, &ComboBox::clicked, this, &GeneralForm::onComboBoxClicked, Qt::UniqueConnection);
		}
	}

	QList <QLabel*> labelList = util::findWidgets<QLabel>(this);
	for (auto& label : labelList)
	{
		if (label && !nameCheckList.contains(label->objectName()))
		{
			util::setLabel(label);
			nameCheckList.append(label->objectName());
		}
	}

	reloadPaths();

	pAfkForm_.hide();

	emit signalDispatcher.applyHashSettingsToUI();

	QVector<QPair<QString, QString>> fileList;
	if (!util::enumAllFiles(util::applicationDirPath() + "/settings", ".json", &fileList))
	{
		return;
	}

	long long idx = 0;
	long long defaultIndex = -1;
	ui.comboBox_setting->blockSignals(true);
	ui.comboBox_setting->clear();
	for (const QPair<QString, QString>& pair : fileList)
	{
		ui.comboBox_setting->addItem(pair.first, pair.second);
		if (pair.first.contains("default.json"))
			defaultIndex = idx;
		++idx;
	}

	if (defaultIndex >= 0)
		ui.comboBox_setting->setCurrentIndex(defaultIndex);

	ui.comboBox_setting->blockSignals(false);
}

GeneralForm::~GeneralForm()
{
	long long currentIndex = getIndex();

	ThreadManager& thread_manager = ThreadManager::getInstance();
	thread_manager.close(currentIndex);
	qDebug() << "~GeneralForm()";
}

void GeneralForm::showEvent(QShowEvent* e)
{
	onResetControlTextLanguage();
	setAttribute(Qt::WA_Mapped);
	QWidget::showEvent(e);
}

void GeneralForm::onResetControlTextLanguage()
{
	const QStringList positionList = { tr("Left"), tr("Right"), tr("None") };

	//下午 黃昏 午夜 早晨 中午
	const QStringList timeList = { tr("Afternoon"), tr("Dusk"), tr("Midnight"), tr("Morning"), tr("Noon") };

	ui.comboBox_position->clear();
	ui.comboBox_locktime->clear();

	ui.comboBox_position->addItems(positionList);
	ui.comboBox_locktime->addItems(timeList);

	emit ui.comboBox_server->clicked();
}

void GeneralForm::onComboBoxClicked()
{
	ComboBox* pComboBox = qobject_cast<ComboBox*>(sender());
	if (!pComboBox)
	{
		return;
	}

	QString name = pComboBox->objectName();
	if (name.isEmpty())
	{
		return;
	}

	if (name == "comboBox_setting")
	{
		QVector<QPair<QString, QString>> fileList;
		if (!util::enumAllFiles(util::applicationDirPath() + "/settings", ".json", &fileList))
		{
			return;
		}

		long long currentIndex = ui.comboBox_setting->currentIndex();
		ui.comboBox_setting->blockSignals(true);
		ui.comboBox_setting->clear();
		for (const QPair<QString, QString>& pair : fileList)
		{
			ui.comboBox_setting->addItem(pair.first, pair.second);
		}

		ui.comboBox_setting->setCurrentIndex(currentIndex);

		ui.comboBox_setting->blockSignals(false);

		return;
	}

	if (name == "comboBox_server")
	{
		createServerList();

		return;
	}

	if (name == "comboBox_paths")
	{

		reloadPaths();
		return;
	}
}

void GeneralForm::reloadPaths()
{
	QListView* pListView = qobject_cast<QListView*>(ui.comboBox_paths->view());
	if (pListView)
	{
		pListView->setMinimumWidth(260);
		pListView->setMaximumWidth(260);
	}

	long long currentIndex = -1;
	{
		util::Config config(QString("%1|%2").arg(__FUNCTION__).arg(__LINE__));
		QStringList paths = config.readArray<QString>("System", "Command", "DirPath");
		QStringList newPaths;

		if (paths.isEmpty())
		{
			QString path;

			if (!util::fileDialogShow(EXECUTIVE_FILE_SUFFIX, QFileDialog::AcceptOpen, &path, this) || path.isEmpty())
				return;

			paths.append(path);
		}

		for (const QString& path : paths)
		{
			if (path.contains(EXECUTIVE_FILE_SUFFIX, Qt::CaseInsensitive) && QFile::exists(path) && !newPaths.contains(path, Qt::CaseInsensitive))
			{
				newPaths.append(path);
			}
		}

		if (newPaths.isEmpty())
		{
			return;
		}

		currentIndex = ui.comboBox_paths->currentIndex();
		ui.comboBox_paths->blockSignals(true);
		ui.comboBox_paths->clear();
		for (const QString& it : newPaths)
		{
			//只顯示 上一級文件夾名稱/fileName
			QFileInfo fileInfo(it);
			QString fName = fileInfo.fileName();
			QString path = fileInfo.path();
			QFileInfo pathInfo(path);
			QString pathName = pathInfo.fileName();
			ui.comboBox_paths->addItem(pathName + "/" + fName, it);
		}

		config.writeArray<QString>("System", "Command", "DirPath", newPaths);
	}

	if (currentIndex != -1)
		ui.comboBox_paths->setCurrentIndex(currentIndex);
	ui.comboBox_paths->blockSignals(false);
}

void GeneralForm::onButtonClicked()
{
	PushButton* pPushButton = qobject_cast<PushButton*>(sender());
	if (!pPushButton)
		return;

	QString name = pPushButton->objectName();
	if (name.isEmpty())
		return;

	long long currentIndex = getIndex();

	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	if (name == "pushButton_addpath")
	{
		QString newPath;

		if (!util::fileDialogShow(EXECUTIVE_FILE_SUFFIX, QFileDialog::AcceptOpen, &newPath, this)
			|| newPath.isEmpty())
			return;

		if (!newPath.contains(EXECUTIVE_FILE_SUFFIX, Qt::CaseInsensitive) || !QFile::exists(newPath))
			return;

		util::Config config(QString("%1|%2").arg(__FUNCTION__).arg(__LINE__));
		QStringList paths = config.readArray<QString>("System", "Command", "DirPath");
		QStringList newPaths;

		for (const QString& path : paths)
		{
			if (path.contains(EXECUTIVE_FILE_SUFFIX, Qt::CaseInsensitive) && QFile::exists(path) && !newPaths.contains(path, Qt::CaseInsensitive))
			{
				newPaths.append(path);
			}
		}

		if (!newPaths.contains(newPath))
			newPaths.append(newPath);

		ui.comboBox_paths->blockSignals(true);
		ui.comboBox_paths->clear();
		for (const QString& it : newPaths)
		{
			//只顯示 上一級文件夾名稱/fileName
			QFileInfo fileInfo(it);
			QString path = fileInfo.path();
			QFileInfo pathInfo(path);
			QString pathName = pathInfo.fileName();
			ui.comboBox_paths->addItem(pathName + "/" + fileInfo.fileName(), it);
		}

		ui.comboBox_paths->setCurrentIndex(ui.comboBox_paths->count() - 1);
		ui.comboBox_paths->blockSignals(false);
		config.writeArray<QString>("System", "Command", "DirPath", newPaths);
		return;
	}

	if (name == "pushButton_setting")
	{
		if (ui.comboBox_setting->currentText().isEmpty())
			return;

		QString fileName = ui.comboBox_setting->currentData().toString();
		if (fileName.isEmpty())
			return;

		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
		emit signalDispatcher.loadHashSettings(fileName, true);
		return;
	}

	if (name == "pushButton_logout")
	{
		if (gamedevice.isValid() && !gamedevice.worker.isNull())
		{
			gamedevice.setEnableHash(util::kLogOutEnable, true);
		}
		return;
	}

	if (name == "pushButton_logback")
	{
		if (gamedevice.isValid() && !gamedevice.worker.isNull())
		{
			gamedevice.setEnableHash(util::kLogBackEnable, true);
		}

		return;
	}

	if (name == "pushButton_clear")
	{
		if (gamedevice.isValid() && !gamedevice.worker.isNull())
		{
			gamedevice.worker->cleanChatHistory();
		}

		return;
	}

	if (name == "pushButton_start")
	{
		onGameStart(0);
		return;
	}

	if (name == "pushButton_joingroup")
	{
		if (!gamedevice.worker.isNull())
			gamedevice.worker->setTeamState(true);
		return;
	}

	if (name == "pushButton_leavegroup")
	{
		if (!gamedevice.worker.isNull())
			gamedevice.worker->setTeamState(false);
		return;
	}

	if (name == "pushButton_dock")
	{
		bool flag = gamedevice.getEnableHash(util::kWindowDockEnable);
		gamedevice.setEnableHash(util::kWindowDockEnable, !flag);
		if (flag)
		{
			ui.pushButton_dock->setText(tr("dock"));
		}
		else
		{
			ui.pushButton_dock->setText(tr("undock"));
		}

		return;
	}

	if (name == "pushButton_afksetting")
	{

		if (pAfkForm_.isHidden())
			pAfkForm_.show();
		else
			pAfkForm_.hide();

		return;
	}

	if (name == "pushButton_watch")
	{

		return;
	}

	if (name == "pushButton_eo")
	{
		if (gamedevice.isValid() && !gamedevice.worker.isNull())
			gamedevice.setEnableHash(util::kEchoEnable, true);
		return;
	}

	if (name == "pushButton_savesettings")
	{
		QString fileName;
		if (!gamedevice.worker.isNull())
			fileName = gamedevice.worker->getCharacter().name;
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
		emit signalDispatcher.saveHashSettings(fileName);
		return;
	}

	if (name == "pushButton_loadsettings")
	{
		QString fileName;
		if (!gamedevice.worker.isNull())
			fileName = gamedevice.worker->getCharacter().name;
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
		emit signalDispatcher.loadHashSettings(fileName);
		return;
	}

	if (name == "pushButton_set")
	{

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

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	//login
	if (name == "checkBox_autologin")
	{
		gamedevice.setEnableHash(util::kAutoLoginEnable, isChecked);
		return;
	}

	if (name == "checkBox_autoreconnect")
	{
		gamedevice.setEnableHash(util::kAutoReconnectEnable, isChecked);
		return;
	}

	//support
	if (name == "checkBox_hidechar")
	{
		gamedevice.setEnableHash(util::kHideCharacterEnable, isChecked);
		return;
	}

	if (name == "checkBox_closeeffect")
	{
		gamedevice.setEnableHash(util::kCloseEffectEnable, isChecked);
		return;
	}

	if (name == "checkBox_autostartscript")
	{
		gamedevice.setEnableHash(util::kAutoStartScriptEnable, isChecked);
		return;
	}

	if (name == "checkBox_hidewindow")
	{
		gamedevice.setEnableHash(util::kHideWindowEnable, isChecked);
		if (isChecked)
			gamedevice.hide();
		else
			gamedevice.show();

		return;
	}

	if (name == "checkBox_mute")
	{
		gamedevice.setEnableHash(util::kMuteEnable, isChecked);
		return;
	}

	if (name == "checkBox_autojoin")
	{
		gamedevice.setEnableHash(util::kAutoJoinEnable, isChecked);
		long long type = gamedevice.getValueHash(util::kAutoFunTypeValue);
		if (isChecked && type != 0)
		{
			gamedevice.setValueHash(util::kAutoFunTypeValue, 0);
		}

		if (isChecked)
		{
			ui.checkBox_fastautowalk->setChecked(!isChecked);
			ui.checkBox_autowalk->setChecked(!isChecked);
		}

		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
		emit signalDispatcher.applyHashSettingsToUI();
		return;
	}

	if (name == "checkBox_locktime")
	{
		gamedevice.setEnableHash(util::kLockTimeEnable, isChecked);
		return;
	}

	if (name == "checkBox_autorestart")
	{
		gamedevice.setEnableHash(util::kAutoRestartGameEnable, isChecked);
		return;
	}

	//support2
	if (name == "checkBox_fastwalk")
	{
		gamedevice.setEnableHash(util::kFastWalkEnable, isChecked);
		return;
	}

	if (name == "checkBox_console")
	{
		HWND hWnd = util::getConsoleHandle();
		if (hWnd != nullptr)
		{
			if (isChecked)
			{
				ShowWindow(hWnd, SW_HIDE);
				ShowWindow(hWnd, SW_SHOW);
			}
			else
				ShowWindow(hWnd, SW_HIDE);

			return;
		}
		return;
	}

	if (name == "checkBox_lockmove")
	{
		gamedevice.setEnableHash(util::kLockMoveEnable, isChecked);
		gamedevice.sendMessage(kEnableMoveLock, isChecked, NULL);
		return;
	}

	if (name == "checkBox_luabattle")
	{
		gamedevice.setEnableHash(util::kBattleLuaModeEnable, isChecked);
		return;
	}

	if (name == "checkBox_forwardsend")
	{
		gamedevice.setEnableHash(util::kForwardSendEnable, isChecked);
		return;
	}

	if (name == "checkBox_lockimage")
	{
		gamedevice.setEnableHash(util::kLockImageEnable, isChecked);
		return;
	}

	if (name == "checkBox_autodropmeat")
	{
		gamedevice.setEnableHash(util::kAutoDropMeatEnable, isChecked);

		return;
	}

	if (name == "checkBox_autodrop")
	{
		bool bOriginal = gamedevice.getEnableHash(util::kAutoDropEnable);
		if (bOriginal == isChecked)
			return;

		gamedevice.setEnableHash(util::kAutoDropEnable, isChecked);

		if (!isChecked)
			return;

		QStringList srcList;
		QStringList dstList;
		QStringList srcSelectList;

		QVariant d = gamedevice.getUserData(util::kUserItemNames);
		if (d.isValid())
		{
			srcSelectList = d.toStringList();
		}

		QString src = gamedevice.getStringHash(util::kAutoDropItemString);
		if (!src.isEmpty())
		{
			srcList = src.split(util::rexOR, Qt::SkipEmptyParts);
		}

		if (!createSelectObjectForm(SelectObjectForm::kAutoDropItem, srcSelectList, srcList, &dstList, this))
			return;

		QString dst = dstList.join("|");
		gamedevice.setStringHash(util::kAutoDropItemString, dst);

		return;
	}

	if (name == "checkBox_autostack")
	{
		gamedevice.setEnableHash(util::kAutoStackEnable, isChecked);
		return;
	}

	if (name == "checkBox_knpc")
	{
		gamedevice.setEnableHash(util::kKNPCEnable, isChecked);
		return;
	}

	if (name == "checkBox_autoanswer")
	{
		gamedevice.setEnableHash(util::kAutoAnswerEnable, isChecked);
		return;
	}

	if (name == "checkBox_autoeatbean")
	{
		gamedevice.setEnableHash(util::kAutoEatBeanEnable, isChecked);
		return;
	}

	//battle
	if (name == "checkBox_autowalk")
	{
		if (isChecked)
		{
			ui.checkBox_fastautowalk->setChecked(!isChecked);
			ui.checkBox_autojoin->setChecked(!isChecked);
		}
		gamedevice.setEnableHash(util::kAutoWalkEnable, isChecked);
		return;
	}

	if (name == "checkBox_fastautowalk")
	{
		if (isChecked)
		{
			ui.checkBox_autowalk->setChecked(!isChecked);
			ui.checkBox_autojoin->setChecked(!isChecked);
		}
		gamedevice.setEnableHash(util::kFastAutoWalkEnable, isChecked);
		return;
	}

	if (name == "checkBox_fastbattle")
	{
		gamedevice.setEnableHash(util::kFastBattleEnable, isChecked);

		if (isChecked)
		{
			ui.checkBox_autobattle->setChecked(!isChecked);
			if (!gamedevice.worker.isNull())
			{
				gamedevice.worker->echo();
			}
		}
		return;
	}

	if (name == "checkBox_autobattle")
	{
		gamedevice.setEnableHash(util::kAutoBattleEnable, isChecked);

		if (isChecked)
		{
			ui.checkBox_fastbattle->setChecked(!isChecked);
			if (!gamedevice.worker.isNull())
			{
				gamedevice.worker->echo();
			}
		}
		return;
	}

	if (name == "checkBox_autocatch")
	{
		gamedevice.setEnableHash(util::kAutoCatchEnable, isChecked);
		return;
	}

	if (name == "checkBox_autoescape")
	{
		gamedevice.setEnableHash(util::kAutoEscapeEnable, isChecked);
		return;
	}

	if (name == "checkBox_lockattck")
	{
		bool bOriginal = gamedevice.getEnableHash(util::kLockAttackEnable);
		if (bOriginal == isChecked)
			return;

		gamedevice.setEnableHash(util::kLockAttackEnable, isChecked);

		if (!isChecked)
			return;

		QStringList srcList;
		QStringList dstList;
		QStringList srcSelectList;

		QVariant d = gamedevice.getUserData(util::kUserEnemyNames);
		if (d.isValid())
			srcSelectList = d.toStringList();

		for (QString& it : srcSelectList)
		{
			it.prepend("%(ename) == ");
		}

		QString src = gamedevice.getStringHash(util::kLockAttackString);
		if (!src.isEmpty())
			srcList = src.split(util::rexOR, Qt::SkipEmptyParts);

		if (!createSelectObjectForm(SelectObjectForm::kLockAttack, srcSelectList, srcList, &dstList, this))
			return;

		QString dst = dstList.join("|");
		gamedevice.setStringHash(util::kLockAttackString, dst);
		return;
	}

	if (name == "checkBox_lockescape")
	{
		bool bOriginal = gamedevice.getEnableHash(util::kLockEscapeEnable);
		if (bOriginal == isChecked)
			return;

		gamedevice.setEnableHash(util::kLockEscapeEnable, isChecked);

		if (!isChecked)
			return;

		QStringList srcList;
		QStringList dstList;
		QStringList srcSelectList;

		QVariant d = gamedevice.getUserData(util::kUserEnemyNames);
		if (d.isValid())
			srcSelectList = d.toStringList();

		QString src = gamedevice.getStringHash(util::kLockEscapeString);
		if (!src.isEmpty())
			srcList = src.split(util::rexOR, Qt::SkipEmptyParts);

		if (!createSelectObjectForm(SelectObjectForm::kLockEscape, srcSelectList, srcList, &dstList, this))
			return;

		QString dst = dstList.join("|");
		gamedevice.setStringHash(util::kLockEscapeString, dst);
		return;
	}

	if (name == "checkBox_battletimeextend")
	{
		gamedevice.setEnableHash(util::kBattleTimeExtendEnable, true);
		return;
	}

	if (name == "checkBox_falldownescape")
	{
		gamedevice.setEnableHash(util::kFallDownEscapeEnable, isChecked);
		return;
	}

	if (name == "checkBox_showexp")
	{
		gamedevice.setEnableHash(util::kShowExpEnable, isChecked);
		return;
	}

	if (name == "checkBox_autoswitch")
	{
		gamedevice.setEnableHash(util::kBattleAutoSwitchEnable, isChecked);
		return;
	}

	if (name == "checkBox_battleautoeo")
	{
		gamedevice.setEnableHash(util::kBattleAutoEOEnable, isChecked);
		return;
	}

	//shortcut switcher
	if (name == "checkBox_switcher_team")
	{
		gamedevice.setEnableHash(util::kSwitcherTeamEnable, isChecked);
		return;
	}

	if (name == "checkBox_switcher_pk")
	{
		gamedevice.setEnableHash(util::kSwitcherPKEnable, isChecked);
		return;
	}

	if (name == "checkBox_switcher_card")
	{
		gamedevice.setEnableHash(util::kSwitcherCardEnable, isChecked);
		return;
	}

	if (name == "checkBox_switcher_trade")
	{
		gamedevice.setEnableHash(util::kSwitcherTradeEnable, isChecked);
		return;
	}

	if (name == "checkBox_switcher_group")
	{
		gamedevice.setEnableHash(util::kSwitcherGroupEnable, isChecked);
		return;
	}

	if (name == "checkBox_switcher_family")
	{
		gamedevice.setEnableHash(util::kSwitcherFamilyEnable, isChecked);
		return;
	}

	if (name == "checkBox_switcher_job")
	{
		gamedevice.setEnableHash(util::kSwitcherJobEnable, isChecked);
		return;
	}

	else if (name == "checkBox_switcher_world")
	{
		gamedevice.setEnableHash(util::kSwitcherWorldEnable, isChecked);
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

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	if (name == "spinBox_speedboost")
	{
		gamedevice.setValueHash(util::kSpeedBoostValue, value);
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

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (name == "comboBox_serverlist")
	{
		{
			util::Config config(QString("%1|%2").arg(__FUNCTION__).arg(__LINE__));
			config.write("System", "Server", "LastServerListSelection", ui.comboBox_serverlist->currentIndex());
		}

		emit ui.comboBox_server->clicked();
		return;
	}

	if (name == "comboBox_server")
	{
		gamedevice.setValueHash(util::kServerValue, value);
		serverListReLoad();
		return;
	}

	if (name == "comboBox_subserver")
	{
		gamedevice.setValueHash(util::kSubServerValue, value);
		return;
	}

	if (name == "comboBox_position")
	{
		gamedevice.setValueHash(util::kPositionValue, value);
		return;
	}

	if (name == "comboBox_locktime")
	{
		gamedevice.setValueHash(util::kLockTimeValue, value);
		if (ui.checkBox_locktime->isChecked())
			gamedevice.sendMessage(kSetTimeLock, true, value);
		return;
	}

	if (name == "comboBox_paths")
	{
		util::Config config(QString("%1|%2").arg(__FUNCTION__).arg(__LINE__));
		long long current = ui.comboBox_paths->currentIndex();
		if (current >= 0)
			config.write("System", "Command", "LastSelection", ui.comboBox_paths->currentIndex());
		return;
	}
}

void GeneralForm::onApplyHashSettingsToUI()
{
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	QHash<util::UserSetting, bool> enableHash = gamedevice.getEnablesHash();
	QHash<util::UserSetting, long long> valueHash = gamedevice.getValuesHash();
	QHash<util::UserSetting, QString> stringHash = gamedevice.getStringsHash();

	{
		util::Config config(QString("%1|%2").arg(__FUNCTION__).arg(__LINE__));
		long long index = config.read<long long>("System", "Command", "LastSelection");

		if (index >= 0 && index < ui.comboBox_paths->count())
		{
			ui.comboBox_paths->blockSignals(true);
			ui.comboBox_paths->setCurrentIndex(index);
			ui.comboBox_paths->blockSignals(false);
		}
		else if (ui.comboBox_paths->count() > 0)
			ui.comboBox_paths->setCurrentIndex(0);

		long long count = config.read<long long>("System", "Server", "ListCount");
		if (count <= 0)
		{
			count = 3;
			config.write("System", "Server", "ListCount", count);
		}

		ui.comboBox_serverlist->blockSignals(true);

		ui.comboBox_serverlist->clear();
		for (long long i = 0; i < count; ++i)
		{
			ui.comboBox_serverlist->addItem(tr("ServerList%1").arg(i + 1), i);
		}

		long long lastServerListSelection = config.read<long long>("System", "Server", "LastServerListSelection");
		if (lastServerListSelection >= 0 && lastServerListSelection < count)
			ui.comboBox_serverlist->setCurrentIndex(lastServerListSelection);
		else if (ui.comboBox_serverlist->count() > 0)
			ui.comboBox_serverlist->setCurrentIndex(0);

		ui.comboBox_serverlist->blockSignals(false);
	}

	long long value = 0;

	//login
	value = valueHash.value(util::kServerValue);
	if (value >= 0 && value < ui.comboBox_server->count())
		ui.comboBox_server->setCurrentIndex(value);
	else
		ui.comboBox_server->setCurrentIndex(0);

	serverListReLoad();

	value = valueHash.value(util::kSubServerValue);
	if (value >= 0 && value < ui.comboBox_subserver->count())
		ui.comboBox_subserver->setCurrentIndex(value);
	else
		ui.comboBox_subserver->setCurrentIndex(0);

	value = valueHash.value(util::kPositionValue);
	if (value >= 0 && value < ui.comboBox_position->count())
		ui.comboBox_position->setCurrentIndex(value);
	else
		ui.comboBox_position->setCurrentIndex(0);

	if (enableHash.value(util::kWindowDockEnable))
		ui.pushButton_dock->setText(tr("undock"));
	else
		ui.pushButton_dock->setText(tr("dock"));

	ui.comboBox_locktime->setCurrentIndex(valueHash.value(util::kLockTimeValue));
	ui.checkBox_autologin->setChecked(enableHash.value(util::kAutoLoginEnable));
	ui.checkBox_autoreconnect->setChecked(enableHash.value(util::kAutoReconnectEnable));

	//support
	ui.checkBox_hidechar->setChecked(enableHash.value(util::kHideCharacterEnable));
	ui.checkBox_closeeffect->setChecked(enableHash.value(util::kCloseEffectEnable));
	ui.checkBox_autostartscript->setChecked(enableHash.value(util::kAutoStartScriptEnable));
	ui.checkBox_hidewindow->setChecked(enableHash.value(util::kHideWindowEnable));
	ui.checkBox_mute->setChecked(enableHash.value(util::kMuteEnable));
	ui.checkBox_autojoin->setChecked(enableHash.value(util::kAutoJoinEnable));
	ui.checkBox_locktime->setChecked(enableHash.value(util::kLockTimeEnable));
	ui.checkBox_autorestart->setChecked(enableHash.value(util::kAutoRestartGameEnable));
	ui.checkBox_showexp->setChecked(enableHash.value(util::kShowExpEnable));

	//sp
	ui.spinBox_speedboost->setValue(valueHash.value(util::kSpeedBoostValue));


	//support2
	ui.checkBox_fastwalk->setChecked(enableHash.value(util::kFastWalkEnable));
	ui.checkBox_lockmove->setChecked(enableHash.value(util::kLockMoveEnable));
	//ui.checkBox_lockimage->setChecked(enableHash.value(util::kLockImageEnable));
	ui.checkBox_luabattle->setChecked(enableHash.value(util::kBattleLuaModeEnable));
	ui.checkBox_forwardsend->setChecked(enableHash.value(util::kForwardSendEnable));
	ui.checkBox_autodropmeat->setChecked(enableHash.value(util::kAutoDropMeatEnable));
	ui.checkBox_autodrop->setChecked(enableHash.value(util::kAutoDropEnable));
	ui.checkBox_autostack->setChecked(enableHash.value(util::kAutoStackEnable));
	ui.checkBox_knpc->setChecked(enableHash.value(util::kKNPCEnable));
	ui.checkBox_autoanswer->setChecked(enableHash.value(util::kAutoAnswerEnable));
	ui.checkBox_autoeatbean->setChecked(enableHash.value(util::kAutoEatBeanEnable));

	//battle
	ui.checkBox_autowalk->setChecked(enableHash.value(util::kAutoWalkEnable));
	ui.checkBox_fastautowalk->setChecked(enableHash.value(util::kFastAutoWalkEnable));
	ui.checkBox_fastbattle->setChecked(enableHash.value(util::kFastBattleEnable));
	ui.checkBox_autobattle->setChecked(enableHash.value(util::kAutoBattleEnable));
	ui.checkBox_autocatch->setChecked(enableHash.value(util::kAutoCatchEnable));
	ui.checkBox_lockattck->setChecked(enableHash.value(util::kLockAttackEnable));
	ui.checkBox_autoescape->setChecked(enableHash.value(util::kAutoEscapeEnable));
	ui.checkBox_lockescape->setChecked(enableHash.value(util::kLockEscapeEnable));
	ui.checkBox_battletimeextend->setChecked(true);//enableHash.value(util::kBattleTimeExtendEnable)
	ui.checkBox_falldownescape->setChecked(enableHash.value(util::kFallDownEscapeEnable));
	ui.checkBox_autoswitch->setChecked(enableHash.value(util::kBattleAutoSwitchEnable));
	ui.checkBox_battleautoeo->setChecked(enableHash.value(util::kBattleAutoEOEnable));

	//switcher
	ui.checkBox_switcher_team->setChecked(enableHash.value(util::kSwitcherTeamEnable));
	ui.checkBox_switcher_pk->setChecked(enableHash.value(util::kSwitcherPKEnable));
	ui.checkBox_switcher_card->setChecked(enableHash.value(util::kSwitcherCardEnable));
	ui.checkBox_switcher_trade->setChecked(enableHash.value(util::kSwitcherTradeEnable));
	ui.checkBox_switcher_group->setChecked(enableHash.value(util::kSwitcherGroupEnable));
	ui.checkBox_switcher_family->setChecked(enableHash.value(util::kSwitcherFamilyEnable));
	ui.checkBox_switcher_job->setChecked(enableHash.value(util::kSwitcherJobEnable));
	ui.checkBox_switcher_world->setChecked(enableHash.value(util::kSwitcherWorldEnable));
}

void GeneralForm::onGameStart(long long delay)
{
	ui.pushButton_start->setEnabled(false);

	//視情況延遲啟動
	if (delay > 0)
	{
		//創建一個定時器和事件循環器
		QTimer timer;
		timer.setSingleShot(true);
		QEventLoop loop;
		connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
		timer.start(delay);
		loop.exec(); //進入事件循環，等待定時器超時，這裡用途是防止阻塞等待時造成界面無響應
	}

	do
	{
		QString path = ui.comboBox_paths->currentData().toString();
		if (path.isEmpty())
			break;

		QFileInfo fileInfo(path);
		if (!fileInfo.exists() || fileInfo.isDir() || fileInfo.suffix() != "exe")
			break;

		QString dirPath = fileInfo.absolutePath();
		QByteArray dirPathUtf8 = dirPath.toUtf8();
		qputenv("GAME_DIR_PATH", dirPathUtf8);

		ThreadManager& thread_manager = ThreadManager::getInstance();

		long long currentIndex = getIndex();

		GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
		gamedevice.currentGameExePath = path;

		MainObject* pMainObject = nullptr;
		if (!thread_manager.createThread(currentIndex, &pMainObject, nullptr) || (nullptr == pMainObject))
			break;

		connect(pMainObject, &MainObject::destroyed, this, &GeneralForm::onMainThradDestroyed, Qt::QueuedConnection);

		pMainObject->thread.start();



		return;
	} while (false);

	ui.pushButton_start->setEnabled(true);
}

//遊戲主線程銷毀之後判斷是否需要重啟遊戲
void GeneralForm::onMainThradDestroyed()
{
	GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	if (gamedevice.getEnableHash(util::kAutoRestartGameEnable))
	{
		onGameStart(5000);
		return;
	}

	//不需要重啟遊戲的話就啟用開始按鈕
	ui.pushButton_start->setEnabled(true);
}

void GeneralForm::createServerList()
{
	long long currentIndex = getIndex();
	long long currentListIndex = ui.comboBox_serverlist->currentIndex();
	if (currentListIndex < 0)
		currentListIndex = 0;
	QStringList list;
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	{
		util::Config config(QString("%1|%2").arg(__FUNCTION__).arg(__LINE__));

		gamedevice.currentServerListIndex.set(currentListIndex);
		list = config.readArray<QString>("System", "Server", QString("List_%1").arg(currentListIndex));
		if (list.isEmpty())
		{
			static const QStringList defaultListSO = {
				"1|「活动互动」1线|活动电信1线, 活动联通1线, 活动移动1线, 活动海外1线",
				"2|「摆摊交易」4线|摆摊电信4线, 摆摊联通4线, 摆摊移动4线, 摆摊海外4线",
				"3|练级挂机5线|族战电信5线, 族战联通5线, 石器移动5线, 石器海外5线",
				"4|练级挂机6线|石器电信6线, 石器联通6线, 石器移动6线, 石器海外6线",
				"5|「庄园族战」2线|族战电信2线, 族战联通2线, 族战移动2线, 族战海外2线",
				"6|练级挂机7线|石器挂机7线",
				"7|练级挂机8线|石器电信8线, 石器联通8线",
				"8|练级挂机13|电信 13专线, 联通 13专线, 移动 13专线, 海外 13专线",
				"9|「双号副本」3线|石器电信3线, 石器联通3线, 石器移动3线, 石器海外3线",
				"10|「练级副本」9线|石器电信9线, 石器联通9线",
				"11|10 19 20线|挂机 10专线, 备用 10专线, 电信 19专线, 其他 19专线, 电信 20专线, 其他 20专线",
				"12|11 21 22线|挂机 11专线, 备用 11专线, 电信 21专线, 其他 21专线, 电信 22专线, 其他 22专线, 电信 23专线, 其他 23专线",
				"13|全球加速12|电信 12专线, 联通 12专线, 移动 12专线, 港澳台 12线, 美国 12专线",
				"14|14 16 17 18|电信 14专线, 联通移动14线, 移动 14专线, 海外 14专线,电信 16专线, 联通 16专线, 移动 16专线, 海外 16专线, 电信 17专线, 联通 17专线, 移动 17专线, 海外 17专线, 电信 18专线, 联通 18专线, 移动 18专线",
				"15|公司专线15|公司电信15, 公司联通15, 「活动互动」1线, 「摆摊交易」4线",
			};

			static const QStringList defaultListSE = {
				"1|1线∥活动互动|电信活动1线, 联通活动1线, 移动活动1线, 海外活动1线",
				"2|2线∥摆摊交易|电信摆摊2线, 联通摆摊2线, 移动摆摊2线, 海外摆摊2线",
				"3|3线∥庄园族战|电信族战3线, 联通族战3线, 移动族战3线, 海外族战3线",
				"4|4线∥练级挂机|电信练级4线, 联通练级4线, 移动练级4线, 海外练级4线",
				"5|5线∥练级挂机|电信练级5线, 联通练级5线, 移动练级5线, 海外练级5线",
				"6|6线∥练级挂机|电信练级6线, 联通练级6线, 移动练级6线, 海外练级6线",
				"7|7线∥练级挂机|电信练级7线, 联通练级7线, 移动练级7线, 海外练级7线",
				"8|8线∥练级挂机|电信练级8线, 联通练级8线, 移动练级8线, 海外练级8线, 备用练级8线",
				"9|9线∥练级挂机|电信练级9线, 联通练级9线, 移动练级9线, 海外练级9线, 备用练级9线",
				"10|15线∥公司专线|电信公司15线, 联通公司15线, 移动公司15线, 海外公司15线",
				"11|21线∥会员专线|电信会员21线, 联通会员21线, 移动会员21线, 海外会员21线",
				"12|22线∥会员专线|电信会员22线, 联通会员22线, 移动会员22线, 海外会员22线",
			};

			static const QStringList defaultListXGSA = {
				"1|满天星|电信线路, 网通线路",
				"2|薰衣草|电信线路, 网通线路",
				"3|紫罗兰|电信线路, 网通线路",
				"4|风信子|电信线路, 网通线路",
				"5|待宵草|电信线路, 网通线路",
				"6|欧薄菏|电信线路, 网通线路",
				"7|车前草|电信线路, 网通线路",
				"8|石竹花|电信线路, 网通线路",
				"9|勿忘我|电信线路, 网通线路",
			};

			const QList<QStringList> defaultList = {
				defaultListSO,
				defaultListSE,
				defaultListXGSA,
			};

			for (long long i = 0; i < defaultList.size(); ++i)
				config.writeArray<QString>("System", "Server", QString("List_%1").arg(i), defaultList.value(i));

			if (currentListIndex >= 0 && currentListIndex < defaultList.size())
				list = defaultList.value(currentListIndex);
		}
	}

	QString currentText = ui.comboBox_server->currentText();
	long long current = ui.comboBox_server->currentIndex();

	ui.comboBox_server->setUpdatesEnabled(false);
	ui.comboBox_subserver->setUpdatesEnabled(false);
	ui.comboBox_server->clear();
	ui.comboBox_subserver->clear();

	QStringList serverNameList;
	QStringList subServerNameList;
	for (const QString& it : list)
	{
		QStringList subList = it.split(util::rexOR, Qt::SkipEmptyParts);
		if (subList.isEmpty())
			continue;

		if (subList.size() != 3)
			continue;

		QString indexStr = subList.takeFirst();
		//檢查是否為數字
		if (indexStr.toLongLong() <= 0)
			continue;

		QString server = subList.takeFirst();

		subList = subList.first().split(util::rexComma, Qt::SkipEmptyParts);
		if (subList.isEmpty())
			continue;

		serverNameList.append(server);
		subServerNameList.append(subList);

		serverList[currentListIndex].insert(server, subList);
		ui.comboBox_server->addItem(server);
		ui.comboBox_subserver->addItems(subList);
	}

	gamedevice.serverNameList.set(serverNameList);
	gamedevice.subServerNameList.set(subServerNameList);

	if (current >= 0)
		ui.comboBox_server->setCurrentIndex(current);
	else
		ui.comboBox_server->setCurrentIndex(0);
	ui.comboBox_subserver->setCurrentIndex(0);
	ui.comboBox_server->setUpdatesEnabled(true);
	ui.comboBox_subserver->setUpdatesEnabled(true);
}

void GeneralForm::serverListReLoad()
{
	long long current = ui.comboBox_subserver->currentIndex();
	long long currentServerList = ui.comboBox_serverlist->currentIndex();
	if (currentServerList < 0)
		currentServerList = 0;

	ui.comboBox_subserver->setUpdatesEnabled(false);
	ui.comboBox_subserver->clear();
	ui.comboBox_subserver->addItems(serverList.value(currentServerList).value(ui.comboBox_server->currentText()));
	if (current >= 0)
		ui.comboBox_subserver->setCurrentIndex(current);
	else
		ui.comboBox_subserver->setCurrentIndex(0);
	ui.comboBox_subserver->setUpdatesEnabled(true);
}