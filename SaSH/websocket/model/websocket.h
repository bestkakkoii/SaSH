#ifndef WEBSOCKET_H_
#define WEBSOCKET_H_

#include <QtWebSockets/QWebSocket>
#include <QVariant>
#include <QtWebSockets/QWebSocketProtocol>
#include <QtWebSockets/QWebSocketHandshakeOptions>
#include <QSslPreSharedKeyAuthenticator>

#include <timer.h>

namespace web
{
	class SocketPrivate;

	class Socket : public QObject
	{
		Q_OBJECT
	public:
		explicit Socket(QWebSocket* original);
		virtual ~Socket();

	public:
		QHostAddress peerAddress() const;
		QString peerName() const;
		quint16 peerPort() const;

		qint64 sendBinaryMessage(const QByteArray& data);
		qint64 sendTextMessage(const QString& message);

		QString errorString() const;

	private: // hide
		QHostAddress localAddress() const;
		quint16 localPort() const;
		void abort();
		QAbstractSocket::SocketError error() const;

		bool flush();
		bool isValid() const;
		const QMaskGenerator* maskGenerator() const;
		qint64 bytesToWrite() const;
		QWebSocketProtocol::CloseCode closeCode() const;
		QString closeReason() const;
		void continueInterruptedHandshake();
		QWebSocketHandshakeOptions handshakeOptions() const;
		void ignoreSslErrors(const QList<QSslError>& errors);
		quint64 maxAllowedIncomingFrameSize() const;
		quint64 maxAllowedIncomingMessageSize() const;
		QString origin() const;
		quint64 outgoingFrameSize() const;
		QAbstractSocket::PauseModes pauseMode() const;
		QNetworkProxy proxy() const;
		qint64 readBufferSize() const;
		QNetworkRequest request() const;
		QUrl requestUrl() const;
		QString resourceName() const;
		void resume();

		void setMaskGenerator(const QMaskGenerator* maskGenerator);
		void setMaxAllowedIncomingFrameSize(quint64 maxAllowedIncomingFrameSize);
		void setMaxAllowedIncomingMessageSize(quint64 maxAllowedIncomingMessageSize);
		void setOutgoingFrameSize(quint64 outgoingFrameSize);
		void setPauseMode(QAbstractSocket::PauseModes pauseMode);
		void setProxy(const QNetworkProxy& networkProxy);
		void setReadBufferSize(qint64 size);
		void setSslConfiguration(const QSslConfiguration& sslConfiguration);
		QSslConfiguration sslConfiguration() const;
		QAbstractSocket::SocketState state() const;
		QString subprotocol() const;
		QWebSocketProtocol::Version version() const;

	signals:
		void aboutToClose();
		void alertReceived(QSsl::AlertLevel level, QSsl::AlertType type, const QString& description);
		void alertSent(QSsl::AlertLevel level, QSsl::AlertType type, const QString& description);
		void authenticationRequired(QAuthenticator* authenticator);
		void binaryFrameReceived(const QByteArray& frame, bool isLastFrame);
		void binaryMessageReceived(const QByteArray& message);
		void bytesWritten(qint64 bytes);
		void connected();
		void disconnected();
		void errorOccurred(QAbstractSocket::SocketError error);
		void handshakeInterruptedOnError(const QSslError& error);
		void peerVerifyError(const QSslError& error);
		void pong(quint64 elapsedTime, const QByteArray& payload);
		void preSharedKeyAuthenticationRequired(QSslPreSharedKeyAuthenticator* authenticator);
		void proxyAuthenticationRequired(const QNetworkProxy& proxy, QAuthenticator* authenticator);
		void readChannelFinished();
		void sslErrors(const QList<QSslError>& errors);
		void stateChanged(QAbstractSocket::SocketState state);
		void textFrameReceived(const QString& frame, bool isLastFrame);
		void textMessageReceived(const QString& message);

	public slots:
		void close(QWebSocketProtocol::CloseCode closeCode = QWebSocketProtocol::CloseCodeNormal, const QString& reason = QString());
		void ignoreSslErrors();
		void open(const QUrl& url);
		void open(const QNetworkRequest& request);
		void open(const QUrl& url, const QWebSocketHandshakeOptions& options);
		void open(const QNetworkRequest& request, const QWebSocketHandshakeOptions& options);
		void ping(const QByteArray& payload = QByteArray());

	public:
		void setTypeString(const QString& typeString);
		QByteArray typeString() const;

		void setProcessId(qint64 processId);
		qint64 processId() const;

		void insertUserData(const QString& key, const QVariant& userData);
		QVariant userData(const QString& key) const;

		qint64 elapsed() const;

		void restartTimer();

	private:
		QWebSocket* socket_ = nullptr;
	private:
		QScopedPointer<SocketPrivate> d_ptr;
		Q_DECLARE_PRIVATE(Socket);

	};
}

#endif // WEBSOCKET_H_