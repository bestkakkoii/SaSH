#ifndef WEBSOCKETCLIENT_P_H_
#define WEBSOCKETCLIENT_P_H_

#include <windows.h>

#include "sahook/websocket/include/websocketclient.h"

#include <QObject>
#include <QtWebSockets/QWebSocket>
#include <QByteArray>
#include <QScopedPointer>
#include <QUrl>

namespace web
{
	class SocketClientPrivate
	{
	public:
		explicit SocketClientPrivate(SocketClient* q)
			: q_ptr(q)
		{
			qRegisterMetaType<QAbstractSocket::SocketState>("QAbstractSocket::SocketState");
			webSocket.setProxy(QNetworkProxy::NoProxy);
		}

	public:
		QWebSocket webSocket;
		HWND windowId = nullptr;
		bool varified = false;

	private:
		Q_DECLARE_PUBLIC(SocketClient);
		Q_DISABLE_COPY_MOVE(SocketClientPrivate);
		SocketClient* q_ptr = nullptr;
	};


}

#endif // WEBSOCKETCLIENT_P_H_
