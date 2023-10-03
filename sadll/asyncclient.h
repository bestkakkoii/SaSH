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

#ifndef ASYNCCLIENT_H
#define ASYNCCLIENT_H

#include <iostream>
#include <vector>
#include <string>
#include <WinSock2.h>
#include <WS2tcpip.h>

#if _MSVC_LANG >= 201703L
#include <memory_resource>
#endif
#include <queue>
#include <mutex>
#include <condition_variable>

#include <MINT/MINT.h>

constexpr const wchar_t* IPV6_DEFAULT = L"::1";
constexpr const wchar_t* IPV4_DEFAULT = L"127.0.0.1";

class AsyncClient
{
private:
	struct SendBuffer {
		char* data = nullptr;
		size_t length = 0u;
	};

public:
	AsyncClient(HWND parentHwnd)
		: parendHwnd_(parentHwnd)
	{
		WSADATA data = {};
		do
		{
			if (WSAStartup(MAKEWORD(2ui16, 2ui16), &data) != 0)
			{
				std::ignore = recordWSALastError(__LINE__);
				break;
			}

			std::wcout << L"WSAStartup OK" << std::endl;

			// Create completion port
			completionPort_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0UL, 0UL);
			if (nullptr == completionPort_)
			{
				std::ignore = recordWinLastError(__LINE__);
				break;
			}

			std::wcout << L"CreateIoCompletionPort OK" << std::endl;
			resource_.reset(new std::pmr::monotonic_buffer_resource(65536u));
			allocator_.reset(new std::pmr::polymorphic_allocator<OVERLAPPED>(resource_.get()));
			allocatorChar_.reset(new std::pmr::polymorphic_allocator<char>(resource_.get()));
			return;
		} while (false);

		WSACleanup();
	}

	virtual ~AsyncClient()
	{
		if (sendThread_ != nullptr)
		{
			sendThread_->join();
			delete sendThread_;
			sendThread_ = nullptr;
		}
		handleConnectError();
		WSACleanup();
	}

	inline void setCloseSocketFunction(int(__stdcall* p)(SOCKET s))
	{
		pclosesocket_ = p;
	}

	inline BOOL asyncConnect(unsigned short type, unsigned short serverPort)
	{
		ADDRESS_FAMILY family = AF_UNSPEC;
		u_short port = htons(serverPort);
		void* pAddrBuf = nullptr;
		sockaddr* serverAddr = nullptr;
		int namelen = 0;
		wchar_t wcsServerIP[256] = {};
		std::fill_n(wcsServerIP, 256, L'\0');

		do
		{
			if (type > 0)
			{
				family = AF_INET6;

				sockaddr_in6 serverAddr6 = {};
				serverAddr6.sin6_family = family;
				serverAddr6.sin6_port = port;
				pAddrBuf = reinterpret_cast<void*>(&serverAddr6.sin6_addr);
				serverAddr = reinterpret_cast<sockaddr*>(&serverAddr6);
				namelen = sizeof(serverAddr6);

				_snwprintf_s(wcsServerIP, _countof(wcsServerIP), _TRUNCATE, L"%s", IPV6_DEFAULT);

				std::wcout << L"current type: IPV6: " << std::wstring(wcsServerIP) << std::endl;
			}
			else
			{
				family = AF_INET;

				sockaddr_in serverAddr4 = {};
				serverAddr4.sin_family = family;
				serverAddr4.sin_port = port;
				pAddrBuf = reinterpret_cast<void*>(&serverAddr4.sin_addr);
				serverAddr = reinterpret_cast<sockaddr*>(&serverAddr4);
				namelen = sizeof(serverAddr4);

				_snwprintf_s(wcsServerIP, _countof(wcsServerIP), _TRUNCATE, L"%s", IPV4_DEFAULT);

				std::wcout << L"current type: IPV4: " << std::wstring(wcsServerIP) << std::endl;
			}

			clientSocket_ = WSASocketW(family, SOCK_STREAM, 0, nullptr, 0u, WSA_FLAG_OVERLAPPED);
			if (INVALID_SOCKET == clientSocket_)
			{
				recordWSALastError(__LINE__);
				break;
			}

			std::wcout << "WSASocketW OK" << std::endl;

			if (InetPtonW(family, wcsServerIP, pAddrBuf) != 1)
			{
				recordWSALastError(__LINE__);
				break;
			}

			std::wcout << "InetNtopW OK" << std::endl;

			if (WSAConnect(clientSocket_, serverAddr, namelen, nullptr, nullptr, nullptr, nullptr) != 0)
			{
				if (recordWSALastError(__LINE__) != WSAEWOULDBLOCK)
					break;
			}

			std::wcout << "WSAConnect OK" << std::endl;

			// Associate the socket with the completion port
			if (CreateIoCompletionPort(reinterpret_cast<HANDLE>(clientSocket_), completionPort_, 0UL, 0UL) == nullptr)
			{
				std::ignore = recordWinLastError(__LINE__);
				break;
			}

			std::wcout << "CreateIoCompletionPort OK" << std::endl;

			if (nullptr == parendHwnd_)
			{
#ifdef _DEBUG
				std::wcout << L"g_ParenthWnd is nullptr." << std::endl;
#endif
				break;
			}

			DWORD_PTR result = 0UL;
			if ((SendMessageTimeoutW(parendHwnd_, kConnectionOK, NULL, NULL, SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, 10000u, &result) == 0L)
				|| (0UL == result))
			{
				std::ignore = recordWinLastError(__LINE__);
				break;
			}

#ifdef _DEBUG
			std::wcout << L"Notified parent window OK" << std::endl;
#endif

			startParallelProcessing();
			return TRUE;
		} while (false);

		handleConnectError();
		return FALSE;
	}

	inline BOOL start()
	{
		DWORD threadId;
		completionThread_ = CreateThread(nullptr, 0u, completionThreadProc, reinterpret_cast<LPVOID>(this), 0UL, &threadId);
		if (completionThread_ == nullptr)
		{
			std::ignore = recordWinLastError(__LINE__);
			return FALSE;
		}

		return TRUE;
	}

	inline BOOL asyncSend(const char* dataBuf, size_t dataLen)
	{
		if (clientSocket_ == INVALID_SOCKET)
			return FALSE;

		if ((nullptr == dataBuf) || (dataLen <= 0))
			return FALSE;

		OVERLAPPED* overlapped = getOverlapped();

		if (nullptr == overlapped)
			return FALSE;

		WSABUF wsabuf = { static_cast<DWORD>(dataLen), const_cast<char*>(dataBuf) };

		if (SOCKET_ERROR == WSASend(clientSocket_, &wsabuf, 1UL, nullptr, 0UL, overlapped, nullptr))
		{
			if (recordWSALastError(__LINE__) != WSA_IO_PENDING)
			{
				releaseOverlapped(overlapped);
				MINT::NtTerminateProcess(GetCurrentProcess(), 0);
			}
		}

		return TRUE;
	}

	inline void asyncReceive(LPWSABUF wsabuf)
	{
		if (INVALID_SOCKET == clientSocket_)
			return;

		if (nullptr == wsabuf)
			return;

		OVERLAPPED* overlapped = getOverlapped();

		if (nullptr == overlapped)
			return;

		char* buffer = getBuffer(wsabuf->len);

		if (nullptr == buffer)
			return;

		wsabuf->buf = buffer;

		DWORD flags = 0;

		if (SOCKET_ERROR == WSARecv(clientSocket_, wsabuf, 1UL, nullptr, &flags, overlapped, nullptr))
		{
			if (recordWSALastError(__LINE__) != WSA_IO_PENDING)
			{
				releaseOverlapped(overlapped);
				releaseBuffer(buffer);
			}

			return;
		}
	}

	void queueSendData(const char* data, size_t length)
	{
		SendBuffer buffer;
		buffer.data = getBuffer(length);
		buffer.length = length;
		std::memcpy(buffer.data, data, length);

		{
			std::lock_guard<std::mutex> lock(sendMutex_);
			sendQueue_.push(std::move(buffer));
		}

		sendCondition_.notify_one();
	}

	inline void startParallelProcessing()
	{
		sendThread_ = new std::thread([this]()
			{
				for (;;)
				{
					SendBuffer buffer;
					{
						std::unique_lock<std::mutex> lock(sendMutex_);
						sendCondition_.wait(lock, [this] { return !sendQueue_.empty(); });
						buffer = std::move(sendQueue_.front());
						sendQueue_.pop();
					}

					if (FALSE == asyncSend(buffer.data, buffer.length))
					{
						releaseBuffer(buffer.data);
						break;
					}

					releaseBuffer(buffer.data);
				}
			});
	}


private:
	static DWORD CALLBACK completionThreadProc(LPVOID lpParam)
	{
		AsyncClient* client = reinterpret_cast<AsyncClient*>(lpParam);
		if (nullptr == client)
			return 1UL;

		for (;;)
		{
			DWORD numBytesTransferred;
			ULONG_PTR completionKey;
			OVERLAPPED* overlapped;

			BOOL result = GetQueuedCompletionStatus(client->completionPort_, &numBytesTransferred, &completionKey, &overlapped, INFINITE);
			std::ignore = client->recordWinLastError(__LINE__);
			if ((FALSE == result) || (0UL == numBytesTransferred))
			{
				if (overlapped != nullptr)
				{
					client->releaseOverlapped(overlapped);
				}
				break;
			}

			if (overlapped != nullptr)
			{
				// Handle the completed I/O operation
				if (0UL == overlapped->Internal)
				{
					if (0UL == overlapped->Offset)
					{
						// Sent data
#ifdef _DEBUG
						std::cout << "Sent: " << std::to_string(numBytesTransferred) << " bytes" << std::endl;
#endif
				}
					else
					{
						// Received data
#ifdef _DEBUG
						std::cout << "Received: " << std::to_string(numBytesTransferred) << " bytes" << std::endl;
#endif
			}
		}
				else
				{
					// I/O operation failed
					client->lastError_ = static_cast<DWORD>(overlapped->Internal);
#ifdef _DEBUG
					std::wcout << L"I/O operation failed with error" << std::endl;
#endif
				}

				client->releaseOverlapped(overlapped);
	}
}

		return 0UL;
	}

	[[nodiscard]] inline OVERLAPPED* getOverlapped()
	{
		OVERLAPPED* ptr = allocator_->allocate(1);
		std::allocator_traits<std::pmr::polymorphic_allocator<OVERLAPPED>>::construct(*allocator_, ptr, OVERLAPPED{});
		return ptr;
	}

	inline void releaseOverlapped(OVERLAPPED* overlapped)
	{
		allocator_->deallocate(overlapped, 1);
	}

	[[nodiscard]] inline char* getBuffer(size_t size)
	{
		char* ptr = allocatorChar_->allocate(size);
		std::allocator_traits<std::pmr::polymorphic_allocator<char>>::construct(*allocatorChar_, ptr, '\0');
		return ptr;
	}

	inline void releaseBuffer(char* buffer)
	{
		allocatorChar_->deallocate(buffer, 1);
	}

	inline DWORD recordWSALastError(int line)
	{
		lastError_ = static_cast<DWORD>(WSAGetLastError());
		if (lastError_ != 0UL)
		{
			std::wcout << L"WSA error: " << std::to_wstring(lastError_) << L" at line " << std::to_wstring(line) << L" detail: " << getLastErrorStringW() << std::endl;
		}

		return lastError_;
	}

	inline DWORD recordWinLastError(int line)
	{
		lastError_ = GetLastError();
		if (lastError_ != 0UL)
		{
			std::wcout << L"Winapi error: " << std::to_wstring(lastError_) << L" at line " << std::to_wstring(line) << L" detail: " << getLastErrorStringW() << std::endl;
		}
		return lastError_;
	}

	[[nodiscard]] inline std::wstring getLastErrorStringW()
	{
		if (0UL == lastError_)
			return L"";

		wchar_t* pwcstr = nullptr;
		FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr, lastError_, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPWSTR>(&pwcstr), 0UL, nullptr);

		std::wstring result;
		if (pwcstr != nullptr)
		{
			result = pwcstr;
			LocalFree(pwcstr);
		}
		return result;
	}

	void handleConnectError()
	{
		if (clientSocket_ != INVALID_SOCKET)
		{
			if (pclosesocket_ != nullptr)
				pclosesocket_(clientSocket_);
			clientSocket_ = INVALID_SOCKET;
		}

		if (completionPort_ != nullptr)
		{
			MINT::NtClose(completionPort_);
			completionPort_ = nullptr;
		}
	}

private:
	SOCKET clientSocket_ = INVALID_SOCKET;
	HANDLE completionPort_ = nullptr;
	HANDLE completionThread_ = nullptr;
	DWORD lastError_ = 0;
	HWND parendHwnd_ = nullptr;
	int(__stdcall* pclosesocket_)(SOCKET s) = nullptr;

	std::mutex sendMutex_;
	std::condition_variable sendCondition_;
	std::queue<SendBuffer> sendQueue_;
	std::thread* sendThread_ = nullptr;

#if _MSVC_LANG >= 201703L
	std::unique_ptr<std::pmr::monotonic_buffer_resource> resource_;
	std::unique_ptr<std::pmr::polymorphic_allocator<OVERLAPPED>> allocator_;
	std::unique_ptr<std::pmr::polymorphic_allocator<char>> allocatorChar_;
#endif
};

#endif //ASYNCCLIENT_H