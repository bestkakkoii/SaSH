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

//menu action forms
#include "form/scriptsettingform.h"

//utilities
#include "signaldispatcher.h"
#include "util.h"
#include "injector.h"

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

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();

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
	MINT::NtTerminateProcess(Injector::getInstance().getProcess(), 0);
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



		return;
	}

	if (actionName == "actionInfo")
	{

		return;
	}

	if (actionName == "actionWibsite")
	{
		QDesktopServices::openUrl(QUrl("https://www.lovesa.cc"));
		return;
	}

	if (actionName == "actionClose")
	{
		close();
		return;
	}

	//other
	if (actionName == "actionOtherInfo")
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
		return;
	}

	if (actionName == "actionScriptSettings")
	{
		ScriptSettingForm* pScriptSettingForm = new ScriptSettingForm;
		if (pScriptSettingForm)
		{
			pScriptSettingForm->setAttribute(Qt::WA_DeleteOnClose);
			pScriptSettingForm->show();
		}
		return;
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
		util::createMenu(pMenuBar_);
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