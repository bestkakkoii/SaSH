/*
				GNU GENERAL PUBLIC LICENSE
				   Version 2, June 1991
COPYRIGHT (C) Bestkakkoii 2024 All Rights Reserved.
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
#ifndef ASYNCCLIENT_H

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>

constexpr const wchar_t* IPV6_DEFAULT = L"::1";
constexpr const wchar_t* IPV4_DEFAULT = L"127.0.0.1";

struct tcp_keepalive {
	ULONG onoff;
	ULONG keepalivetime;
	ULONG keepaliveinterval;
};

#define SIO_KEEPALIVE_VALS                  _WSAIOW(IOC_VENDOR,4)

class SyncClient
{
private:
	SOCKET clientSocket = INVALID_SOCKET;
	DWORD lastError_ = 0;

public:
	SyncClient(HWND parentHwnd, long long index)
		: parendHwnd_(parentHwnd)
		, index_(index)
	{
		WSADATA data;
		if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
		{
			lastError_ = WSAGetLastError();
			std::wcout << L"WSAStartup failed with error: " << getLastError() << std::endl;
			return;
		}
	}

	virtual ~SyncClient()
	{
		if (pclosesocket_ != nullptr && INVALID_SOCKET != clientSocket_)
		{
			pclosesocket_(clientSocket_);
			clientSocket_ = INVALID_SOCKET;
		}
		WSACleanup();
	}

	inline BOOL __fastcall Connect(unsigned short type, unsigned short serverPort)
	{
		ADDRESS_FAMILY family = AF_UNSPEC;
		u_short port = htons(serverPort);
		void* pAddrBuf = nullptr;
		sockaddr* serverAddr = nullptr;
		int namelen = 0;
		wchar_t wcsServerIP[256] = {};

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

			clientSocket_ = ::socket(family, SOCK_STREAM, IPPROTO_TCP);
			if (INVALID_SOCKET == clientSocket_)
			{
				break;
			}

			std::wcout << "WSASocketW OK" << std::endl;

			if (InetPtonW(family, wcsServerIP, pAddrBuf) != 1)
			{
				break;
			}

			std::wcout << "InetNtopW OK" << std::endl;

			if (connect(clientSocket_, serverAddr, namelen) == SOCKET_ERROR)
			{
				if (WSAGetLastError() != WSAEWOULDBLOCK)
					break;
			}

			std::wcout << "WSAConnect OK" << std::endl;

			if (nullptr == parendHwnd_)
			{
				std::wcout << L"g_ParenthWnd is nullptr." << std::endl;
				break;
			}

			std::string message = "hs|";
			message += std::to_string(index_);
			message += "\n";
			if (Send(message) == FALSE)
				break;

			std::wcout << L"Notified parent window OK" << std::endl;
			return TRUE;
		} while (false);

		return FALSE;
	}

	inline BOOL __fastcall Send(const char* dataBuf, size_t dataLen)
	{
		if (INVALID_SOCKET == clientSocket_)
			return FALSE;

		if (dataBuf == nullptr)
			return FALSE;

		if (dataLen <= 0)
			return FALSE;

		//regular
		//int result = ::send(clientSocket_, dataBuf, dataLen, 0);
		//WSASend
		WSABUF wsaBuf = {};
		wsaBuf.buf = const_cast<char*>(dataBuf);
		wsaBuf.len = dataLen;
		DWORD dwSend = 0;
		int result = WSASend(clientSocket_, &wsaBuf, 1, &dwSend, 0, nullptr, nullptr);
		if (result == SOCKET_ERROR)
		{
			lastError_ = WSAGetLastError();
			Close();
			return FALSE;
		}
		else if (result == 0)
		{
		}

		return TRUE;
	}

	inline BOOL __fastcall Send(std::string str)
	{
		if (INVALID_SOCKET == clientSocket_)
			return FALSE;

		if (str.empty())
			return FALSE;

		//regular
		//int result = ::send(clientSocket_, str.c_str(), str.length(), 0);
		//WSASend
		WSABUF wsaBuf = {};
		wsaBuf.buf = const_cast<char*>(str.c_str());
		wsaBuf.len = str.length();
		DWORD dwSend = 0;
		int result = WSASend(clientSocket_, &wsaBuf, 1, &dwSend, 0, nullptr, nullptr);
		if (result == SOCKET_ERROR)
		{
			lastError_ = WSAGetLastError();
			Close();
			return FALSE;
		}
		else if (result == 0)
		{
		}

		return TRUE;
	}

	inline BOOL Recv(char* dataBuf, size_t dataLen)
	{
		if (INVALID_SOCKET == clientSocket_)
			return FALSE;

		if (dataBuf == nullptr)
			return FALSE;

		if (dataLen <= 0)
			return FALSE;

		int result = precv_(clientSocket_, dataBuf, dataLen, 0);
		if (result == SOCKET_ERROR)
		{
			lastError_ = WSAGetLastError();
			Close();
			return FALSE;
		}
		else if (result == 0)
		{
		}

		return TRUE;
	}

	inline void Close()
	{
		if (pclosesocket_ != nullptr && INVALID_SOCKET != clientSocket_)
		{
			pclosesocket_(clientSocket_);
			clientSocket_ = INVALID_SOCKET;
		}

		extern HWND g_MainHwnd;
		PostMessageW(g_MainHwnd, kUninitialize, NULL, NULL);
	}

	inline std::wstring __fastcall getLastError()
	{
		if (lastError_ == 0)
		{
			return L"";
		}

		wchar_t* msg = nullptr;
		FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr, lastError_, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&msg, 0, nullptr);

		std::wstring result(msg);
		std::wcerr << result << std::wstring(L"(") + std::to_wstring(lastError_) + std::wstring(L")") << std::endl;
		LocalFree(msg);
		return result;
	}

	inline void __fastcall setCloseSocketFunction(int(__stdcall* p)(SOCKET s))
	{
		pclosesocket_ = p;
	}

	inline void __fastcall setRecvFunction(int(__stdcall* p)(SOCKET s, char* buf, int len, int flags))
	{
		precv_ = p;
	}

private:
	long long index_;
	SOCKET clientSocket_ = INVALID_SOCKET;
	HWND parendHwnd_ = nullptr;
	int(__stdcall* pclosesocket_)(SOCKET s) = nullptr;
	//recv
	int(__stdcall* precv_)(SOCKET s, char* buf, int len, int flags) = nullptr;
};

#endif
#endif //SYNCCLIENT_H