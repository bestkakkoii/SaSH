module;

#include <Windows.h>
#include <TlHelp32.h>
#include <atomic>

#include <QReadWriteLock>
#include <QFile>
#include <QIODevice>
#include <QSet>
#include <QMutex>
#include <QLockFile>

#ifndef MINT_USE_SEPARATE_NAMESPACE
#define MINT_USE_SEPARATE_NAMESPACE
#include <MINT/MINT.h>
#endif

export module Scoped;

//智能句柄
export class ScopedHandle
{
public:
	typedef enum
	{
		CREATE_TOOLHELP32_SNAPSHOT,
		CREATE_REMOTE_THREAD,
		DUPLICATE_HANDLE,
		CREATE_EVENT,
	}HANDLE_TYPE;

	ScopedHandle() = default;

	explicit ScopedHandle(HANDLE_TYPE h)
	{
		if (h == CREATE_EVENT)
			createEvent();
	}

	explicit ScopedHandle(HANDLE handle) : handle_(handle) {}
	ScopedHandle(HANDLE_TYPE h, DWORD dwFlags, DWORD th32ProcessID)
		: enableAutoClose_(true)
	{
		if (h == CREATE_TOOLHELP32_SNAPSHOT)
			createToolhelp32Snapshot(dwFlags, th32ProcessID);
	}

	explicit ScopedHandle(DWORD dwProcess, bool bAutoClose = true)
		: enableAutoClose_(bAutoClose)
	{
		openProcess(dwProcess);
	}


	explicit ScopedHandle(int dwProcess, bool bAutoClose = true)
		: enableAutoClose_(bAutoClose)
	{
		openProcess(static_cast<DWORD>(dwProcess));
	}

	explicit ScopedHandle(__int64 dwProcess, bool bAutoClose = true)
		: enableAutoClose_(bAutoClose)
	{
		openProcess(static_cast<DWORD>(dwProcess));
	}

	ScopedHandle(HANDLE_TYPE h, HANDLE ProcessHandle, PVOID StartRoutine, PVOID Argument)
		: enableAutoClose_(true)
	{
		enablePrivilege(::GetCurrentProcess());
		if (h == CREATE_REMOTE_THREAD)
			createThreadEx(ProcessHandle, StartRoutine, Argument);
	}


	ScopedHandle(HANDLE_TYPE h, HANDLE hSourceProcessHandle, HANDLE hSourceHandle, HANDLE hTargetProcessHandle, DWORD dwOptions)
		: enableAutoClose_(true)
	{
		enablePrivilege(::GetCurrentProcess());
		if (h == DUPLICATE_HANDLE)
			duplicateHandle(hSourceProcessHandle, hSourceHandle, hTargetProcessHandle, dwOptions);
	}


	virtual ~ScopedHandle()
	{
		if (enableAutoClose_)
			this->closeHandle();
	}


	void reset(DWORD dwProcessId)
	{
		if (NULL != dwProcessId)
		{
			closeHandle();
			openProcess(dwProcessId);
		}
	}

	void reset(HANDLE handle)
	{
		if (NULL != handle)
		{
			closeHandle();
			handle_ = handle;
		}
	}

	void reset()
	{
		closeHandle();
		handle_ = nullptr;
	}

	inline bool isValid() const
	{
		return ((handle_) != INVALID_HANDLE_VALUE) && (handle_);
	}

	inline operator HANDLE() const
	{
		return handle_;
	}

	//==
	inline bool operator==(const ScopedHandle& other) const
	{
		return handle_ == other.handle_;
	}

	//== HANDLE
	inline bool operator==(const HANDLE& other) const
	{
		return handle_ == other;
	}

	//!= HANDLE
	inline bool operator!=(const HANDLE& other) const
	{
		return handle_ != other;
	}

	//!=
	inline bool operator!=(const ScopedHandle& other) const
	{
		return handle_ != other.handle_;
	}

	// copy constructor
	ScopedHandle(const ScopedHandle& other)
	{
		lock_.lockForWrite();
		handle_ = other.handle_;
		this->enableAutoClose_ = other.enableAutoClose_;
		lock_.unlock();
	}

	// copy assignment
	ScopedHandle& operator=(const ScopedHandle& other)
	{
		if (this != &other)
		{
			lock_.lockForWrite();
			handle_ = other.handle_;
			this->enableAutoClose_ = other.enableAutoClose_;
			lock_.unlock();
		}
		return *this;
	}

	//copy assignment from HANDLE
	ScopedHandle& operator=(const HANDLE& other)
	{
		lock_.lockForWrite();
		handle_ = other;
		lock_.unlock();
		return *this;
	}

	// move constructor
	ScopedHandle(ScopedHandle&& other) noexcept
	{
		lock_.lockForWrite();
		handle_ = other.handle_;
		this->enableAutoClose_ = other.enableAutoClose_;
		other.handle_ = nullptr;
		other.enableAutoClose_ = false;
		lock_.unlock();
	}

	// move assignment
	ScopedHandle& operator=(ScopedHandle&& other) noexcept
	{
		if (this != &other)
		{
			lock_.lockForWrite();
			handle_ = other.handle_;
			this->enableAutoClose_ = other.enableAutoClose_;
			other.handle_ = nullptr;
			other.enableAutoClose_ = false;
			lock_.unlock();
		}
		return *this;
	}

	inline void setAutoClose(bool bAutoClose) { this->enableAutoClose_ = bAutoClose; }

public:
	static BOOL enablePrivilege(HANDLE hProcess, const wchar_t* SE = SE_DEBUG_NAME)
	{
		if (!hProcess) return FALSE;
		BOOL fOk = FALSE;
		do
		{
			HANDLE hToken = openProcessToken(hProcess, TOKEN_ALL_ACCESS);
			if (hToken == NULL || hToken == INVALID_HANDLE_VALUE)
				break;

			TOKEN_PRIVILEGES tp = {};

			if (LookupPrivilegeValueW(NULL, SE, &tp.Privileges[0].Luid) == FALSE)
				break;

			tp.PrivilegeCount = 1;
			tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
			MINT::NtAdjustPrivilegesToken(hToken, FALSE, &tp, sizeof(tp), nullptr, nullptr);

			fOk = (::GetLastError() == ERROR_SUCCESS);
		} while (false);
		return fOk;
	}

private:
	void closeHandle()
	{
		QWriteLocker locker(&lock_);
		enablePrivilege(::GetCurrentProcess());
		HANDLE h = handle_;
		if (!h
			|| ((h) == INVALID_HANDLE_VALUE)
			|| ((h) == ::GetCurrentProcess()))
			return;

		BOOL ret = NT_SUCCESS(MINT::NtClose(h));
		if (ret)
		{
			subHandleCount();
			handle_ = nullptr;
			h = nullptr;
		}
	}

	void openProcess(DWORD dwProcess)
	{
		QWriteLocker locker(&lock_);
		enablePrivilege(::GetCurrentProcess());
		HANDLE hprocess = NtOpenProcess(dwProcess);
		if (!hprocess || ((hprocess) == INVALID_HANDLE_VALUE))
		{
			hprocess = ZwOpenProcess(dwProcess);
			if (!hprocess || ((hprocess) == INVALID_HANDLE_VALUE))
			{
				hprocess = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcess);
				if (!hprocess || ((hprocess) == INVALID_HANDLE_VALUE))
				{
					hprocess = nullptr;
				}
			}
		}

		if (hprocess && ((hprocess) != INVALID_HANDLE_VALUE))
		{
			addHandleCount();
			handle_ = hprocess;
			hprocess = nullptr;
			enablePrivilege(hprocess);
		}
		else
			handle_ = nullptr;
	}

	void createToolhelp32Snapshot(DWORD dwFlags, DWORD th32ProcessID)
	{
		QWriteLocker locker(&lock_);
		enablePrivilege(::GetCurrentProcess());
		HANDLE hSnapshot = ::CreateToolhelp32Snapshot(dwFlags, th32ProcessID);
		if (hSnapshot && ((hSnapshot) != INVALID_HANDLE_VALUE))
		{
			addHandleCount();
			handle_ = hSnapshot;
			hSnapshot = nullptr;
		}
		else
			handle_ = nullptr;
	}

	void createThreadEx(HANDLE ProcessHandle, PVOID StartRoutine, PVOID Argument)
	{
		QWriteLocker locker(&lock_);
		enablePrivilege(::GetCurrentProcess());
		HANDLE hThread = nullptr;
		BOOL ret = NT_SUCCESS(MINT::NtCreateThreadEx(&hThread, THREAD_ALL_ACCESS, nullptr, ProcessHandle, StartRoutine, Argument, FALSE, NULL, NULL, NULL, nullptr));
		if (ret && hThread && ((hThread) != INVALID_HANDLE_VALUE))
		{
			LARGE_INTEGER pTimeout = {};
			pTimeout.QuadPart = -1ll * 10000000ll;
			MINT::NtWaitForSingleObject(hThread, FALSE, &pTimeout);
			addHandleCount();
			handle_ = hThread;
			hThread = nullptr;
		}
		else
			handle_ = nullptr;
	}

	void duplicateHandle(HANDLE hSourceProcessHandle, HANDLE hSourceHandle, HANDLE hTargetProcessHandle, DWORD dwOptions)
	{
		QWriteLocker locker(&lock_);
		enablePrivilege(::GetCurrentProcess());
		HANDLE hTargetHandle = nullptr;
		//BOOL ret = ::DuplicateHandle(hSourceProcessHandle, hSourceHandle, hTargetProcessHandle, &hTargetHandle, 0, FALSE, dwOptions);
		BOOL ret = NT_SUCCESS(MINT::NtDuplicateObject(hSourceProcessHandle, hSourceHandle, hTargetProcessHandle, &hTargetHandle, 0, FALSE, dwOptions));
		if (ret && hTargetHandle && ((hTargetHandle) != INVALID_HANDLE_VALUE))
		{
			addHandleCount();
			handle_ = hTargetHandle;
			hTargetHandle = nullptr;
		}
		else
			handle_ = nullptr;
	}

	static HANDLE openProcessToken(HANDLE ProcessHandle, ACCESS_MASK DesiredAccess)
	{
		HANDLE hToken = nullptr;
		BOOL ret = NT_SUCCESS(MINT::NtOpenProcessToken(ProcessHandle, DesiredAccess, &hToken));
		if (ret && hToken && ((hToken) != INVALID_HANDLE_VALUE))
		{
			return hToken;
		}
		else
			return nullptr;
	}

	void createEvent()
	{
		QWriteLocker locker(&lock_);
		enablePrivilege(::GetCurrentProcess());
		HANDLE hEvent = ::CreateEventW(nullptr, FALSE, FALSE, nullptr);
		if (hEvent && ((hEvent) != INVALID_HANDLE_VALUE))
		{
			addHandleCount();
			handle_ = hEvent;
			hEvent = nullptr;
		}
		else
			handle_ = nullptr;
	}

	HANDLE NtOpenProcess(DWORD dwProcess)
	{
		HANDLE ProcessHandle = NULL;
		using namespace MINT;
		OBJECT_ATTRIBUTES ObjectAttribute = {
			sizeof(OBJECT_ATTRIBUTES), 0, 0, 0, 0, 0 };
		CLIENT_ID ClientId = {};

		InitializeObjectAttributes(&ObjectAttribute, 0, 0, 0, 0);
		ClientId.UniqueProcess = reinterpret_cast<PVOID>(static_cast<__int64>(dwProcess));
		ClientId.UniqueThread = (PVOID)0;

		BOOL ret = NT_SUCCESS(MINT::NtOpenProcess(&ProcessHandle, MAXIMUM_ALLOWED,
			&ObjectAttribute, &ClientId));

		return ret && ProcessHandle && ((ProcessHandle) != INVALID_HANDLE_VALUE) ? ProcessHandle : nullptr;
	};


	HANDLE ZwOpenProcess(DWORD dwProcess)
	{
		HANDLE ProcessHandle = (HANDLE)0;
		MINT::OBJECT_ATTRIBUTES ObjectAttribute = {
			sizeof(MINT::OBJECT_ATTRIBUTES), 0, 0, 0, 0, 0 };
		ObjectAttribute.Attributes = NULL;
		MINT::CLIENT_ID ClientIds = {};
		ClientIds.UniqueProcess = reinterpret_cast<HANDLE>(static_cast<__int64>(dwProcess));
		ClientIds.UniqueThread = (HANDLE)0;
		BOOL ret = NT_SUCCESS(MINT::ZwOpenProcess(&ProcessHandle, PROCESS_ALL_ACCESS, &ObjectAttribute,
			&ClientIds));

		return ret && ProcessHandle && ((ProcessHandle) != INVALID_HANDLE_VALUE) ? ProcessHandle : nullptr;
	};

	inline static void addHandleCount()
	{
		while (atlock_.test_and_set(std::memory_order_acquire));
		++handleCount_;
		atlock_.clear(std::memory_order_release);
	}

	inline static void subHandleCount()
	{
		while (atlock_.test_and_set(std::memory_order_acquire));
		--handleCount_;
		atlock_.clear(std::memory_order_release);
	}

	inline static LONG getHandleCount()
	{
		while (atlock_.test_and_set(std::memory_order_acquire));
		long val = handleCount_.load();
		atlock_.clear(std::memory_order_release);
		return val;
	}

private:
	mutable QReadWriteLock lock_;

	bool enableAutoClose_ = true;

	HANDLE handle_ = nullptr;

private:
	inline static std::atomic_long handleCount_ = 0L;
	inline static std::atomic_flag atlock_ = ATOMIC_FLAG_INIT;
};

//智能文件句柄類
export class ScopedFile : public QFile
{
public:
	ScopedFile(const QString& filename, OpenMode ioFlags)
		: QFile(filename)
	{
		if (QFile::isOpen())
		{
			QFile::flush();
			QFile::close();
		}
		QFile::open(ioFlags);
	}

	explicit ScopedFile(const QString& filename)
		: QFile(filename)
	{}

	ScopedFile() = default;

	bool openWriteNew()
	{
		if (QFile::isOpen())
		{
			QFile::flush();
			QFile::close();
		}

		return QFile::open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Unbuffered);
	}

	bool openWrite()
	{
		if (QFile::isOpen())
		{
			QFile::flush();
			QFile::close();
		}

		return QFile::open(QIODevice::WriteOnly | QIODevice::Unbuffered);
	}

	bool openRead()
	{
		if (QFile::isOpen())
		{
			QFile::flush();
			QFile::close();
		}

		return QFile::open(QIODevice::ReadOnly | QIODevice::Unbuffered);
	}

	bool openReadWrite()
	{
		if (QFile::isOpen())
		{
			QFile::flush();
			QFile::close();
		}

		return QFile::open(QIODevice::ReadWrite | QIODevice::Unbuffered);
	}

	bool openReadWriteNew()
	{
		if (QFile::isOpen())
		{
			QFile::flush();
			QFile::close();
		}

		return QFile::open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Unbuffered);
	}

	bool openWriteAppend()
	{
		if (QFile::isOpen())
		{
			QFile::flush();
			QFile::close();
		}

		return QFile::open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Unbuffered);
	}

	virtual ~ScopedFile()
	{
		if (m_maps.size())
		{
			for (uchar* map : m_maps)
			{
				QFile::unmap(map);
			}
			m_maps.clear();
		}

		if (QFile::isOpen())
		{
			QFile::flush();
			QFile::close();
		}
	}

	template <typename T>
	bool mmap(T*& p, __int64 offset, __int64 size, QFile::MemoryMapFlags flags = QFileDevice::MapPrivateOption)//QFile::NoOptions
	{
		uchar* uc = QFile::map(offset, size, flags);
		if (uc)
		{
			m_maps.insert(uc);
			p = reinterpret_cast<T*>(uc);
		}
		return uc != nullptr;
	}

	QFile& file() { return *this; }

private:
	QSet<uchar*> m_maps;
};

//帶文件鎖的智能文件句柄類
export class ScopedFileLocker
{
public:
	explicit ScopedFileLocker(const QString& fileName)
		:lock_(fileName)
	{
		isFileLocked = lock_.tryLock();
	}

	virtual ~ScopedFileLocker()
	{
		if (isFileLocked)
		{
			lock_.unlock();
		}
	}

	bool isLocked() const
	{
		return isFileLocked;
	}

private:
	QLockFile lock_;
	bool isFileLocked = false;
};

//非強制的智能鎖類
export class ScopedLocker
{
public:
	explicit ScopedLocker(QMutex* lock)
		:lock_(*lock)
	{
		isLocked_ = lock_.tryLock();
	}

	virtual ~ScopedLocker()
	{
		if (isLocked_)
		{
			lock_.unlock();
		}
	}

	bool isLocked() const
	{
		return isLocked_;
	}



private:
	QMutex& lock_;
	bool isLocked_ = false;

};