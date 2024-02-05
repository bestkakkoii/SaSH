/*
				GNU GENERAL PUBLIC LICENSE
				   Version 2, June 1991
COPYRIGHT (C) Bestkakkoii 2024 All Rights Reserved.
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

#include "mainthread.h"

//utilities
#include "signaldispatcher.h"
#include <util.h>
#include <gamedevice.h>
#include "script/interpreter.h"
#include "net/rpc.h"

safe::hash<long long, MainForm*> g_mainFormHash;

void MainForm::createMenu(QMenuBar* pMenuBar)
{
	if (pMenuBar == nullptr)
		return;

#pragma region StyleSheet
	constexpr const char* styleText = R"(
				QMenu {
					background-color: white; /*整個背景*/
				}
				QMenu::item {
					background-color: white;

				}
				QMenu::item:selected {

				}
				QMenu::item:pressed {

				}
			)";
#pragma endregion

	pMenuBar->setStyleSheet(styleText);
	pMenuBar->setAttribute(Qt::WA_StyledBackground, true);
	pMenuBar->clear();

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	using KeyType = long long;
#else
	using KeyType = QKeyCombination;
#endif

	auto createAction = [this](QMenu* parent, const QString& text, const QString& name, KeyType key)
		{
			if (!parent)
				return;

			QString newText = text;
			bool isCheck = false;
			if (newText.startsWith("^"))
			{
				newText.remove(0, 1);
				isCheck = true;
			}

			QAction* pAction = q_check_ptr(new QAction(newText, parent));
			sash_assume(pAction != nullptr);
			if (pAction == nullptr)
				return;

			if (!newText.isEmpty() && !name.isEmpty())
			{
				pAction->setObjectName(name);
				pAction->setShortcut(QKeySequence(key));
				if (isCheck)
					pAction->setCheckable(true);

				connect(pAction, &QAction::triggered, this, &MainForm::onMenuActionTriggered);

				if (name == "actionHide")
				{
					hideTrayAction_ = pAction;
				}

				if (name == "actionHideBar")
				{
					bool bret = false;
					{
						util::Config config(QString("%1|%2").arg(__FUNCTION__).arg(__LINE__));
						bret = config.read<bool>("MainFormClass", "Menu", "HideBar");
					}
					pAction->setChecked(bret);
					emit pAction->triggered();
				}

				if (name == "actionHideControl")
				{
					bool bret = false;
					{
						util::Config config(QString("%1|%2").arg(__FUNCTION__).arg(__LINE__));
						bret = config.read<bool>("MainFormClass", "Menu", "HideControl");
					}
					pAction->setChecked(bret);
					emit pAction->triggered();
				}

				parent->addAction(pAction);
			}
			else
				pAction->setSeparator(true);

			parent->addAction(pAction);
		};

	auto create = [&createAction](const QVector<std::tuple<QString, QString, KeyType>>& table, QMenu* pMenu)
		{
			for (const std::tuple<QString, QString, KeyType>& tuple : table)
			{
				if (std::get<0>(tuple).isEmpty() || std::get<1>(tuple).isEmpty())
				{
					createAction(pMenu, "", "", Qt::Key_unknown);
					continue;
				}

				createAction(pMenu, std::get<0>(tuple), std::get<1>(tuple), std::get<2>(tuple));
			}
		};

	const QVector<std::tuple<QString, QString, KeyType>> systemTable = {
		{ QObject::tr("hide"), "actionHide", Qt::Key_F9},
		{ QObject::tr("hidegame"), "actionHideGame", Qt::Key_F10},
		{ "^" + QObject::tr("hidebar"), "actionHideBar", Qt::Key_unknown},
		{ "^" + QObject::tr("hidecontrol"), "actionHideControl", Qt::Key_unknown},
		{ "","" , Qt::Key_unknown},
		{ QObject::tr("website"), "actionWebsite", Qt::Key_unknown },
		{ QObject::tr("scriptdoc"), "actionInfo", Qt::Key_unknown },
		{ "", "", Qt::Key_unknown },
		{ QObject::tr("close"), "actionClose", Qt::ALT | Qt::Key_F4 },
		{ QObject::tr("close game"), "actionCloseGame", Qt::ALT | Qt::Key_F4 },
		{ "", "", Qt::Key_unknown },
		{ QObject::tr("closeAll"), "actionCloseAll", Qt::Key_unknown },
	};

	const QVector<std::tuple<QString, QString, KeyType>> otherTable = {
		{ QObject::tr("otherinfo"), "actionOtherInfo", Qt::Key_F5 },
		{ QObject::tr("afksetting"), "actionAfkSetting", Qt::Key_F6 },
		{ QObject::tr("scripteditor"), "actionScriptEditor", Qt::Key_F7 },
		{ "","", Qt::Key_unknown },
		{ QObject::tr("map"), "actionMap", Qt::Key_F8 },
	};

	const QVector<std::tuple<QString, QString, KeyType>> fileTable = {
		{ QObject::tr("save"), "actionSave", Qt::CTRL | Qt::Key_S },
		{ QObject::tr("load"), "actionLoad", Qt::CTRL | Qt::Key_O },
		{ "","", Qt::Key_unknown },
		{ QObject::tr("checkupdate"), "actionUpdate", Qt::CTRL | Qt::Key_U },
	};

	std::unique_ptr<QMenu> pMenuSystem(q_check_ptr(new QMenu(QObject::tr("system"))));
	sash_assume(pMenuSystem != nullptr);
	std::unique_ptr<QMenu> pMenuOther(q_check_ptr(new QMenu(QObject::tr("other"))));
	sash_assume(pMenuOther != nullptr);
	std::unique_ptr<QMenu> pMenuFile(q_check_ptr(new QMenu(QObject::tr("file"))));
	sash_assume(pMenuFile != nullptr);

	if (pMenuSystem == nullptr || pMenuOther == nullptr || pMenuFile == nullptr)
		return;

	QMenu* _pMenuSystem = pMenuSystem.release();
	QMenu* _pMenuOther = pMenuOther.release();
	QMenu* _pMenuFile = pMenuFile.release();

	pMenuBar->addMenu(_pMenuSystem);
	pMenuBar->addMenu(_pMenuOther);
	pMenuBar->addMenu(_pMenuFile);

	create(systemTable, _pMenuSystem);
	create(otherTable, _pMenuOther);
	create(fileTable, _pMenuFile);
}

static bool isValidChar(const char* charPtr)
{
	try
	{
		if (charPtr != nullptr)
		{
			//test ptr
			char c = *charPtr;
			if (c == '\0')
			{
				qDebug() << __FUNCTION__ << "charPtr is empty";
				return false;
			}

			return true;
		}
		else
		{
			qDebug() << __FUNCTION__ << "charPtr is nullptr";
			return false;
		}
	}
	catch (...)
	{
		// 捕获异常，处理不合法指针的情况
		qDebug() << __FUNCTION__ << "invalid charPtr:" << util::toQString(reinterpret_cast<long long>(charPtr), 16).toUpper();
		return false;
	}
}

//接收原生的窗口消息
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
bool MainForm::nativeEvent(const QByteArray& eventType, void* message, long* result)
#else
bool MainForm::nativeEvent(const QByteArray& eventType, void* message, qintptr* result)
#endif
{
	std::ignore = eventType;
	MSG* msg = static_cast<MSG*>(message);
	long long currentIndex = getIndex();

	switch (msg->message)
	{
	case WM_MOUSEMOVE + WM_USER:
	{
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
		emit signalDispatcher.updateCursorLabelTextChanged(QString("%1,%2").arg(GET_X_LPARAM(msg->lParam)).arg(GET_Y_LPARAM(msg->lParam)));
		*result = 1;
		return true;
	}
	case WM_KEYUP + WM_USER + VK_DELETE:
	{
		GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
		if (!gamedevice.worker.isNull())
		{
			gamedevice.worker->cleanChatHistory();
			*result = 1;
		}
		return true;
	}
	case InterfaceMessage::kGetCurrentId:
	{
		*result = currentIndex;
		++interfaceCount_;
		updateStatusText();
		return true;
	}
	case InterfaceMessage::kRunScript:
	{
		long long id = msg->wParam;
		std::unique_ptr<Interpreter> interpreter(new Interpreter(id));
		sash_assume(interpreter != nullptr);
		if (nullptr == interpreter)
		{
			updateStatusText(tr("server is off"));
			return true;
		}

		const char* cstr = reinterpret_cast<char*>(msg->lParam);
		if (!isValidChar(cstr))
		{
			updateStatusText(tr("invalid lparam"));
			return true;
		}

		//檢查是否為合法字符串指針
		QByteArray utf8str(cstr);
		if (utf8str.isEmpty())
		{
			updateStatusText(tr("content is empty"));
			return true;
		}

		QString script = util::toQString(utf8str);
		connect(interpreter.get(), &Interpreter::finished, this, [this, id]()
			{
				interpreter_hash_.remove(id);
				updateStatusText();
			});

		interpreter_hash_.insert(id, std::move(interpreter));

		++interfaceCount_;
		updateStatusText();
		interpreter->doString(script);
		*result = 1;
		return true;
	}
	case InterfaceMessage::kStopScript:
	{
		long long id = msg->wParam;
		Interpreter* interpreter = interpreter_hash_.value(id).get();
		if (nullptr == interpreter)
		{
			updateStatusText(tr("server is off"));
			return true;
		}

		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(id);
		emit signalDispatcher.scriptStoped();
		interpreter_hash_.remove(id);

		++interfaceCount_;
		updateStatusText();
		*result = 1;
		return true;
	}
	case InterfaceMessage::kRunFile:
	{
		long long id = msg->wParam;
		GameDevice& gamedevice = GameDevice::getInstance(id);
		if (gamedevice.IS_SCRIPT_FLAG.get())
		{
			updateStatusText(tr("already run"));
			return true;
		}

		const char* cstr = reinterpret_cast<char*>(msg->lParam);
		if (!isValidChar(cstr))
		{
			updateStatusText(tr("invalid lparam"));
			return true;
		}

		QByteArray utf8str(cstr);
		if (utf8str.isEmpty())
		{
			updateStatusText(tr("path is empty"));
			return true;
		}

		QString fileName = util::toQString(utf8str);
		QFileInfo fileInfo(fileName);

		if (fileInfo.isAbsolute())
		{
			if (!QFile::exists(fileName))
			{
				updateStatusText(tr("file not exist"));
				return true;
			}
		}
		else
		{
			if (fileName.endsWith(".txt"))
				fileName.remove(".txt");

			QStringList files;
			util::searchFiles(util::applicationDirPath(), fileName, ".txt", &files, false);
			if (files.isEmpty())
			{
				updateStatusText(tr("file not exist"));
				return true;
			}

			fileName = files.first();
		}


		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(id);
		gamedevice.currentScriptFileName.set(fileName);
		emit signalDispatcher.loadFileToTable(fileName, true);

		++interfaceCount_;
		updateStatusText();
		*result = 1;
		return true;
	}
	case InterfaceMessage::kStopFile:
	{
		long long id = msg->wParam;
		GameDevice& gamedevice = GameDevice::getInstance(id);
		if (!gamedevice.IS_SCRIPT_FLAG.get())
		{
			updateStatusText(tr("not run yet"));
			return true;
		}

		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(id);
		emit signalDispatcher.scriptStoped();

		++interfaceCount_;
		updateStatusText();
		*result = 1;
		return true;
	}
	case InterfaceMessage::kRunGame:
	{
		long long id = msg->wParam;
		GameDevice& gamedevice = GameDevice::getInstance(id);
		if (!gamedevice.worker.isNull())
		{
			updateStatusText(tr("server already on"));
			return true;
		}

		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(id);
		emit signalDispatcher.gameStarted();

		++interfaceCount_;
		updateStatusText();
		*result = 1;
		return true;
	}
	case InterfaceMessage::kCloseGame:
	{
		long long id = msg->wParam;
		GameDevice& gamedevice = GameDevice::getInstance(id);
		if (gamedevice.worker.isNull())
		{
			updateStatusText(tr("server is off"));
			return true;
		}

		gamedevice.close();
		++interfaceCount_;
		updateStatusText();
		*result = 1;
		return true;
	}
	case InterfaceMessage::kGetGameState:
	{
		if (result == nullptr)
			return true;

		long long id = msg->wParam;
		GameDevice& gamedevice = GameDevice::getInstance(id);
		long long value = 0;
		if (gamedevice.worker.isNull())
			return true;

		if (!gamedevice.IS_TCP_CONNECTION_OK_TO_USE.get())
			return true;

		if (gamedevice.IS_INJECT_OK.get())
		{
			value = 1;

			if (!gamedevice.worker->getOnlineFlag())
				value = 2;
			else
			{
				if (!gamedevice.worker->getBattleFlag())
					value = 3;
				else
					value = 4;
			}
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

		long long id = msg->wParam;
		GameDevice& gamedevice = GameDevice::getInstance(id);
		long long value = 0;
		if (gamedevice.IS_SCRIPT_FLAG.get())
			value = 1;

		++interfaceCount_;
		updateStatusText();
		*result = value;
		return true;
	}
	case InterfaceMessage::kMultiFunction:
	{
		long long id = msg->wParam;
		long long type = LOWORD(msg->lParam);
		long long arg = HIWORD(msg->lParam);
		*result = 0;

#ifndef LEAK_TEST
		switch (type)
		{
		case WindowInfo:
		{
			if (arg > 0)
			{
				pInfoForm_.setCurrentPage(arg);
				pInfoForm_.hide();
				pInfoForm_.show();
				++interfaceCount_;
				updateStatusText();
				*result = static_cast<long>(pInfoForm_.winId());
			}
			else
			{
				pInfoForm_.hide();
				++interfaceCount_;
				updateStatusText();
			}
			break;
		}
		case WindowMap:
		{
			if (arg > 0)
			{
				mapWidget_.hide();
				mapWidget_.show();
				++interfaceCount_;
				updateStatusText();
				*result = static_cast<long>(mapWidget_.winId());
			}
			else
			{
				mapWidget_.hide();
				++interfaceCount_;
				updateStatusText();
			}
			break;
		}
		case WindowScript:
		{
			if (arg > 0)
			{
				pScriptEditor_.hide();
				pScriptEditor_.show();
				++interfaceCount_;
				updateStatusText();
				*result = static_cast<long>(pScriptEditor_.winId());
			}
			else
			{
				pScriptEditor_.hide();
				++interfaceCount_;
				updateStatusText();
			}
			break;
		}
		case SelectServerList:
		{
			if (arg < 0)
			{
				updateStatusText("invalid arg");
				break;
			}

			util::Config config(QString("%1|%2").arg(__FUNCTION__).arg(__LINE__));
			config.write("System", "Server", "LastServerListSelection", arg);
			SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(id);
			emit signalDispatcher.applyHashSettingsToUI();
			++interfaceCount_;
			updateStatusText();
			*result = 1;

			break;
		}
		case SelectProcessList:
		{
			if (arg < 0)
			{
				updateStatusText("invalid arg");
				break;
			}

			util::Config config(QString("%1|%2").arg(__FUNCTION__).arg(__LINE__));
			config.write("System", "Command", "LastSelection", arg);
			SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(id);
			emit signalDispatcher.applyHashSettingsToUI();
			++interfaceCount_;
			updateStatusText();
			*result = 1;

			break;
		}
		case ToolTrayShow:
		{
			if (hideTrayAction_ == nullptr)
				break;

			emit hideTrayAction_->triggered();
			++interfaceCount_;
			updateStatusText();
			*result = 1;
			break;
		}
		case HideGame:
		{
			GameDevice& gamedevice = GameDevice::getInstance(id);
			SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(id);
			gamedevice.setEnableHash(util::kHideWindowEnable, arg > 0);
			emit signalDispatcher.applyHashSettingsToUI();
			++interfaceCount_;
			updateStatusText();
			*result = 1;
			break;
		}
		default:
			break;
		}
#endif
		return true;
	}
	case InterfaceMessage::kSortWindow:
	{
		do
		{
			const char* chwndstrs = reinterpret_cast<char*>(msg->lParam);
			if (!isValidChar(chwndstrs))
			{
				updateStatusText(tr("invalid lparam"));
				return true;
			}

			QByteArray hwndstrs(chwndstrs);
			if (hwndstrs.isEmpty())
			{
				updateStatusText(tr("hwndstr is empty"));
				return true;
			}

			QString str = util::toQString(hwndstrs);
			if (str.isEmpty())
			{
				updateStatusText(tr("hwndstr is empty"));
				return true;
			}

			QStringList strlist = str.split(util::rexOR);
			if (strlist.isEmpty())
			{
				updateStatusText(tr("invalid hwndstr str"));
				return true;
			}

			QVector<HWND> hwnds;
			for (const QString& s : strlist)
			{
				bool ok = false;
				long long nhwnd = s.simplified().toLongLong(&ok);
				if (!ok && nhwnd > 0)
					continue;

				HWND hWnd = reinterpret_cast<HWND>(nhwnd);
				if (!IsWindow(hWnd) || !IsWindowVisible(hWnd) || !IsWindowEnabled(hWnd))
					continue;

				hwnds.append(hWnd);
			}

			if (hwnds.isEmpty())
			{
				updateStatusText(tr("no valid hwnd"));
				return true;
			}

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
			{
				updateStatusText(tr("invalid lparam"));
				break;
			}

			//檢查是否為合法字符串指針
			QByteArray hwndstrs(chwndstrs);
			if (hwndstrs.isEmpty())
			{
				updateStatusText(tr("hwndstr is empty"));
				break;
			}

			QString str = util::toQString(hwndstrs);
			if (str.isEmpty())
			{
				updateStatusText(tr("invalid hwndstr str"));
				break;
			}

			QStringList strlist = str.split(util::rexOR);
			if (strlist.isEmpty())
			{
				updateStatusText(tr("invalid hwndstr str"));
				break;
			}

			QList<HWND> hwnds;
			for (const QString& s : strlist)
			{
				bool ok = false;
				long long nhwnd = s.simplified().toLongLong(&ok);
				if (!ok && nhwnd > 0)
					continue;

				HWND hWnd = reinterpret_cast<HWND>(nhwnd);
				if (!IsWindow(hWnd) || !IsWindowVisible(hWnd) || !IsWindowEnabled(hWnd))
					continue;

				hwnds.append(hWnd);
			}

			if (hwnds.isEmpty())
			{
				updateStatusText(tr("no valid hwnd"));
				break;
			}

			if (pThumbnailForm_ == nullptr)
			{
				QThumbnailForm* pThumbnailForm = q_check_ptr(new QThumbnailForm(hwnds));
				sash_assume(pThumbnailForm != nullptr);
				if (pThumbnailForm == nullptr)
				{
					updateStatusText(tr("create thumbnail form failed"));
					break;
				}

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
	case InterfaceMessage::kOpenNewWindow:
	{
		*result = 0;
		unsigned long long preid = msg->wParam;
		long long id = -1;
		if (preid < SASH_MAX_THREAD)
		{
			id = preid;
		}

		long long* pId = reinterpret_cast<long long*>(msg->lParam);
		MainForm* p = MainForm::createNewWindow(id, pId);
		if (p == nullptr)
		{
			*result = -1;
			updateStatusText(tr("create window failed"));
			return true;
		}

		++interfaceCount_;
		updateStatusText();
		*result = static_cast<long>(p->winId());

		return true;
	}
	case InterfaceMessage::kGetGamePid:
	{
		*result = 0;
		long long id = msg->wParam;
		GameDevice& gamedevice = GameDevice::getInstance(id);
		if (gamedevice.worker.isNull())
		{
			updateStatusText(tr("server is off"));
			return true;
		}

		++interfaceCount_;
		updateStatusText();
		*result = gamedevice.getProcessId();

		return true;
	}
	case InterfaceMessage::kGetGameHwnd:
	{
		*result = 0;
		long long id = msg->wParam;
		GameDevice& gamedevice = GameDevice::getInstance(id);
		if (gamedevice.worker.isNull())
		{
			updateStatusText(tr("server is off"));
			return true;
		}

		++interfaceCount_;
		updateStatusText();
		*result = reinterpret_cast<long long>(gamedevice.getProcessWindow());
		return true;
	}
	case InterfaceMessage::kLoadSettings:
	{
		*result = 0;
		long long id = msg->wParam;
		long long pathptr = msg->lParam;
		if (pathptr == 0)
		{
			updateStatusText(tr("invalid lparam"));
			return true;
		}

		const char* chwndstrs = reinterpret_cast<char*>(pathptr);
		if (!isValidChar(chwndstrs))
		{
			updateStatusText(tr("invalid lparam"));
			return true;
		}

		QString utf8str = util::toQString(chwndstrs);
		if (utf8str.isEmpty())
		{
			updateStatusText(tr("path is empty"));
			return true;
		}

		QFileInfo fileInfo(utf8str);
		if (!fileInfo.exists())
		{
			updateStatusText(tr("file not exist"));
			return true;
		}

		if (fileInfo.suffix() != "json")
		{
			updateStatusText(tr("not json"));
			return true;
		}

		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(id);
		emit signalDispatcher.loadHashSettings(utf8str, true);
		emit signalDispatcher.applyHashSettingsToUI();
		++interfaceCount_;
		updateStatusText();
		*result = 1;
		return true;
	}
	case InterfaceMessage::kSetAutoLogin:
	{
		*result = 0;
		long long id = msg->wParam;
		GameDevice& gamedevice = GameDevice::getInstance(id);
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(id);
		do
		{
			LoginInfo* pLoginInfo = reinterpret_cast<LoginInfo*>(msg->lParam);
			qDebug() << __FUNCTION__ << "wParam:" << QString::number((int)(msg->wParam)) << "lParam:" << QString::number((int)(msg->lParam), 16);
			if (pLoginInfo == nullptr)
			{
				*result = 0;
				qDebug() << __FUNCTION__ << "pLoginInfo is nullptr" << util::toQString((int)(msg->lParam), 16);
				updateStatusText(tr("invalid lparam") + " " + util::toQString((int)(msg->lParam), 16));
				break;
			}

			qDebug() << __FUNCTION__ << "server:" << pLoginInfo->server << "subserver:" << pLoginInfo->subserver << "position:" << pLoginInfo->position;
			qDebug() << "pusername:" << util::toQString(reinterpret_cast<long long>(pLoginInfo->username), 16).toUpper();
			qDebug() << "ppassword:" << util::toQString(reinterpret_cast<long long>(pLoginInfo->password), 16).toUpper();

			if (!isValidChar(pLoginInfo->username))
			{
				*result = 2;
				qDebug() << __FUNCTION__ << "invalid user:" << util::toQString(reinterpret_cast<long long>(pLoginInfo->username), 16).toUpper();
				updateStatusText(tr("invalid user:0x%1").arg(util::toQString(reinterpret_cast<long long>(pLoginInfo->username), 16).toUpper()));
				break;
			}

			if (!isValidChar(pLoginInfo->password))
			{
				*result = 2;
				qDebug() << __FUNCTION__ << "invalid psw:" << util::toQString(reinterpret_cast<long long>(pLoginInfo->password), 16).toUpper();
				updateStatusText(tr("invalid psw:0x%1").arg(util::toQString(reinterpret_cast<long long>(pLoginInfo->password), 16).toUpper()));
				break;
			}

			long long server = pLoginInfo->server;
			long long subserver = pLoginInfo->subserver;
			long long position = pLoginInfo->position;
			QString username = util::toQString(pLoginInfo->username);
			QString password = util::toQString(pLoginInfo->password);

			qDebug() << __FUNCTION__ << "server:" << server << "subserver:" << subserver << "position:" << position << "username:" << username << "password:" << password;

			if (server < 0 || server > 25)
			{
				*result = 3;
				qDebug() << __FUNCTION__ << "server out of range:" << server;
				updateStatusText(tr("server out of range"));
				break;
			}

			if (subserver < 0 || subserver > 25)
			{
				*result = 4;
				qDebug() << __FUNCTION__ << "subserver out of range:" << subserver;
				updateStatusText(tr("subser out of range"));
				break;
			}

			if (position < 0 || position >= sa::MAX_CHARACTER + 1)
			{
				*result = 5;
				qDebug() << __FUNCTION__ << "pos out of range:" << position;
				updateStatusText(tr("pos out of range"));
				break;
			}

			gamedevice.setEnableHash(util::kAutoLoginEnable, true);
			gamedevice.setStringHash(util::kGameAccountString, username);
			gamedevice.setStringHash(util::kGamePasswordString, password);
			gamedevice.setValueHash(util::kServerValue, server);
			gamedevice.setValueHash(util::kSubServerValue, subserver);
			gamedevice.setValueHash(util::kPositionValue, position);
			qDebug() << __FUNCTION__ << "set ok apply to ui now.";
			emit signalDispatcher.applyHashSettingsToUI();
			++interfaceCount_;
			updateStatusText();
			*result = 1;
			return true;
		} while (false);

		gamedevice.setEnableHash(util::kAutoLoginEnable, false);
		gamedevice.setStringHash(util::kGameAccountString, "");
		gamedevice.setStringHash(util::kGamePasswordString, "");
		gamedevice.setValueHash(util::kServerValue, 0);
		gamedevice.setValueHash(util::kSubServerValue, 0);
		gamedevice.setValueHash(util::kPositionValue, 0);
		emit signalDispatcher.applyHashSettingsToUI();
		return true;
	}
	default:
		break;
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
		long long currentIndex = getIndex();
		GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
		HWND hWnd = gamedevice.getProcessWindow();
		if (hWnd == nullptr)
			break;

		if (!gamedevice.getEnableHash(util::kWindowDockEnable))
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
		long long width = static_cast<long long>(rect.right - rect.left);
		//654 for win11
		SetWindowPos(hWnd, HWND_TOP, pos.x() - width + 2, pos.y() - 31, 0, 0, SWP_ASYNCWINDOWPOS | SWP_NOSIZE);

	} while (false);


}

//更新接口調用次數顯示
void MainForm::updateStatusText(const QString text)
{
	QString msg = tr("count:%1").arg(interfaceCount_);
	if (!text.isEmpty())
	{
		msg += " " + QString(tr("msg:%1").arg(text));
	}
	else
	{
		msg += " " + QString(tr("msg:%1").arg(tr("no error")));
	}
	ui.groupBox_basicinfo->setTitle(msg);
}

MainForm* MainForm::createNewWindow(long long idToAllocate, long long* pId)
{
	do
	{
		long long uniqueId = UniqueIdManager::getInstance().allocateUniqueId(idToAllocate);
		if (uniqueId < 0)
			break;

		if (uniqueId >= SASH_MAX_THREAD)
			break;

		if (g_mainFormHash.contains(uniqueId))
		{
			MainForm* pMainForm = g_mainFormHash.value(uniqueId, nullptr);
			if (pMainForm != nullptr)
			{
				pMainForm->trayIcon_.hide();
				pMainForm->show();
				pMainForm->markAsClose_ = false;
				if (pId != nullptr)
					*pId = pMainForm->getIndex();
				return pMainForm;
			}
		}

		MainForm* pMainForm = q_check_ptr(new MainForm(uniqueId, nullptr));
		sash_assume(pMainForm != nullptr);
		if (pMainForm == nullptr)
			break;

		pMainForm->show();

		g_mainFormHash.insert(uniqueId, pMainForm);

		if (pId != nullptr)
			*pId = pMainForm->getIndex();

		std::ignore = QtConcurrent::run([uniqueId]()
			{
				QThread::msleep(500);
				QStringList paths;
				SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(uniqueId);
				util::searchFiles(util::applicationDirPath(), "default", ".json", &paths, false);
				if (!paths.isEmpty())
					emit signalDispatcher.loadHashSettings(paths.front(), true);
				else
					emit signalDispatcher.saveHashSettings(util::applicationDirPath() + "/settings/default.json", true);
			});

		return pMainForm;
	} while (false);

	return nullptr;
}

MainForm::MainForm(long long index, QWidget* parent)
	: QMainWindow(parent)
	, Indexer(index)
	, pGeneralForm_(index, nullptr)
#ifndef LEAK_TEST
	, pMapForm_(index, nullptr)
	, pOtherForm_(index, nullptr)
	, pScriptForm_(index, nullptr)
	, pInfoForm_(index, -1, nullptr)
	, mapWidget_(index, nullptr)
	, pScriptEditor_(index, nullptr)
#endif
{
	ui.setupUi(this);
	setFont(util::getFont());
	setAttribute(Qt::WA_StyledBackground, true);
	setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	setFixedWidth(290);

	HWND hWnd = reinterpret_cast<HWND>(winId());
	HMENU hMenu = GetSystemMenu(hWnd, FALSE);
	if (hMenu != nullptr)
		DeleteMenu(hMenu, SC_MAXIMIZE, MF_BYCOMMAND);

	util::setWidget(this);

	qRegisterMetaType<QVariant>("QVariant");
	qRegisterMetaType<QVariant>("QVariant&");

#ifndef LEAK_TEST
	pInfoForm_.hide();
	mapWidget_.hide();
	pScriptEditor_.hide();
#endif

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(index);
	signalDispatcher.setParent(this);

	GameDevice& gamedevice = GameDevice::getInstance(index);
	WId wid = this->winId();
	gamedevice.setParentWidget(reinterpret_cast<HWND>(wid));

	connect(&signalDispatcher, &SignalDispatcher::saveHashSettings, this, &MainForm::onSaveHashSettings, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::loadHashSettings, this, &MainForm::onLoadHashSettings, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::messageBoxShow, this, &MainForm::onMessageBoxShow, Qt::BlockingQueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::inputBoxShow, this, &MainForm::onInputBoxShow, Qt::BlockingQueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::updateMainFormTitle, this, &MainForm::onUpdateMainFormTitle, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::appendScriptLog, this, &MainForm::onAppendScriptLog, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::appendChatLog, this, &MainForm::onAppendChatLog, Qt::QueuedConnection);

	QMenuBar* pMenuBar = q_check_ptr(new QMenuBar(this));
	sash_assume(pMenuBar != nullptr);
	if (pMenuBar != nullptr)
	{
		pMenuBar_ = pMenuBar;
		pMenuBar->setObjectName("menuBar");
		setMenuBar(pMenuBar);
	}

	createTrayIcon();

	ui.progressBar_pchp->setType(ProgressBar::kHP);
	ui.progressBar_pcmp->setType(ProgressBar::kMP);
	ui.progressBar_pethp->setType(ProgressBar::kHP);
	ui.progressBar_ridehp->setType(ProgressBar::kHP);

	connect(&signalDispatcher, &SignalDispatcher::updateCharHpProgressValue, ui.progressBar_pchp, &ProgressBar::onCurrentValueChanged);
	connect(&signalDispatcher, &SignalDispatcher::updateCharMpProgressValue, ui.progressBar_pcmp, &ProgressBar::onCurrentValueChanged);
	connect(&signalDispatcher, &SignalDispatcher::updatePetHpProgressValue, ui.progressBar_pethp, &ProgressBar::onCurrentValueChanged);
	connect(&signalDispatcher, &SignalDispatcher::updateRideHpProgressValue, ui.progressBar_ridehp, &ProgressBar::onCurrentValueChanged);

	connect(&signalDispatcher, &SignalDispatcher::updateStatusLabelTextChanged, this, &MainForm::onUpdateStatusLabelTextChanged);
	connect(&signalDispatcher, &SignalDispatcher::updateMapLabelTextChanged, this, &MainForm::onUpdateMapLabelTextChanged);
	connect(&signalDispatcher, &SignalDispatcher::updateCursorLabelTextChanged, this, &MainForm::onUpdateCursorLabelTextChanged);
	connect(&signalDispatcher, &SignalDispatcher::updateCoordsPosLabelTextChanged, this, &MainForm::onUpdateCoordsPosLabelTextChanged);
	connect(&signalDispatcher, &SignalDispatcher::updateLoginTimeLabelTextChanged, this, &MainForm::onUpdateTimeLabelTextChanged);
	connect(&signalDispatcher, &SignalDispatcher::updateCharInfoStone, this, &MainForm::onUpdateStonePosLabelTextChanged);

	ui.tabWidget_main->clear();

	ui.tabWidget_main->addTab(&pGeneralForm_, tr("general"));

#ifndef LEAK_TEST
	ui.tabWidget_main->addTab(&pMapForm_, tr("map"));

	ui.tabWidget_main->addTab(&pOtherForm_, tr("other"));

	ui.tabWidget_main->addTab(&pScriptForm_, tr("script"));
#endif

	util::setTab(ui.tabWidget_main);

	util::FormSettingManager formManager(this);
	formManager.loadSettings();

	trayIcon_.hide();

	onUpdateStatusLabelTextChanged(util::kLabelStatusNotOpen);

	onResetControlTextLanguage();

	//RPC& rpc = RPC::getInstance();
	//QSharedPointer<sol::state> device = rpc.getDevice(getIndex());
	//device->set_function("print", &MainForm::print, this);
}

std::string MainForm::print(std::string str)
{
	std::cout << str << std::endl;
	QString returnStr = QString("rt:%1").arg(util::toQString(str));
	return util::toConstData(returnStr);
}

MainForm::~MainForm()
{
	qDebug() << "MainForm::~MainForm()";
	MINT::NtTerminateProcess(GetCurrentProcess(), 0);
}

void MainForm::showEvent(QShowEvent* e)
{
	setUpdatesEnabled(true);
	blockSignals(false);

	setAttribute(Qt::WA_Mapped);
	QMainWindow::showEvent(e);
}

void MainForm::closeEvent(QCloseEvent* e)
{
	e->ignore();
	onSaveHashSettings(util::applicationDirPath() + "/settings/backup.json", true);

	util::FormSettingManager formManager(this);
	formManager.saveSettings();

	markAsClose_ = true;
	trayIcon_.hide();
	hide();

	//GameDevice::getInstance(getIndex()).close();
	GameDevice::getInstance(getIndex()).sendMessage(kUninitialize, NULL, NULL);
	UniqueIdManager::getInstance().deallocateUniqueId(getIndex());

	for (const auto& it : g_mainFormHash)
	{
		if (!it->markAsClose_)
		{
			setUpdatesEnabled(false);
			blockSignals(true);
			QMainWindow::closeEvent(e);
			return;
		}
	}

	MINT::NtTerminateProcess(GetCurrentProcess(), 0);
}

void MainForm::createTrayIcon()
{
	QMenu* trayMenu = nullptr;
	QAction* openAction = nullptr;
	QAction* closeAction = nullptr;
	do
	{
		trayIcon_.setParent(this);

		QIcon icon = QIcon(":/image/ico.png");
		trayIcon_.setIcon(icon);
		trayIcon_.setToolTip(windowTitle());

		trayMenu = q_check_ptr(new QMenu(this));
		sash_assume(trayMenu != nullptr);
		if (trayMenu == nullptr)
			break;

		openAction = q_check_ptr(new QAction(tr("open"), this));
		sash_assume(openAction != nullptr);
		if (openAction == nullptr)
			break;

		closeAction = q_check_ptr(new QAction(tr("close"), this));
		sash_assume(closeAction != nullptr);
		if (closeAction == nullptr)
			break;

		trayMenu->addAction(openAction);
		trayMenu->addAction(closeAction);
		connect(openAction, &QAction::triggered, this, &QMainWindow::show);

		connect(closeAction, &QAction::triggered, this, &QMainWindow::close);

		trayIcon_.setContextMenu(trayMenu);

		connect(&trayIcon_, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason)
			{
				if (reason == QSystemTrayIcon::DoubleClick)
				{
					hide();
					show();
					trayIcon_.hide();
					GameDevice& gamedevice = GameDevice::getInstance(getIndex());
					gamedevice.show();
				}
			});

		trayIcon_.show();
		return;
	} while (false);

	if (trayMenu != nullptr)
	{
		delete trayMenu;
		trayMenu = nullptr;
	}

	if (openAction != nullptr)
	{
		delete openAction;
		openAction = nullptr;
	}

	if (closeAction != nullptr)
	{
		delete closeAction;
		closeAction = nullptr;
	}
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

	long long currentIndex = getIndex();

	//system
	if (actionName == "actionHide")
	{
		if (!isVisible())
		{
			show();
		}
		else
		{
			trayIcon_.show();
			trayIcon_.setToolTip(windowTitle());
			//trayIcon_.showMessage(windowTitle(), tr("The program has been minimized to the system tray"), QSystemTrayIcon::Information, 5000);
			hide();
			GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
			gamedevice.hide();
		}
		return;
	}

	if (actionName == "actionHideGame")
	{
		GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
		if (gamedevice.worker.isNull())
			return;

		if (gamedevice.getEnableHash(util::kHideWindowEnable))
		{
			gamedevice.hide();
			gamedevice.setEnableHash(util::kHideWindowEnable, false);
			emit SignalDispatcher::getInstance(currentIndex).applyHashSettingsToUI();
			setFocus();
		}
		else
		{
			gamedevice.show();
			gamedevice.setEnableHash(util::kHideWindowEnable, true);
			emit SignalDispatcher::getInstance(currentIndex).applyHashSettingsToUI();
			setFocus();
		}
		return;
	}

	if (actionName == "actionHideBar")
	{
		util::Config config(QString("%1|%2").arg(__FUNCTION__).arg(__LINE__));
		config.write("MainFormClass", "Menu", "HideBar", pAction->isChecked());
		if (pAction->isChecked())
		{
			ui.progressBar_pchp->hide();
			ui.progressBar_pcmp->hide();
			ui.progressBar_pethp->hide();
			ui.progressBar_ridehp->hide();
		}
		else
		{
			ui.progressBar_pchp->show();
			ui.progressBar_pcmp->show();
			ui.progressBar_pethp->show();
			ui.progressBar_ridehp->show();
		}
		return;
	}

	if (actionName == "actionHideControl")
	{
		{
			util::Config config(QString("%1|%2").arg(__FUNCTION__).arg(__LINE__));
			config.write("MainFormClass", "Menu", "HideControl", pAction->isChecked());
		}

		if (pAction->isChecked())
		{
			ui.tabWidget_main->hide();
		}
		else
		{
			ui.tabWidget_main->show();
		}
		return;
	}

	if (actionName == "actionInfo")
	{
		QDesktopServices::openUrl(QUrl("https://gitee.com/Bestkakkoii/sash/wikis/pages"));
		return;
	}

	if (actionName == "actionWebsite")
	{
		CopyRightDialog* pCopyRightDialog = q_check_ptr(new CopyRightDialog(this));
		sash_assume(pCopyRightDialog != nullptr);
		if (pCopyRightDialog != nullptr)
		{
			pCopyRightDialog->exec();
		}
		return;
	}

	if (actionName == "acrionCreateNew")
	{
		return;
	}

	if (actionName == "actionClose")
	{
		close();
		return;
	}

	if (actionName == "actionCloseGame")
	{
		GameDevice::getInstance(currentIndex).close();
		return;
	}

	if (actionName == "actionCloseAll")
	{
		QProcess kill;
		qDebug() << util::applicationName();
		kill.start("taskkill", QStringList() << "/f" << "/im" << util::applicationName() + ".exe");
		kill.waitForFinished();
		MINT::NtTerminateProcess(GetCurrentProcess(), 0);
		return;
	}

	//other
	if (actionName == "actionOtherInfo")
	{
#ifndef LEAK_TEST
		if (pInfoForm_.isVisible())
		{
			pInfoForm_.hide();
			return;
		}

		pInfoForm_.hide();
		pInfoForm_.show();
		setFocus();
#endif
		return;
	}

	if (actionName == "actionAfkSetting")
	{
		if (pGeneralForm_.pAfkForm_.isVisible())
		{
			pGeneralForm_.pAfkForm_.hide();
			return;
		}

		pGeneralForm_.pAfkForm_.show();
		setFocus();
	}

	if (actionName == "actionMap")
	{
#ifndef LEAK_TEST
		if (mapWidget_.isVisible())
		{
			mapWidget_.hide();
			return;
		}

		mapWidget_.hide();
		mapWidget_.show();
		setFocus();
#endif
		return;
	}

	if (actionName == "actionScriptEditor")
	{
#ifndef LEAK_TEST
		if (pScriptEditor_.isVisible())
		{
			pScriptEditor_.hide();
			return;
		}
		pScriptEditor_.hide();
		pScriptEditor_.show();
		setFocus();
#endif
		return;
	}

	if (actionName == "actionSave")
	{
		QString fileName;
		GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
		if (!gamedevice.worker.isNull())
			fileName = gamedevice.worker->getCharacter().name;
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
		emit signalDispatcher.saveHashSettings(fileName);
		return;
	}

	if (actionName == "actionLoad")
	{
		QString fileName;
		GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
		if (!gamedevice.worker.isNull())
			fileName = gamedevice.worker->getCharacter().name;
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
		emit signalDispatcher.loadHashSettings(fileName);
		return;
	}

	if (actionName == "actionUpdate")
	{
		QString current;
		QString result;
		QString formatedDiff;
		long long ret;

		bool bret = Downloader::checkUpdate(&current, &result, &formatedDiff);
		QString detail = tr("Current version:%1\nNew version:%2").arg(current).arg(result);
		if (bret)
		{
			onMessageBoxShow(\
				tr("Update process will cause all the games to be closed, are you sure to continue?"), \
				2,
				formatedDiff, \
				& ret, \
				tr("New version were found"),
				detail);
		}
		else
		{
			onMessageBoxShow(\
				tr("Do you still want to update?"), \
				1,
				formatedDiff, \
				& ret, \
				tr("No new version available"),
				detail);
		}

		if (ret != QMessageBox::Yes)
			return;

		downloader_.start(Downloader::Source::SaSHServer);

		return;
	}
}

bool MainForm::onResetControlTextLanguage()
{
	this->ui.retranslateUi(this);

	long long currentIndex = getIndex();

#ifdef _WIN64
	setWindowTitle(QString("[%1]SaSH64").arg(currentIndex));
#else
	setWindowTitle(QString("[%1]SaSH").arg(currentIndex));
#endif
	trayIcon_.setToolTip(windowTitle());

	if (pMenuBar_ != nullptr)
	{
		createMenu(pMenuBar_);
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
	//ui.tabWidget_main->setTabText(4, "lua");

	ui.progressBar_pchp->onCurrentValueChanged(255, 9999, 9999);
	ui.progressBar_pcmp->onCurrentValueChanged(255, 9999, 9999);
	ui.progressBar_pethp->onCurrentValueChanged(255, 9999, 9999);
	ui.progressBar_ridehp->onCurrentValueChanged(255, 9999, 9999);

	return true;
}

void MainForm::onUpdateStatusLabelTextChanged(long long status)
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
		{ util::kLabelStatusGettingCharList, tr("getting player list") },
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
		{ util::kLabelStatusNoUsernameAndPassword, tr("no account and password")},
		{ util::kLabelStatusNoUsername, tr("no account")},
		{ util::kLabelStatusNoPassword, tr("no password")},
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

void MainForm::onUpdateTimeLabelTextChanged(const QString& text)
{
	ui.label_time->setText(text);
}

void MainForm::onUpdateStonePosLabelTextChanged(long long ntext)
{
	ui.label_stone->setText(util::toQString(ntext));
}

void MainForm::onUpdateMainFormTitle(const QString& text)
{
	long long currentIndex = getIndex();
#ifdef _WIN64
	setWindowTitle(QString("[%1]SaSH-%2").arg(currentIndex).arg(text).simplified().replace("SaSH-", "SaSHx64-"));
#else
	setWindowTitle(QString("[%1]SaSH-%2").arg(currentIndex).arg(text));
#endif
	trayIcon_.setToolTip(windowTitle());
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

		settingfiledialog dialog(fileName, this);

		if (dialog.exec() == QDialog::Accepted)
		{
			fileName = dialog.returnValue;
			if (fileName.isEmpty())
				return;
		}
		else
			return;
	}

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	QHash<util::UserSetting, bool> enableHash = gamedevice.getEnablesHash();
	QHash<util::UserSetting, long long> valueHash = gamedevice.getValuesHash();
	QHash<util::UserSetting, QString> stringHash = gamedevice.getStringsHash();
	QString key;
	util::UserSetting hkey = util::kSettingNotUsed;
	const QHash<util::UserSetting, QString> jsonKeyHash = util::user_setting_string_hash;

	util::Config config(fileName, QString("%1|%2").arg(__FUNCTION__).arg(__LINE__));
	for (auto iter = enableHash.constBegin(); iter != enableHash.constEnd(); ++iter)
	{
		hkey = iter.key();
		bool hvalue = iter.value();
		key = jsonKeyHash.value(hkey, "Invalid");
		if (key.endsWith("Enable"))
			config.write("User", "Enable", key, hvalue);
	}

	for (auto iter = valueHash.constBegin(); iter != valueHash.constEnd(); ++iter)
	{
		hkey = iter.key();
		long long hvalue = iter.value();
		key = jsonKeyHash.value(hkey, "Invalid");
		if (key.endsWith("Value"))
			config.write("User", "Value", key, hvalue);
	}

	for (auto iter = stringHash.begin(); iter != stringHash.end(); ++iter)
	{
		hkey = iter.key();
		QString hvalue = iter.value();
		key = jsonKeyHash.value(hkey, "Invalid");
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

		settingfiledialog dialog(fileName, this);
		if (dialog.exec() == QDialog::Accepted)
		{
			fileName = dialog.returnValue;
			if (fileName.isEmpty())
				return;
		}
		else
			return;
	}

	if (!QFile::exists(fileName))
		return;

	long long currentIndex = getIndex();

	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	QHash<util::UserSetting, bool> enableHash;
	QHash<util::UserSetting, long long> valueHash;
	QHash<util::UserSetting, QString> stringHash;
	QString key;
	util::UserSetting hkey = util::kSettingNotUsed;
	const QHash<util::UserSetting, QString> jsonKeyHash = util::user_setting_string_hash;

	{
		util::Config config(fileName, QString("%1|%2").arg(__FUNCTION__).arg(__LINE__));
		for (auto iter = jsonKeyHash.constBegin(); iter != jsonKeyHash.constEnd(); ++iter)
		{
			key = iter.value();
			if (!key.endsWith("Enable"))
				continue;
			bool value = config.read<bool>("User", "Enable", key);
			hkey = iter.key();
			enableHash.insert(hkey, value);
		}

		for (auto iter = jsonKeyHash.constBegin(); iter != jsonKeyHash.constEnd(); ++iter)
		{
			key = iter.value();
			if (!key.endsWith("Value"))
				continue;
			long long value = config.read<long long>("User", "Value", key);
			hkey = iter.key();
			valueHash.insert(hkey, value);
		}

		for (auto iter = jsonKeyHash.constBegin(); iter != jsonKeyHash.constEnd(); ++iter)
		{
			key = iter.value();
			if (!key.endsWith("String"))
				continue;
			QString value = config.read<QString>("User", "String", key);
			hkey = iter.key();
			stringHash.insert(hkey, value);
		}
	}

	gamedevice.setEnablesHash(enableHash);
	gamedevice.setValuesHash(valueHash);
	gamedevice.setStringsHash(stringHash);

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	emit signalDispatcher.applyHashSettingsToUI();
}

//消息框
void MainForm::onMessageBoxShow(const QString& text, long long type, QString title, long long* pnret, QString topText, QString detail)
{
	QString newText = text;
	newText.replace("\\r\\n", "\r\n");
	newText.replace("\\n", "\n");
	newText.replace("\\t", "\t");
	newText.replace("\\v", "\v");
	newText.replace("\\b", "\b");
	newText.replace("\\f", "\f");
	newText.replace("\\a", "\a");

	if (title.isEmpty())
	{
		if (type == 2)
			title = tr("warning");
		else if (type == 3)
			title = tr("critical");
		else
			title = tr("info");
	}

	QMessageBox::Icon icon = QMessageBox::Icon::NoIcon;

	if (type == 2)
		icon = QMessageBox::Icon::Warning;
	else if (type == 3)
		icon = QMessageBox::Icon::Critical;
	else
		icon = QMessageBox::Icon::Information;

	QMessageBox msgBox(this);
	msgBox.setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint);
	msgBox.setModal(true);
	msgBox.setAttribute(Qt::WA_QuitOnClose);
	msgBox.setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
	msgBox.setIcon(icon);
	msgBox.setTextFormat(Qt::TextFormat::RichText);
	msgBox.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard | Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard);
	msgBox.setWindowModality(Qt::ApplicationModal);
	msgBox.setMinimumWidth(300);

	msgBox.setWindowTitle(title);
	if (!detail.isEmpty())
		msgBox.setDetailedText(detail);

	if (topText.isEmpty())
		msgBox.setText(text);
	else
	{
		msgBox.setInformativeText(text);
		msgBox.setText(topText);
	}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	msgBox.setDefaultButton(QMessageBox::StandardButton::Yes);
	msgBox.setEscapeButton(QMessageBox::StandardButton::No);
	msgBox.setStandardButtons(QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No);
	msgBox.setButtonText(QMessageBox::Yes, tr("yes"));
	msgBox.setButtonText(QMessageBox::No, tr("no"));
#else
	msgBox.addButton(tr("yes"), QMessageBox::ButtonRole::YesRole);
	msgBox.addButton(tr("no"), QMessageBox::ButtonRole::NoRole);
#endif

	long long ret = msgBox.exec();
	if (pnret != nullptr)
		*pnret = ret;

	emit messageBoxFinished();
}

//输入框
void MainForm::onInputBoxShow(const QString& text, long long type, QVariant* retvalue)
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

	inputDialog.setAttribute(Qt::WA_QuitOnClose);
	inputDialog.setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
	inputDialog.setModal(true);
	inputDialog.setMinimumWidth(300);
	inputDialog.setLabelText(newText);
	QInputDialog::InputMode mode = static_cast<QInputDialog::InputMode>(type);
	inputDialog.setInputMode(mode);
	if (mode == QInputDialog::IntInput)
	{
		inputDialog.setIntMinimum(std::numeric_limits<int>::min());
		inputDialog.setIntMaximum(std::numeric_limits<int>::max());
		if (retvalue->isValid())
			inputDialog.setIntValue(retvalue->toLongLong());
	}
	else if (mode == QInputDialog::DoubleInput)
	{
		inputDialog.setDoubleMinimum(std::numeric_limits<double>::min());
		inputDialog.setDoubleMaximum(std::numeric_limits<double>::max());
		if (retvalue->isValid())
			inputDialog.setDoubleValue(retvalue->toDouble());
	}
	else
	{
		if (retvalue->isValid())
			inputDialog.setTextValue(retvalue->toString());
	}

	inputDialog.setWindowFlags(inputDialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);
	int ret = inputDialog.exec();
	if (ret != QDialog::Accepted)
	{
		emit inputBoxFinished();
		return;
	}

	switch (type)
	{
	case QInputDialog::IntInput:
		*retvalue = static_cast<long long>(inputDialog.intValue());
		break;
	case QInputDialog::DoubleInput:
		*retvalue = static_cast<long long>(inputDialog.doubleValue());
		break;
	case QInputDialog::TextInput:
		*retvalue = inputDialog.textValue();
		break;
	}

	emit inputBoxFinished();
}

//腳本日誌
void MainForm::onAppendScriptLog(const QString& text, long long color)
{
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	QString newText = text;
	newText.replace("\\r\\n", "\n");
	newText.replace("\\n", "\n");
	newText.replace("\\t", "\t");
	newText.replace("\\v", "\v");
	newText.replace("\\b", "\b");
	newText.replace("\\f", "\f");
	newText.replace("\\a", "\a");

	QStringList list = text.split("\n", Qt::SkipEmptyParts);
	for (const auto& it : list)
	{
		gamedevice.scriptLogModel.append(it, color);
	}
	emit gamedevice.scriptLogModel.dataAppended();
}

//對話日誌
void MainForm::onAppendChatLog(const QString& text, long long color)
{
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	gamedevice.chatLogModel.append(text, color);
	emit gamedevice.chatLogModel.dataAppended();
}

#if 0
bool MainForm::createWinapiFileDialog(const QString& startDir, QStringList filters, QString* selectedFilePath)
{
	// 創建文件打開對話框結構體
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = nullptr;
	ofn.lpstrFile = nullptr;
	ofn.lpstrFileTitle = nullptr;
	ofn.lpstrInitialDir = reinterpret_cast<const wchar_t*>(startDir.utf16());
	WCHAR wfilter[1024] = {};
	memset(wfilter, 0, sizeof(wfilter));
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

	// 構建過濾器字符串
	QString filterStr = "All Files\0*.*\0\0";
	if (!filters.isEmpty())
	{
		for (QString& it : filters)
		{
			QFileInfo fi(it);
			if (fi.suffix().isEmpty())
			{
				//"doc\0*.doc\0xls\0*.xls\0ppt\0*.ppt\0all\0*.*\0\0"
				it = QString("*.%1|*.%1").arg(fi.suffix());
			}
			else
			{
				//"doc\0*.doc\0xls\0*.xls\0ppt\0*.ppt\0all\0*.*\0\0"
				it = QString("%1|%1").arg(it);
			}
		}

		filterStr = filters.join("|");
		filterStr.append("||");
	}

	std::wstring wstr = filterStr.toStdWString();

	_snwprintf_s(wfilter, _countof(wfilter), _TRUNCATE, L"%s", wstr.c_str());
	//replace all | to \0
	for (size_t i = 0; i < _countof(wfilter); ++i)
	{
		if (wfilter[i] == L'|')
			wfilter[i] = L'\0';
	}

	ofn.lpstrFilter = wfilter;

	// 分配緩沖區來保存選定的文件路徑
	wchar_t filePath[MAX_PATH] = L"\0";
	ofn.lpstrFile = filePath;

	// 打開文件對話框
	if (GetOpenFileName(&ofn))
	{
		*selectedFilePath = QString::fromWCharArray(filePath);
		return true; // 用戶選擇了文件
	}
	else
	{
		return false; // 用戶取消了操作或發生錯誤
	}
}
#endif