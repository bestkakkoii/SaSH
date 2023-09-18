/*
* Copyright (c) 2019-2022 Bestkakkoii
*
* This software is provided 'as-is', without any express or implied
* warranty.  In no event will the authors be held liable for any damages
* arising from the use of this software.
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*/

/* Smart Handle */
#ifndef SCOPEDHANDLE_H
#define SCOPEDHANDLE_H
#include <Windows.h>
#ifndef MINT_USE_SEPARATE_NAMESPACE
#define MINT_USE_SEPARATE_NAMESPACE
#include <MINT/MINT.h>
#endif
#include <QReadWriteLock>
#include <atomic>

class ScopedHandle
{
private:
	void closeHandle();
	void openProcess(qint64 dwProcess);
	void createToolhelp32Snapshot(DWORD dwFlags, qint64 th32ProcessID);
	void createThreadEx(HANDLE ProcessHandle, PVOID StartRoutine, PVOID Argument);
	void duplicateHandle(HANDLE hSourceProcessHandle, HANDLE hSourceHandle, HANDLE hTargetProcessHandle, DWORD dwOptions);
	void openProcessToken(HANDLE ProcessHandle, ACCESS_MASK DesiredAccess);
	void createEvent();

	HANDLE NtOpenProcess(qint64 dwProcess);
	HANDLE ZwOpenProcess(qint64 dwProcess);

	mutable QReadWriteLock m_lock;

	bool enableAutoClose = true;

	HANDLE m_handle = nullptr;

	static std::atomic_int64_t m_handleCount;

	static std::atomic_flag m_atlock;

	inline static void addHandleCount()
	{
		while (m_atlock.test_and_set(std::memory_order_acquire));
		m_handleCount.fetch_add(1, std::memory_order_release);
		m_atlock.clear(std::memory_order_release);
	}

	inline static void subHandleCount()
	{
		while (m_atlock.test_and_set(std::memory_order_acquire));
		m_handleCount.fetch_sub(1, std::memory_order_release);
		m_atlock.clear(std::memory_order_release);
	}

	inline static qint64 getHandleCount()
	{
		while (m_atlock.test_and_set(std::memory_order_acquire));
		qint64 val = m_handleCount.load(std::memory_order_acquire);
		m_atlock.clear(std::memory_order_release);
		return val;
	}

public:
	typedef enum
	{
		CREATE_TOOLHELP32_SNAPSHOT,
		CREATE_REMOTE_THREAD,
		DUPLICATE_HANDLE,
		CREATE_EVENT,
	}HANDLE_TYPE;

	ScopedHandle() = default;
	explicit ScopedHandle(HANDLE_TYPE h);
	explicit ScopedHandle(HANDLE handle) : m_handle(handle) {}
	ScopedHandle(HANDLE_TYPE h, DWORD dwFlags, DWORD th32ProcessID);
	ScopedHandle(qint64 dwProcess, bool bAutoClose = true);
	ScopedHandle(HANDLE_TYPE h, HANDLE ProcessHandle, PVOID StartRoutine, PVOID Argument);
	ScopedHandle(HANDLE_TYPE h, HANDLE hSourceProcessHandle, HANDLE hSourceHandle, HANDLE hTargetProcessHandle, DWORD dwOptions);
	virtual ~ScopedHandle();

	void reset(qint64 dwProcessId);
	void reset(HANDLE handle);
	void reset();

	inline bool isValid() const
	{
		return ((this->m_handle) != INVALID_HANDLE_VALUE) && (this->m_handle != nullptr);
	}

	inline operator HANDLE() const
	{
		return m_handle;
	}

	//==
	inline bool operator==(const ScopedHandle& other) const
	{
		return this->m_handle == other.m_handle;
	}

	//== HANDLE
	inline bool operator==(const HANDLE& other) const
	{
		return this->m_handle == other;
	}

	//!= HANDLE
	inline bool operator!=(const HANDLE& other) const
	{
		return this->m_handle != other;
	}

	//!=
	inline bool operator!=(const ScopedHandle& other) const
	{
		return this->m_handle != other.m_handle;
	}

	// copy constructor
	ScopedHandle(const ScopedHandle& other)
	{
		m_lock.lockForWrite();
		this->m_handle = other.m_handle;
		this->enableAutoClose = other.enableAutoClose;
		m_lock.unlock();
	}

	// copy assignment
	ScopedHandle& operator=(const ScopedHandle& other)
	{
		if (this != &other)
		{
			m_lock.lockForWrite();
			this->m_handle = other.m_handle;
			this->enableAutoClose = other.enableAutoClose;
			m_lock.unlock();
		}
		return *this;
	}

	//copy assignment from HANDLE
	ScopedHandle& operator=(const HANDLE& other)
	{
		m_lock.lockForWrite();
		this->m_handle = other;
		m_lock.unlock();
		return *this;
	}

	// move constructor
	ScopedHandle(ScopedHandle&& other) noexcept
	{
		m_lock.lockForWrite();
		this->m_handle = other.m_handle;
		this->enableAutoClose = other.enableAutoClose;
		other.m_handle = nullptr;
		other.enableAutoClose = false;
		m_lock.unlock();
	}

	// move assignment
	ScopedHandle& operator=(ScopedHandle&& other) noexcept
	{
		if (this != &other)
		{
			m_lock.lockForWrite();
			this->m_handle = other.m_handle;
			this->enableAutoClose = other.enableAutoClose;
			other.m_handle = nullptr;
			other.enableAutoClose = false;
			m_lock.unlock();
		}
		return *this;
	}

	inline void setAutoClose(bool bAutoClose) { this->enableAutoClose = bAutoClose; }

	BOOL EnableDebugPrivilege(HANDLE hProcess, const wchar_t* SE = SE_DEBUG_NAME);
};

#endif // QSCOPEDHANDLE_H