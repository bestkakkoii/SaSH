#include "stdafx.h"
#include "generalform.h"
#include "selectobjectform.h"

#include "mainthread.h"
#include <util.h>
#include <injector.h>
#include "signaldispatcher.h"

//#ifndef _DEBUG
//#include "net/webauthenticator.h"
//#endif

GeneralForm::GeneralForm(QWidget* parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	connect(this, &GeneralForm::resetControlTextLanguage, this, &GeneralForm::onResetControlTextLanguage, Qt::UniqueConnection);

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	connect(&signalDispatcher, &SignalDispatcher::setStartButtonEnabled, ui.pushButton_start, &QPushButton::setEnabled, Qt::UniqueConnection);
	connect(&signalDispatcher, &SignalDispatcher::applyHashSettingsToUI, this, &GeneralForm::onApplyHashSettingsToUI, Qt::UniqueConnection);
	connect(&signalDispatcher, &SignalDispatcher::gameStarted, this, &GeneralForm::onGameStart, Qt::UniqueConnection);


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
		{
			connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onComboBoxCurrentIndexChanged(int)), Qt::UniqueConnection);
			connect(comboBox, &ComboBox::clicked, this, &GeneralForm::onComboBoxClicked, Qt::UniqueConnection);
		}
	}

	emit ui.comboBox_paths->clicked();
	emit signalDispatcher.applyHashSettingsToUI();

	//驗證
#ifdef WEBAUTHENTICATOR_H
#ifndef _DEBUG
	static bool isFirstInstance = false;
	if (!isFirstInstance)
	{

		QtConcurrent::run([]()
			{
				Net::Authenticator* g_Authenticator = Net::Authenticator::getInstance();
				QScopedPointer<QString> username(new QString("satester"));
				QScopedPointer<QString> encode_password(new QString("AwJk8DlkCUVxRMgaHDEMEHQR"));
				if (g_Authenticator->Login(*username, *encode_password))
					isFirstInstance = true;
				else
					MINT::NtTerminateProcess(GetCurrentProcess(), 0);
			});
	}
#endif
#endif
}

GeneralForm::~GeneralForm()
{
	ThreadManager& thread_manager = ThreadManager::getInstance();
	thread_manager.close();
	qDebug() << "~GeneralForm()";
}

void GeneralForm::onResetControlTextLanguage()
{
	const QStringList positionList = { tr("Left"), tr("Right") };

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
		return;

	QString name = pComboBox->objectName();
	if (name.isEmpty())
		return;

	if (name == "comboBox_setting")
	{
		QVector<QPair<QString, QString>> fileList;
		if (!util::enumAllFiles(util::applicationDirPath() + "/settings", ".json", &fileList))
			return;

		int currentIndex = ui.comboBox_setting->currentIndex();
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
		QListView* pListView = qobject_cast<QListView*>(ui.comboBox_paths->view());
		if (pListView)
		{
			pListView->setMinimumWidth(260);
			pListView->setMaximumWidth(260);
		}

		const QString fileName(qgetenv("JSON_PATH"));
		if (fileName.isEmpty())
			return;

		util::Config config(fileName);
		QStringList paths = config.readStringArray("System", "Command", "DirPath");
		QStringList newPaths;

		if (paths.isEmpty())
		{
			QString path;
			if (!util::createFileDialog(util::SA_NAME, &path, this))
				return;
			else
				paths.append(path);
		}

		for (const QString& path : paths)
		{
			if (path.contains(util::SA_NAME) && QFile::exists(path) && !newPaths.contains(path))
			{
				newPaths.append(path);
			}
		}

		if (newPaths.isEmpty())
			return;

		int currentIndex = ui.comboBox_paths->currentIndex();
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
		config.writeStringArray("System", "Command", "DirPath", newPaths);
		ui.comboBox_paths->setCurrentIndex(currentIndex);
		ui.comboBox_paths->blockSignals(false);
		return;
	}
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

	if (name == "pushButton_addpath")
	{
		const QString fileName(qgetenv("JSON_PATH"));
		if (fileName.isEmpty())
			return;

		QString newPath;
		if (!util::createFileDialog(util::SA_NAME, &newPath, this))
			return;

		if (!newPath.contains(util::SA_NAME) || !QFile::exists(newPath))
			return;

		util::Config config(fileName);
		QStringList paths = config.readStringArray("System", "Command", "DirPath");
		QStringList newPaths;

		for (const QString& path : paths)
		{
			if (path.contains(util::SA_NAME) && QFile::exists(path) && !newPaths.contains(path))
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
			QString fileName = fileInfo.fileName();
			QString path = fileInfo.path();
			QFileInfo pathInfo(path);
			QString pathName = pathInfo.fileName();
			ui.comboBox_paths->addItem(pathName + "/" + fileName, it);
		}

		ui.comboBox_paths->setCurrentIndex(ui.comboBox_paths->count() - 1);
		ui.comboBox_paths->blockSignals(false);
		config.writeStringArray("System", "Command", "DirPath", newPaths);
	}

	if (name == "pushButton_setting")
	{
		if (ui.comboBox_setting->currentText().isEmpty())
			return;

		QString fileName = ui.comboBox_setting->currentData().toString();
		if (fileName.isEmpty())
			return;

		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
		emit signalDispatcher.loadHashSettings(fileName, true);
	}

	if (name == "pushButton_logout")
	{
		//bool flag = injector.getEnableHash(util::kLogOutEnable);
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

	if (name == "pushButton_logback")
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

	if (name == "pushButton_clear")
	{
		if (injector.isValid() && !injector.server.isNull())
		{
			injector.server->cleanChatHistory();
		}

		return;
	}

	if (name == "pushButton_start")
	{
		onGameStart();
		return;
	}

	if (name == "pushButton_joingroup")
	{
		return;
	}

	if (name == "pushButton_leavegroup")
	{

		return;
	}

	if (name == "pushButton_dock")
	{
		bool flag = injector.getEnableHash(util::kWindowDockEnable);
		injector.setEnableHash(util::kWindowDockEnable, !flag);
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

	if (name == "pushButton_pick")
	{
		injector.server->useMagic(CHAR_EQUIPPLACENUM, 0);
		return;
	}
	if (name == "pushButton_watch")
	{

		return;
	}

	if (name == "pushButton_eo")
	{
		injector.setEnableHash(util::kEchoEnable, true);
		return;
	}

	if (name == "pushButton_savesettings")
	{
		QString fileName;
		if (!injector.server.isNull())
			fileName = injector.server->getPC().name;
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
		emit signalDispatcher.saveHashSettings(fileName);
		return;
	}

	if (name == "pushButton_loadsettings")
	{
		QString fileName;
		if (!injector.server.isNull())
			fileName = injector.server->getPC().name;
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
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

	Injector& injector = Injector::getInstance();

	//login
	if (name == "checkBox_autologin")
	{
		injector.setEnableHash(util::kAutoLoginEnable, isChecked);
		return;
	}

	if (name == "checkBox_autoreconnect")
	{
		injector.setEnableHash(util::kAutoReconnectEnable, isChecked);
		return;
	}

	//support
	if (name == "checkBox_hidechar")
	{
		injector.setEnableHash(util::kHideCharacterEnable, isChecked);
		return;
	}

	if (name == "checkBox_closeeffect")
	{
		injector.setEnableHash(util::kCloseEffectEnable, isChecked);
		return;
	}

	if (name == "checkBox_optimize")
	{
		injector.setEnableHash(util::kOptimizeEnable, isChecked);
		return;
	}

	if (name == "checkBox_hidewindow")
	{
		injector.setEnableHash(util::kHideWindowEnable, isChecked);
		if (isChecked)
			injector.hide();
		else
			injector.show();

		return;
	}

	if (name == "checkBox_mute")
	{
		injector.setEnableHash(util::kMuteEnable, isChecked);
		return;
	}

	if (name == "checkBox_autojoin")
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

	if (name == "checkBox_locktime")
	{
		injector.setEnableHash(util::kLockTimeEnable, isChecked);
		return;
	}

	if (name == "checkBox_autofreememory")
	{
		injector.setEnableHash(util::kAutoFreeMemoryEnable, isChecked);
		return;
	}

	//support2
	if (name == "checkBox_fastwalk")
	{
		injector.setEnableHash(util::kFastWalkEnable, isChecked);
		return;
	}

	if (name == "checkBox_passwall")
	{
		injector.setEnableHash(util::kPassWallEnable, isChecked);
		return;
	}

	if (name == "checkBox_lockmove")
	{
		injector.setEnableHash(util::kLockMoveEnable, isChecked);
		injector.sendMessage(Injector::kEnableMoveLock, isChecked, NULL);
		return;
	}

	if (name == "checkBox_lockimage")
	{
		injector.setEnableHash(util::kLockImageEnable, isChecked);
		return;
	}

	if (name == "checkBox_autodropmeat")
	{
		injector.setEnableHash(util::kAutoDropMeatEnable, isChecked);

		return;
	}

	if (name == "checkBox_autodrop")
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

	if (name == "checkBox_autostack")
	{
		injector.setEnableHash(util::kAutoStackEnable, isChecked);
		return;
	}

	if (name == "checkBox_knpc")
	{
		injector.setEnableHash(util::kKNPCEnable, isChecked);
		return;
	}

	if (name == "checkBox_autoanswer")
	{
		injector.setEnableHash(util::kAutoAnswerEnable, isChecked);
		return;
	}

	if (name == "checkBox_autoeatbean")
	{
		injector.setEnableHash(util::kAutoEatBeanEnable, isChecked);
		return;
	}

	//battle
	if (name == "checkBox_autowalk")
	{
		if (isChecked)
		{
			ui.checkBox_fastautowalk->setChecked(!isChecked);
		}
		injector.setEnableHash(util::kAutoWalkEnable, isChecked);
		return;
	}

	if (name == "checkBox_fastautowalk")
	{
		if (isChecked)
		{
			ui.checkBox_autowalk->setChecked(!isChecked);
		}
		injector.setEnableHash(util::kFastAutoWalkEnable, isChecked);
		return;
	}

	if (name == "checkBox_fastbattle")
	{
		if (isChecked)
		{
			ui.checkBox_autobattle->setChecked(!isChecked);
		}

		bool bOriginal = injector.getEnableHash(util::kFastBattleEnable);
		injector.setEnableHash(util::kFastBattleEnable, isChecked);
		if (!bOriginal && isChecked && !injector.server.isNull())
		{
			injector.server->asyncBattleWork(false);//async
		}
		return;
	}

	if (name == "checkBox_autobattle")
	{
		if (isChecked)
		{
			ui.checkBox_fastbattle->setChecked(!isChecked);
		}

		bool bOriginal = injector.getEnableHash(util::kAutoBattleEnable);
		injector.setEnableHash(util::kAutoBattleEnable, isChecked);
		if (!bOriginal && isChecked && !injector.server.isNull())
		{
			injector.server->asyncBattleWork(false);//async
		}

		return;
	}

	if (name == "checkBox_autocatch")
	{
		injector.setEnableHash(util::kAutoCatchEnable, isChecked);
		return;
	}

	if (name == "checkBox_lockattck")
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

	if (name == "checkBox_autoescape")
	{
		injector.setEnableHash(util::kAutoEscapeEnable, isChecked);
		return;
	}

	if (name == "checkBox_lockescape")
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

	if (name == "checkBox_battletimeextend")
	{
		injector.setEnableHash(util::kBattleTimeExtendEnable, isChecked);
		return;
	}

	if (name == "checkBox_falldownescape")
	{
		injector.setEnableHash(util::kFallDownEscapeEnable, isChecked);
		return;
	}

	if (name == "checkBox_showexp")
	{
		injector.setEnableHash(util::kShowExpEnable, isChecked);
		return;
	}

	//shortcut switcher
	if (name == "checkBox_switcher_team")
	{
		injector.setEnableHash(util::kSwitcherTeamEnable, isChecked);
		return;
	}

	if (name == "checkBox_switcher_pk")
	{
		injector.setEnableHash(util::kSwitcherPKEnable, isChecked);
		return;
	}

	if (name == "checkBox_switcher_card")
	{
		injector.setEnableHash(util::kSwitcherCardEnable, isChecked);
		return;
	}

	if (name == "checkBox_switcher_trade")
	{
		injector.setEnableHash(util::kSwitcherTradeEnable, isChecked);
		return;
	}

	if (name == "checkBox_switcher_family")
	{
		injector.setEnableHash(util::kSwitcherFamilyEnable, isChecked);
		return;
	}

	if (name == "checkBox_switcher_job")
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
	if (name == "comboBox_serverlist")
	{
		const QString fileName(qgetenv("JSON_PATH"));
		if (fileName.isEmpty())
			return;

		{
			util::Config config(fileName);
			config.write("System", "Server", "LastServerListSelection", ui.comboBox_serverlist->currentIndex());
		}

		emit ui.comboBox_server->clicked();
	}

	if (name == "comboBox_server")
	{
		injector.setValueHash(util::kServerValue, value);
		int currentIndex = ui.comboBox_subserver->currentIndex();
		int currentServerList = ui.comboBox_serverlist->currentIndex();
		if (currentServerList < 0)
			currentServerList = 0;

		ui.comboBox_subserver->setUpdatesEnabled(false);
		ui.comboBox_subserver->clear();
		ui.comboBox_subserver->addItems(serverList.value(currentServerList).value(ui.comboBox_server->currentText()));
		if (currentIndex >= 0)
			ui.comboBox_subserver->setCurrentIndex(currentIndex);
		else
			ui.comboBox_subserver->setCurrentIndex(0);
		ui.comboBox_subserver->setUpdatesEnabled(true);
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

	if (name == "comboBox_paths")
	{
		const QString fileName(qgetenv("JSON_PATH"));
		if (fileName.isEmpty())
			return;
		util::Config config(fileName);
		int current = ui.comboBox_paths->currentIndex();
		if (current >= 0)
			config.write("System", "Command", "LastSelection", ui.comboBox_paths->currentIndex());
	}
}

void GeneralForm::onApplyHashSettingsToUI()
{
	Injector& injector = Injector::getInstance();
	util::SafeHash<util::UserSetting, bool> enableHash = injector.getEnableHash();
	util::SafeHash<util::UserSetting, int> valueHash = injector.getValueHash();
	util::SafeHash<util::UserSetting, QString> stringHash = injector.getStringHash();

	const QString fileName(qgetenv("JSON_PATH"));
	if (!fileName.isEmpty())
	{
		util::Config config(fileName);
		int index = config.readInt("System", "Command", "LastSelection");

		if (index >= 0 && index < ui.comboBox_paths->count())
		{
			ui.comboBox_paths->blockSignals(true);
			ui.comboBox_paths->setCurrentIndex(index);
			ui.comboBox_paths->blockSignals(false);
		}
		else if (ui.comboBox_paths->count() > 0)
			ui.comboBox_paths->setCurrentIndex(0);

		int count = config.readInt("System", "Server", "ListCount");
		if (count <= 0)
		{
			count = 2;
			config.write("System", "Server", "ListCount", count);
		}

		ui.comboBox_serverlist->blockSignals(true);
		ui.comboBox_serverlist->clear();
		for (int i = 0; i < count; ++i)
		{
			ui.comboBox_serverlist->addItem(tr("ServerList%1").arg(i + 1), i);
		}

		int lastServerListSelection = config.readInt("System", "Server", "LastServerListSelection");
		if (lastServerListSelection >= 0 && lastServerListSelection < count)
			ui.comboBox_serverlist->setCurrentIndex(lastServerListSelection);
		else if (ui.comboBox_serverlist->count() > 0)
			ui.comboBox_serverlist->setCurrentIndex(0);

		ui.comboBox_serverlist->blockSignals(false);
	}

	int value = 0;

	//login
	value = valueHash.value(util::kServerValue);
	if (value >= 0 && value < ui.comboBox_server->count())
		ui.comboBox_server->setCurrentIndex(value);
	else
		ui.comboBox_server->setCurrentIndex(0);

	value = valueHash.value(util::kSubServerValue);
	if (value >= 0 && value < ui.comboBox_server->count())
		ui.comboBox_subserver->setCurrentIndex(value);
	else
		ui.comboBox_subserver->setCurrentIndex(0);

	value = valueHash.value(util::kPositionValue);
	if (value >= 0 && value < ui.comboBox_server->count())
		ui.comboBox_position->setCurrentIndex(value);
	else
		ui.comboBox_position->setCurrentIndex(0);

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
	ui.checkBox_showexp->setChecked(enableHash.value(util::kShowExpEnable));

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

void GeneralForm::onGameStart()
{
	QString path = ui.comboBox_paths->currentData().toString();
	if (path.isEmpty())
		return;

	QFile file(path);
	if (!file.exists())
		return;

	ui.pushButton_start->setEnabled(false);
	setFocus();
	update();
	QCoreApplication::processEvents();

	QFileInfo fileInfo(path);
	QString dirPath = fileInfo.absolutePath();
	QByteArray dirPathUtf8 = dirPath.toUtf8();
	qputenv("GAME_DIR_PATH", dirPathUtf8);

	Injector& injector = Injector::getInstance();
	injector.currentGameExePath = path;

	injector.server.reset(new Server(this));
	if (injector.server.isNull())
		return;

	if (!injector.server->start(this))
		return;

	ThreadManager& thread_manager = ThreadManager::getInstance();
	if (!thread_manager.createThread(nullptr))
	{
		ui.pushButton_start->setEnabled(true);
	}
}

int g_CurrentListIndex = 0;
void GeneralForm::createServerList()
{
	int currentListIndex = ui.comboBox_serverlist->currentIndex();
	if (currentListIndex < 0)
		currentListIndex = 0;
	QStringList list;

	{
		const QString fileName(qgetenv("JSON_PATH"));
		util::Config config(fileName);

		g_CurrentListIndex = currentListIndex;
		list = config.readStringArray("System", "Server", QString("List_%1").arg(currentListIndex));
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

			list = currentListIndex == 0 ? defaultListSO : defaultListSE;
			config.writeStringArray("System", "Server", QString("List_%1").arg(currentListIndex), list);
		}
	}

	QString currentText = ui.comboBox_server->currentText();
	int currentIndex = ui.comboBox_server->currentIndex();

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
		if (indexStr.toInt() <= 0)
			continue;

		QString server = subList.takeFirst();

		subList = subList.first().split(util::rexComma, Qt::SkipEmptyParts);
		if (subList.isEmpty())
			continue;

		serverNameList.append(server);
		subServerNameList.append(subList);

		serverList[currentListIndex].insert(server, subList);
		ui.comboBox_server->addItem(server);
		if (currentText != server)
			continue;
		ui.comboBox_subserver->addItems(subList);
	}

	Injector& injector = Injector::getInstance();
	injector.serverNameList = serverNameList;
	injector.subServerNameList = subServerNameList;

	if (currentIndex >= 0)
		ui.comboBox_server->setCurrentIndex(currentIndex);
	else
		ui.comboBox_server->setCurrentIndex(0);
	ui.comboBox_subserver->setCurrentIndex(0);
	ui.comboBox_server->setUpdatesEnabled(true);
	ui.comboBox_subserver->setUpdatesEnabled(true);
}