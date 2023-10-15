/*
				GNU GENERAL PUBLIC LICENSE
				   Version 2, June 1991
COPYRIGHT (C) Bestkakkoii 2023 All Rights Reserved.
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/

#include "stdafx.h"
#include "scopedhandle.h"
#include <TlHelp32.h>
#include <qglobal.h>

std::atomic_long ScopedHandle::handleCount_ = 0L;
std::atomic_flag ScopedHandle::atlock_ = ATOMIC_FLAG_INIT;

ScopedHandle::ScopedHandle(HANDLE_TYPE h)
{
	if (h == CREATE_EVENT)
		createEvent();
}

ScopedHandle::ScopedHandle(HANDLE_TYPE h, DWORD dwFlags, DWORD th32ProcessID)
	: enableAutoClose_(true)
{
	if (h == CREATE_TOOLHELP32_SNAPSHOT)
		createToolhelp32Snapshot(dwFlags, th32ProcessID);
}

ScopedHandle::ScopedHandle(DWORD dwProcess, bool bAutoClose)
	: enableAutoClose_(bAutoClose)
{
	openProcess(dwProcess);
}

void ScopedHandle::reset(DWORD dwProcessId)
{
	if (NULL != dwProcessId)
	{
		closeHandle();
		openProcess(dwProcessId);
	}
}

void ScopedHandle::reset()
{
	closeHandle();
	handle_ = nullptr;
}

void ScopedHandle::reset(HANDLE handle)
{
	if (NULL != handle)
	{
		closeHandle();
		handle_ = handle;
	}
}

ScopedHandle::ScopedHandle(int dwProcess, bool bAutoClose)
	: enableAutoClose_(bAutoClose)
{
	openProcess(static_cast<DWORD>(dwProcess));
}

ScopedHandle::ScopedHandle(long long dwProcess, bool bAutoClose)
	: enableAutoClose_(bAutoClose)
{
	openProcess(static_cast<DWORD>(dwProcess));
}

ScopedHandle::ScopedHandle(HANDLE_TYPE h, HANDLE ProcessHandle, PVOID StartRoutine, PVOID Argument)
	: enableAutoClose_(true)
{
	enablePrivilege(::GetCurrentProcess());
	if (h == CREATE_REMOTE_THREAD)
		createThreadEx(ProcessHandle, StartRoutine, Argument);
}

ScopedHandle::ScopedHandle(HANDLE_TYPE h, HANDLE hSourceProcessHandle, HANDLE hSourceHandle, HANDLE hTargetProcessHandle, DWORD dwOptions)
	: enableAutoClose_(true)
{
	enablePrivilege(::GetCurrentProcess());
	if (h == DUPLICATE_HANDLE)
		duplicateHandle(hSourceProcessHandle, hSourceHandle, hTargetProcessHandle, dwOptions);
}

ScopedHandle::~ScopedHandle()
{
	if (enableAutoClose_)
		this->closeHandle();
}

void ScopedHandle::closeHandle()
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

HANDLE ScopedHandle::NtOpenProcess(DWORD dwProcess)
{
	HANDLE ProcessHandle = NULL;
	using namespace MINT;
	OBJECT_ATTRIBUTES ObjectAttribute = {
		sizeof(OBJECT_ATTRIBUTES), 0, 0, 0, 0, 0 };
	CLIENT_ID ClientId = {};

	InitializeObjectAttributes(&ObjectAttribute, 0, 0, 0, 0);
	ClientId.UniqueProcess = reinterpret_cast<PVOID>(static_cast<long long>(dwProcess));
	ClientId.UniqueThread = (PVOID)0;

	BOOL ret = NT_SUCCESS(MINT::NtOpenProcess(&ProcessHandle, MAXIMUM_ALLOWED,
		&ObjectAttribute, &ClientId));

	return ret && ProcessHandle && ((ProcessHandle) != INVALID_HANDLE_VALUE) ? ProcessHandle : nullptr;
};

HANDLE ScopedHandle::ZwOpenProcess(DWORD dwProcess)
{
	HANDLE ProcessHandle = (HANDLE)0;
	MINT::OBJECT_ATTRIBUTES ObjectAttribute = {
		sizeof(MINT::OBJECT_ATTRIBUTES), 0, 0, 0, 0, 0 };
	ObjectAttribute.Attributes = NULL;
	MINT::CLIENT_ID ClientIds = {};
	ClientIds.UniqueProcess = reinterpret_cast<HANDLE>(static_cast<long long>(dwProcess));
	ClientIds.UniqueThread = (HANDLE)0;
	BOOL ret = NT_SUCCESS(MINT::ZwOpenProcess(&ProcessHandle, PROCESS_ALL_ACCESS, &ObjectAttribute,
		&ClientIds));

	return ret && ProcessHandle && ((ProcessHandle) != INVALID_HANDLE_VALUE) ? ProcessHandle : nullptr;
};

void ScopedHandle::openProcess(DWORD dwProcess)
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

void ScopedHandle::createToolhelp32Snapshot(DWORD dwFlags, DWORD th32ProcessID)
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

void ScopedHandle::createThreadEx(HANDLE ProcessHandle, PVOID StartRoutine, PVOID Argument)
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

void ScopedHandle::duplicateHandle(HANDLE hSourceProcessHandle, HANDLE hSourceHandle, HANDLE hTargetProcessHandle, DWORD dwOptions)
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

void ScopedHandle::createEvent()
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

HANDLE ScopedHandle::openProcessToken(HANDLE ProcessHandle, ACCESS_MASK DesiredAccess)
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

// 提權函數：提升為DEBUG權限
BOOL ScopedHandle::enablePrivilege(HANDLE hProcess, const wchar_t* SE)
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