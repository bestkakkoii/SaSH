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
	explicit ScopedHandle(HANDLE handle) : handle_(handle) {}
	ScopedHandle(HANDLE_TYPE h, DWORD dwFlags, DWORD th32ProcessID);
	ScopedHandle(DWORD dwProcess, bool bAutoClose = true);
	ScopedHandle(int dwProcess, bool bAutoClose = true);
	ScopedHandle(HANDLE_TYPE h, HANDLE ProcessHandle, PVOID StartRoutine, PVOID Argument);
	ScopedHandle(HANDLE_TYPE h, HANDLE hSourceProcessHandle, HANDLE hSourceHandle, HANDLE hTargetProcessHandle, DWORD dwOptions);
	virtual ~ScopedHandle();

	void reset(DWORD dwProcessId);
	void reset(HANDLE handle);
	void reset();

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
	static BOOL 	enablePrivilege(HANDLE hProcess, const wchar_t* SE = SE_DEBUG_NAME);

private:
	void closeHandle();
	void openProcess(DWORD dwProcess);
	void createToolhelp32Snapshot(DWORD dwFlags, DWORD th32ProcessID);
	void createThreadEx(HANDLE ProcessHandle, PVOID StartRoutine, PVOID Argument);
	void duplicateHandle(HANDLE hSourceProcessHandle, HANDLE hSourceHandle, HANDLE hTargetProcessHandle, DWORD dwOptions);
	static HANDLE openProcessToken(HANDLE ProcessHandle, ACCESS_MASK DesiredAccess);
	void createEvent();

	HANDLE NtOpenProcess(DWORD dwProcess);
	HANDLE ZwOpenProcess(DWORD dwProcess);

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
	static std::atomic_long handleCount_;

	static std::atomic_flag atlock_;
};

#endif // QSCOPEDHANDLE_H