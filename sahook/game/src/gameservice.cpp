#include "stdafx.h"
#include "sahook/game/include/gameservice_p.h"

#pragma region GameServicePrivate

game::ServicePrivate::ServicePrivate(Service* q)
	: Implement(GetModuleHandleW(nullptr))
	, q_ptr(q)
	//, d(processBase_)

{


}

game::ServicePrivate::~ServicePrivate()
{
}

bool game::ServicePrivate::compareLocation(qint64 x, qint64 y, qint64 direction) const
{
	if (x != xCache_ || y != yCache_ || direction != directionCache_)
	{
		return false;
	}

	return true;
}

#pragma endregion

game::Service::Service(QObject* parent)
	: QObject(parent)
	, d_ptr(new game::ServicePrivate(this))
{

}

game::Service::~Service()
{
}

game::Service& game::Service::instance()
{
	static Service instance;
	return instance;
}

void game::Service::setMainWindow(HWND mainWindow)
{
	Q_D(Service);
	if (nullptr == mainWindow || mainWindow == d->g_MainHwnd)
	{
		return;
	}

	d->g_MainHwnd = mainWindow;
}

void game::Service::setWindowProcEnable(bool enable)
{
	Q_D(Service);
	if (enable)
	{
		if (nullptr == d->g_OldWndProc)
		{
			d->g_OldWndProc = reinterpret_cast<WNDPROC>(SetWindowLongW(d->g_MainHwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(game::Service::WndProc)));
		}
	}
	else
	{
		if (nullptr != d->g_OldWndProc)
		{
			SetWindowLongW(d->g_MainHwnd, GWLP_WNDPROC, (LONG_PTR)d->g_OldWndProc);
			d->g_OldWndProc = nullptr;
		}
	}
}

BOOL game::Service::postMessage(UINT Msg, WPARAM wParam, LPARAM lParam, HWND hWnd) const
{
	Q_D(const Service);
	if (hWnd == nullptr)
	{
		hWnd = d->g_MainHwnd;
	}

	BOOL ret = PostMessageW(hWnd, Msg, wParam, lParam);
	if (!ret)
	{
		qDebug() << L"PostMessageW failed: " << GetLastError();
		qDebug() << L"HWND: " << (int)d->g_MainHwnd << L", Msg: " << Msg << L", wParam: " << wParam << L", lParam:" << lParam;
	}
	return ret;
}

BOOL game::Service::sendMessage(UINT Msg, WPARAM wParam, LPARAM lParam, HWND hWnd) const
{
	Q_D(const Service);
	if (hWnd == nullptr)
	{
		hWnd = d->g_MainHwnd;
	}

	BOOL ret = SendMessageTimeoutW(hWnd, Msg, wParam, lParam, SMTO_NORMAL | SMTO_ERRORONEXIT, 1000, nullptr);
	if (!ret)
	{
		qDebug() << L"SendMessageW failed: " << GetLastError();
		qDebug() << L"HWND: " << (int)d->g_MainHwnd << L", Msg: " << Msg << L", wParam: " << wParam << L", lParam:" << lParam;
	}
	return ret;
}

void game::Service::handleServerConnected()
{
	qDebug() << __FUNCTION__;
}

void game::Service::handleServerMessage(const QByteArray& binaryMessage)
{
	qDebug() << __FUNCTION__;

	QJsonParseError jsonParseError;

	QJsonDocument jsonDocument = QJsonDocument::fromJson(binaryMessage, &jsonParseError);
	if (jsonParseError.error != QJsonParseError::NoError)
	{
		qDebug().noquote() << "Client Json Error:" << jsonParseError.errorString() << "\nOriginal:" << QString::fromUtf8(binaryMessage);
		return;
	}

	tool::JsonReply reply(jsonDocument);

	QString action = reply.action();

	QString message = reply.message();

	QString command = reply.command();

	qDebug() << "Action:" << action << "Command:" << command << "Message:" << message;

	Q_D(Service);

	if ("initialize" == command && "hello client" == message)
	{
		qDebug().noquote() << __FUNCTION__ << __LINE__ << "initialize";
		setWindowProcEnable(true);

		WPARAM wParam = reply.parameter("index").toInt();
		LPARAM lParam = reply.parameter("parentWId").toInt();

		sendMessage(kInitialize, wParam, lParam);

		reply.setMessage("hello server");
		emit sendWebSocketBinaryMessage(reply.toUtf8());
		return;
	}

	if ("uninitialize" == command && "goodbye client" == message)
	{
		qDebug().noquote() << __FUNCTION__ << __LINE__ << "uninitialize";
		sendMessage(kUninitialize, NULL, NULL);
		setWindowProcEnable(false);
		return;
	}

	if ("closeDisconnectDialog" == action)
	{
		//sendMessage(WM_CLOSE_DISCONNECT_DIALOG, NULL, NULL);
		return;
	}

	if ("closeAllDialog" == action)
	{
		//sendMessage(WM_CLOSE_ALL_DIALOG, NULL, NULL);
		return;
	}

	if ("getData" == command)
	{
		return;
	}

	if ("send" == action)
	{
		std::vector<std::variant<long long, std::string>> args;

		QString message = reply.message();
		QByteArray ba = message.toUtf8();
		//sendMessage(WM_SEND_PACKET, reinterpret_cast<WPARAM>(ba.data()), NULL);
		return;
	}

	if ("announce" == action)
	{
		QString message = reply.message();
		int color = reply.data("color").toInt(4);
		int size = reply.data("size").toInt(3);

		QByteArray ba = message.toUtf8();
		//sendMessage(WM_ANNOUNCE, reinterpret_cast<WPARAM>(ba.data()), (color << 0) | (size << 8));
		return;
	}

	if ("move" == action)
	{
		//sendMessage(WM_MOVE_CHARACTER, reply.data("x").toInt(0), reply.data("y").toInt(0));
		return;
	}

	if ("selectServer" == action)
	{
		bool enable = reply.data("enable").toBool(false);
		int server = reply.data("server").toInt(0);
		//sendMessage(WM_SET_SELECT_SERVER_LOOP, enable ? 1 : 0, server);
		return;
	}

	if ("selectSubServer" == action)
	{
		bool enable = reply.data("enable").toBool(false);
		int server = reply.data("server").toInt(0);
		int subserver = reply.data("subServer").toInt(0);
		//sendMessage(WM_SET_SELECT_SUBSERVER_LOOP, enable ? 1 : 0, (server << 0) | (subserver << 8));
		return;
	}

	if ("selectCharacter" == action)
	{
		bool enable = reply.data("enable").toBool(false);
		int pos = reply.data("position").toInt(0);
		//sendMessage(WM_SET_SELECT_CHARACTER_LOOP, enable ? 1 : 0, pos);
		return;
	}

	if ("request" == action)
	{
		QString field = reply.field();

		QMetaEnum metaEnum = QMetaEnum::fromType<User::Settings>();

		User::Settings key = static_cast<User::Settings>(metaEnum.keyToValue(field.toUtf8()));

		QJsonValue value = reply.parameter("value");

		switch (key)
		{
		case User::Settings::autoBattleEnable:
		{
			bool enable = value.toBool(false);
			//d->battleAIEnable_ = enable;
			break;
		}
		case User::Settings::rewardDialogEnable:
		{
			bool enable = value.toBool(false);
			//d->rewardDialogEnable_ = enable;
			break;
		}
		case User::Settings::dialogEnable:
		{
			bool enable = value.toBool(false);
			//d->dialogEnable_ = enable;
			break;
		}
		case User::Settings::warpAnimeEnable:
		{
			bool enable = value.toBool(false);
			//d->warpAnimeEnable_ = enable;
			if (enable)
			{
				//d->dialogAnimation_.restore();
			}
			else
			{
				//d->dialogAnimation_.rewrite();
			}
			break;
		}
		case User::Settings::walkSpeedValue:
		{
			qint64 speed = value.toInt(100);
			//d->walkSpeed_ = speed;
			break;
		}
		case User::Settings::nicMacAddressString:
		{
			//d->currentMAC_ = value.toString();
			break;
		}
		}

		return;
	}
}

int game::Service::getWorldStatue() const
{
	Q_D(const Service);
	/*lock read*/
	std::shared_lock<std::shared_mutex> lock(d->g_statusLock);
	return *d->g_world_status;
}
int game::Service::getGameStatue() const
{
	Q_D(const Service);
	/*lock read*/
	std::shared_lock<std::shared_mutex> lock(d->g_statusLock);
	return *d->g_game_status;
}