#ifndef WEBSOCKETCLIENT_H_
#define WEBSOCKETCLIENT_H_

#include <windows.h>

#include <QObject>
#include <QtWebSockets/QWebSocket>
#include <QByteArray>
#include <QScopedPointer>
#include <QUrl>

namespace web
{
	class SocketClientPrivate;

	class SocketClient : public QObject
	{
		Q_OBJECT;

	public:
		static SocketClient& instance(QObject* parent = nullptr);

	public:
		explicit SocketClient(QObject* parent = nullptr);
		virtual ~SocketClient();

		void connectToServer(const QUrl& url);
		void sendBinaryMessage(const QByteArray& message);
		void sendTextMessage(const QString& message);
		void closeConnection();

		void setWindowId(HWND windowId);

	signals:
		void connected();
		void disconnected();
		void messageReceived(const QByteArray& message);
		void errorOccurred(const QString& errorString);

	private slots:
		void handleConnected();
		void handleTextMessage(const QString& message);
		void handleBinaryMessage(const QByteArray& message);
		void handleSocketError(QAbstractSocket::SocketError error);

	private:
		Q_DECLARE_PRIVATE(SocketClient);
		Q_DISABLE_COPY_MOVE(SocketClient);
		QScopedPointer<SocketClientPrivate> d_ptr;
	};
}

#endif // WEBSOCKETCLIENT_H_
