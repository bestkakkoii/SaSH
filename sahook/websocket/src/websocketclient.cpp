#include "stdafx.h"
#include "sahook/websocket/include/websocketclient_p.h"

web::SocketClient::SocketClient(QObject* parent)
	: QObject(parent)
	, d_ptr(new SocketClientPrivate(this))
{
	Q_D(SocketClient);

	connect(&d->webSocket, &QWebSocket::connected, this, &SocketClient::handleConnected, Qt::QueuedConnection);
	connect(&d->webSocket, &QWebSocket::disconnected, this, &SocketClient::disconnected, Qt::QueuedConnection);
	connect(&d->webSocket, &QWebSocket::textMessageReceived, this, &SocketClient::handleTextMessage, Qt::QueuedConnection);
	connect(&d->webSocket, &QWebSocket::binaryMessageReceived, this, &SocketClient::handleBinaryMessage, Qt::QueuedConnection);
	connect(&d->webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this, &SocketClient::handleSocketError, Qt::QueuedConnection);
}

web::SocketClient::~SocketClient() = default;

web::SocketClient& web::SocketClient::instance(QObject* parent)
{
	static SocketClient instance(parent);
	return instance;
}

void web::SocketClient::connectToServer(const QUrl& url)
{
	Q_D(SocketClient);
	d->webSocket.open(url);
}

void web::SocketClient::sendBinaryMessage(const QByteArray& message)
{
	Q_D(SocketClient);
	d->webSocket.sendBinaryMessage(message);
}

void web::SocketClient::sendTextMessage(const QString& message)
{
	Q_D(SocketClient);
	d->webSocket.sendTextMessage(message);
}

void web::SocketClient::closeConnection()
{
	Q_D(SocketClient);
	d->webSocket.close();
}

void web::SocketClient::setWindowId(HWND windowId)
{
	Q_D(SocketClient);
	d->windowId = windowId;
}

/**
 * 處理收到的消息
 *
 * 生成的 JSON 格式文本參考:
 * {
 *     "action": "executeOperation",
 *     "data": {
 *		   "command": "hello server",
 *     },
 *     "conversationId": "conv123"
 * }
 */
void web::SocketClient::handleConnected()
{
	emit connected();
}

void web::SocketClient::handleTextMessage(const QString&)
{

}

void web::SocketClient::handleBinaryMessage(const QByteArray& message)
{
	Q_D(SocketClient);

	emit messageReceived(message);
}

void web::SocketClient::handleSocketError(QAbstractSocket::SocketError)
{
	Q_D(SocketClient);
	emit errorOccurred(d->webSocket.errorString());
}