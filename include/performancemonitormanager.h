#ifndef PERFORMANCEMONITORMANAGER_H_
#define PERFORMANCEMONITORMANAGER_H_

#include <QObject>
#include <QTimer>
#include <performancemonitor.h>


class PerformanceMonitorManager : public QObject
{
    Q_OBJECT
public:
    static PerformanceMonitorManager& instance(QObject* parent = nullptr)
    {
        static PerformanceMonitorManager instance(parent);
        return instance;
    }

    explicit PerformanceMonitorManager(QObject* parent = nullptr);

    virtual ~PerformanceMonitorManager();

public slots:
    void handlePerformanceData(const PerformanceData& data);

private:
    QTimer timer_;

    Q_DISABLE_COPY_MOVE(PerformanceMonitorManager);
};

#endif // PERFORMANCEMONITORMANAGER_H_