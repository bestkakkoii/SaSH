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
#include "mainform.h"

//TabWidget pages
#include "form/selectobjectform.h"
#include "form/generalform.h"
#include "form/mapform.h"
#include "form/otherform.h"
#include "form/scriptform.h"
#include "form/luascriptform.h"
#include "form/infoform.h"
#include "form/mapwidget.h"
#include "form/copyrightdialog.h"

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

	auto createAction = [](QMenu* parent, const QString& text, const QString& name, const int& key)
	{
		if (!parent)
			return;

		QString shortcutText = QKeySequence(key).toString(QKeySequence::NativeText);

		QFontMetrics fontMetrics(QApplication::font());
		int textWidth = fontMetrics.horizontalAdvance(text);
		int shortcutWidth = fontMetrics.horizontalAdvance(shortcutText);
		int totalWidth = 130;  // 文本和快捷键部分的总宽度
		int spaceCount = (totalWidth - textWidth - shortcutWidth) / fontMetrics.horizontalAdvance(' ');

		QString alignedText = text + QString(spaceCount, ' ') + shortcutText;

		QAction* pAction = new QAction(alignedText, parent);
		if (!pAction)
			return;
		if (!text.isEmpty() && !name.isEmpty())
		{
			pAction->setObjectName(name);
			pAction->setShortcut(QKeySequence(key));
			parent->addAction(pAction);
		}
		else
			pAction->setSeparator(true);

		parent->addAction(pAction);
	};

	auto create = [&createAction](const QVector<std::tuple<QString, QString, int>>& table, QMenu* pMenu)
	{
		for (const std::tuple<QString, QString, int>& tuple : table)
		{
			if (std::get<0>(tuple).isEmpty() || std::get<1>(tuple).isEmpty())
			{
				createAction(pMenu, "", "", Qt::Key_unknown);
				continue;
			}

			createAction(pMenu, std::get<0>(tuple), std::get<1>(tuple), std::get<2>(tuple));
		}
	};

	const QVector<std::tuple<QString, QString, int>> systemTable = {
		{ QObject::tr("hide"), "actionHide", Qt::Key_F9},
		{ "","" , Qt::Key_unknown},
		{ QObject::tr("website"), "actionWebsite", Qt::Key_unknown },
		{ QObject::tr("scriptdoc"), "actionInfo", Qt::Key_unknown },
		{ "", "", Qt::Key_unknown },
		{ QObject::tr("close"), "actionClose", Qt::ALT | Qt::Key_F4 },
		{ QObject::tr("close game"), "actionCloseGame", Qt::ALT | Qt::Key_F4 },
	};

	const QVector<std::tuple<QString, QString, int>> otherTable = {
		{ QObject::tr("otherinfo"), "actionOtherInfo", Qt::Key_F5 },
		{ QObject::tr("script settings"), "actionScriptSettings", Qt::Key_F7 },
		{ "","", Qt::Key_unknown },
		{ QObject::tr("map"), "actionMap", Qt::Key_F8 },
	};

	const QVector<std::tuple<QString, QString, int>> fileTable = {
		{ QObject::tr("save"), "actionSave", Qt::CTRL + Qt::Key_S },
		{ QObject::tr("load"), "actionLoad", Qt::CTRL + Qt::Key_O },
		{ "","", Qt::Key_unknown },
		{ QObject::tr("checkupdate"), "actionUpdate", Qt::CTRL + Qt::Key_U },
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

		//檢查是否為合法字符串指針
		QByteArray utf8str = reinterpret_cast<char*>(msg->lParam);
		if (utf8str.isEmpty())
			return true;

		int id = msg->wParam;
		interpreter_hash_.insert(id, interpreter);

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
			return true;

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

		++interfaceCount_;
		updateStatusText();
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

		++interfaceCount_;
		updateStatusText();
		*result = value;
		return true;
	}
	case InterfaceMessage::kOpenWindow:
	{
		int type = msg->wParam;
		int arg = msg->lParam;
		*result = 0;

		switch (type)
		{
		case WindowInfo:
		{
			if (pInfoForm_ == nullptr)
			{
				pInfoForm_ = new InfoForm(arg);
				if (pInfoForm_)
				{
					connect(pInfoForm_, &InfoForm::destroyed, [this]() { pInfoForm_ = nullptr; });
					pInfoForm_->setAttribute(Qt::WA_DeleteOnClose);
					pInfoForm_->show();

					++interfaceCount_;
					updateStatusText();
					*result = static_cast<long>(pInfoForm_->winId());
				}
			}
			break;
		}
		case WindowMap:
		{
			if (mapWidget_ == nullptr)
			{
				mapWidget_ = new MapWidget(nullptr);
				if (mapWidget_)
				{
					connect(mapWidget_, &InfoForm::destroyed, [this]() { mapWidget_ = nullptr; });
					mapWidget_->setAttribute(Qt::WA_DeleteOnClose);
					mapWidget_->show();

					++interfaceCount_;
					updateStatusText();
					*result = static_cast<long>(mapWidget_->winId());
				}
			}
			break;
		}
		case WindowScript:
		{
			if (pScriptSettingForm_ == nullptr)
			{
				pScriptSettingForm_ = new ScriptSettingForm;
				if (pScriptSettingForm_)
				{
					connect(pScriptSettingForm_, &InfoForm::destroyed, [this]() { pScriptSettingForm_ = nullptr; });
					pScriptSettingForm_->setAttribute(Qt::WA_DeleteOnClose);
					pScriptSettingForm_->show();

					++interfaceCount_;
					updateStatusText();
					*result = static_cast<long>(pScriptSettingForm_->winId());
				}
			}
			break;
		}
		default:
			break;
		}

		return true;
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

			++interfaceCount_;
			updateStatusText();
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

				++interfaceCount_;
				updateStatusText();
				*result = 1;
				return true;
			}
			else
			{
				pThumbnailForm_->initThumbnailWidget(hwnds);

				++interfaceCount_;
				updateStatusText();
				*result = 2;
				return true;
			}

		} while (false);

		if (pThumbnailForm_ != nullptr)
		{
			pThumbnailForm_->close();
			pThumbnailForm_ = nullptr;

			++interfaceCount_;
			updateStatusText();
			*result = 1;
		}
		return true;
	}
	default:
		break;
	}

	return false;
}

//void MainForm::paintEvent(QPaintEvent* e)
//{
//	Injector& injector = Injector::getInstance();
//	HWND hWnd = injector.getProcessWindow();
//	if (hWnd != nullptr)
//	{
//		//movewindow to refresh
//		RECT rect;
//		GetWindowRect(hWnd, &rect);
//		MoveWindow(hWnd, 0, 0, rect.right - rect.left, rect.bottom - rect.top, TRUE);
//	}
//
//
//	QMainWindow::paintEvent(e);
//}

//窗口移動事件
void MainForm::moveEvent(QMoveEvent* e)
{
	do
	{
		if (e == nullptr)
			return;


		QPoint pos = e->pos();

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
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	setFixedSize(290, 481);
	setStyleSheet("QMainWindow{ border-radius: 10px; background-color: rgb(245, 245, 245); } ");

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
		connect(&signalDispatcher, &SignalDispatcher::updatePlayerInfoStone, this, &MainForm::onUpdateStonePosLabelTextChanged);
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

		pLuaScriptForm_ = new LuaScriptForm;
		if (pLuaScriptForm_)
		{
			ui.tabWidget_main->addTab(pLuaScriptForm_, tr("lua"));
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

	emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusNotOpen);
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
		CopyRightDialog* pCopyRightDialog = new CopyRightDialog(this);
		if (pCopyRightDialog)
		{
			pCopyRightDialog->exec();
		}

		//		QMessageBox::information(this, "SaSH",
		//			QString(u8R"(
		//作者:
		//飞 Philip
		//
		//Copyright ©2019-2023 Bestkakkoii llc. All rights reserved.
		//
		//论坛:
		//https://www.lovesa.cc
		//
		//QQ 群:
		//224068611
		//
		//当前版本:
		//%1
		//
		//特别感谢:
		//eric, 辉, match_stick, 手柄, 老花, 小雅 热心帮忙测试、查找bug，和给予大量优质的建议
		//)")
		//.arg(util::buildDateTime(nullptr)));
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
		QString current;
		QString result;
		QMessageBox::StandardButton ret;
		if (QDownloader::checkUpdate(&current, &result))
		{
			ret = QMessageBox::warning(this, tr("Update"), \
				tr("Current version:%1\nNew version:%2 were found!\n\nUpdate process will cause all the games to be closed, are you sure to continue?") \
				.arg(current).arg(result), \
				QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
		}
		else
		{
			ret = QMessageBox::information(this, tr("Update"), tr("Current version:%1\nNew version:%2\nNo new version available. Do you still want to update?") \
				.arg(current).arg(result), \
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

	switch (acp)
	{
	case 936:
		translator_.load(QString("%1/translations/qt_zh_CN.qm").arg(util::applicationDirPath()));
		break;
		//English
	case 950:
		translator_.load(QString("%1/translations/qt_zh_TW.qm").arg(util::applicationDirPath()));
		break;
		//Chinese
	default:
		translator_.load(QString("%1/translations/qt_en_US.qm").arg(util::applicationDirPath()));
		break;
	}

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
	ui.tabWidget_main->setTabText(2, tr("other"));
	ui.tabWidget_main->setTabText(3, tr("script"));
	ui.tabWidget_main->setTabText(4, "lua");

	if (pGeneralForm_)
		emit pGeneralForm_->resetControlTextLanguage();

	if (pInfoForm_)
		emit pInfoForm_->resetControlTextLanguage();

	if (pOtherForm_)
		emit pOtherForm_->resetControlTextLanguage();
}

void MainForm::onUpdateStatusLabelTextChanged(int status)
{
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
		{ util::kLabelStatusLoginFailed, tr("login failed")},
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

void MainForm::onUpdateStonePosLabelTextChanged(int ntext)
{
	ui.label_stone->setText(QString::number(ntext));
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

		QString directory = util::applicationDirPath() + "/settings/";
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

	Injector& injector = Injector::getInstance();
	util::SafeHash<util::UserSetting, bool> enableHash = injector.getEnableHash();
	util::SafeHash<util::UserSetting, int> valueHash = injector.getValueHash();
	util::SafeHash<util::UserSetting, QString> stringHash = injector.getStringHash();

	QHash<util::UserSetting, QString> jsonKeyHash = util::user_setting_string_hash;

	util::Config config(fileName);
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

		QString directory = util::applicationDirPath() + "/settings/";
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

	Injector& injector = Injector::getInstance();
	util::SafeHash<util::UserSetting, bool> enableHash;
	util::SafeHash<util::UserSetting, int> valueHash;
	util::SafeHash<util::UserSetting, QString> stringHash;

	QHash<util::UserSetting, QString> jsonKeyHash = util::user_setting_string_hash;

	{
		util::Config config(fileName);
		for (QHash < util::UserSetting, QString>::const_iterator iter = jsonKeyHash.begin(); iter != jsonKeyHash.end(); ++iter)
		{
			QString key = iter.value();
			if (!key.endsWith("Enable"))
				continue;
			bool value = config.read<bool>("User", "Enable", key);
			util::UserSetting hkey = iter.key();
			enableHash.insert(hkey, value);
		}

		for (QHash < util::UserSetting, QString>::const_iterator iter = jsonKeyHash.begin(); iter != jsonKeyHash.end(); ++iter)
		{
			QString key = iter.value();
			if (!key.endsWith("Value"))
				continue;
			int value = config.read<int>("User", "Value", key);
			util::UserSetting hkey = iter.key();
			valueHash.insert(hkey, value);
		}

		for (QHash < util::UserSetting, QString>::const_iterator iter = jsonKeyHash.begin(); iter != jsonKeyHash.end(); ++iter)
		{
			QString key = iter.value();
			if (!key.endsWith("String"))
				continue;
			QString value = config.read<QString>("User", "String", key);
			util::UserSetting hkey = iter.key();
			stringHash.insert(hkey, value);
		}
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

	QInputDialog inputDialog;
	inputDialog.setModal(true);
	inputDialog.setLabelText(newText);
	QInputDialog::InputMode mode = static_cast<QInputDialog::InputMode>(type);
	inputDialog.setInputMode(mode);
	if (mode == QInputDialog::IntInput)
	{
		inputDialog.setIntMinimum(INT_MIN);
		inputDialog.setIntMaximum(INT_MAX);
		if (retvalue->isValid())
			inputDialog.setIntValue(retvalue->toInt());
	}
	else if (mode == QInputDialog::DoubleInput)
	{
		inputDialog.setDoubleMinimum(-DBL_MAX);
		inputDialog.setDoubleMaximum(DBL_MAX);
		if (retvalue->isValid())
			inputDialog.setDoubleValue(retvalue->toDouble());
	}
	else
	{
		if (retvalue->isValid())
			inputDialog.setTextValue(retvalue->toString());
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