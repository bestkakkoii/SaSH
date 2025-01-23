#ifndef WEBSOCKET_P_H_
#define WEBSOCKET_P_H_

#include "websocket.h"
#include <QPointer>

namespace web
{
	class SocketPrivate
	{
	public:
		explicit SocketPrivate(Socket* original);
		virtual ~SocketPrivate();

		void setTypeString(const QString& typeString) { typeString_ = typeString; }
		QByteArray typeString() const { return typeString_.toUtf8(); }

		void setProcessId(qint64 processId) { processId_ = processId; }
		qint64 processId() const { return processId_; }

		void insertUserData(const QString& key, const QVariant& userData) { userData_.insert(key, userData); }
		QVariant userData(const QString& key) const { return userData_.value(key); }

		qint64 elapsed() const { return timer_.elapsed(); }

		void restartTimer() { timer_.restart(); }

	private:
		QString typeString_;
		qint64 processId_ = 0;
		QHash<QString, QVariant> userData_;
		Timer timer_;

	public:
		QPointer<Socket> q_ptr;
		Q_DECLARE_PUBLIC(Socket);

	};
}
#endif // WEBSOCKET_P_H_
