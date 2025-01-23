#include "stdafx.h"

#include "BCGAssistant/dispatcher/signaldispatcher.h"
#include <performancemonitormanager.h>
#include <tool.h>

PerformanceMonitorManager::PerformanceMonitorManager(QObject* parent)
    : QObject(parent)
{
    PerformanceMonitor& monitor = PerformanceMonitor::instance();

    connect(&monitor, &PerformanceMonitor::performanceDataUpdated, this, &PerformanceMonitorManager::handlePerformanceData, Qt::QueuedConnection);

    connect(&timer_, &QTimer::timeout, this, [&monitor]()
        {

            monitor.updatePerformanceData();
        }, Qt::QueuedConnection);

    timer_.start(5000);
}

PerformanceMonitorManager::~PerformanceMonitorManager()
{
    timer_.stop();
}

void PerformanceMonitorManager::handlePerformanceData(const PerformanceData& data)
{
    tool::JsonReply reply("notify");
    reply.setStatus(true);
    reply.setCommand("getPerformanceData");
    reply.setParameters(
        {
            {"field",  "process"},
            {"cpuUsage",  data.cpuUsage},
            {"memoryUsage", data.memoryUsage},
            {"threadCount", data.threadCount},
            {"handleCount", data.handleCount}
        }
    );

    SignalDispatcher& signalDispatcher = SignalDispatcher::instance();
    emit signalDispatcher.sendBinaryMessageToFrontend(reply.toUtf8());
}
