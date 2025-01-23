#include "stdafx.h"
#include "SaSH/websocket/model/websocket_p.h"

web::SocketPrivate::SocketPrivate(Socket* original)
	: q_ptr(original)
{
}

web::SocketPrivate::~SocketPrivate()
{
}

web::Socket::Socket(QWebSocket* original)
	: d_ptr(new SocketPrivate(this))
	, socket_(original)
{
	qRegisterMetaType<QAbstractSocket::SocketState>("QAbstractSocket::SocketState");

	connect(socket_, &QWebSocket::aboutToClose, this, [this]() { emit aboutToClose(); });
	connect(socket_, &QWebSocket::alertReceived, this, [this](QSsl::AlertLevel level, QSsl::AlertType type, const QString& description) { emit alertReceived(level, type, description); }, Qt::QueuedConnection);
	connect(socket_, &QWebSocket::alertSent, this, [this](QSsl::AlertLevel level, QSsl::AlertType type, const QString& description) { emit alertSent(level, type, description); }, Qt::QueuedConnection);
	connect(socket_, &QWebSocket::authenticationRequired, this, [this](QAuthenticator* authenticator) { emit authenticationRequired(authenticator); }, Qt::QueuedConnection);
	connect(socket_, &QWebSocket::binaryFrameReceived, this, [this](const QByteArray& frame, bool isLastFrame) { emit binaryFrameReceived(frame, isLastFrame); }, Qt::QueuedConnection);
	connect(socket_, &QWebSocket::binaryMessageReceived, this, [this](const QByteArray& message) { emit binaryMessageReceived(message); }, Qt::QueuedConnection);
	connect(socket_, &QWebSocket::bytesWritten, this, [this](qint64 bytes) { emit bytesWritten(bytes); }, Qt::QueuedConnection);
	connect(socket_, &QWebSocket::connected, this, [this]() { emit connected(); }, Qt::QueuedConnection);
	connect(socket_, &QWebSocket::disconnected, this, [this]() { emit disconnected(); }, Qt::QueuedConnection);
	connect(socket_, &QWebSocket::errorOccurred, this, [this](QAbstractSocket::SocketError error) { emit errorOccurred(error); }, Qt::QueuedConnection);
	connect(socket_, &QWebSocket::handshakeInterruptedOnError, this, [this](const QSslError& error) { emit handshakeInterruptedOnError(error); }, Qt::QueuedConnection);
	connect(socket_, &QWebSocket::peerVerifyError, this, [this](const QSslError& error) { emit peerVerifyError(error); }, Qt::QueuedConnection);
	connect(socket_, &QWebSocket::pong, this, [this](quint64 elapsedTime, const QByteArray& payload) { emit pong(elapsedTime, payload); }, Qt::QueuedConnection);
	connect(socket_, &QWebSocket::preSharedKeyAuthenticationRequired, this, [this](QSslPreSharedKeyAuthenticator* authenticator) { emit preSharedKeyAuthenticationRequired(authenticator); }, Qt::QueuedConnection);
	connect(socket_, &QWebSocket::proxyAuthenticationRequired, this, [this](const QNetworkProxy& proxy, QAuthenticator* authenticator) { emit proxyAuthenticationRequired(proxy, authenticator); }, Qt::QueuedConnection);
	connect(socket_, &QWebSocket::readChannelFinished, this, [this]() { emit readChannelFinished(); }, Qt::QueuedConnection);
	connect(socket_, &QWebSocket::sslErrors, this, [this](const QList<QSslError>& errors) { emit sslErrors(errors); }, Qt::QueuedConnection);
	connect(socket_, &QWebSocket::stateChanged, this, [this](QAbstractSocket::SocketState state) { emit stateChanged(state); }, Qt::QueuedConnection);
	connect(socket_, &QWebSocket::textFrameReceived, this, [this](const QString& frame, bool isLastFrame) { emit textFrameReceived(frame, isLastFrame); }, Qt::QueuedConnection);
	connect(socket_, &QWebSocket::textMessageReceived, this, [this](const QString& message) { emit textMessageReceived(message); }, Qt::QueuedConnection);

	// 設置代理
	socket_->setProxy(QNetworkProxy::NoProxy);

	// 設置最大允許的傳入幀大小
	socket_->setMaxAllowedIncomingFrameSize(10LL * 1024 * 1024);

	// 設置最大允許的傳入消息大小
	socket_->setMaxAllowedIncomingMessageSize(20LL * 1024 * 1024);

	// 設置傳出幀大小
	socket_->setOutgoingFrameSize(10LL * 1024 * 1024);

	// 設置讀取緩衝區大小
	socket_->setReadBufferSize(20LL * 1024 * 1024);

	// 設置 SSL 配置
	QSslConfiguration sslConfig = socket_->sslConfiguration();
	sslConfig.setPeerVerifyMode(QSslSocket::VerifyPeer);
	sslConfig.setProtocol(QSsl::TlsV1_3OrLater);
	socket_->setSslConfiguration(sslConfig);

	// 設置暫停模式
	socket_->setPauseMode(QAbstractSocket::PauseOnSslErrors);

	socket_->ping("{}");
}

web::Socket::~Socket()
{
	socket_->deleteLater();
}

void web::Socket::abort()
{
	socket_->abort();
}

qint64 web::Socket::bytesToWrite() const
{
	return socket_->bytesToWrite();
}

QWebSocketProtocol::CloseCode web::Socket::closeCode() const
{
	return socket_->closeCode();
}

QString web::Socket::closeReason() const
{
	return socket_->closeReason();
}

void web::Socket::continueInterruptedHandshake()
{
	socket_->continueInterruptedHandshake();
}

QAbstractSocket::SocketError web::Socket::error() const
{
	return socket_->error();
}

QString web::Socket::errorString() const
{
	return socket_->errorString();
}

bool web::Socket::flush()
{
	return socket_->flush();
}

QWebSocketHandshakeOptions web::Socket::handshakeOptions() const
{
	return socket_->handshakeOptions();
}

void web::Socket::ignoreSslErrors(const QList<QSslError>& errors)
{
	socket_->ignoreSslErrors(errors);
}

bool web::Socket::isValid() const
{
	return socket_->isValid();
}

QHostAddress web::Socket::localAddress() const
{
	return socket_->localAddress();
}

quint16 web::Socket::localPort() const
{
	return socket_->localPort();
}

const QMaskGenerator* web::Socket::maskGenerator() const
{
	return socket_->maskGenerator();
}

quint64 web::Socket::maxAllowedIncomingFrameSize() const
{
	return socket_->maxAllowedIncomingFrameSize();
}

quint64 web::Socket::maxAllowedIncomingMessageSize() const
{
	return socket_->maxAllowedIncomingMessageSize();
}

QString web::Socket::origin() const
{
	return socket_->origin();
}

quint64 web::Socket::outgoingFrameSize() const
{
	return socket_->outgoingFrameSize();
}

QAbstractSocket::PauseModes web::Socket::pauseMode() const
{
	return socket_->pauseMode();
}

QHostAddress web::Socket::peerAddress() const
{
	return socket_->peerAddress();
}

QString web::Socket::peerName() const
{
	return socket_->peerName();
}

quint16 web::Socket::peerPort() const
{
	return socket_->peerPort();
}

QNetworkProxy web::Socket::proxy() const
{
	return socket_->proxy();
}

qint64 web::Socket::readBufferSize() const
{
	return socket_->readBufferSize();
}

QNetworkRequest web::Socket::request() const
{
	return socket_->request();
}

QUrl web::Socket::requestUrl() const
{
	return socket_->requestUrl();
}

QString web::Socket::resourceName() const
{
	return socket_->resourceName();
}

void web::Socket::resume()
{
	socket_->resume();
}

qint64 web::Socket::sendBinaryMessage(const QByteArray& data)
{
	return socket_->sendBinaryMessage(data);
}

qint64 web::Socket::sendTextMessage(const QString& message)
{
	return socket_->sendTextMessage(message);
}

void web::Socket::setMaskGenerator(const QMaskGenerator* maskGenerator)
{
	socket_->setMaskGenerator(maskGenerator);
}

void web::Socket::setMaxAllowedIncomingFrameSize(quint64 maxAllowedIncomingFrameSize)
{
	socket_->setMaxAllowedIncomingFrameSize(maxAllowedIncomingFrameSize);
}

void web::Socket::setMaxAllowedIncomingMessageSize(quint64 maxAllowedIncomingMessageSize)
{
	socket_->setMaxAllowedIncomingMessageSize(maxAllowedIncomingMessageSize);
}

void web::Socket::setOutgoingFrameSize(quint64 outgoingFrameSize)
{
	socket_->setOutgoingFrameSize(outgoingFrameSize);
}

void web::Socket::setPauseMode(QAbstractSocket::PauseModes pauseMode)
{
	socket_->setPauseMode(pauseMode);
}

void web::Socket::setProxy(const QNetworkProxy& networkProxy)
{
	socket_->setProxy(networkProxy);
}

void web::Socket::setReadBufferSize(qint64 size)
{
	socket_->setReadBufferSize(size);
}

void web::Socket::setSslConfiguration(const QSslConfiguration& sslConfiguration)
{
	socket_->setSslConfiguration(sslConfiguration);
}

QSslConfiguration web::Socket::sslConfiguration() const
{
	return socket_->sslConfiguration();
}

QAbstractSocket::SocketState web::Socket::state() const
{
	return socket_->state();
}

QString web::Socket::subprotocol() const
{
	return socket_->subprotocol();
}

QWebSocketProtocol::Version web::Socket::version() const
{
	return socket_->version();
}

void web::Socket::close(QWebSocketProtocol::CloseCode closeCode, const QString& reason)
{
	socket_->close(closeCode, reason);
}

void web::Socket::ignoreSslErrors()
{
	socket_->ignoreSslErrors();
}

void web::Socket::open(const QUrl& url)
{
	socket_->open(url);
}

void web::Socket::open(const QNetworkRequest& request)
{
	socket_->open(request);
}

void web::Socket::open(const QUrl& url, const QWebSocketHandshakeOptions& options)
{
	socket_->open(url, options);
}

void web::Socket::open(const QNetworkRequest& request, const QWebSocketHandshakeOptions& options)
{
	socket_->open(request, options);
}

void web::Socket::ping(const QByteArray& payload)
{
	socket_->ping(payload);
}

void web::Socket::setTypeString(const QString& typeString)
{
	Q_D(Socket);
	d->typeString_ = typeString;
}

QByteArray web::Socket::typeString() const
{
	const Q_D(Socket);
	return d->typeString_.toUtf8();
}

void web::Socket::setProcessId(qint64 processId)
{
	Q_D(Socket);
	d->processId_ = processId;
}

qint64 web::Socket::processId() const
{
	const Q_D(Socket);
	return d->processId_;
}

void web::Socket::insertUserData(const QString& key, const QVariant& userData)
{
	Q_D(Socket);
	d->insertUserData(key, userData);
}

QVariant web::Socket::userData(const QString& key) const
{
	const Q_D(Socket);
	return d->userData(key);
}

qint64 web::Socket::elapsed() const
{
	const Q_D(Socket);
	return d->timer_.elapsed();
}

void web::Socket::restartTimer()
{
	Q_D(Socket);
	d->timer_.restart();
}