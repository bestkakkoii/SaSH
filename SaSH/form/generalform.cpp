#include "stdafx.h"
#include "generalform.h"
#include "selectobjectform.h"

#include "mainthread.h"
#include "util.h"
#include "injector.h"
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

	QList <QComboBox*> comboBoxList = util::findWidgets<QComboBox>(this);
	for (auto& comboBox : comboBoxList)
	{
		if (comboBox)
			connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onComboBoxCurrentIndexChanged(int)), Qt::UniqueConnection);
	}


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

		config.writeStringList("System", "ServerList", serverList);
	}

	QStringList subServerList = config.readStringList("System", "SubServerList");
	if (subServerList.isEmpty())
	{
		subServerList = QStringList{
			tr("Telecom"), tr("UnitedNetwork"), tr("Easyown"), tr("Oversea"), tr("Backup"),
		};

		config.writeStringList("System", "SubServerList", subServerList);
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

	if (name == "pushButton_start")
	{
		setFocus();
		pPushButton->setEnabled(false);
		constexpr const char* SA_NAME = u8"sa_8001sf.exe";
		const QString fileName(qgetenv("JSON_PATH"));
		if (fileName.isEmpty())
			return;
		util::Config config(fileName);
		QString path = config.readString("System", "Command", "DirPath");
		if (path.isEmpty() || !path.contains(SA_NAME))
		{
			if (!util::createFileDialog(SA_NAME, &path, this))
				return;
		}

		if (!QFile::exists(path))
		{
			if (!util::createFileDialog(SA_NAME, &path, this))
				return;
		}

		config.write("System", "Command", "DirPath", path);
		QFileInfo fileInfo(path);
		QString dirPath = fileInfo.absolutePath();
		QByteArray dirPathUtf8 = dirPath.toUtf8();
		qputenv("GAME_DIR_PATH", dirPathUtf8);


		Injector& injector = Injector::getInstance();
		injector.server.reset(new Server(nullptr));
		if (injector.server.isNull())
			return;

		ThreadManager& thread_manager = ThreadManager::getInstance();
		if (!thread_manager.createThread())
		{
			pPushButton->setEnabled(true);
		}
		return;
	}

	if (name == "pushButton_join")
	{

		return;
	}

	if (name == "pushButton_leave")
	{

		return;
	}

	if (name == "pushButton_sell")
	{

		return;
	}

	if (name == "pushButton_pick")
	{

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
		onSaveHashSettings();
		return;
	}

	if (name == "pushButton_loadsettings")
	{
		onLoadHashSettings();
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

	if (name == "checkBox_mute")
	{
		injector.setEnableHash(util::kMuteEnable, isChecked);
		return;
	}

	if (name == "checkBox_autojoin")
	{
		injector.setEnableHash(util::kAutoJoinEnable, isChecked);
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
			srcList = src.split("|", Qt::SkipEmptyParts);
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

	if (name == "checkBox_forceleavebattle")
	{
		injector.setEnableHash(util::kForceLeaveBattleEnable, isChecked);
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
		injector.setEnableHash(util::kFastBattleEnable, isChecked);
		return;
	}

	if (name == "checkBox_autobattle")
	{
		if (isChecked)
		{
			ui.checkBox_fastbattle->setChecked(!isChecked);
		}
		injector.setEnableHash(util::kAutoBattleEnable, isChecked);
		return;
	}

	if (name == "checkBox_autocatch")
	{
		injector.setEnableHash(util::kAutoCatchEnable, isChecked);
		return;
	}

	if (name == "checkBox_lockattck")
	{
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

	if (name == "checkBox_switcher_world")
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
	QComboBox* pComboBox = qobject_cast<QComboBox*>(sender());
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

void GeneralForm::onSaveHashSettings(const QString& name)
{
	QString newFileName = name;
	if (newFileName.isEmpty())
		newFileName = "default";

	newFileName += ".json";

	QString directory = QCoreApplication::applicationDirPath() + "/settings/";
	QString fileName(directory + newFileName);

	QDir dir(directory);
	if (!dir.exists())
	{
		dir.mkpath(directory);
	}

	QFileDialog dialog(this);
	dialog.setFileMode(QFileDialog::AnyFile);
	dialog.setAcceptMode(QFileDialog::AcceptSave);
	dialog.setDirectory(directory);
	dialog.setNameFilter(tr("Json Files (*.json)"));
	dialog.selectFile(newFileName);

	if (dialog.exec() == QDialog::Accepted)
	{
		QStringList fileNames = dialog.selectedFiles();
		if (fileNames.size() > 0)
		{
			fileName = fileNames[0];
		}
	}
	else
		return;

	util::Config config(fileName);

	Injector& injector = Injector::getInstance();
	util::SafeHash<util::UserSetting, bool> enableHash = injector.getEnableHash();
	util::SafeHash<util::UserSetting, int> valueHash = injector.getValueHash();
	util::SafeHash<util::UserSetting, QString> stringHash = injector.getStringHash();

	QHash<util::UserSetting, QString> jsonKeyHash = util::user_setting_string_hash;

	for (QHash < util::UserSetting, bool>::const_iterator iter = enableHash.begin(); iter != enableHash.end(); ++iter)
	{
		util::UserSetting hkey = iter.key();
		bool hvalue = iter.value();
		QString key = jsonKeyHash.value(hkey, "Invalid");
		config.write("User", "Enable", key, hvalue);
	}

	for (QHash<util::UserSetting, int>::const_iterator iter = valueHash.begin(); iter != valueHash.end(); ++iter)
	{
		util::UserSetting hkey = iter.key();
		int hvalue = iter.value();
		QString key = jsonKeyHash.value(hkey, "Invalid");
		config.write("User", "Value", key, hvalue);
	}

	for (QHash < util::UserSetting, QString>::const_iterator iter = stringHash.begin(); iter != stringHash.end(); ++iter)
	{
		util::UserSetting hkey = iter.key();
		QString hvalue = iter.value();
		QString key = jsonKeyHash.value(hkey, "Invalid");
		config.write("User", "String", key, hvalue);
	}

}

void GeneralForm::onLoadHashSettings(const QString& name)
{
	QString newFileName = name;
	if (newFileName.isEmpty())
		newFileName = "default";

	newFileName += ".json";

	QString directory = QCoreApplication::applicationDirPath() + "/settings/";
	QString fileName(directory + newFileName);

	QDir dir(directory);
	if (!dir.exists())
	{
		dir.mkpath(directory);
	}

	QFileDialog dialog(this);
	dialog.setFileMode(QFileDialog::ExistingFile);
	dialog.setAcceptMode(QFileDialog::AcceptOpen);
	dialog.setDirectory(directory);
	dialog.setNameFilter(tr("Json Files (*.json)"));
	dialog.selectFile(newFileName);

	if (dialog.exec() == QDialog::Accepted)
	{
		QStringList fileNames = dialog.selectedFiles();
		if (fileNames.size() > 0)
		{
			fileName = fileNames[0];
		}
	}
	else
		return;

	util::Config config(fileName);

	Injector& injector = Injector::getInstance();
	util::SafeHash<util::UserSetting, bool> enableHash;
	util::SafeHash<util::UserSetting, int> valueHash;
	util::SafeHash<util::UserSetting, QString> stringHash;

	QHash<util::UserSetting, QString> jsonKeyHash = util::user_setting_string_hash;

	for (QHash < util::UserSetting, QString>::const_iterator iter = jsonKeyHash.begin(); iter != jsonKeyHash.end(); ++iter)
	{
		QString key = iter.value();
		bool value = config.readBool("User", "Enable", key);
		util::UserSetting hkey = iter.key();
		enableHash.insert(hkey, value);
	}

	for (QHash < util::UserSetting, QString>::const_iterator iter = jsonKeyHash.begin(); iter != jsonKeyHash.end(); ++iter)
	{
		QString key = iter.value();
		int value = config.readInt("User", "Value", key);
		util::UserSetting hkey = iter.key();
		valueHash.insert(hkey, value);
	}

	for (QHash < util::UserSetting, QString>::const_iterator iter = jsonKeyHash.begin(); iter != jsonKeyHash.end(); ++iter)
	{
		QString key = iter.value();
		QString value = config.readString("User", "String", key);
		util::UserSetting hkey = iter.key();
		stringHash.insert(hkey, value);
	}

	injector.setEnableHash(enableHash);
	injector.setValueHash(valueHash);
	injector.setStringHash(stringHash);

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	emit signalDispatcher.applyHashSettingsToUI();
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