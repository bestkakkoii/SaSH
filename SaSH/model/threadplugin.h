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

#pragma once

#include <QThread>
#include <QObject>
#include <QWaitCondition>
#include <QMutex>
#include <QReadWriteLock>

#include "signaldispatcher.h"
#include <indexer.h>
#include <shared_mutex>

#include <exception>
#include <stdexcept>

class ExceptionHandler
{
public:
	ExceptionHandler()
	{
		std::set_terminate(ExceptionHandler::handleTerminate);
		std::set_new_handler(ExceptionHandler::handleOutOfMemory);
	}

	virtual ~ExceptionHandler()
	{
		std::set_terminate(nullptr);
		std::set_new_handler(nullptr);
	}

	static void handleTerminate()
	{
		try
		{
			std::rethrow_exception(std::current_exception());
		}
		catch (const std::exception& e)
		{
			QString errorMessage = QString("Terminating due to unhandled std::exception: %1").arg(QString::fromUtf8(e.what()));
			std::wstring errorMessageW = errorMessage.toStdWString();
			showErrorMessageBox(errorMessageW);

			if (shouldTerminateProgram())
			{
				std::terminate();
			}
		}
		catch (...)
		{
			showErrorMessageBox(L"Terminating due to unknown exception.");

			if (shouldTerminateProgram())
			{
				std::terminate();
			}
		}
	}

	static void handleUnexpected()
	{
		showErrorMessageBox(L"Unexpected exception.");
		if (shouldTerminateProgram())
			std::terminate();
	}

	static void handleOutOfMemory()
	{
		showErrorMessageBox(L"Out of memory.");
		std::exit(EXIT_FAILURE);
	}

	static bool shouldTerminateProgram()
	{
		QStringList list = printStackTrace();
		if (!list.isEmpty())
		{
			QString stackTrace = list.join("\n");
			std::wstring stackTraceW = stackTrace.toStdWString();
			showErrorMessageBox(stackTraceW);
			return false;
		}
		return true;
	}

private:
	static QStringList printStackTrace()
	{
		QStringList out;
		HANDLE process = GetCurrentProcess();
		SymInitialize(process, NULL, TRUE);

		CONTEXT context;
		memset(&context, 0, sizeof(CONTEXT));
		context.ContextFlags = CONTEXT_FULL;
		RtlCaptureContext(&context);

		STACKFRAME stackFrame;
		memset(&stackFrame, 0, sizeof(STACKFRAME));
		DWORD machineType = IMAGE_FILE_MACHINE_I386;

#if defined(_M_X64) || defined(__amd64__)
		machineType = IMAGE_FILE_MACHINE_AMD64;
#endif

		stackFrame.AddrPC.Offset = context.Eip;
		stackFrame.AddrPC.Mode = AddrModeFlat;
		stackFrame.AddrFrame.Offset = context.Ebp;
		stackFrame.AddrFrame.Mode = AddrModeFlat;
		stackFrame.AddrStack.Offset = context.Esp;
		stackFrame.AddrStack.Mode = AddrModeFlat;

		SYMBOL_INFO* symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
		if (symbol)
		{
			symbol->MaxNameLen = 255;
			symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

			while (StackWalk(machineType, process, GetCurrentThread(), &stackFrame, &context, NULL, SymFunctionTableAccess, SymGetModuleBase, NULL))
			{
				SymFromAddr(process, stackFrame.AddrPC.Offset, NULL, symbol);

				QString stackEntry = QString("%1: %2 - 0x%3").arg(out.size()).arg(QString::fromUtf8(symbol->Name)).arg(symbol->Address, 0, 16);
				out.append(stackEntry);
			}

			free(symbol);
		}

		SymCleanup(process);
		return out;
	}


	static void showErrorMessageBox(const std::wstring& message)
	{
		MessageBoxW(nullptr, message.c_str(), L"Fatal Error", MB_OK | MB_ICONERROR);
	}
};

class ThreadPlugin : public QObject, public Indexer
{
	Q_OBJECT
public:
	ThreadPlugin(qint64 index, QObject* parent)
		: QObject(parent)
		, Indexer(index)
	{
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(index);
		connect(&signalDispatcher, &SignalDispatcher::nodifyAllStop, this, &ThreadPlugin::requestInterruption);
	}

	virtual ~ThreadPlugin() = default;

	Q_REQUIRED_RESULT inline bool isInterruptionRequested() const
	{
		std::shared_lock<std::shared_mutex> lock(rwLock_);
		return isInterruptionRequested_.load(std::memory_order_acquire);
	}

	inline bool isPaused() const { return isPaused_; }

	inline void checkPause()
	{
		std::unique_lock<std::mutex> lock(pausedMutex_);
		if (isPaused_.load(std::memory_order_acquire))
		{
			pausedCondition_.wait(lock);
		}
	}

public slots:
	void requestInterruption()
	{
		std::unique_lock<std::shared_mutex> lock(rwLock_);
		isInterruptionRequested_.store(true, std::memory_order_release);
	}

	void paused()
	{
		pausedMutex_.lock();
		isPaused_.store(true, std::memory_order_release);
		pausedMutex_.unlock();
	}

	void resumed()
	{
		{
			pausedMutex_.lock();
			isPaused_.store(false, std::memory_order_release);
			pausedMutex_.unlock();
		}
		pausedCondition_.notify_all();
	}

private:
	QSharedPointer<ExceptionHandler> exceptionHandler_ = QSharedPointer<ExceptionHandler>::create();

	std::atomic_bool isInterruptionRequested_ = false;
	mutable std::shared_mutex rwLock_;

	std::atomic_bool isPaused_ = false;
	std::condition_variable pausedCondition_;
	std::mutex pausedMutex_;
};