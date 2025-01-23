#pragma once
#include <QTcpServer>
#include <QTcpSocket>
#include <QHash>
#include <QByteArray>
#include <QNetworkInterface>
#include <QMetaMethod>
#include "indexer.h"

enum ReturnType { NoReturn, ReturnQString, ReturnLongLong, UnknownReturn };

// 存储方法及其返回值类型
struct MethodInfo {
	QMetaMethod method;
	QObject* receiver;
	ReturnType returnType;
};

class RPCSocket : public QTcpSocket, public Indexer
{
	Q_OBJECT
public:
	RPCSocket(qintptr socketDescriptor, QObject* parent = nullptr);

	void setDevice(QSharedPointer<sol::state> device) { device_ = device; }

	void call(const QString& functionName, const QStringList& args);

signals:
	void closed();
	void writed(QByteArray data);

public slots:
	void onReadyRead();

	void onClose();

	void onWrite(QByteArray data);

private:
	bool isFunctionExist(const QString& functionName);

public:
	QThread thread;

private:
	QWeakPointer<sol::state> device_;
	bool init = false;
};

class RPCServer : public QTcpServer
{
	Q_OBJECT
public:
	explicit RPCServer(QObject* parent = nullptr);

	virtual ~RPCServer();

	bool start(unsigned short port);

	void clear();

	inline QList<RPCSocket*> getClientSockets() { return clientSockets_; }

	QSharedPointer<sol::state> getDevice(long long id);

public slots:
	void onClientFirstReadyRead();

protected:
	virtual void incomingConnection(qintptr socketDescriptor) override;

private:
	QList<RPCSocket*> clientSockets_;
	QHash<long long, RPCSocket*> clientSocketHash;
	QHash<long long, QSharedPointer<sol::state>> devices_;
};

class RPC : public QObject
{
	Q_OBJECT
private:
	Q_DISABLE_COPY_MOVE(RPC)


private:

private:
	static RPC* instance;
	RPC(QObject* parent = nullptr);

public:
	static void initialize(QObject* parent)
	{
		if (instance == nullptr)
		{
			instance = q_check_ptr(new RPC(parent));
			sash_assume(instance != nullptr);
		}
	}

	static RPC& getInstance()
	{
		return *instance;
	}

public:
	bool listen(unsigned short port);

	void close();

	QSharedPointer<sol::state> getDevice(long long id)
	{
		return rpcServer.getDevice(id);
	}

private:
	RPCServer rpcServer;
};
