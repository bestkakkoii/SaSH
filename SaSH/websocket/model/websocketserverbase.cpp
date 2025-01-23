#include "stdafx.h"
#include "websocketserverbase.h"

web::SocketServerBase::SocketServerBase(const QString& serverName, QObject* parent)
    : QWebSocketServer(serverName, QWebSocketServer::SslMode::NonSecureMode, parent)
{

}

web::SocketServerBase::~SocketServerBase()
{

}

web::Socket* web::SocketServerBase::getNextPendingConnection()
{
    return new web::Socket(QWebSocketServer::nextPendingConnection());
}