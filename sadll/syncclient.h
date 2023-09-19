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

#ifndef SYNCCLIENT_H
#define SYNCCLIENT_H

#if 0
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>

class SyncClient
{
	DISABLE_COPY_MOVE(SyncClient)
private:
	SOCKET clientSocket = INVALID_SOCKET;
	DWORD lastError_ = 0;

public:
	SyncClient()
	{
		WSADATA data;
		if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
		{
#ifdef _DEBUG
			std::cout << "WSAStartup failed with error: " << WSAGetLastError() << std::endl;
#endif
			return;
		}
	}

	virtual ~SyncClient()
	{
		SyncClient& instance = getInstance();

		closesocket(instance.clientSocket);
		WSACleanup();

	}

	bool Connect(const std::string& serverIP, unsigned short serverPort)
	{
		GameService& g_GameService = GameService::getInstance();
		SyncClient& instance = getInstance();
		instance.clientSocket = ::socket(AF_INET6, SOCK_STREAM, 0);//g_GameService.psocket(AF_INET6, SOCK_STREAM, 0);
		if (instance.clientSocket == INVALID_SOCKET)
		{
#ifdef _DEBUG
			std::cout << "socket failed with error: " << WSAGetLastError() << std::endl;
#endif
			WSACleanup();
			return false;
		}

		int bufferSize = 8191;
		int minBufferSize = 1;
		setsockopt(instance.clientSocket, SOL_SOCKET, SO_RCVBUF, (char*)&bufferSize, sizeof(bufferSize));
		setsockopt(instance.clientSocket, SOL_SOCKET, SO_SNDBUF, (char*)&bufferSize, sizeof(bufferSize));
		setsockopt(instance.clientSocket, SOL_SOCKET, SO_RCVLOWAT, (char*)&minBufferSize, sizeof(minBufferSize));
		setsockopt(instance.clientSocket, SOL_SOCKET, SO_SNDLOWAT, (char*)&minBufferSize, sizeof(minBufferSize));
		//DWORD timeout = 5000; // 超時時間為 5000 毫秒
		//setsockopt(clientSocket, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));
		//setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

		LINGER lingerOption = { 0, 0 };
		lingerOption.l_onoff = 0;        // 立即中止
		lingerOption.l_linger = 0;      // 延遲時間不適用
		setsockopt(instance.clientSocket, SOL_SOCKET, SO_LINGER, (char*)&lingerOption, sizeof(lingerOption));
		int keepAliveOption = 1;  // 開啟保持活動機制
		setsockopt(instance.clientSocket, SOL_SOCKET, TCP_KEEPALIVE, (char*)&keepAliveOption, sizeof(keepAliveOption));

		sockaddr_in6 serverAddr{};
		serverAddr.sin6_family = AF_INET6;
		serverAddr.sin6_port = htons(serverPort);

		if (inet_pton(AF_INET6, serverIP.c_str(), &(serverAddr.sin6_addr)) <= 0)
		{
#ifdef _DEBUG
			std::cout << "inet_pton failed. Error Code : " << WSAGetLastError() << std::endl;
#endif
			closesocket(instance.clientSocket);
			return false;
		}

		if (connect(instance.clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) != 0)
		{
#ifdef _DEBUG
			std::cout << "connect failed. Error Code : " << WSAGetLastError() << std::endl;
#endif
			closesocket(instance.clientSocket);
			return false;
		}
		extern HWND g_ParenthWnd;
#ifdef _DEBUG
		std::cout << "connect success. " << g_ParenthWnd << " " << SendMessageW(g_ParenthWnd, util::kConnectionOK, NULL, NULL) << std::endl;
#else
		SendMessageW(g_ParenthWnd, util::kConnectionOK, NULL, NULL);
#endif

		return true;
	}

	int Send(char* dataBuf, int dataLen)
	{
		SyncClient& instance = getInstance();
		if (INVALID_SOCKET == instance.clientSocket)
			return false;

		if (dataBuf == nullptr)
			return false;

		if (dataLen <= 0)
			return false;

		GameService& g_GameService = GameService::getInstance();
		//std::unique_ptr <char[]> buf(new char[dataLen]());
		//memset(buf.get(), 0, dataLen);
		//memcpy_s(buf.get(), dataLen, dataBuf, dataLen);

		//int result = g_GameService.psend(instance.clientSocket, buf.get(), dataLen, 0);
		int result = ::send(instance.clientSocket, dataBuf, dataLen, 0);//g_GameService.psend(instance.clientSocket, dataBuf, dataLen, 0);
		if (result == SOCKET_ERROR)
		{
			lastError_ = WSAGetLastError();
			return 0;
		}
		else if (result == 0)
		{
		}

		return result;
	}

	int Receive(char* buf, size_t buflen)
	{
		SyncClient& instance = getInstance();
		if (INVALID_SOCKET == instance.clientSocket)
			return 0;

		if (buf == nullptr)
			return false;

		if (buflen <= 0)
			return false;

		GameService& g_GameService = GameService::getInstance();
		memset(buf, 0, buflen);
		int len = g_GameService.precv(instance.clientSocket, buf, buflen, 0);
		if (len == 0)
		{
		}
		return len;
	}

	std::wstring getLastError()
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
};
#endif

#endif //SYNCCLIENT_H