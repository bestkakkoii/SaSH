#ifndef WEBSOCKETSERVERBASE_H_
#define WEBSOCKETSERVERBASE_H_

#include <QWebSocketServer>
#include "websocket.h"

namespace web
{
    class Socket;

    class SocketServerBase : public QWebSocketServer
    {
        Q_OBJECT
    public:
        explicit SocketServerBase(const QString& serverName, QObject* parent = nullptr);
        virtual ~SocketServerBase();

    public:
        Socket* getNextPendingConnection();

    };
}

#endif // WEBSOCKETSERVERBASE_H_