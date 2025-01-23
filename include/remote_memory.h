#ifndef REMOTE_MEMORY_H_
#define REMOTE_MEMORY_H_

#include <windows.h>
#include <QString>

namespace remote_memory
{
    typedef struct _IAT_EAT_INFO
    {
        char ModuleName[256] = {};
        char FuncName[64] = {};
        ULONG64 Address = 0;
        ULONG64 RecordAddr = 0;
        ULONG64 ModBase = 0; // just for export table
    } IAT_EAT_INFO, * PIAT_EAT_INFO;

    bool enablePrivilege(HANDLE hProcess);
    bool enablePrivilege();

    HANDLE openProcess(DWORD processId);

    HMODULE getModuleHandle(HANDLE hProcess, const QString& szModuleName);

    // 獲得32位進程中某個DLL導出函數的地址
    bool getProcAddress32(HANDLE hProcess, const QString& ModuleName, const QSet<QString>& targetFuncs, QHash<QString, ULONG64>* ptrs);

    quint64 alloc(HANDLE hProcess, quint64 size);

    bool free(HANDLE hProcess, quint64 desiredAccess);

    bool protect(HANDLE hProcess, quint64 address, quint64 size, quint64 flNewProtect, quint64* flOldProtect);

    bool write(HANDLE hProcess, quint64 desiredAccess, const void* buffer, quint64 dwSize);

    template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T> && !std::is_pointer_v<T>>>
    bool write(HANDLE hProcess, quint64 desiredAccess, T data);

    template bool write<char>(HANDLE hProcess, quint64 desiredAccess, char data);
    template bool write<qint16>(HANDLE hProcess, quint64 desiredAccess, qint16 data);
    template bool write<qint32>(HANDLE hProcess, quint64 desiredAccess, qint32 data);
    template bool write<float>(HANDLE hProcess, quint64 desiredAccess, float data);
    template bool write<long>(HANDLE hProcess, quint64 desiredAccess, long data);
    template bool write<qint64>(HANDLE hProcess, quint64 desiredAccess, qint64 data);
    template bool write<uchar>(HANDLE hProcess, quint64 desiredAccess, uchar data);
    template bool write<quint16>(HANDLE hProcess, quint64 desiredAccess, quint16 data);
    template bool write<quint32>(HANDLE hProcess, quint64 desiredAccess, quint32 data);
    template bool write<ulong>(HANDLE hProcess, quint64 desiredAccess, ulong data);
    template bool write<quint64>(HANDLE hProcess, quint64 desiredAccess, quint64 data);
    template bool write<qreal>(HANDLE hProcess, quint64 desiredAccess, qreal data);

    bool write(HANDLE hProcess, quint64 desiredAccess, const QString& str);

    bool read(HANDLE hProcess, quint64 desiredAccess, void* buffer, quint64 size);

    template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T> && !std::is_pointer_v<T>>>
    [[nodiscard]] T read(HANDLE hProcess, quint64 desiredAccess);

    template char read<char>(HANDLE hProcess, quint64 desiredAccess);
    template qint16 read<qint16>(HANDLE hProcess, quint64 desiredAccess);
    template qint32 read<qint32>(HANDLE hProcess, quint64 desiredAccess);
    template float read<float>(HANDLE hProcess, quint64 desiredAccess);
    template long read<long>(HANDLE hProcess, quint64 desiredAccess);
    template qint64 read<qint64>(HANDLE hProcess, quint64 desiredAccess);
    template uchar read<uchar>(HANDLE hProcess, quint64 desiredAccess);
    template quint16 read<quint16>(HANDLE hProcess, quint64 desiredAccess);
    template quint32 read<quint32>(HANDLE hProcess, quint64 desiredAccess);
    template ulong read<ulong>(HANDLE hProcess, quint64 desiredAccess);
    template quint64 read<quint64>(HANDLE hProcess, quint64 desiredAccess);
    template qreal read<qreal>(HANDLE hProcess, quint64 desiredAccess);

    void closeHandle(HANDLE hProcess);

    bool createRemoteThread(PHANDLE ThreadHandle, HANDLE ProcessHandle, PVOID StartRoutine, PVOID Argument);

    HANDLE duplicateHandle(HANDLE hSourceProcessHandle, HANDLE hSourceHandle, HANDLE hTargetProcessHandle, DWORD dwOptions);

    bool closeMutex(HANDLE target = GetCurrentProcess());

    [[nodiscard]] QString __fastcall read(HANDLE hProcess, quint64 desiredAccess, quint64 size);

    bool injectLibrary32(
        DWORD processId,
        QString dllPath,
        HMODULE* phDllModule,
        quint64* phGameModule, QString* replyMessage);

    class Page
    {
    public:
        enum Encode
        {
            kAnsi,
            kUnicode,
        };

        Page(HANDLE hProcess, qint64 size);

        Page(HANDLE hProcess, const QString& str, Encode use_unicode);

        // copy constructor
        Page(const Page& other)
            : lpAddress_(other.lpAddress_)
        {
        }
        //copy assignment
        Page& operator=(const Page& other)
        {
            lpAddress_ = other.lpAddress_;
            hProcess_ = other.hProcess_;
            return *this;
        }
        // move constructor
        Page(Page&& other) noexcept
            : lpAddress_(other.lpAddress_)
            , hProcess_(other.hProcess_)

        {
            other.lpAddress_ = 0;
        }
        // move assignment
        Page& operator=(Page&& other) noexcept
        {
            lpAddress_ = other.lpAddress_;
            hProcess_ = other.hProcess_;
            other.lpAddress_ = 0;
            return *this;
        }

        operator quint64() const;

        Page& operator=(qint64 other);

        Page* operator&();

        virtual ~Page();

        void clear();

        [[nodiscard]] bool isNull() const;

        [[nodiscard]] bool isValid()const;

        [[nodiscard]] bool isData(BYTE* data, qint64 size) const;

    private:
        quint64 lpAddress_ = NULL;
        HANDLE hProcess_ = nullptr;
    };


}

class ScopedProcessHandle
{
public:
    explicit ScopedProcessHandle(DWORD processId);

    ScopedProcessHandle(HANDLE hProcess);

    virtual ~ScopedProcessHandle();

    [[nodiscard]] bool isValid() const;

    operator HANDLE() const;

private:
    HANDLE hProcess_ = nullptr;
};

class ScopedThreadHandle
{
public:
    ScopedThreadHandle(HANDLE processHandle, PVOID startRoutine, PVOID argument);

    virtual ~ScopedThreadHandle();

    bool isValid() const;

    void wait() const;

private:

    HANDLE hThread_ = nullptr;
};
#endif // REMOTE_MEMORY_H_