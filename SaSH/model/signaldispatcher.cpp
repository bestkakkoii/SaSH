import Safe;
#include <QDebug>
#include "signaldispatcher.h"

SafeHash<__int64, SignalDispatcher*> g_signalDispatcherInstances = {};

SignalDispatcher::SignalDispatcher(__int64 index)
	: Indexer(index)
{
}

SignalDispatcher::~SignalDispatcher()
{
	qDebug() << "SignalDispatcher is distoryed!!";
}

SignalDispatcher& SignalDispatcher::getInstance(__int64 index)
{
	if (!g_signalDispatcherInstances.contains(index))
	{
		SignalDispatcher* instance = new SignalDispatcher(index);
		g_signalDispatcherInstances.insert(index, instance);
	}
	return *g_signalDispatcherInstances.value(index);
}

bool SignalDispatcher::contains(__int64 index)
{
	return g_signalDispatcherInstances.contains(index);
}

void SignalDispatcher::setParent(QObject* parent) { QObject::setParent(parent); }

void SignalDispatcher::remove(__int64 index)
{
	if (!g_signalDispatcherInstances.contains(index))
		return;

	SignalDispatcher* instance = g_signalDispatcherInstances.take(index);
	if (instance != nullptr)
	{
		instance->deleteLater();
	}
}