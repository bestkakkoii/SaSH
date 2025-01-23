#include "stdafx.h"
#include "performancemonitor.h"
#include <QDebug>
#include <QProcess>
#include <QTextStream>
#include <wbemidl.h>
#include <comdef.h>

#pragma comment(lib, "wbemuuid.lib")

class PerformanceMonitorPrivate
{
public:
    PerformanceMonitorPrivate();
    ~PerformanceMonitorPrivate();

    void initialize();
    bool executeQuery(const QString& query, IEnumWbemClassObject** pEnumerator);

    IWbemLocator* pLoc;
    IWbemServices* pSvc;

    DWORD current_process_id_;
    PerformanceData data_;

    ThreadPerformanceData threadData_;
    PerformanceMonitor* parent_;
};

PerformanceMonitorPrivate::PerformanceMonitorPrivate()
    : pLoc(nullptr)
    , pSvc(nullptr)
    , current_process_id_(0)
{
    initialize();
}

PerformanceMonitorPrivate::~PerformanceMonitorPrivate()
{
    if (pSvc)
    {
        pSvc->Release();
    }

    if (pLoc)
    {
        pLoc->Release();
    }
}

void PerformanceMonitorPrivate::initialize()
{
    HRESULT hres = CoCreateInstance(
        CLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator,
        (LPVOID*)&pLoc
    );

    if (FAILED(hres))
    {
        qCritical().noquote() << "Failed to create IWbemLocator object. Error code =" << hres;
        CoUninitialize();
        return;
    }

    hres = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"),
        NULL,
        NULL,
        0,
        NULL,
        0,
        0,
        &pSvc
    );

    if (FAILED(hres))
    {
        qCritical().noquote() << "Could not connect to WMI. Error code =" << hres;
        pLoc->Release();
        CoUninitialize();
        return;
    }

    hres = CoSetProxyBlanket(
        pSvc,
        RPC_C_AUTHN_WINNT,
        RPC_C_AUTHZ_NONE,
        NULL,
        RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE
    );

    if (FAILED(hres))
    {
        qCritical().noquote() << "Could not set proxy blanket. Error code =" << hres;
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return;
    }
}

bool PerformanceMonitorPrivate::executeQuery(const QString& query, IEnumWbemClassObject** pEnumerator)
{
    BSTR bstrQuery = SysAllocString(query.toStdWString().c_str());
    HRESULT hres = pSvc->ExecQuery(
        bstr_t("WQL"),
        bstrQuery,
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        pEnumerator
    );
    SysFreeString(bstrQuery);

    if (FAILED(hres))
    {
        qCritical().noquote() << "Query failed. Error code =" << (ULONG)hres << "Query:" << query;
        if (WBEM_E_INVALID_QUERY == hres)
        {
            qCritical().noquote() << "Invalid query syntax";
        }
        return false;
    }

    return true;
}

PerformanceMonitor& PerformanceMonitor::instance()
{
    static PerformanceMonitor instance;
    return instance;
}

PerformanceMonitor::PerformanceMonitor(QObject* parent)
    : QObject(parent)
    , d_ptr(std::make_unique<PerformanceMonitorPrivate>())
    , refCount_(1)  // Initialize reference count
    , isThreadPerformanceQuery_(false)
{
    d_ptr->parent_ = this;
}

PerformanceMonitor::~PerformanceMonitor() = default;

void PerformanceMonitor::updatePerformanceData(DWORD processId)
{
    if (0 == processId)
    {
        processId = _getpid();
    }

    d_ptr->current_process_id_ = processId;

    QString query = QString("SELECT PercentProcessorTime, WorkingSetPrivate, ThreadCount, HandleCount FROM Win32_PerfFormattedData_PerfProc_Process WHERE IDProcess = %1").arg(processId);

    IEnumWbemClassObject* pEnumerator = nullptr;
    if (!d_ptr->executeQuery(query, &pEnumerator))
    {
        qCritical().noquote() << "Query execution failed";
        return;
    }

    // 使用 NextAsync 方法
    HRESULT hr = pEnumerator->NextAsync(1, this);
    if (FAILED(hr))
    {
        qCritical().noquote() << "NextAsync failed. Error code:" << hr;
        pEnumerator->Release();
        return;
    }
}

void PerformanceMonitor::updateThreadPerformanceData(DWORD threadId)
{
    isThreadPerformanceQuery_ = true;

    QString query = QString("SELECT * FROM Win32_PerfRawData_PerfProc_Thread WHERE IDThread = %1").arg(threadId);

    IEnumWbemClassObject* pEnumerator = nullptr;
    if (!d_ptr->executeQuery(query, &pEnumerator))
    {
        qCritical().noquote() << "Query execution failed";
        return;
    }

    // 使用 NextAsync 方法
    HRESULT hr = pEnumerator->NextAsync(1, this);
    if (FAILED(hr))
    {
        qCritical().noquote() << "NextAsync failed. Error code:" << hr;
        pEnumerator->Release();
        return;
    }
}

// IWbemObjectSink 方法的實作
HRESULT STDMETHODCALLTYPE PerformanceMonitor::Indicate(LONG lObjectCount, IWbemClassObject** apObjArray)
{
    if (0 == lObjectCount || nullptr == apObjArray)
    {
        return WBEM_S_FALSE;
    }

    if (isThreadPerformanceQuery_)
    {
        IWbemClassObject* pclsObj = apObjArray[0];
        ThreadPerformanceData data = {};
        VARIANT vtProp;
        VariantInit(&vtProp);

        // 獲取線程ID
        HRESULT hr = pclsObj->Get(L"IDThread", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr) && vtProp.vt == VT_UINT)
        {
            data.threadId = vtProp.uintVal;
        }
        VariantClear(&vtProp);

        // 獲取CPU使用率
        VariantInit(&vtProp);
        hr = pclsObj->Get(L"PercentProcessorTime", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr) && vtProp.vt == VT_I8)
        {
            data.cpuUsage = vtProp.ullVal;
        }
        VariantClear(&vtProp);

        // 獲取線程執行時間
        VariantInit(&vtProp);
        hr = pclsObj->Get(L"ElapsedTime", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr) && vtProp.vt == VT_I8)
        {
            data.elapsedTime = vtProp.llVal;
        }
        VariantClear(&vtProp);

        // 獲取線程狀態
        VariantInit(&vtProp);
        hr = pclsObj->Get(L"ThreadState", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr) && vtProp.vt == VT_I8)
        {
            data.threadState = vtProp.lVal;
        }
        VariantClear(&vtProp);

        // 獲取內存使用量
        VariantInit(&vtProp);
        hr = pclsObj->Get(L"PrivateBytes", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR)
        {
            data.memoryUsage = QString::fromWCharArray(vtProp.bstrVal).toDouble() / (1024.0 * 1024.0); // 轉換為MB
        }
        VariantClear(&vtProp);

        emit threadPerformanceDataUpdated(data);
    }
    else
    {
        IWbemClassObject* pclsObj = apObjArray[0];
        VARIANT vtProp;
        VariantInit(&vtProp);

        // 獲取進程名稱
        HRESULT hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR)
        {
            d_ptr->data_.processName = QString::fromWCharArray(vtProp.bstrVal);
        }
        VariantClear(&vtProp);

        VariantInit(&vtProp);
        hr = pclsObj->Get(L"PercentProcessorTime", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR)
        {
            d_ptr->data_.cpuUsage = QString::fromWCharArray(vtProp.bstrVal).toDouble();
        }
        VariantClear(&vtProp);

        VariantInit(&vtProp);
        hr = pclsObj->Get(L"WorkingSetPrivate", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR)
        {
            d_ptr->data_.memoryUsage = QString::fromWCharArray(vtProp.bstrVal).toDouble() / (1024.0 * 1024.0);
        }
        VariantClear(&vtProp);

        VariantInit(&vtProp);
        hr = pclsObj->Get(L"ThreadCount", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr) && vtProp.vt == VT_I4)
        {
            d_ptr->data_.threadCount = vtProp.lVal;
        }
        VariantClear(&vtProp);

        VariantInit(&vtProp);
        hr = pclsObj->Get(L"HandleCount", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr) && vtProp.vt == VT_I4)
        {
            d_ptr->data_.handleCount = vtProp.lVal;
        }
        VariantClear(&vtProp);

        emit performanceDataUpdated(d_ptr->data_);
    }

    return WBEM_S_NO_ERROR;
}

HRESULT STDMETHODCALLTYPE PerformanceMonitor::SetStatus(LONG lFlags, HRESULT hResult, BSTR strParam, IWbemClassObject* pObjParam)
{
    if (WBEM_STATUS_COMPLETE == lFlags)
    {
        // 查詢完成
        //qDebug() << "WMI query completed with status:" << hResult;
    }
    else if (WBEM_STATUS_PROGRESS == lFlags)
    {
        // 處理進度
        // qDebug() << "WMI query in progress.";
    }

    return WBEM_S_NO_ERROR;
}

// IUnknown 方法的實作
ULONG STDMETHODCALLTYPE PerformanceMonitor::AddRef()
{
    return InterlockedIncrement(&refCount_);
}

ULONG STDMETHODCALLTYPE PerformanceMonitor::Release()
{
    ULONG uRet = InterlockedDecrement(&refCount_);
    if (uRet == 0)
    {
        delete this;
    }

    return uRet;
}

HRESULT STDMETHODCALLTYPE PerformanceMonitor::QueryInterface(REFIID riid, void** ppv)
{
    if (IID_IUnknown == riid || IID_IWbemObjectSink == riid)
    {
        *ppv = static_cast<IWbemObjectSink*>(this);
        AddRef();
        return S_OK;
    }

    *ppv = nullptr;
    return E_NOINTERFACE;
}
