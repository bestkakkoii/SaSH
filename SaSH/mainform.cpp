#include "stdafx.h"
#include "mainform.h"

//TabWidget pages
#include "form/selectobjectform.h"
#include "form/generalform.h"
#include "form/mapform.h"
#include "form/afkform.h"
#include "form/otherform.h"
#include "form/scriptform.h"
#include "form/infoform.h"
#include "form/mapwidget.h"

//menu action forms
#include "form/scriptsettingform.h"

//utilities
#include "signaldispatcher.h"
#include <util.h>
#include <injector.h>

void createMenu(QMenuBar* pMenuBar)
{
	if (!pMenuBar)
		return;

#pragma region StyleSheet
	constexpr const char* styleText = u8R"(
				QMenu {
					background-color: rgb(249, 249, 249); /*整個背景*/
					border: 0px;
					/*item寬度*/
					width: 150px;
				
				}
				QMenu::item {
					font-size: 9pt;
					/*color: rgb(225, 225, 225); 字體顏色*/
					border: 2px; solid rgb(249, 249, 249); /*item選框*/
					background-color: rgb(249, 249, 249);
					padding: 10px 10px; /*設置菜單項文字上下和左右的內邊距，效果就是菜單中的條目左右上下有了間隔*/
					margin: 2px 2px; /*設置菜單項的外邊距*/
					/*item高度*/	
					height: 10px;
				}
				QMenu::item:selected {
					background-color: rgb(240, 240, 240); /*選中的樣式*/
					border: 2px solid rgb(249, 249, 249); /*选中状态下的边框*/
				}
				QMenu::item:pressed {
					/*菜單項按下效果
					border: 0px; /*solid rgb(60, 60, 61);*/
					background-color: rgb(50, 130, 246);
				}
			)";
#pragma endregion

	pMenuBar->setStyleSheet(styleText);
	pMenuBar->setAttribute(Qt::WA_StyledBackground, true);
	pMenuBar->clear();

	auto createAction = [](QMenu* parent, const QString& text = "", const QString& name = "")->void
	{
		if (!parent)
			return;

		QAction* pAction = new QAction(text, parent);
		if (!pAction)
			return;
		if (!text.isEmpty() && !name.isEmpty())
		{
			pAction->setObjectName(name);
		}
		else
			pAction->setSeparator(true);
		parent->addAction(pAction);
	};

	auto create = [&createAction](const QVector<QPair<QString, QString>>& table, QMenu* pMenu)
	{
		for (auto& pair : table)
		{
			if (pair.first.isEmpty() || pair.second.isEmpty())
			{
				createAction(pMenu);
				continue;
			}
			createAction(pMenu, pair.first, pair.second);
		}
	};

	const QVector<QPair<QString, QString>> systemTable = {
		{ QObject::tr("hide"), "actionHide" },
		{ "","" },
		{ QObject::tr("website"), "actionWebsite" },
		{ QObject::tr("info"), "actionInfo" },
		{ "", "" },
		{ QObject::tr("close"), "actionClose" },
		{ QObject::tr("close game"), "actionCloseGame" },
	};

	const QVector<QPair<QString, QString>> otherTable = {
		{ QObject::tr("otherinfo"), "actionOtherInfo" },
		{ QObject::tr("map"), "actionMap" },
		{ "","" },
		{ QObject::tr("script settings"), "actionScriptSettings" }
	};

	const QVector<QPair<QString, QString>> fileTable = {
		{ QObject::tr("save"), "actionSave" },
		{ QObject::tr("load"), "actionLoad" },
	};

	QMenu* pMenuSystem = new QMenu(QObject::tr("system"));
	if (!pMenuSystem)
		return;
	pMenuSystem->setStyleSheet(styleText);
	pMenuBar->addMenu(pMenuSystem);

	QMenu* pMenuOther = new QMenu(QObject::tr("other"));
	if (!pMenuOther)
		return;
	pMenuOther->setStyleSheet(styleText);
	pMenuBar->addMenu(pMenuOther);

	QMenu* pMenuFile = new QMenu(QObject::tr("file"));
	if (!pMenuFile)
		return;
	pMenuFile->setStyleSheet(styleText);
	pMenuBar->addMenu(pMenuFile);

	create(systemTable, pMenuSystem);
	create(otherTable, pMenuOther);
	create(fileTable, pMenuFile);
}

//接收原生的窗口消息
bool MainForm::nativeEvent(const QByteArray& eventType, void* message, long* result)
{
	MSG* msg = static_cast<MSG*>(message);
	Injector& injector = Injector::getInstance();
	switch (msg->message)
	{
	case WM_MOUSEMOVE + WM_USER:
	{
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
		signalDispatcher.updateCursorLabelTextChanged(QString("%1,%2").arg(GET_X_LPARAM(msg->lParam)).arg(GET_Y_LPARAM(msg->lParam)));
		break;
	}
	default:
		break;
	}

	return false;
}

MainForm::MainForm(QWidget* parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_QuitOnClose);
	setAttribute(Qt::WA_StyledBackground, true);
	setAttribute(Qt::WA_StaticContents, true);
	setStyleSheet(R"(
		QMainWindow{ border-radius: 10px; }
	)");

	qRegisterMetaType<QVariant>("QVariant");
	qRegisterMetaType<QVariant>("QVariant&");

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();

	connect(&signalDispatcher, &SignalDispatcher::saveHashSettings, this, &MainForm::onSaveHashSettings, Qt::UniqueConnection);
	connect(&signalDispatcher, &SignalDispatcher::loadHashSettings, this, &MainForm::onLoadHashSettings, Qt::UniqueConnection);
	connect(&signalDispatcher, &SignalDispatcher::messageBoxShow, this, &MainForm::onMessageBoxShow, Qt::BlockingQueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::inputBoxShow, this, &MainForm::onInputBoxShow, Qt::BlockingQueuedConnection);

	QMenuBar* pMenuBar = new QMenuBar(this);
	if (pMenuBar)
	{
		pMenuBar_ = pMenuBar;
		pMenuBar->setObjectName("menuBar");
		setMenuBar(pMenuBar);

	}

	{
		ui.progressBar_pchp->setType(ProgressBar::kHP);
		ui.progressBar_pcmp->setType(ProgressBar::kMP);
		ui.progressBar_pethp->setType(ProgressBar::kHP);
		ui.progressBar_ridehp->setType(ProgressBar::kHP);
		connect(&signalDispatcher, &SignalDispatcher::updateCharHpProgressValue, ui.progressBar_pchp, &ProgressBar::onCurrentValueChanged);
		connect(&signalDispatcher, &SignalDispatcher::updateCharMpProgressValue, ui.progressBar_pcmp, &ProgressBar::onCurrentValueChanged);
		connect(&signalDispatcher, &SignalDispatcher::updatePetHpProgressValue, ui.progressBar_pethp, &ProgressBar::onCurrentValueChanged);
		connect(&signalDispatcher, &SignalDispatcher::updateRideHpProgressValue, ui.progressBar_ridehp, &ProgressBar::onCurrentValueChanged);
	}

	{
		connect(&signalDispatcher, &SignalDispatcher::updateStatusLabelTextChanged, this, &MainForm::onUpdateStatusLabelTextChanged);
		connect(&signalDispatcher, &SignalDispatcher::updateMapLabelTextChanged, this, &MainForm::onUpdateMapLabelTextChanged);
		connect(&signalDispatcher, &SignalDispatcher::updateCursorLabelTextChanged, this, &MainForm::onUpdateCursorLabelTextChanged);
		connect(&signalDispatcher, &SignalDispatcher::updateCoordsPosLabelTextChanged, this, &MainForm::onUpdateCoordsPosLabelTextChanged);
	}



	ui.tabWidget_main->clear();
	util::setTab(ui.tabWidget_main);

	resetControlTextLanguage();

	pGeneralForm_ = new GeneralForm;
	if (pGeneralForm_)
	{
		ui.tabWidget_main->addTab(pGeneralForm_, tr("general"));
	}

	{
		pMapForm_ = new MapForm;
		if (pMapForm_)
		{
			ui.tabWidget_main->addTab(pMapForm_, tr("map"));
		}

		pAfkForm_ = new AfkForm;
		if (pAfkForm_)
		{
			ui.tabWidget_main->addTab(pAfkForm_, tr("afk"));
		}

		pOtherForm_ = new OtherForm;
		if (pOtherForm_)
		{
			ui.tabWidget_main->addTab(pOtherForm_, tr("other"));
		}

		pScriptForm_ = new ScriptForm;
		if (pScriptForm_)
		{
			ui.tabWidget_main->addTab(pScriptForm_, tr("script"));
		}
	}

	resetControlTextLanguage();

	ui.progressBar_pchp->onCurrentValueChanged(0, 0, 100);
	ui.progressBar_pcmp->onCurrentValueChanged(0, 0, 100);
	ui.progressBar_pethp->onCurrentValueChanged(0, 0, 100);
	ui.progressBar_ridehp->onCurrentValueChanged(0, 0, 100);

	util::FormSettingManager formManager(this);
	formManager.loadSettings();

	WId wid = this->winId();
	QString qwid = QString::number(wid);
	qputenv("SASH_HWND", qwid.toUtf8());

	onUpdateStatusLabelTextChanged(util::kLabelStatusNotOpen);
}

MainForm::~MainForm()
{
	Injector::getInstance().close();
	MINT::NtTerminateProcess(GetCurrentProcess(), 0);
	qDebug() << "MainForm::~MainForm()";
}

void MainForm::showEvent(QShowEvent* e)
{
	setAttribute(Qt::WA_Mapped);
	QMainWindow::showEvent(e);
}

void MainForm::closeEvent(QCloseEvent* e)
{
	util::FormSettingManager formManager(this);
	formManager.saveSettings();
	qApp->closeAllWindows();

}

void MainForm::onMenuActionTriggered()
{
	QAction* pAction = qobject_cast<QAction*>(sender());
	if (!pAction)
		return;

	QString actionName = pAction->objectName();
	if (actionName.isEmpty())
		return;

	//system
	if (actionName == "actionHide")
	{
		//驗證測試
		//util::Crypt crypt;
		//Net::Authenticator& g_Authenticator = Net::Authenticator::getInstance();

		//QString username = "bestkakkoii";
		//QString encode_password = crypt.encryptToString("Gdd3ae6h49");
		//if (g_Authenticator.Login(username, encode_password))
		//	g_Authenticator.Logout();

	}

	else if (actionName == "actionInfo")
	{

	}

	else if (actionName == "actionWibsite")
	{
		QDesktopServices::openUrl(QUrl("https://www.lovesa.cc"));
	}

	if (actionName == "actionClose")
	{
		close();
	}

	else if (actionName == "actionCloseGame")
	{
		Injector::getInstance().close();
	}

	//other
	else if (actionName == "actionOtherInfo")
	{
		if (!pInfoForm_)
		{

			pInfoForm_ = new InfoForm;
			if (pInfoForm_)
			{
				connect(pInfoForm_, &InfoForm::destroyed, [&]() { pInfoForm_ = nullptr; });
				pInfoForm_->setAttribute(Qt::WA_DeleteOnClose);
				pInfoForm_->show();
			}
		}
	}

	else if (actionName == "actionMap")
	{
		MapWidget* mapWidget = new MapWidget(nullptr);
		if (mapWidget)
		{
			mapWidget->setAttribute(Qt::WA_DeleteOnClose);
			mapWidget->show();
		}
	}

	else if (actionName == "actionScriptSettings")
	{
		ScriptSettingForm* pScriptSettingForm = new ScriptSettingForm;
		if (pScriptSettingForm)
		{
			pScriptSettingForm->setAttribute(Qt::WA_DeleteOnClose);
			pScriptSettingForm->show();
		}
	}

	else if (actionName == "actionSave")
	{
		QString fileName;
		Injector& injector = Injector::getInstance();
		if (!injector.server.isNull())
			fileName = injector.server->pc.name;
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
		emit signalDispatcher.saveHashSettings(fileName);
	}

	else if (actionName == "actionLoad")
	{
		QString fileName;
		Injector& injector = Injector::getInstance();
		if (!injector.server.isNull())
			fileName = injector.server->pc.name;
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
		emit signalDispatcher.loadHashSettings(fileName);
	}
}

void MainForm::resetControlTextLanguage()
{
	const UINT acp = ::GetACP();
#if QT_NO_DEBUG

	switch (acp)
	{
	case 936:
		translator_.load(QString("%1/translations/qt_zh_CN.qm").arg(QApplication::applicationDirPath()));
		break;
		//English
	case 950:
		translator_.load(QString("%1/translations/qt_zh_TW.qm").arg(QApplication::applicationDirPath()));
		break;
		//Chinese
	default:
		translator_.load(QString("%1/translations/qt_en_US.qm").arg(QApplication::applicationDirPath()));
		break;
	}
#else
	switch (acp)
	{
	case 936:
		translator_.load(R"(D:\Users\bestkakkoii\Desktop\vs_project\SaSH\SaSH\translations\qt_zh_CN.qm)");
		break;
		//English
	case 950:
		translator_.load(R"(D:\Users\bestkakkoii\Desktop\vs_project\SaSH\SaSH\translations\qt_zh_TW.qm)");
		break;
		//Chinese
	default:
		translator_.load(R"(D:\Users\bestkakkoii\Desktop\vs_project\SaSH\SaSH\translations\qt_en_US.qm)");
		break;
	}
#endif

	qApp->installTranslator(&translator_);
	this->ui.retranslateUi(this);

	constexpr int VERSION_MAJOR = 1;
	constexpr int VERSION_MINOR = 0;
	constexpr int VERSION_PATCH = 0;

	const QString strversion = QString("%1.%2%3").arg(VERSION_MAJOR).arg(VERSION_MINOR).arg(VERSION_PATCH);
	setWindowTitle(tr("SaSH - Beta %1").arg(strversion));

	if (pMenuBar_)
	{
		createMenu(pMenuBar_);
		QList<QAction*> actions = pMenuBar_->actions();
		for (auto action : actions)
		{
			if (action->menu())
			{
				QList<QAction*> sub_actions = action->menu()->actions();
				for (auto sub_action : sub_actions)
				{
					connect(sub_action, &QAction::triggered, this, &MainForm::onMenuActionTriggered);
				}
			}
			else
			{
				connect(action, &QAction::triggered, this, &MainForm::onMenuActionTriggered);
			}
		}
	}

	ui.progressBar_pchp->setName(tr("char"));
	ui.progressBar_pcmp->setName(tr(""));
	ui.progressBar_pethp->setName(tr("pet"));
	ui.progressBar_ridehp->setName(tr("ride"));

	//reset tab text
	ui.tabWidget_main->setTabText(0, tr("general"));
	ui.tabWidget_main->setTabText(1, tr("map"));
	ui.tabWidget_main->setTabText(2, tr("afk"));
	ui.tabWidget_main->setTabText(3, tr("other"));
	ui.tabWidget_main->setTabText(4, tr("script"));


	if (pGeneralForm_)
		emit pGeneralForm_->resetControlTextLanguage();

	if (pInfoForm_)
		emit pInfoForm_->resetControlTextLanguage();

	if (pAfkForm_)
		emit pAfkForm_->resetControlTextLanguage();

	if (pOtherForm_)
		emit pOtherForm_->resetControlTextLanguage();

}

void MainForm::onUpdateStatusLabelTextChanged(int status)
{
	/*
		kLabelStatusNotUsed = 0,//未知
		kLabelStatusNotOpen,//未開啟石器
		kLabelStatusOpening,//開啟石器中
		kLabelStatusOpened,//已開啟石器
		kLabelStatusLogining,//登入
		kLabelStatusSignning,//簽入中
		kLabelStatusSelectServer,//選擇伺服器
		kLabelStatusSelectSubServer,//選擇分伺服器
		kLabelStatusGettingPlayerList,//取得人物中
		kLabelStatusSelectPosition,//選擇人物中
		kLabelStatusLoginSuccess,//登入成功
		kLabelStatusInNormal,//平時
		kLabelStatusInBattle,//戰鬥中
	*/
	const QHash<util::UserStatus, QString> hash = {
		{ util::kLabelStatusNotUsed, tr("unknown") },
		{ util::kLabelStatusNotOpen, tr("not open") },
		{ util::kLabelStatusOpening, tr("opening") },
		{ util::kLabelStatusOpened, tr("opened") },
		{ util::kLabelStatusLogining, tr("logining") },
		{ util::kLabelStatusSignning, tr("signning") },
		{ util::kLabelStatusSelectServer, tr("select server") },
		{ util::kLabelStatusSelectSubServer, tr("select sub server") },
		{ util::kLabelStatusGettingPlayerList, tr("getting player list") },
		{ util::kLabelStatusSelectPosition, tr("select position") },
		{ util::kLabelStatusLoginSuccess, tr("login success") },
		{ util::kLabelStatusInNormal, tr("in normal") },
		{ util::kLabelStatusInBattle, tr("in battle") },
		{ util::kLabelStatusBusy, tr("busy") },
		{ util::kLabelStatusTimeout, tr("timeout") },
		{ util::kLabelNoUserNameOrPassword, tr("no username or password") },
		{ util::kLabelStatusDisconnected, tr("disconnected")}
	};
	ui.label_status->setText(tr("status:") + hash.value(static_cast<util::UserStatus>(status), tr("unknown")));
}
void MainForm::onUpdateMapLabelTextChanged(const QString& text)
{
	ui.label_map->setText(tr("map:") + text);
}
void MainForm::onUpdateCursorLabelTextChanged(const QString& text)
{
	ui.label_cursor->setText(tr("cursor:") + text);
}
void MainForm::onUpdateCoordsPosLabelTextChanged(const QString& text)
{
	ui.label_coords->setText(tr("coordis:") + text);
}

void MainForm::onSaveHashSettings(const QString& name, bool isFullPath)
{
	QString fileName;
	if (isFullPath)
	{
		fileName = name;
	}
	else
	{
		QString newFileName = name;
		if (newFileName.isEmpty())
			newFileName = "default";

		newFileName += ".json";

		QString directory = QCoreApplication::applicationDirPath() + "/settings/";
		fileName = QString(directory + newFileName);

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
	}

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
		if (key.endsWith("Enable"))
			config.write("User", "Enable", key, hvalue);
	}

	for (QHash<util::UserSetting, int>::const_iterator iter = valueHash.begin(); iter != valueHash.end(); ++iter)
	{
		util::UserSetting hkey = iter.key();
		int hvalue = iter.value();
		QString key = jsonKeyHash.value(hkey, "Invalid");
		if (key.endsWith("Value"))
			config.write("User", "Value", key, hvalue);
	}

	for (QHash < util::UserSetting, QString>::const_iterator iter = stringHash.begin(); iter != stringHash.end(); ++iter)
	{
		util::UserSetting hkey = iter.key();
		QString hvalue = iter.value();
		QString key = jsonKeyHash.value(hkey, "Invalid");
		if (key.endsWith("String"))
			config.write("User", "String", key, hvalue);
	}

}

void MainForm::onLoadHashSettings(const QString& name, bool isFullPath)
{
	QString fileName;
	if (isFullPath)
	{
		fileName = name;
	}
	else
	{
		QString newFileName = name;
		if (newFileName.isEmpty())
			newFileName = "default";

		newFileName += ".json";

		QString directory = QCoreApplication::applicationDirPath() + "/settings/";
		fileName = QString(directory + newFileName);

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
	}

	if (!QFile::exists(fileName))
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
		if (!key.endsWith("Enable"))
			continue;
		bool value = config.readBool("User", "Enable", key);
		util::UserSetting hkey = iter.key();
		enableHash.insert(hkey, value);
	}

	for (QHash < util::UserSetting, QString>::const_iterator iter = jsonKeyHash.begin(); iter != jsonKeyHash.end(); ++iter)
	{
		QString key = iter.value();
		if (!key.endsWith("Value"))
			continue;
		int value = config.readInt("User", "Value", key);
		util::UserSetting hkey = iter.key();
		valueHash.insert(hkey, value);
	}

	for (QHash < util::UserSetting, QString>::const_iterator iter = jsonKeyHash.begin(); iter != jsonKeyHash.end(); ++iter)
	{
		QString key = iter.value();
		if (!key.endsWith("String"))
			continue;
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

void MainForm::onMessageBoxShow(const QString& text, int type)
{
	if (type == 2)
		QMessageBox::warning(this, tr("warning"), text);
	else if (type == 3)
		QMessageBox::critical(this, tr("critical"), text);
	else
		QMessageBox::information(this, tr("info"), text);
}

void MainForm::onInputBoxShow(const QString& text, int type, QVariant* retvalue)
{
	if (retvalue == nullptr)
		return;

	QInputDialog inputDialog(this);
	inputDialog.setLabelText(text);
	inputDialog.setInputMode(static_cast<QInputDialog::InputMode>(type));
	auto ret = inputDialog.exec();
	if (ret != QDialog::Accepted)
		return;

	switch (type)
	{
	case QInputDialog::IntInput:
		*retvalue = inputDialog.intValue();
		break;
	case QInputDialog::DoubleInput:
		*retvalue = inputDialog.doubleValue();
		break;
	case QInputDialog::TextInput:
		*retvalue = inputDialog.textValue();
		break;
	}
}