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
//#include "qlog.hpp"
#include <TlHelp32.h>
#include <qglobal.h>

std::atomic_long ScopedHandle::m_handleCount = 0L;
std::atomic_flag ScopedHandle::m_atlock = ATOMIC_FLAG_INIT;

ScopedHandle::ScopedHandle(HANDLE_TYPE h)
{
	if (h == CREATE_EVENT)
		createEvent();
}

ScopedHandle::ScopedHandle(HANDLE_TYPE h, DWORD dwFlags, DWORD th32ProcessID)
	: enableAutoClose(true)
{
	if (h == CREATE_TOOLHELP32_SNAPSHOT)
		createToolhelp32Snapshot(dwFlags, th32ProcessID);
}

ScopedHandle::ScopedHandle(DWORD dwProcess, bool bAutoClose)
	: enableAutoClose(bAutoClose)
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
	this->m_handle = nullptr;
}

void ScopedHandle::reset(HANDLE handle)
{
	if (NULL != handle)
	{
		closeHandle();
		m_handle = handle;
	}
}

ScopedHandle::ScopedHandle(int dwProcess, bool bAutoClose)
	: enableAutoClose(bAutoClose)
{
	openProcess(static_cast<DWORD>(dwProcess));
}

ScopedHandle::ScopedHandle(HANDLE_TYPE h, HANDLE ProcessHandle, PVOID StartRoutine, PVOID Argument)
	: enableAutoClose(true)
{
	if (h == CREATE_REMOTE_THREAD)
		createThreadEx(ProcessHandle, StartRoutine, Argument);
}

ScopedHandle::ScopedHandle(HANDLE_TYPE h, HANDLE hSourceProcessHandle, HANDLE hSourceHandle, HANDLE hTargetProcessHandle, DWORD dwOptions)
	: enableAutoClose(true)
{
	if (h == DUPLICATE_HANDLE)
		duplicateHandle(hSourceProcessHandle, hSourceHandle, hTargetProcessHandle, dwOptions);
}

ScopedHandle::~ScopedHandle()
{
	if (enableAutoClose)
		this->closeHandle();
}

void ScopedHandle::closeHandle()
{
	QWriteLocker locker(&m_lock);
	HANDLE h = this->m_handle;
	if (!h
		|| ((h) == INVALID_HANDLE_VALUE)
		|| ((h) == ::GetCurrentProcess()))
		return;

	BOOL ret = NT_SUCCESS(MINT::NtClose(h));
	if (ret)
	{
		subHandleCount();
		//print << "CloseHandle: " << (h) << "Total Handle:" << getHandleCount() << Qt::endl;
		this->m_handle = nullptr;
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
	ClientId.UniqueProcess = (PVOID)dwProcess;
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
	ClientIds.UniqueProcess = (HANDLE)dwProcess;
	ClientIds.UniqueThread = (HANDLE)0;
	BOOL ret = NT_SUCCESS(MINT::ZwOpenProcess(&ProcessHandle, PROCESS_ALL_ACCESS, &ObjectAttribute,
		&ClientIds));

	return ret && ProcessHandle && ((ProcessHandle) != INVALID_HANDLE_VALUE) ? ProcessHandle : nullptr;
};

void ScopedHandle::openProcess(DWORD dwProcess)
{
	QWriteLocker locker(&m_lock);
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
		//print << "openProcess:" << (hprocess) << "Total Handle:" << getHandleCount() << Qt::endl;
		this->m_handle = hprocess;
		hprocess = nullptr;
	}
	else
		this->m_handle = nullptr;
}

void ScopedHandle::createToolhelp32Snapshot(DWORD dwFlags, DWORD th32ProcessID)
{

	QWriteLocker locker(&m_lock);
	HANDLE hSnapshot = ::CreateToolhelp32Snapshot(dwFlags, th32ProcessID);
	if (hSnapshot && ((hSnapshot) != INVALID_HANDLE_VALUE))
	{
		addHandleCount();
		//print << "createToolhelp32Snapshot:" << (hSnapshot) << "Total Handle:" << getHandleCount() << Qt::endl;
		this->m_handle = hSnapshot;
		hSnapshot = nullptr;
	}
	else
		this->m_handle = nullptr;
}


void ScopedHandle::openProcessToken(HANDLE ProcessHandle, ACCESS_MASK DesiredAccess)
{
	QWriteLocker locker(&m_lock);
	HANDLE hToken = nullptr;
	BOOL ret = NT_SUCCESS(MINT::NtOpenProcessToken(ProcessHandle, DesiredAccess, &hToken));

	if (ret && hToken && ((hToken) != INVALID_HANDLE_VALUE))
	{
		addHandleCount();
		//print << "openProcessToken:" << (hToken) << "Total Handle:" << getHandleCount() << Qt::endl;
		this->m_handle = hToken;
		hToken = nullptr;
	}
	else
		this->m_handle = nullptr;
}

// 提權函數：提升為DEBUG權限
BOOL ScopedHandle::EnableDebugPrivilege(HANDLE hProcess, const wchar_t* SE)
{
	if (!hProcess) return FALSE;
	BOOL fOk = FALSE;
	do
	{
		this->openProcessToken(hProcess, TOKEN_ALL_ACCESS);
		if (!isValid()) break;

		TOKEN_PRIVILEGES tp = {};

		if (LookupPrivilegeValue(NULL, SE, &tp.Privileges[0].Luid) == FALSE)
			break;

		tp.PrivilegeCount = 1;
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		MINT::NtAdjustPrivilegesToken(this->m_handle, FALSE, &tp, sizeof(tp), NULL, NULL);

		fOk = (::GetLastError() == ERROR_SUCCESS);
	} while (false);
	return fOk;
}

void ScopedHandle::createThreadEx(HANDLE ProcessHandle, PVOID StartRoutine, PVOID Argument)
{
	QWriteLocker locker(&m_lock);
	HANDLE hThread = nullptr;
	BOOL ret = NT_SUCCESS(MINT::NtCreateThreadEx(&hThread, THREAD_ALL_ACCESS, NULL, ProcessHandle, StartRoutine, Argument, FALSE, NULL, NULL, NULL, NULL));
	if (ret && hThread && ((hThread) != INVALID_HANDLE_VALUE))
	{
		LARGE_INTEGER pTimeout = {};
		pTimeout.QuadPart = -1ll * 10000000ll;
		pTimeout.QuadPart = -1ll * 10000000ll;
		MINT::NtWaitForSingleObject(hThread, TRUE, &pTimeout);
		addHandleCount();
		//print << "createThreadEx:" << (hThread) << "Total Handle:" << getHandleCount() << Qt::endl;
		this->m_handle = hThread;
		hThread = nullptr;
	}
	else
		this->m_handle = nullptr;
}

void ScopedHandle::duplicateHandle(HANDLE hSourceProcessHandle, HANDLE hSourceHandle, HANDLE hTargetProcessHandle, DWORD dwOptions)
{
	QWriteLocker locker(&m_lock);
	HANDLE hTargetHandle = nullptr;
	//BOOL ret = ::DuplicateHandle(hSourceProcessHandle, hSourceHandle, hTargetProcessHandle, &hTargetHandle, 0, FALSE, dwOptions);
	BOOL ret = NT_SUCCESS(MINT::NtDuplicateObject(hSourceProcessHandle, hSourceHandle, hTargetProcessHandle, &hTargetHandle, 0, FALSE, dwOptions));
	if (ret && hTargetHandle && ((hTargetHandle) != INVALID_HANDLE_VALUE))
	{
		addHandleCount();
		//print << "duplicateHandle:" << (hTargetHandle) << "Total Handle:" << getHandleCount() << Qt::endl;
		this->m_handle = hTargetHandle;
		hTargetHandle = nullptr;
	}
	else
		this->m_handle = nullptr;
}

void ScopedHandle::createEvent()
{
	QWriteLocker locker(&m_lock);
	HANDLE hEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hEvent && ((hEvent) != INVALID_HANDLE_VALUE))
	{
		addHandleCount();
		//print << "createEvent:" << (hEvent) << "Total Handle:" << getHandleCount() << Qt::endl;
		this->m_handle = hEvent;
		hEvent = nullptr;
	}
	else
		this->m_handle = nullptr;
}