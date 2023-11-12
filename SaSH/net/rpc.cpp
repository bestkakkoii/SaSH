#include "stdafx.h"
#include "rpc.h"
#include "util.h"

RPC* RPC::instance = nullptr;

RPCSocket::RPCSocket(qintptr socketDescriptor, QObject* parent)
	: QTcpSocket(nullptr)
	, Indexer(-1)
{
	std::ignore = parent;
	setSocketDescriptor(socketDescriptor);
	setReadBufferSize(65500);

	setSocketOption(QAbstractSocket::LowDelayOption, 1);
	setSocketOption(QAbstractSocket::KeepAliveOption, 1);
	setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, 0);
	setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 0);
	setSocketOption(QAbstractSocket::TypeOfServiceOption, 64);
	connect(this, &RPCSocket::closed, this, &RPCSocket::onClose, Qt::QueuedConnection);
	connect(this, &RPCSocket::writed, this, &RPCSocket::onWrite, Qt::QueuedConnection);

	moveToThread(&thread);
	thread.start();
}

void RPCSocket::onClose()
{
	disconnectFromHost();
}

bool RPCSocket::isFunctionExist(const QString& functionName)
{
	if (device_.isNull())
		return false;

	if (functionName.isEmpty())
		return false;

	std::string funcName = util::toConstData(functionName);

	QSharedPointer<sol::state> device = device_.toStrongRef();

	sol::object obj = device->get<sol::object>(funcName);
	if (!obj.valid())
		return false;

	sol::type type = obj.get_type();
	if (type != sol::type::function)
		return false;

	return true;
}

void RPCSocket::call(const QString& functionName, const QStringList& args)
{
	if (device_.isNull())
		return;

	if (functionName.isEmpty())
		return;

	std::string funcName = util::toConstData(functionName);

	QSharedPointer<sol::state> device = device_.toStrongRef();

	sol::object obj = device->get<sol::object>(funcName);
	if (!obj.valid())
		return;

	sol::type type = obj.get_type();
	if (type != sol::type::function)
		return;

	sol::state_view lua(device->lua_state());
	std::vector <sol::object> va;
	bool ok = false;
	long long tmpInt = 0;
	std::string tmpStr;

	for (const QString& arg : args)
	{
		tmpInt = arg.toLongLong(&ok, 16);
		if (ok)
		{
			va.emplace_back(sol::make_object(lua, tmpInt));
		}
		else
		{
			tmpStr = util::toConstData(arg);
			va.emplace_back(sol::make_object(lua, tmpStr));
		}
	}

	sol::protected_function func = obj.as<sol::protected_function>();

	sol::protected_function_result result = func(sol::as_args(std::vector <sol::object>(va.begin(), va.end())));

	QByteArray responseArray;
	if (!result.valid())
	{
		sol::error err = result;
		QString errWhat = util::toQString(err.what());
		QString errStr = QString("%1|%2|%3").arg(getIndex()).arg(functionName).arg(errWhat);
		responseArray = errStr.toUtf8();
		emit writed(std::move(responseArray));
		return;
	}

	sol::object ret = result;
	if (!ret.valid())
		return;

	QString resultStr;
	QString resultValue;
	if (ret.is<bool>())
	{
		resultValue = ret.as<bool>() ? "true" : "false";
	}
	else if (ret.is<long long>())
	{
		resultValue = util::toQString(ret.as<long long>());
	}
	else if (ret.is<double>())
	{
		resultValue = util::toQString(ret.as<double>());
	}
	else if (ret.is<std::string>())
	{
		resultValue = util::toQString(ret.as<std::string>());
	}
	resultStr = QString("%1|%2|%3").arg(getIndex()).arg(functionName).arg(resultValue);
	responseArray = resultStr.toUtf8();
	emit writed(std::move(responseArray));
}

void RPCSocket::onReadyRead()
{
	QByteArray data = readAll();

	// Convert the data to a QString, assuming it is UTF-8 encoded.
	QString message = QString::fromUtf8(data);

	// For subsequent messages, proceed with normal processing.
	QStringList parts = message.split(util::rexOR);
	if (parts.isEmpty())
	{
		std::cout << "message is empty" << std::endl;
		qDebug() << "Malformed message, disconnect.";
		return;
	}

	long long cmpClientId = parts.takeFirst().toLongLong();
	if (cmpClientId != getIndex())
	{
		std::cout << "client id is not equal" << std::endl;
		qDebug() << "Malformed message, disconnect.";
		return;
	}

	if (parts.isEmpty())
	{
		std::cout << "message is empty" << std::endl;
		qDebug() << "Malformed message, disconnect.";
		return;
	}

	// The first part should be the method name.
	QString methodName = parts.takeFirst();
	if (methodName.isEmpty())
	{
		std::cout << "method name is empty" << std::endl;
		qDebug() << "Malformed message, disconnect.";
		return;
	}

	call(methodName, parts);
}

void RPCSocket::onWrite(QByteArray data)
{
	if (!data.endsWith("\n"))
		data.append("\n");

	write(data);
	flush();
	waitForBytesWritten();
}

RPCServer::RPCServer(QObject* parent)
	: QTcpServer(parent)
{

}

RPCServer::~RPCServer()
{
	qDebug() << "RPCServer is distroyed!!";
}

bool RPCServer::start(unsigned short port)
{
	QOperatingSystemVersion version = QOperatingSystemVersion::current();

	if (version > QOperatingSystemVersion::Windows7)
	{
		if (!this->listen(QHostAddress::AnyIPv6, port))
		{
			qDebug() << "ipv6 Failed to listen on socket";
			QString msg = tr("Failed to listen on IPV6 socket");
			std::wstring wstr = msg.toStdWString();
			MessageBoxW(NULL, wstr.c_str(), L"Error", MB_OK | MB_ICONERROR);
			return false;
		}
	}
	else
	{
		if (!this->listen(QHostAddress::AnyIPv6, port))
		{
			qDebug() << "ipv4 Failed to listen on socket";
			QString msg = tr("Failed to listen on IPV4 socket");
			std::wstring wstr = msg.toStdWString();
			MessageBoxW(NULL, wstr.c_str(), L"Error", MB_OK | MB_ICONERROR);
			return false;
		}
	}

	return true;
}

void RPCServer::clear()
{
	for (RPCSocket* clientSocket : clientSockets_)
	{
		if (clientSocket == nullptr)
			continue;

		clientSocket->close();

		if (clientSocket->state() == QAbstractSocket::ConnectedState)
			clientSocket->waitForDisconnected();
		clientSocket->deleteLater();
	}

	clientSockets_.clear();
	clientSocketHash.clear();
}

void RPCServer::incomingConnection(qintptr socketDescriptor)
{
	RPCSocket* clientSocket = q_check_ptr(new RPCSocket(socketDescriptor, nullptr));
	sash_assume(clientSocket != nullptr);
	if (clientSocket == nullptr)
		return;

	addPendingConnection(clientSocket);
	clientSockets_.append(clientSocket);
	connect(clientSocket, &RPCSocket::disconnected, this, [this, clientSocket]()
		{
			clientSocketHash.remove(clientSocket->getIndex());
			clientSockets_.removeOne(clientSocket);
			clientSocket->thread.quit();
			clientSocket->thread.wait();
			clientSocket->deleteLater();
		});

	connect(clientSocket, &RPCSocket::readyRead, this, &RPCServer::onClientFirstReadyRead, Qt::QueuedConnection);
}

QSharedPointer<sol::state> RPCServer::getDevice(long long id)
{
	if (!devices_.contains(id))
	{
		QSharedPointer<sol::state> device(QSharedPointer<sol::state>::create());
		//open all libs
		device->open_libraries(
			sol::lib::base,
			sol::lib::package,
			sol::lib::os,
			sol::lib::string,
			sol::lib::math,
			sol::lib::table,
			sol::lib::debug,
			sol::lib::utf8,
			sol::lib::coroutine,
			sol::lib::io
		);

		device->set("__INDEX", id);

		devices_.insert(id, device);
		return device;
}

	return devices_.value(id);
}

//client very first custom handshake
void RPCServer::onClientFirstReadyRead()
{
	RPCSocket* clientSocket = qobject_cast<RPCSocket*>(sender());
	if (clientSocket == nullptr)
		return;

	QByteArray data = clientSocket->readAll();

	// Convert the data to a QString, assuming it is UTF-8 encoded.
	QString message = QString::fromUtf8(data);

	// If the client has not been assigned an identifier yet, this must be the handshake message.
	long long clientId = clientSocketHash.key(clientSocket, -1);
	if (-1 != clientId)
		return;

	QStringList parts = message.split('|');
	// HANDSHAKE|identifier
	if (parts.count() != 2 || parts.value(0).toLower() != "shake")
	{
		std::cout << "Malformed handshake message, disconnect." << std::endl;
		qDebug() << "Malformed handshake message, disconnect.";
		emit clientSocket->closed(); // Malformed handshake message, disconnect.
		return;
	}

	bool ok;
	long long identifier = parts.value(1).toLongLong(&ok);
	if (!ok || identifier < 0 || identifier >= SASH_MAX_THREAD || clientSocketHash.contains(identifier))
	{
		std::cout << "Invalid or duplicate identifier, disconnect." << std::endl;
		qDebug() << "Invalid or duplicate identifier, disconnect.";
		emit clientSocket->closed(); // Invalid or duplicate identifier, disconnect.
		return;
	}

	// Save the client with its identifier.
	clientSocket->setIndex(identifier);
	clientSocket->setDevice(getDevice(identifier));

	clientSocketHash.insert(identifier, clientSocket);

	disconnect(clientSocket, &RPCSocket::readyRead, this, &RPCServer::onClientFirstReadyRead);
	connect(clientSocket, &RPCSocket::readyRead, clientSocket, &RPCSocket::onReadyRead, Qt::QueuedConnection);

	std::cout << "Client connected with identifier " << identifier << std::endl;
	qDebug() << "Client connected with identifier" << identifier;

	QString response = QString("%1|shake|ok").arg(identifier);
	QByteArray responseArray = response.toUtf8();
	std::cout << std::string(responseArray.constData()) << std::endl;
	emit clientSocket->writed(std::move(responseArray));
}

RPC::RPC(QObject* parent)
	: QObject(nullptr)
	, rpcServer(parent)
{

}

bool RPC::listen(unsigned short port)
{
	return rpcServer.start(port);
}

void RPC::close()
{
	rpcServer.close();
	rpcServer.clear();
}