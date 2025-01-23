#ifndef PERFORMANCEMONITOR_H
#define PERFORMANCEMONITOR_H

#include <windows.h>
#include <memory>
#include <wbemidl.h>

#include <QObject>
#include <QString>

class PerformanceMonitorPrivate;

struct PerformanceData
{
    QString processName;
    qreal cpuUsage = 0.0;
    qreal memoryUsage = 0.0; // in MB
    qint64 threadCount = 0;
    qint64 handleCount = 0;
};

struct ThreadPerformanceData
{
    qreal cpuUsage = 0.0; // in percent
    qint64 threadId = 0;
    qint64 elapsedTime = 0; // in seconds
    qint64 threadState = 0; // thread state
    qreal memoryUsage = 0.0; // in MB
};

class PerformanceMonitor : public QObject, public IWbemObjectSink
{
    Q_OBJECT

public:
    static PerformanceMonitor& instance();

    void updatePerformanceData(DWORD processId = 0);

    void updateThreadPerformanceData(DWORD threadId);

    // IWbemObjectSink interface methods
    HRESULT STDMETHODCALLTYPE Indicate(LONG lObjectCount, IWbemClassObject** apObjArray) override;
    HRESULT STDMETHODCALLTYPE SetStatus(LONG lFlags, HRESULT hResult, BSTR strParam, IWbemClassObject* pObjParam) override;
    // IUnknown 方法的實作
    ULONG STDMETHODCALLTYPE AddRef() override;

    ULONG STDMETHODCALLTYPE Release() override;

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;
signals:
    void performanceDataUpdated(const PerformanceData& data);
    void threadPerformanceDataUpdated(const ThreadPerformanceData& data);

private:
    explicit PerformanceMonitor(QObject* parent = nullptr);
    ~PerformanceMonitor();

    PerformanceMonitor(const PerformanceMonitor&) = delete;
    PerformanceMonitor& operator=(const PerformanceMonitor&) = delete;

    std::unique_ptr<PerformanceMonitorPrivate> d_ptr;
    LONG refCount_;
    bool isThreadPerformanceQuery_;
};

#endif // PERFORMANCEMONITOR_H
