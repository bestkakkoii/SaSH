#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>

class AsyncClient
{
private:
	SOCKET clientSocket_ = INVALID_SOCKET;
	HANDLE completionPort_ = nullptr;
	DWORD lastError_ = 0;
	std::vector<OVERLAPPED*> overlappedPool_;
	std::vector<char*> bufferPool_;
	HANDLE completionThread_ = nullptr;

public:
	AsyncClient()
	{
		WSADATA data;
		if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
		{
			lastError_ = WSAGetLastError();
#ifdef _DEBUG
			std::cout << "WSAStartup failed with error: " << WSAGetLastError() << std::endl;
#else
			std::wstring error = GetLastError() + L" (@" + std::to_wstring(__LINE__) + L")";
			MessageBox(nullptr, error.c_str(), L"Error", MB_OK | MB_ICONERROR);
#endif
			return;
		}

		// Create completion port
		completionPort_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
		if (completionPort_ == nullptr)
		{
			lastError_ = ::GetLastError();
#ifdef _DEBUG
			std::wcout << L"CreateIoCompletionPort failed with error: " << GetLastError() << std::endl;
#else
			std::wstring error = GetLastError() + L" (@" + std::to_wstring(__LINE__) + L")";
			MessageBox(nullptr, error.c_str(), L"Error", MB_OK | MB_ICONERROR);
#endif
			WSACleanup();
			return;
		}
	}

	virtual ~AsyncClient()
	{
		CloseHandle(completionPort_);

		for (OVERLAPPED* overlapped : overlappedPool_)
			delete overlapped;

		for (char* buffer : bufferPool_)
			delete[] buffer;

		WSACleanup();
	}

	bool Connect(const std::string& serverIP, unsigned short serverPort)
	{
		GameService& g_GameService = GameService::getInstance();
		clientSocket_ = WSASocket(AF_INET6, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
		if (clientSocket_ == INVALID_SOCKET)
		{
			lastError_ = WSAGetLastError();
#ifdef _DEBUG
			std::cout << "WSASocket failed with error: " << WSAGetLastError() << std::endl;
#else
			std::wstring error = GetLastError() + L" (@" + std::to_wstring(__LINE__) + L")";
			MessageBox(nullptr, error.c_str(), L"Error", MB_OK | MB_ICONERROR);
#endif
			return false;
		}

		sockaddr_in6 serverAddr{};
		serverAddr.sin6_family = AF_INET6;
		serverAddr.sin6_port = htons(serverPort);

		if (inet_pton(AF_INET6, serverIP.c_str(), &(serverAddr.sin6_addr)) <= 0)
		{
			lastError_ = WSAGetLastError();
#ifdef _DEBUG
			std::cout << "inet_pton failed. Error Code : " << WSAGetLastError() << std::endl;
#else
			std::wstring error = GetLastError() + L" (@" + std::to_wstring(__LINE__) + L")";
			MessageBox(nullptr, error.c_str(), L"Error", MB_OK | MB_ICONERROR);
#endif
			g_GameService.pclosesocket(clientSocket_);
			return false;
		}

		if (g_GameService.pconnect(clientSocket_, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) != 0)
		{
			lastError_ = WSAGetLastError();
			if (lastError_ != WSAEWOULDBLOCK)
			{
#ifdef _DEBUG
				std::cout << "connect failed. Error Code : " << WSAGetLastError() << std::endl;
#else
				std::wstring error = GetLastError() + L" (@" + std::to_wstring(__LINE__) + L")";
				MessageBox(nullptr, error.c_str(), L"Error", MB_OK | MB_ICONERROR);
#endif
				g_GameService.pclosesocket(clientSocket_);
				return false;
			}
		}

		// Associate the socket with the completion port
		if (CreateIoCompletionPort(reinterpret_cast<HANDLE>(clientSocket_), completionPort_, 0, 0) == nullptr)
		{
			lastError_ = ::GetLastError();
#ifdef _DEBUG
			std::wcout << L"CreateIoCompletionPort failed with error: " << GetLastError() << std::endl;
#else
			std::wstring error = GetLastError() + L" (@" + std::to_wstring(__LINE__) + L")";
			MessageBox(nullptr, error.c_str(), L"Error", MB_OK | MB_ICONERROR);
#endif
			g_GameService.pclosesocket(clientSocket_);
			return false;
		}

#ifdef _DEBUG
		std::cout << "Connect success." << std::endl;
#endif
		extern HWND g_ParenthWnd;//sadll.cpp
		SendMessageW(g_ParenthWnd, util::kConnectionOK, NULL, NULL);

		return true;
	}

	void Start()
	{
		DWORD threadId;
		completionThread_ = CreateThread(nullptr, 0, CompletionThreadProc, this, 0, &threadId);
		if (completionThread_ == nullptr)
		{
			lastError_ = ::GetLastError();
#ifdef _DEBUG
			std::wcout << L"CreateThread failed with error: " << GetLastError() << std::endl;
#else
			std::wstring error = GetLastError() + L" (@" + std::to_wstring(__LINE__) + L")";
			MessageBox(nullptr, error.c_str(), L"Error", MB_OK | MB_ICONERROR);
#endif
			return;
		}
	}

	void Send(const char* dataBuf, int dataLen)
	{
		if (clientSocket_ == INVALID_SOCKET)
			return;

		if (dataBuf == nullptr || dataLen <= 0)
			return;

		OVERLAPPED* overlapped = GetOverlapped();
		char* buffer = GetBuffer(dataLen);

		memset(overlapped, 0, sizeof(OVERLAPPED));
		memset(buffer, 0, dataLen);
		memcpy_s(buffer, dataLen, dataBuf, dataLen);

		WSABUF wsabuf = { (DWORD)dataLen , buffer };

		if (WSASend(clientSocket_, &wsabuf, 1, nullptr, 0, overlapped, nullptr) == SOCKET_ERROR)
		{
			lastError_ = WSAGetLastError();
			if (lastError_ != WSA_IO_PENDING)
			{
#ifdef _DEBUG
				std::cout << "WSASend failed with error: " << lastError_ << std::endl;
#else
				std::wstring error = GetLastError() + L" (@" + std::to_wstring(__LINE__) + L")";
				MessageBox(nullptr, error.c_str(), L"Error", MB_OK | MB_ICONERROR);
#endif
				ReleaseOverlapped(overlapped);
				ReleaseBuffer(buffer);
				MINT::NtTerminateProcess(GetCurrentProcess(), 0);
			}
		}
	}

	void Receive(LPWSABUF wsabuf)
	{
		if (clientSocket_ == INVALID_SOCKET)
			return;

		if (wsabuf == nullptr)
			return;

		OVERLAPPED* overlapped = GetOverlapped();
		char* buffer = GetBuffer(wsabuf->len);

		memset(overlapped, 0, sizeof(OVERLAPPED));
		memset(buffer, 0, wsabuf->len);

		wsabuf->buf = buffer;

		DWORD flags = 0;

		if (WSARecv(clientSocket_, wsabuf, 1, nullptr, &flags, overlapped, nullptr) == SOCKET_ERROR)
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				lastError_ = WSAGetLastError();
#ifdef _DEBUG
				std::cout << "WSARecv failed with error: " << lastError_ << std::endl;
#endif
				ReleaseOverlapped(overlapped);
				ReleaseBuffer(buffer);
			}

			return;
		}
	}

	std::wstring GetLastError()
	{
		if (lastError_ == 0)
		{
			return L"";
		}

		wchar_t* msg = nullptr;
		FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr, lastError_, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&msg, 0, nullptr);

		std::wstring result(msg);
		LocalFree(msg);
		return result;
	}

private:
	static DWORD CALLBACK CompletionThreadProc(LPVOID lpParam)
	{
		AsyncClient* client = reinterpret_cast<AsyncClient*>(lpParam);
		if (client == nullptr)
			return 1;

		for (;;)
		{
			DWORD numBytesTransferred;
			ULONG_PTR completionKey;
			OVERLAPPED* overlapped;

			BOOL result = GetQueuedCompletionStatus(client->completionPort_, &numBytesTransferred, &completionKey, &overlapped, INFINITE);

			if (!result || numBytesTransferred == 0)
			{
				if (overlapped != nullptr)
				{
					client->lastError_ = WSAGetLastError();
#ifdef _DEBUG
					std::cout << "I/O operation failed with error: " << client->lastError_ << std::endl;
#else
					std::wstring error = client->GetLastError() + L" (@" + std::to_wstring(__LINE__) + L")";
					MessageBox(nullptr, error.c_str(), L"Error", MB_OK | MB_ICONERROR);
#endif
					client->ReleaseOverlapped(overlapped);
				}
				break;
			}

			if (overlapped != nullptr)
			{
				// Handle the completed I/O operation
				if (overlapped->Internal == 0)
				{
					if (overlapped->Offset == 0)
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
					std::cout << "I/O operation failed with error: " << client->lastError_ << std::endl;
#else
					std::wstring error = client->GetLastError() + L" (@" + std::to_wstring(__LINE__) + L")";
					MessageBox(nullptr, error.c_str(), L"Error", MB_OK | MB_ICONERROR);
#endif
				}

				client->ReleaseOverlapped(overlapped);
			}
		}

		return 0;
	}

	OVERLAPPED* GetOverlapped()
	{
		OVERLAPPED* overlapped;
		if (overlappedPool_.empty())
		{
			overlapped = new OVERLAPPED();
		}
		else
		{
			overlapped = overlappedPool_.back();
			overlappedPool_.pop_back();
		}
		return overlapped;
	}

	inline void ReleaseOverlapped(OVERLAPPED* overlapped)
	{
		overlappedPool_.push_back(overlapped);
	}

	char* GetBuffer(size_t size)
	{
		char* buffer;
		if (bufferPool_.empty())
		{
			buffer = new char[size];
		}
		else
		{
			buffer = bufferPool_.back();
			bufferPool_.pop_back();
		}
		return buffer;
	}

	inline void ReleaseBuffer(char* buffer)
	{
		bufferPool_.push_back(buffer);
	}
};

//int main()
//{
//	AsyncClient client;
//	if (client.Connect("127.0.0.1", 8080))
//	{
//		client.Start();
//	}
//
//	return 0;
//}
