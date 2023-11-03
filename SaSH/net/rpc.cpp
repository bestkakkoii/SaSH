#include "stdafx.h"
#include "rpc.h"

RPC* RPC::instance = nullptr;

void createConsole()
{
	if (!AllocConsole())
	{
		return;
	}
	FILE* fDummy;
	freopen_s(&fDummy, "CONOUT$", "w", stdout);
	freopen_s(&fDummy, "CONOUT$", "w", stderr);
	freopen_s(&fDummy, "CONIN$", "r", stdin);
	std::cout.clear();
	std::clog.clear();
	std::cerr.clear();
	std::cin.clear();

	// std::wcout, std::wclog, std::wcerr, std::wcin
	HANDLE hConOut = CreateFile(TEXT("CONOUT$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	HANDLE hConIn = CreateFile(TEXT("CONIN$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	SetStdHandle(STD_OUTPUT_HANDLE, hConOut);
	SetStdHandle(STD_ERROR_HANDLE, hConOut);
	SetStdHandle(STD_INPUT_HANDLE, hConIn);
	std::wcout.clear();
	std::wclog.clear();
	std::wcerr.clear();
	std::wcin.clear();

	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);
	setlocale(LC_ALL, "en_US.UTF-8");
}

RPC::RPC(ProtocolType protocolType, QObject* parent)
	: QObject(parent)
	, protocolType(protocolType)
	, server(q_check_ptr(new QTcpServer(parent)))
{
}

bool RPC::listen(quint16 port)
{
	if (protocolType == IPV6)
		server->listen(QHostAddress::AnyIPv6, port);
	else
		server->listen(QHostAddress::AnyIPv4, port);

	connect(server, &QTcpServer::newConnection, this, &RPC::onNewConnection);
	return server->isListening();
}

void RPC::close()
{
	server->close();
}

void RPC::setProtocol(ProtocolType type)
{
	if (!server->isListening())
	{
		protocolType = type;
	}
}

void RPC::onNewConnection()
{
	static bool first = true;
	if (first)
	{
		first = false;
#ifndef _DEBUG
		createConsole();
#endif
	}

	QTcpSocket* client = server->nextPendingConnection();
	connect(client, &QTcpSocket::readyRead, this, &RPC::onReadyRead);
	connect(client, &QTcpSocket::disconnected, this, &RPC::onDisconnected);
}

void RPC::onReadyRead()
{
	QTcpSocket* client = qobject_cast<QTcpSocket*>(sender());
	QByteArray data = client->readAll();
	std::cout << std::string(data.constData()) << std::endl;
	processMessage(client, data);
}

void RPC::onDisconnected()
{
	QTcpSocket* client = qobject_cast<QTcpSocket*>(sender());
	long long clientId = clients.key(client, -1);
	if (-1 != clientId)
	{
		clients.remove(clientId);

		// Remove all methods registered for this client.
		methods.remove(clientId);

		// Handle the client disconnecting.
		qDebug() << "Client disconnected.";
		//delete
		client->deleteLater();
	}
}

void RPC::processMessage(QTcpSocket* client, const QByteArray& data)
{
	// Convert the data to a QString, assuming it is UTF-8 encoded.
	QString message = QString::fromUtf8(data);

	// If the client has not been assigned an identifier yet, this must be the handshake message.
	long long clientId = clients.key(client, -1);
	if (-1 == clientId)
	{
		QStringList parts = message.split('|');
		// HANDSHAKE|identifier
		if (parts.count() != 2 || parts.value(0) != "HANDSHAKE")
		{
			std::cout << "Malformed handshake message, disconnect." << std::endl;
			qDebug() << "Malformed handshake message, disconnect.";
			client->disconnectFromHost(); // Malformed handshake message, disconnect.
			return;
		}

		bool ok;
		qlonglong identifier = parts.value(1).toLongLong(&ok);
		if (!ok || identifier < 0 || identifier >= SASH_MAX_THREAD || clients.contains(identifier))
		{
			std::cout << "Invalid or duplicate identifier, disconnect." << std::endl;
			qDebug() << "Invalid or duplicate identifier, disconnect.";
			client->disconnectFromHost(); // Invalid or duplicate identifier, disconnect.
			return;
		}

		// Save the client with its identifier.
		clients.insert(identifier, client);
		std::cout << "Client connected with identifier " << identifier << std::endl;
		qDebug() << "Client connected with identifier" << identifier;

		QString response = QString("%1|HANDSHAKE|OK\n").arg(identifier);
		QByteArray responseArray = response.toUtf8();
		std::cout << std::string(responseArray.constData()) << std::endl;
		client->write(responseArray);
		client->flush();
		client->waitForBytesWritten();
		return; // Handshake complete, wait for next message.
	}

	// For subsequent messages, proceed with normal processing.
	QStringList parts = message.split(util::rexOR);
	if (parts.isEmpty())
	{
		std::cout << "message is empty" << std::endl;
		qDebug() << "Malformed message, disconnect.";
		return;
	}

	long long cmpClientId = parts.takeFirst().toLongLong();
	if (cmpClientId != clientId)
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
	if (!methods.value(clientId).contains(methodName))
	{
		std::cout << "Method not registered for this client" << std::endl;
		qDebug() << "Method not registered for this client, disconnect.";
		// Method not registered for this client, disconnect.
		return;
	}

	// Retrieve the method and the object instance for this client.
	MethodInfo methodInfo = methods[clientId].value(methodName);
	QMetaMethod method = methodInfo.method;
	QObject* receiver = methodInfo.receiver;
	ReturnType returnType = methodInfo.returnType;

	// Prepare the return value container based on the returnType.
	QVariant returnValue;

	// Prepare the parameters for the method call.
	QVector<QGenericArgument> arguments;
	for (const QString& part : parts)
	{
		bool ok;
		qlonglong number = part.toLongLong(&ok);
		if (ok) {
			QGenericArgument argument("qlonglong", &number);
			arguments.append(argument);
		}
		else {
			QGenericArgument argument("QString", part.toUtf8().constData());
			arguments.append(argument);
		}
	}

	// Call the method with the arguments.
	bool success = false;
	QString response;
	switch (returnType)
	{
	case ReturnQString:
	{
		QString returnString;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
		success = method.invoke(receiver, Qt::DirectConnection
			, Q_RETURN_ARG(QString, returnString)
			, arguments.value(0)
			, arguments.value(1)
			, arguments.value(2)
			, arguments.value(3)
			, arguments.value(4)
			, arguments.value(5)
			, arguments.value(6)
			, arguments.value(7)
			, arguments.value(8)
			, arguments.value(9));
#else
		success = method.invoke(receiver,
			Qt::DirectConnection,
			QGenericReturnArgument("QString", returnString.data())
			, arguments.value(0)
			, arguments.value(1)
			, arguments.value(2)
			, arguments.value(3)
			, arguments.value(4)
			, arguments.value(5)
			, arguments.value(6)
			, arguments.value(7)
			, arguments.value(8)
			, arguments.value(9));
#endif

		returnValue = returnString;
		break;
	}
	case ReturnLongLong:
	{
		qlonglong returnLongLong = 0;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
		success = method.invoke(receiver, Qt::DirectConnection
			, Q_RETURN_ARG(qlonglong, returnLongLong)
			, arguments.value(0)
			, arguments.value(1)
			, arguments.value(2)
			, arguments.value(3)
			, arguments.value(4)
			, arguments.value(5)
			, arguments.value(6)
			, arguments.value(7)
			, arguments.value(8)
			, arguments.value(9));
#else
		success = method.invoke(receiver,
			Qt::DirectConnection,
			QGenericReturnArgument("qlonglong", &returnLongLong)
			, arguments.value(0)
			, arguments.value(1)
			, arguments.value(2)
			, arguments.value(3)
			, arguments.value(4)
			, arguments.value(5)
			, arguments.value(6)
			, arguments.value(7)
			, arguments.value(8)
			, arguments.value(9));
#endif

		returnValue = returnLongLong;

		break;
	}
	case NoReturn:
	{
		success = method.invoke(receiver, Qt::DirectConnection
			, arguments.value(0)
			, arguments.value(1)
			, arguments.value(2)
			, arguments.value(3)
			, arguments.value(4)
			, arguments.value(5)
			, arguments.value(6)
			, arguments.value(7)
			, arguments.value(8)
			, arguments.value(9));

		break;
	}
	default:
		// Handle unknown or unsupported return types.
		break;
	}

	if (!success) {
		// Handle the error, the method invocation failed.
		return;
	}

	// Prepare the response message.   index|functionName|returnValue
	response = QString("%1|%2|%3\n").arg(clientId).arg(methodName).arg(returnValue.toString());

	// Send the response to the client.
	QByteArray responseArray = response.toUtf8();
	std::cout << std::string(responseArray.constData()) << std::endl;
	client->write(responseArray);
	client->flush();
	client->waitForBytesWritten();
}
