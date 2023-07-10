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
#include "model/qthumbnailform.h"
#include "update/qdownloader.h"

//utilities
#include "signaldispatcher.h"
#include <util.h>
#include <injector.h>
#include "script/interpreter.h"

void createMenu(QMenuBar* pMenuBar)
{
	if (!pMenuBar)
		return;

#pragma region StyleSheet
	//constexpr const char* styleText = u8R"(
	//			QMenu {
	//				background-color: rgb(249, 249, 249); /*整個背景*/
	//				border: 0px;
	//				/*item寬度*/
	//				width: 150px;
	//			
	//			}
	//			QMenu::item {
	//				font-size: 9pt;
	//				/*color: rgb(225, 225, 225); 字體顏色*/
	//				border: 2px; solid rgb(249, 249, 249); /*item選框*/
	//				background-color: rgb(249, 249, 249);
	//				padding: 10px 10px; /*設置菜單項文字上下和左右的內邊距，效果就是菜單中的條目左右上下有了間隔*/
	//				margin: 2px 2px; /*設置菜單項的外邊距*/
	//				/*item高度*/	
	//				height: 10px;
	//			}
	//			QMenu::item:selected {
	//				background-color: rgb(240, 240, 240); /*選中的樣式*/
	//				border: 2px solid rgb(249, 249, 249); /*選中狀態下的邊框*/
	//			}
	//			QMenu::item:pressed {
	//				/*菜單項按下效果
	//				border: 0px; /*solid rgb(60, 60, 61);*/
	//				background-color: rgb(50, 130, 246);
	//			}
	//		)";
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
					border: 2px solid rgb(249, 249, 249); /*選中狀態下的邊框*/
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
		{ QObject::tr("scriptdoc"), "actionInfo" },
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
		{ "","" },
		{ QObject::tr("checkupdate"), "actionUpdate" },
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

enum InterfaceMessage
{
	kInterfaceMessage = WM_USER + 2048,
	kRunScript,			// kInterfaceMessage + 1
	kStopScript,		// kInterfaceMessage + 2
	kRunFile,			// kInterfaceMessage + 3
	kStopFile,			// kInterfaceMessage + 4
	kRunGame,			// kInterfaceMessage + 5
	kCloseGame,			// kInterfaceMessage + 6
	kGetGameState,		// kInterfaceMessage + 7
	kScriptState,		// kInterfaceMessage + 8
	kOpenWindow,		// kInterfaceMessage + 9
	kSortWindow,		// kInterfaceMessage + 10
	kThumbnail,			// kInterfaceMessage + 11
};

enum InterfaceWindowType
{
	WindowNone = 0,		//無
	WindowInfo,			//信息窗口
	WindowMap,			//地圖窗口
	WindowScript,		//腳本窗口
};

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
		emit signalDispatcher.updateCursorLabelTextChanged(QString("%1,%2").arg(GET_X_LPARAM(msg->lParam)).arg(GET_Y_LPARAM(msg->lParam)));
		return true;
	}
	case WM_KEYUP + WM_USER + VK_DELETE:
	{
		if (!injector.server.isNull())
			injector.server->cleanChatHistory();
		return true;
	}
	case Injector::kConnectionOK://TCP握手
	{
		if (!injector.server.isNull())
		{
			injector.server->IS_TCP_CONNECTION_OK_TO_USE = true;
			qDebug() << "tcp ok";
		}
		return true;
	}
	case InterfaceMessage::kRunScript:
	{
		QSharedPointer<Interpreter> interpreter(new Interpreter());
		if (interpreter.isNull())
			return true;

		int id = msg->wParam;
		interpreter_hash_.insert(id, interpreter);

		//檢查是否為合法字符串指針
		QByteArray utf8str = reinterpret_cast<char*>(msg->lParam);
		if (utf8str.isEmpty())
			break;

		QString script = QString::fromUtf8(utf8str);
		connect(interpreter.data(), &Interpreter::finished, this, [this, id]()
			{
				interpreter_hash_.remove(id);
				updateStatusText();
			});
		++interfaceCount_;
		updateStatusText();
		interpreter->doString(script, nullptr, Interpreter::kNotShare);
		*result = 1;
		return true;
	}
	case InterfaceMessage::kStopScript:
	{
		QSharedPointer<Interpreter> interpreter = interpreter_hash_.value(msg->wParam, nullptr);
		if (interpreter.isNull())
			return true;

		interpreter->requestInterruption();
		interpreter_hash_.remove(msg->wParam);
		++interfaceCount_;
		updateStatusText();
		*result = 1;
		return true;
	}
	case InterfaceMessage::kRunFile:
	{
		Injector& injector = Injector::getInstance();
		if (injector.IS_SCRIPT_FLAG.load(std::memory_order_acquire))
			return true;

		//檢查是否為合法字符串指針
		QByteArray utf8str = reinterpret_cast<char*>(msg->lParam);
		if (utf8str.isEmpty())
			break;

		QString fileName = QString::fromUtf8(utf8str);
		if (!QFile::exists(fileName))
			return true;

		pScriptForm_->loadFile(fileName);
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
		emit signalDispatcher.loadFileToTable(fileName);
		emit signalDispatcher.scriptStarted();
		++interfaceCount_;
		updateStatusText();
		*result = 1;
		return true;
	}
	case InterfaceMessage::kStopFile:
	{
		Injector& injector = Injector::getInstance();
		if (!injector.IS_SCRIPT_FLAG.load(std::memory_order_acquire))
			return true;

		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
		emit signalDispatcher.scriptStoped();
		++interfaceCount_;
		updateStatusText();
		*result = 1;
		return true;
	}
	case InterfaceMessage::kRunGame:
	{
		Injector& injector = Injector::getInstance();
		if (!injector.server.isNull())
			return true;

		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
		emit signalDispatcher.gameStarted();
		++interfaceCount_;
		updateStatusText();
		*result = 1;
		return true;
	}
	case InterfaceMessage::kCloseGame:
	{
		Injector& injector = Injector::getInstance();
		if (injector.server.isNull())
			return true;

		injector.close();
		++interfaceCount_;
		updateStatusText();
		*result = 1;
		return true;
	}
	case InterfaceMessage::kGetGameState:
	{
		if (result == nullptr)
			return true;

		Injector& injector = Injector::getInstance();
		int value = 0;
		if (injector.server.isNull())
			return true;

		bool ok = injector.server->IS_TCP_CONNECTION_OK_TO_USE;
		if (!ok)
			return true;
		else
			value = 1;

		if (!injector.server->getOnlineFlag())
			value = 2;
		else
		{
			if (!injector.server->getBattleFlag())
				value = 3;
			else
				value = 4;
		}

		*result = value;
		return true;
	}
	case InterfaceMessage::kScriptState:
	{
		if (result == nullptr)
			return true;

		Injector& injector = Injector::getInstance();
		int value = 0;
		if (injector.IS_SCRIPT_FLAG.load(std::memory_order_acquire))
			value = 1;

		*result = value;
		return true;
	}
	case InterfaceMessage::kOpenWindow:
	{
		int type = msg->wParam;
		int arg = msg->lParam;

		switch (type)
		{
		case WindowInfo:
		{
			*result = 0;
			if (pInfoForm_ == nullptr)
			{
				pInfoForm_ = new InfoForm(arg);
				if (pInfoForm_)
				{
					connect(pInfoForm_, &InfoForm::destroyed, [this]() { pInfoForm_ = nullptr; });
					pInfoForm_->setAttribute(Qt::WA_DeleteOnClose);
					pInfoForm_->show();
					*result = static_cast<long>(pInfoForm_->winId());
				}
			}
			return true;
		}
		case WindowMap:
		{
			*result = 0;
			if (mapWidget_ == nullptr)
			{
				mapWidget_ = new MapWidget(nullptr);
				if (mapWidget_)
				{
					connect(mapWidget_, &InfoForm::destroyed, [this]() { mapWidget_ = nullptr; });
					mapWidget_->setAttribute(Qt::WA_DeleteOnClose);
					mapWidget_->show();
					*result = static_cast<long>(mapWidget_->winId());
				}
			}
			return true;
		}
		case WindowScript:
		{
			*result = 0;
			if (pScriptSettingForm_ == nullptr)
			{
				pScriptSettingForm_ = new ScriptSettingForm;
				if (pScriptSettingForm_)
				{
					connect(pScriptSettingForm_, &InfoForm::destroyed, [this]() { pScriptSettingForm_ = nullptr; });
					pScriptSettingForm_->setAttribute(Qt::WA_DeleteOnClose);
					pScriptSettingForm_->show();
					*result = static_cast<long>(pScriptSettingForm_->winId());
				}
			}
			return true;
		}
		default:
			break;
		}

		break;
	}
	case InterfaceMessage::kSortWindow:
	{
		do
		{
			const char* chwndstrs = reinterpret_cast<char*>(msg->lParam);
			if (chwndstrs == nullptr)
				break;

			//檢查是否為合法字符串指針
			QByteArray hwndstrs = reinterpret_cast<char*>(msg->lParam);
			if (hwndstrs.isEmpty())
				break;

			QString str = QString::fromUtf8(hwndstrs);
			if (str.isEmpty())
				break;

			QStringList strlist = str.split(util::rexOR);
			if (strlist.isEmpty())
				break;

			QVector<HWND> hwnds;
			for (const QString& str : strlist)
			{
				bool ok = false;
				qint64 nhwnd = str.simplified().toLongLong(&ok);
				if (!ok && nhwnd > 0)
					continue;

				HWND hWnd = reinterpret_cast<HWND>(nhwnd);
				if (!IsWindow(hWnd) || !IsWindowVisible(hWnd) || !IsWindowEnabled(hWnd))
					continue;

				hwnds.append(hWnd);
			}

			if (hwnds.isEmpty())
				break;

			bool ok = msg->wParam > 0;

			util::sortWindows(hwnds, ok);
			*result = 1;

		} while (false);

		return true;
	}
	case InterfaceMessage::kThumbnail:
	{
		*result = 0;
		do
		{
			const char* chwndstrs = reinterpret_cast<char*>(msg->lParam);
			if (chwndstrs == nullptr)
				break;

			//檢查是否為合法字符串指針
			QByteArray hwndstrs = reinterpret_cast<char*>(msg->lParam);
			if (hwndstrs.isEmpty())
				break;

			QString str = QString::fromUtf8(hwndstrs);
			if (str.isEmpty())
				break;

			QStringList strlist = str.split(util::rexOR);
			if (strlist.isEmpty())
				break;

			QList<HWND> hwnds;
			for (const QString& str : strlist)
			{
				bool ok = false;
				qint64 nhwnd = str.simplified().toLongLong(&ok);
				if (!ok && nhwnd > 0)
					continue;

				HWND hWnd = reinterpret_cast<HWND>(nhwnd);
				if (!IsWindow(hWnd) || !IsWindowVisible(hWnd) || !IsWindowEnabled(hWnd))
					continue;

				hwnds.append(hWnd);
			}

			if (hwnds.isEmpty())
				break;

			if (pThumbnailForm_ == nullptr)
			{
				QThumbnailForm* pThumbnailForm = q_check_ptr(new QThumbnailForm(hwnds));
				if (pThumbnailForm == nullptr)
					break;

				pThumbnailForm_ = pThumbnailForm;

				connect(pThumbnailForm, &QThumbnailForm::destroyed, [this]() { pThumbnailForm_ = nullptr; });
				pThumbnailForm->move(0, 0);
				pThumbnailForm->show();
				*result = 1;
				return true;
			}
			else
			{
				pThumbnailForm_->initThumbnailWidget(hwnds);
				*result = 2;
				return true;
			}

		} while (false);

		if (pThumbnailForm_ != nullptr)
		{
			pThumbnailForm_->close();
			pThumbnailForm_ = nullptr;
			*result = 1;
		}
		return true;
	}
	default:
	{
		break;
	}
	}

	return false;
}

//窗口移動事件
void MainForm::moveEvent(QMoveEvent* e)
{
	do
	{
		if (e == nullptr)
			return;


		QPoint pos = e->pos();
		qDebug() << pos;
		Injector& injector = Injector::getInstance();
		HWND hWnd = injector.getProcessWindow();
		if (hWnd == nullptr)
			break;

		if (!injector.getEnableHash(util::kWindowDockEnable))
			break;

		//HWND獲取寬度
		RECT rect;
		GetWindowRect(hWnd, &rect);

		//窗口如果不可見則恢復
		if (!IsWindowVisible(hWnd))
		{
			ShowWindow(hWnd, SW_RESTORE);
			ShowWindow(hWnd, SW_SHOW);
		}

		//將目標窗口吸附在本窗口左側
		PostMessageW(hWnd, WM_MOVE + WM_USER, 0, MAKELPARAM(pos.x() - 654, pos.y() - 31));


	} while (false);


}

//更新接口調用次數顯示
void MainForm::updateStatusText()
{
	ui.groupBox_basicinfo->setTitle(tr("basic info - count:%1, subscript:%2").arg(interfaceCount_).arg(interpreter_hash_.size()));
}

MainForm::MainForm(QWidget* parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_QuitOnClose);
	setAttribute(Qt::WA_StyledBackground, true);
	setAttribute(Qt::WA_StaticContents, true);
	setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
	setStyleSheet(R"(
		QMainWindow{ border-radius: 10px; }
	)");

	qRegisterMetaType<QVariant>("QVariant");
	qRegisterMetaType<QVariant>("QVariant&");

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	signalDispatcher.setParent(this);

	connect(&signalDispatcher, &SignalDispatcher::saveHashSettings, this, &MainForm::onSaveHashSettings, Qt::UniqueConnection);
	connect(&signalDispatcher, &SignalDispatcher::loadHashSettings, this, &MainForm::onLoadHashSettings, Qt::UniqueConnection);
	connect(&signalDispatcher, &SignalDispatcher::messageBoxShow, this, &MainForm::onMessageBoxShow, Qt::BlockingQueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::inputBoxShow, this, &MainForm::onInputBoxShow, Qt::BlockingQueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::updateMainFormTitle, this, &MainForm::onUpdateMainFormTitle, Qt::UniqueConnection);
	connect(&signalDispatcher, &SignalDispatcher::appendScriptLog, this, &MainForm::onAppendScriptLog, Qt::UniqueConnection);
	connect(&signalDispatcher, &SignalDispatcher::appendChatLog, this, &MainForm::onAppendChatLog, Qt::UniqueConnection);

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
			pAfkForm_->hide();
			QPushButton* pButton = new QPushButton(tr("afk"));
			if (pButton)
			{
				//insert to tab
				ui.tabWidget_main->addTab(pButton, tr("afk"));
				//push to open afkform
				connect(pButton, &QPushButton::clicked, this, [this]()
					{
						pAfkForm_->show();
					});
			}

			//ui.tabWidget_main->addTab(pAfkForm_, tr("afk"));
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
	qDebug() << "MainForm::~MainForm()";
	Injector::getInstance().close();
	MINT::NtTerminateProcess(GetCurrentProcess(), 0);
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

//菜單點擊事件
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
		if (trayIcon == nullptr)
		{
			trayIcon = new QSystemTrayIcon(this);
			QIcon icon = QIcon(":/image/ico.png");
			trayIcon->setIcon(icon);
			QMenu* trayMenu = new QMenu(this);
			QAction* openAction = new QAction(tr("open"), this);
			QAction* closeAction = new QAction(tr("close"), this);
			trayMenu->addAction(openAction);
			trayMenu->addAction(closeAction);
			connect(openAction, &QAction::triggered, this, &QMainWindow::show);
			connect(openAction, &QAction::triggered, [this]()
				{
					trayIcon->hide();
				});
			connect(closeAction, &QAction::triggered, this, &QMainWindow::close);

			trayIcon->setContextMenu(trayMenu);
			connect(trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason)
				{
					if (reason == QSystemTrayIcon::DoubleClick)
					{
						this->show();
						trayIcon->hide();
					}
				});
			trayIcon->setToolTip(windowTitle());
		}
		hide();
		trayIcon->showMessage(tr("Tip"), tr("The program has been minimized to the system tray"), QSystemTrayIcon::Information, 2000);
		trayIcon->show();
	}

	else if (actionName == "actionInfo")
	{
		QDesktopServices::openUrl(QUrl("https://gitee.com/Bestkakkoii/sash/wikis/pages"));
	}

	else if (actionName == "actionWebsite")
	{
		QMessageBox::information(this, "SaSH",
			QString(u8"飞 Philip\n\nCopyright ©2019-2023 Bestkakkoii llc. All rights reserved.\n\nURL:https://www.lovesa.cc\nQQ Group:\n224068611\n\nCurrent Version:\n%1")
			.arg(util::buildDateTime(nullptr)));
	}

	else if (actionName == "actionClose")
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
		if (pInfoForm_ == nullptr)
		{
			pInfoForm_ = new InfoForm;
			if (pInfoForm_)
			{
				connect(pInfoForm_, &InfoForm::destroyed, [this]() { pInfoForm_ = nullptr; });
				pInfoForm_->setAttribute(Qt::WA_DeleteOnClose);
				pInfoForm_->show();
			}
		}
	}

	else if (actionName == "actionMap")
	{
		if (mapWidget_ == nullptr)
		{
			mapWidget_ = new MapWidget(nullptr);
			if (mapWidget_)
			{
				connect(mapWidget_, &InfoForm::destroyed, [this]() { mapWidget_ = nullptr; });
				mapWidget_->setAttribute(Qt::WA_DeleteOnClose);
				mapWidget_->show();
			}
		}
	}

	else if (actionName == "actionScriptSettings")
	{
		if (pScriptSettingForm_ == nullptr)
		{
			pScriptSettingForm_ = new ScriptSettingForm;
			if (pScriptSettingForm_)
			{
				connect(pScriptSettingForm_, &InfoForm::destroyed, [this]() { pScriptSettingForm_ = nullptr; });
				pScriptSettingForm_->setAttribute(Qt::WA_DeleteOnClose);
				pScriptSettingForm_->show();
			}
		}
	}

	else if (actionName == "actionSave")
	{
		QString fileName;
		Injector& injector = Injector::getInstance();
		if (!injector.server.isNull())
			fileName = injector.server->getPC().name;
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
		emit signalDispatcher.saveHashSettings(fileName);
	}

	else if (actionName == "actionLoad")
	{
		QString fileName;
		Injector& injector = Injector::getInstance();
		if (!injector.server.isNull())
			fileName = injector.server->getPC().name;
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
		emit signalDispatcher.loadHashSettings(fileName);
	}

	else if (actionName == "actionUpdate")
	{
		QString result;
		QMessageBox::StandardButton ret;
		if (QDownloader::checkUpdate(&result))
		{
			ret = QMessageBox::warning(this, tr("Update"), \
				tr("New version:%1 were found!\n\nUpdate process will cause all the games to be closed, are you sure to continue?").arg(result), \
				QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
		}
		else
		{
			ret = QMessageBox::information(this, tr("Update"), tr("No new version available. Do you still want to update?"), \
				QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
		}

		if (QMessageBox::No == ret)
			return;

		QDownloader* downloader = q_check_ptr(new QDownloader());
		if (downloader)
		{
			hide();
			downloader->show();
		}
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
	//constexpr int VERSION_PATCH = 0;
	const QString strversion = QString("%1.%2%3").arg(VERSION_MAJOR).arg(VERSION_MINOR).arg("." + util::buildDateTime(nullptr));
	setWindowTitle(tr("SaSH - %1").arg(strversion));

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
		{ util::kLabelStatusDisconnected, tr("disconnected")},
		{ util::kLabelStatusConnecting, tr("connecting")},
	};
	ui.label_status->setText(hash.value(static_cast<util::UserStatus>(status), tr("unknown")));
}

void MainForm::onUpdateMapLabelTextChanged(const QString& text)
{
	ui.label_map->setText(text);
}

void MainForm::onUpdateCursorLabelTextChanged(const QString& text)
{
	ui.label_cursor->setText(text);
}

void MainForm::onUpdateCoordsPosLabelTextChanged(const QString& text)
{
	ui.label_coords->setText(text);
}

void MainForm::onUpdateMainFormTitle(const QString& text)
{
	setWindowTitle(QString("SaSH-%1").arg(text));
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

//消息框
void MainForm::onMessageBoxShow(const QString& text, int type, int* pnret)
{
	QMessageBox::StandardButton button = QMessageBox::StandardButton::NoButton;

	QString newText = text;
	newText.replace("\\r\\n", "\r\n");
	newText.replace("\\n", "\n");
	newText.replace("\\t", "\t");
	newText.replace("\\v", "\v");
	newText.replace("\\b", "\b");
	newText.replace("\\f", "\f");
	newText.replace("\\a", "\a");

	if (pnret)
	{
		if (type == 2)
			button = QMessageBox::warning(this, tr("warning"), newText, QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::No);
		else if (type == 3)
			button = QMessageBox::critical(this, tr("critical"), newText, QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::No);
		else
			button = QMessageBox::information(this, tr("info"), newText, QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::No);

		*pnret = button;
	}
	else
	{
		if (type == 2)
			button = QMessageBox::warning(this, tr("warning"), newText);
		else if (type == 3)
			button = QMessageBox::critical(this, tr("critical"), newText);
		else
			button = QMessageBox::information(this, tr("info"), newText);
	}

}

//输入框
void MainForm::onInputBoxShow(const QString& text, int type, QVariant* retvalue)
{
	if (retvalue == nullptr)
		return;

	QString newText = text;
	newText.replace("\\r\\n", "\r\n");
	newText.replace("\\n", "\n");
	newText.replace("\\t", "\t");
	newText.replace("\\v", "\v");
	newText.replace("\\b", "\b");
	newText.replace("\\f", "\f");
	newText.replace("\\a", "\a");

	QInputDialog inputDialog(this);
	inputDialog.setLabelText(newText);
	QInputDialog::InputMode mode = static_cast<QInputDialog::InputMode>(type);
	inputDialog.setInputMode(mode);
	if (mode == QInputDialog::IntInput)
	{
		inputDialog.setIntMinimum(INT_MIN);
		inputDialog.setIntMaximum(INT_MAX);
	}
	else if (mode == QInputDialog::DoubleInput)
	{
		inputDialog.setDoubleMinimum(-DBL_MAX);
		inputDialog.setDoubleMaximum(DBL_MAX);
	}

	inputDialog.setWindowFlags(inputDialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);
	auto ret = inputDialog.exec();
	if (ret != QDialog::Accepted)
		return;

	switch (type)
	{
	case QInputDialog::IntInput:
		*retvalue = static_cast<qint64>(inputDialog.intValue());
		break;
	case QInputDialog::DoubleInput:
		*retvalue = static_cast<qint64>(inputDialog.doubleValue());
		break;
	case QInputDialog::TextInput:
		*retvalue = inputDialog.textValue();
		break;
	}
}

//腳本日誌
void MainForm::onAppendScriptLog(const QString& text, int color)
{
	Injector& injector = Injector::getInstance();
	if (!injector.scriptLogModel.isNull())
	{
		injector.scriptLogModel->append(text, color);
		emit injector.scriptLogModel->dataAppended();
	}
}

//對話日誌
void MainForm::onAppendChatLog(const QString& text, int color)
{
	Injector& injector = Injector::getInstance();
	if (!injector.chatLogModel.isNull())
	{
		injector.chatLogModel->append(text, color);
		emit injector.chatLogModel->dataAppended();
	}
}