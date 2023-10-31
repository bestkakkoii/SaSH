#pragma once
#include <QTcpServer>
#include <QTcpSocket>
#include <QHash>
#include <QByteArray>
#include <QNetworkInterface>
#include <QMetaMethod>

class RPC : public QObject
{
	Q_OBJECT
private:
	Q_DISABLE_COPY_MOVE(RPC)

public:
	enum ProtocolType
	{
		IPV4,
		IPV6
	};

private:
	enum ReturnType { NoReturn, ReturnQString, ReturnLongLong, UnknownReturn };

	// 存储方法及其返回值类型
	struct MethodInfo {
		QMetaMethod method;
		QObject* receiver;
		ReturnType returnType;
	};

private:
	static RPC* instance;
	RPC(ProtocolType protocolType, QObject* parent = nullptr);

public:
	static void initialize(ProtocolType type, QObject* parent)
	{
		if (instance == nullptr)
		{
			instance = new RPC(type, parent);
		}
	}

	static RPC& getInstance()
	{
		return *instance;
	}

public:
	bool listen(quint16 port);

	void close();

	void setProtocol(ProtocolType type);

	bool isConnected(int identifier)
	{
		return clients.contains(identifier);
	}

	void reg(qlonglong clientId, const QString& customMethodName, const char* methodSignature, QObject* receiver) {
		// 在对象的元对象中查找方法签名对应的方法
		int methodIndex = receiver->metaObject()->indexOfMethod(QMetaObject::normalizedSignature(methodSignature));
		if (methodIndex == -1) {
			qWarning() << "Method" << methodSignature << "not found in" << receiver->metaObject()->className();
			return;
		}

		QMetaMethod method = receiver->metaObject()->method(methodIndex);

		// 确定返回类型
		ReturnType returnType = UnknownReturn;
		if (method.returnType() == QMetaType::QString)
			returnType = ReturnQString;
		else if (method.returnType() == QMetaType::LongLong)
			returnType = ReturnLongLong;
		else if (method.returnType() == QMetaType::Void)
			returnType = NoReturn;
		else
			qWarning() << "Unknown return type" << method.returnType() << "for method" << methodSignature;

		// 用自定义的方法名存储方法信息
		MethodInfo methodInfo = { method, receiver, returnType };
		methods[clientId].insert(customMethodName, methodInfo);
		qDebug() << "Registered method" << customMethodName << "for client" << clientId;
	}

private slots:
	void onNewConnection();

	void onDisconnected();

	void onReadyRead();

private:
	void processMessage(QTcpSocket* client, const QByteArray& data);

	ProtocolType protocolType;
	QTcpServer* server;
	QHash<int, QTcpSocket*> clients;
	QHash<qlonglong, QHash<QString, MethodInfo>> methods;
	QHash<QString, ReturnType> returnTypes;
};
