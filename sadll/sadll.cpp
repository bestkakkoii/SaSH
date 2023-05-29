#include "pch.h"
#include "sadll.h"
#include "autil.h"

#pragma comment(lib, "detours.lib")
#pragma comment(lib, "ws2_32.lib")

WNDPROC g_OldWndProc = nullptr;
HWND g_MainHwnd = nullptr;
HMODULE g_hGameModule = nullptr;
HMODULE g_hDllModule = nullptr;
WCHAR g_szGameModulePath[MAX_PATH] = { 0 };
DWORD g_MainThreadId = 0;
HANDLE g_MainThreadHandle = nullptr;
HWND g_ParenthWnd = nullptr;

template<typename T>
static inline T CONVERT_GAMEVAR(DWORD offset) { return (T)((reinterpret_cast<ULONG_PTR>(g_hGameModule) + offset)); }


constexpr size_t LINEBUFSIZ = 8192;
int net_readbuflen = 0;
char net_readbuf[LINEBUFSIZ] = {};
char rpc_linebuffer[Autil::NETBUFSIZ] = {};

void clearNetBuffer()
{
	net_readbuflen = 0;
	memset(net_readbuf, 0, sizeof(net_readbuf));
	memset(rpc_linebuffer, 0, sizeof(rpc_linebuffer));
	Autil::util_Release();
}

int appendReadBuf(char* buf, int size)
{
	if ((net_readbuflen + size) > Autil::NETBUFSIZ)
		return -1;

	memcpy_s(net_readbuf + net_readbuflen, Autil::NETBUFSIZ, buf, size);
	net_readbuflen += size;
	return 0;
}

int shiftReadBuf(int size)
{
	int i;

	if (size > net_readbuflen)
		return -1;
	for (i = size; i < net_readbuflen; i++)
	{
		net_readbuf[i - size] = net_readbuf[i];
	}
	net_readbuflen -= size;
	return 0;
}

int getLineFromReadBuf(char* output, int maxlen)
{
	int i, j;
	if (net_readbuflen >= Autil::NETBUFSIZ)
		return -1;

	for (i = 0; i < net_readbuflen && i < (maxlen - 1); i++)
	{
		if (net_readbuf[i] == '\n')
		{
			memcpy_s(output, Autil::NETBUFSIZ, net_readbuf, i);
			output[i] = '\0';

			for (j = i + 1; j >= 0; j--)
			{
				if (j >= Autil::NETBUFSIZ)
					continue;
				if (output[j] == '\r')
				{
					output[j] = '\0';
					break;
				}
			}

			shiftReadBuf(i + 1);

			if (net_readbuflen < 8192)
				net_readbuf[net_readbuflen] = '\0';
			return 0;
		}
	}
	return -1;
}

#ifdef _DEBUG
void CreateConsole()
{
	if (!AllocConsole())
	{
		return;
	}
	FILE* fDummy;
	freopen_s(&fDummy, "CONOUT$", "w", stdout);
	freopen_s(&fDummy, "CONOUT$", "w", stderr);
	freopen_s(&fDummy, "CONIN$", "r", stdin);
	std::cout.clear();
	std::clog.clear();
	std::cerr.clear();
	std::cin.clear();

	// std::wcout, std::wclog, std::wcerr, std::wcin
	HANDLE hConOut = CreateFile(TEXT("CONOUT$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	HANDLE hConIn = CreateFile(TEXT("CONIN$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	SetStdHandle(STD_OUTPUT_HANDLE, hConOut);
	SetStdHandle(STD_ERROR_HANDLE, hConOut);
	SetStdHandle(STD_INPUT_HANDLE, hConIn);
	std::wcout.clear();
	std::wclog.clear();
	std::wcerr.clear();
	std::wcin.clear();

}
#endif

int CALLBACK WndProc(HWND hWnd, DWORD message, LPARAM wParam, LPARAM lParam);
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID)
{
	if (DLL_PROCESS_ATTACH == ul_reason_for_call)
	{
		GetModuleFileNameW(NULL, g_szGameModulePath, MAX_PATH);
		g_hDllModule = hModule;
		DWORD ThreadId = GetCurrentThreadId();
		if (g_MainThreadId && g_MainThreadId != ThreadId)
			return TRUE;

		g_hGameModule = GetModuleHandleW(nullptr);
		g_MainThreadId = ThreadId;
		g_MainThreadHandle = GetCurrentThread();
		HWND hWnd = util::GetCurrentWindowHandle();
		if (hWnd)
		{
			g_MainHwnd = hWnd;
			g_OldWndProc = reinterpret_cast<WNDPROC>(GetWindowLongPtrA(hWnd, GWL_WNDPROC));
			SetWindowLongPtrW(hWnd, GWL_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc));
#ifdef _DEBUG
			CreateConsole();
#endif
		}
		DisableThreadLibraryCalls(hModule);
	}
	else if (DLL_PROCESS_DETACH == ul_reason_for_call)
	{
		STATICINS(GameService);
		g_GameService.uninitialize();
		SetWindowLongPtrW(g_MainHwnd, GWL_WNDPROC, reinterpret_cast<LONG_PTR>(g_OldWndProc));
	}

	return TRUE;
}

wchar_t g_szGameTitle[1024] = { 0 };
int CALLBACK WndProc(HWND hWnd, DWORD message, LPARAM wParam, LPARAM lParam)
{
	using namespace util;
	STATICINS(GameService);
	switch (message)
	{
	case WM_NULL:
		return 1;
	case WM_CLOSE:
	{
		MINT::NtTerminateProcess(GetCurrentProcess(), 0);
		return 1;
	}
	case WM_MOUSEMOVE:
	{
		PostMessageW(g_ParenthWnd, message + WM_USER, wParam, lParam);
		break;
	}
	case kInitialize:
	{
		g_GameService.initialize(static_cast<unsigned short>(wParam));
		g_ParenthWnd = reinterpret_cast<HWND>(lParam);
		return 1;
	}
	case kUninitialize:
	{
		g_GameService.uninitialize();
		std::thread t([]()
			{
				FreeLibraryAndExitThread(g_hDllModule, 0);
				FreeLibrary(g_hDllModule);
			});
		t.detach();
		return 1;
	}
	case kGetModule:
	{
		return reinterpret_cast<int>(GetModuleHandle(nullptr));
	}
	case kSendPacket:
	{
		if (*g_GameService.g_sockfd == INVALID_SOCKET)
			return 0;

		DWORD dwBytesSent = 0;
		int nRet = g_GameService.psend(*g_GameService.g_sockfd, reinterpret_cast<char*>(wParam), static_cast<int>(lParam), NULL);
#ifdef _DEBUG
		//std::cout << "kSendPacket:::  sockfd:" << std::to_string(*g_GameService.g_sockfd)
		//	<< " address:0x" << std::hex << wParam
		//	<< " size:" << std::to_string(lParam)
		//	<< " errorcode:" << WSAGetLastError() << std::endl;
#endif
		return nRet;
	}
	///////////////////////////////////////////////////////////
	case kEnableEffect:
	{
		g_GameService.WM_EnableEffect(wParam);
		return 1;
	}
	case kEnablePlayerShow:
	{
		g_GameService.WM_EnablePlayerShow(wParam);
		return 1;
	}
	case kSetTimeLock:
	{
		g_GameService.WM_SetTimeLock(wParam, lParam);
		return 1;
	}
	case kEnableSound:
	{
		g_GameService.WM_EnableSound(wParam);
		return 1;
	}
	case kEnableImageLock:
	{
		g_GameService.WM_EnableImageLock(wParam);
		return 1;
	}
	case kEnablePassWall:
	{
		g_GameService.WM_EnablePassWall(wParam);
		return 1;
	}
	case kEnableFastWalk:
	{
		g_GameService.WM_EnableFastWalk(wParam);
		return 1;
	}
	case kSetBoostSpeed:
	{
		g_GameService.WM_SetBoostSpeed(wParam, lParam);
		return 1;
	}
	case kEnableMoveLock:
	{
		g_GameService.WM_EnableMoveLock(wParam);
		return 1;
	}
	case kMuteSound:
	{
		g_GameService.WM_MuteSound(wParam);
		return 1;
	}
	case kEnableBattleDialog:
	{
		g_GameService.WM_EnableBattleDialog(wParam);
		return 1;
	}
	case kSetGameStatus:
	{
		g_GameService.WM_SetGameStatus(wParam);
		return 1;
	}
	case kSetBLockPacket:
	{
		g_GameService.WM_SetBLockPacket(wParam);
		return 1;
	}
	//action
	case kSendAnnounce:
	{
		g_GameService.WM_Announce(reinterpret_cast<char*>(wParam), lParam);
		return 1;
	}
	case kSetMove:
	{
		g_GameService.WM_Move(wParam, lParam);
		return 1;
	}
	default:
		break;
	}
	return CallWindowProcW(g_OldWndProc, g_MainHwnd, message, wParam, lParam);
}

extern "C"
{
	//new socket
	SOCKET WSAAPI New_socket(int af, int type, int protocol)
	{
		STATICINS(GameService);
		return g_GameService.New_socket(af, type, protocol);
	}

	//new send
	int WSAAPI New_send(SOCKET s, const char* buf, int len, int flags)
	{
		STATICINS(GameService);
		return g_GameService.New_send(s, buf, len, flags);
	}

	//new recv
	int WSAAPI New_recv(SOCKET s, char* buf, int len, int flags)
	{
		STATICINS(GameService);
		return g_GameService.New_recv(s, buf, len, flags);
	}

	//new WSARecv
	int WSAAPI New_WSARecv(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags,
		LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
	{
		STATICINS(GameService);
		return g_GameService.New_WSARecv(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, lpOverlapped, lpCompletionRoutine);
	}

	//new closesocket
	int WSAAPI New_closesocket(SOCKET s)
	{
		STATICINS(GameService);
		return g_GameService.New_closesocket(s);
	}

	//Sleep
	void WINAPI New_Sleep(DWORD dwMilliseconds)
	{
		STATICINS(GameService);
		SwitchToThread();
		g_GameService.pSleep(dwMilliseconds);
	}

	//PlaySound
	void New_PlaySound(int a, int b, int c)
	{
		STATICINS(GameService);
		return g_GameService.New_PlaySound(a, b, c);
	}

	//BattleProc
	void New_BattleProc()
	{
		STATICINS(GameService);
		return g_GameService.New_BattleProc();
	}

	//BattleCommandReady
	void New_BattleCommandReady()
	{
		STATICINS(GameService);
		return g_GameService.New_BattleCommandReady();
	}
}

class SyncClient
{
	DISABLE_COPY_MOVE(SyncClient)
private:
	SOCKET clientSocket = INVALID_SOCKET;
	DWORD lastError_ = 0;

	static SyncClient& getInstance()
	{
		static SyncClient* instance = new SyncClient();
		return *instance;
	}
public:
	SyncClient()
	{

		WSADATA data;
		if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
		{
			std::cout << "WSAStartup failed with error: " << WSAGetLastError() << std::endl;
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
		STATICINS(GameService);
		SyncClient& instance = getInstance();
		instance.clientSocket = g_GameService.psocket(AF_INET6, SOCK_STREAM, 0);
		if (instance.clientSocket == INVALID_SOCKET)
		{
			std::cout << "socket failed with error: " << WSAGetLastError() << std::endl;
			WSACleanup();
			return false;
		}
		int bufferSize = 8192;
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
			std::cout << "inet_pton failed. Error Code : " << WSAGetLastError() << std::endl;
			closesocket(instance.clientSocket);
			return false;
		}

		if (connect(instance.clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) != 0)
		{
			std::cout << "connect failed. Error Code : " << WSAGetLastError() << std::endl;
			closesocket(instance.clientSocket);
			return false;
		}

		return true;
	}

	int Send(char* dataBuf, int dataLen)
	{
		SyncClient& instance = getInstance();
		if (INVALID_SOCKET == instance.clientSocket)
		{
			return false;
		}

		if (!dataBuf)
		{
			return false;
		}

		if (dataLen <= 0)
		{
			return false;
		}

		STATICINS(GameService);
		std::unique_ptr <char[]> buf(new char[dataLen]());
		ZeroMemory(buf.get(), dataLen);
		memcpy_s(buf.get(), dataLen, dataBuf, dataLen);

		int result = g_GameService.psend(instance.clientSocket, buf.get(), dataLen, 0);
		if (result == SOCKET_ERROR)
		{
			lastError_ = WSAGetLastError();

			return 0;
		}

		return result;
	}
	/*
	bool Send(LPWSABUF dataBuf)
	{
		if (INVALID_SOCKET == clientSocket)
		{
			return false;
		}

		if (!dataBuf)
		{
			return false;
		}

		if (!dataBuf->buf)
		{
			return false;
		}

		STATICINS(GameService);

		char buf[8192] = { 0 };
		RtlZeroMemory(buf, sizeof(buf));
		memcpy_s(buf, 8192, dataBuf->buf, dataBuf->len);

		int result = g_GameService.psend(clientSocket, buf, dataBuf->len, 0);
		if (result == SOCKET_ERROR)
		{
			lastError_ = WSAGetLastError();

			return false;
		}

		return true;
	}
	*/
	int Receive(char* buf, size_t buflen)
	{
		SyncClient& instance = getInstance();
		if (INVALID_SOCKET == instance.clientSocket)
		{
			return 0;
		}

		if (!buf)
		{
			return false;
		}

		if (buflen <= 0)
		{
			return false;
		}

		STATICINS(GameService);
		RtlZeroMemory(buf, buflen);
		int len = g_GameService.precv(instance.clientSocket, buf, buflen, 0);
		return len;
	}

	std::wstring getLastError()
	{
		//format error
		//FormatMessageW
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

void GameService::initialize(unsigned short port)
{
	if (isInitialized_.load(std::memory_order_acquire))
		return;

	isInitialized_.store(true, std::memory_order_release);

	SetConsoleCP(936);
	SetConsoleOutputCP(936);
	setlocale(LC_ALL, "Chinese.GB2312");

	g_sockfd = CONVERT_GAMEVAR<int*>(0x421B404);
	g_world_status = CONVERT_GAMEVAR<int*>(0x4230DD8);
	g_game_status = CONVERT_GAMEVAR<int*>(0x4230DF0);

	pPlaySound = CONVERT_GAMEVAR<pfnPlaySound>(0x88190);
	pBattleProc = CONVERT_GAMEVAR<pfnBattleProc>(0x3940);
	pBattleCommandReady = CONVERT_GAMEVAR<pfnBattleCommandReady>(0x6B9A0);

	Autil::PersonalKey = CONVERT_GAMEVAR<char*>(0x4AC0898);

	psocket = CONVERT_GAMEVAR<pfnsocket>(0x91764);//::socket;
	psend = CONVERT_GAMEVAR<pfnsend>(0x9171C); //::send;
	precv = CONVERT_GAMEVAR<pfnrecv>(0x91728);
	//pWSARecv = ::WSARecv;
	pclosesocket = CONVERT_GAMEVAR<pfnclosesocket>(0x91716);//::closesocket;

	pSleep = ::Sleep;

	//禁止遊戲內切AI
	DWORD paddr = CONVERT_GAMEVAR <DWORD>(0x1DF82);
	util::MemoryMove(paddr, "\xB8\x00\x00\x00\x00", 5);//只要點pgup或pgdn就會切0
	paddr = CONVERT_GAMEVAR <DWORD>(0x1DFE6);
	util::MemoryMove(paddr, "\xBA\x00\x00\x00\x00", 5);//只要點pgup或pgdn就會切0
	//paddr = CONVERT_GAMEVAR <DWORD>(0x1F2EF);
	//util::MemoryMove(paddr, "\xB9\x00\x00\x00\x00", 5);
	int* pEnableAI = CONVERT_GAMEVAR<int*>(0xD9050);
	pEnableAI = 0;

	DetourRestoreAfterWith();
	DetourTransactionBegin();
	DetourUpdateThread(g_MainThreadHandle);
	DetourAttach(&(PVOID&)psocket, ::New_socket);
	DetourAttach(&(PVOID&)psend, ::New_send);
	DetourAttach(&(PVOID&)precv, ::New_recv);

	//DetourAttach(&(PVOID&)pWSARecv, ::New_WSARecv);
	DetourAttach(&(PVOID&)pclosesocket, ::New_closesocket);
	DetourAttach(&(PVOID&)pSleep, ::New_Sleep);
	DetourAttach(&(PVOID&)pPlaySound, ::New_PlaySound);
	DetourAttach(&(PVOID&)pBattleProc, ::New_BattleProc);
	DetourAttach(&(PVOID&)pBattleCommandReady, ::New_BattleCommandReady);
	DetourTransactionCommit();

	char* testStr = (char*)0x984444;
	strcpy_s(testStr, 20, "H|F");
	char* testStr2 = (char*)0x985444;
	strcpy_s(testStr2, 20, "W|FF|FF");

	Autil::util_Init();
	clearNetBuffer();

	if (!syncClient_)
	{
		syncClient_.reset(new SyncClient());
		if (!syncClient_->Connect("::1", port))
		{
			PostMessageW(g_MainHwnd, util::kUninitialize, NULL, NULL);
		}
	}
}

void GameService::uninitialize()
{
	if (!isInitialized_.load(std::memory_order_acquire))
		return;

	isInitialized_.store(false, std::memory_order_release);

	if (syncClient_)
		syncClient_.reset();

	DetourRestoreAfterWith();
	DetourTransactionBegin();
	DetourUpdateThread(g_MainThreadHandle);
	DetourAttach(&(PVOID&)psocket, ::New_socket);
	DetourAttach(&(PVOID&)psend, ::New_send);
	//DetourAttach(&(PVOID&)pWSARecv, ::New_WSARecv);
	DetourAttach(&(PVOID&)pclosesocket, ::New_closesocket);
	DetourAttach(&(PVOID&)pSleep, ::New_Sleep);
	DetourAttach(&(PVOID&)pPlaySound, ::New_PlaySound);
	DetourAttach(&(PVOID&)pBattleProc, ::New_BattleProc);
	DetourAttach(&(PVOID&)pBattleCommandReady, ::New_BattleCommandReady);
	DetourTransactionCommit();
}

//hooks
SOCKET WSAAPI GameService::New_socket(int af, int type, int protocol)
{
	SOCKET ret = psocket(af, type, protocol);
	return ret;
}

int WSAAPI GameService::New_send(SOCKET s, const char* buf, int len, int flags)
{
	int ret = 0;

#ifdef _DEBUG
	//size_t size = strlen(buf);
	//if (size > 0 && size < 200)
	//{
	//	std::string str(buf);
	//	std::cout << "[ToSA Server] size:" << std::to_string(size) << " data:" << str << std::endl;
	//}
#endif

	ret = psend(s, buf, len, flags);
	return ret;
}

int SaDispatchMessage(int fd, char* encoded)
{
	using namespace Autil;

	int		func = 0, fieldcount = 0;
	int		iChecksum = 0, iChecksumrecv = 0;
	std::unique_ptr<char[]> raw(new char[Autil::NETBUFSIZ]());

	util_DecodeMessage(raw.get(), Autil::NETBUFSIZ, encoded);
	util_SplitMessage(raw.get(), Autil::NETBUFSIZ, const_cast<char*>(Autil::SEPARATOR));
	if (util_GetFunctionFromSlice(&func, &fieldcount) != 1)
	{
		return 0;
	}

	//std::cout << "fun: " << std::to_string(func) << " fieldcount: " << std::to_string(fieldcount) << std::endl;

	auto CHECKFUN = [&func](int cmpfunc) -> bool { return (func == cmpfunc); };

	if (CHECKFUN(LSSPROTO_EN_RECV) /*Battle EncountFlag //開始戰鬥 7*/)
	{
		Autil::SliceCount = 0;
		return 2;
	}
	else if (CHECKFUN(LSSPROTO_CLIENTLOGIN_RECV)/*選人畫面 72*/)
	{
		return 0;
	}
	else if (CHECKFUN(LSSPROTO_CHARLIST_RECV)/*選人頁面資訊 80*/)
	{
		return 0;
	}
	Autil::SliceCount = 0;
	return 1;
}

//recv
int WSAAPI GameService::New_recv(SOCKET s, char* buf, int len, int flags)
{
	using namespace Autil;

	std::unique_ptr <char[]> raw(new char[len + 1]());
	memcpy_s(raw.get(), len, buf, len);
	raw.get()[len] = '\0';

	int recvlen = precv(s, rpc_linebuffer, LINEBUFSIZ, flags);
	if (recvlen > 0)
	{
		memcpy_s(raw.get(), len, rpc_linebuffer, len);
		int sendlen = syncClient_->Send(raw.get(), recvlen);
		std::cout << "send to SASH: size:" << std::to_string(sendlen) << std::endl;

		appendReadBuf(rpc_linebuffer, recvlen);

		while ((recvlen != SOCKET_ERROR) && (net_readbuflen > 0))
		{
			RtlZeroMemory(&rpc_linebuffer, sizeof(rpc_linebuffer));
			if (!getLineFromReadBuf(rpc_linebuffer, sizeof(rpc_linebuffer)))
			{
				int ret = SaDispatchMessage(s, rpc_linebuffer);
				if (ret < 0)
				{
					//代表此段數據已到結尾
					clearNetBuffer();
					break;
				}
				else if (ret == 0)
				{
					//錯誤的數據 或 需要清除緩存
					clearNetBuffer();
					break;
				}
				else if (ret == 2)
				{
					if (isBLockPacket_)
					{
						std::cout << "************************* Block *************************" << std::endl;

						//需要移除不傳給遊戲的數據
						clearNetBuffer();
						SetLastError(0);
						return 1;
					}
				}
			}
			else
			{
				//數據讀完了
				util_Release();
				break;
			}
		}
	}

	memcpy_s(buf, len, raw.get(), len);

	return recvlen;
}

int WSAAPI GameService::New_WSARecv(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags,
	LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
	int ret = pWSARecv(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, lpOverlapped, lpCompletionRoutine);
	return ret;
}

int WSAAPI GameService::New_closesocket(SOCKET s)
{
	int ret = pclosesocket(s);
	return ret;
}

void __cdecl GameService::New_PlaySound(int a, int b, int c)
{
	if (!g_muteSound)
	{
		pPlaySound(a, b, c);
	}
}

void __cdecl GameService::New_BattleProc()
{
	if (g_enableBattleDialog)
	{
		pBattleProc();
	}
}

void __cdecl GameService::New_BattleCommandReady()
{
	pBattleCommandReady();
}
////////////////////////////////////////////////////

void GameService::WM_SetGameStatus(int status)
{
	int G = *g_game_status;
	if (G != status)
		*g_game_status = status;
}

void GameService::WM_MuteSound(bool enable)
{
	g_muteSound = enable;
}

void GameService::WM_EnableBattleDialog(bool enable)
{
	DWORD addr = CONVERT_GAMEVAR<DWORD>(0x98C9);
	DWORD addr2 = CONVERT_GAMEVAR<DWORD>(0x9D07);
	DWORD addrSoundCharOpen = CONVERT_GAMEVAR<DWORD>(0x98DA);
	DWORD addrSoundCharOpen2 = CONVERT_GAMEVAR<DWORD>(0x9D13);
	DWORD addrSoundCharClose = CONVERT_GAMEVAR<DWORD>(0x667D);
	DWORD addrSoundCharClose2 = CONVERT_GAMEVAR<DWORD>(0x93C9);
	int* timerAddr = CONVERT_GAMEVAR<int*>(0xE21E0);
	DWORD timerAddr2 = CONVERT_GAMEVAR<DWORD>(0x9841);
	DWORD timerAddr3 = CONVERT_GAMEVAR<DWORD>(0x9DB6);
	DWORD mainLoopAddr = CONVERT_GAMEVAR<DWORD>(0xA914);
	if (enable)
	{
		////sa_8001sf.exe+98C9 - A3 E4214E00           - mov [sa_8001sf.exe+E21E4],eax { (0) }
		//util::MemoryMove(addr, "\xA3\xE4\x21\x4E\x00", 5);
		////sa_8001sf.exe+9D07 - 89 15 E4214E00        - mov [sa_8001sf.exe+E21E4],edx { (0) }
		//util::MemoryMove(addr2, "\x89\x15\xE4\x21\x4E\x00", 6);
		////sa_8001sf.exe+98DA - E8 B1E80700           - call sa_8001sf.exe+88190
		//util::MemoryMove(addrSoundCharOpen, "\xE8\xB1\xE8\x07\x00", 5);
		////sa_8001sf.exe+9D13 - E8 78E40700           - call sa_8001sf.exe+88190
		//util::MemoryMove(addrSoundCharOpen2, "\xE8\x78\xE4\x07\x00", 5);
		////sa_8001sf.exe+667D - E8 0E1B0800           - call sa_8001sf.exe+88190
		//util::MemoryMove(addrSoundCharClose, "\xE8\x0E\x1B\x08\x00", 5);
		////sa_8001sf.exe+93C9 - E8 C2ED0700           - call sa_8001sf.exe+88190
		//util::MemoryMove(addrSoundCharClose2, "\xE8\xC2\xED\x07\x00", 5);
		////sa_8001sf.exe+954F - A1 E0214E00           - mov eax,[sa_8001sf.exe+E21E0] { (0) }
		//*timerAddr = 1;
		////sa_8001sf.exe+9841 - 89 1D E0214E00        - mov [sa_8001sf.exe+E21E0],ebx { (1) }
		//util::MemoryMove(timerAddr2, "\x89\x1D\xE0\x21\x4E\x00", 6);
		////sa_8001sf.exe+9DB6 - 89 2D E0214E00        - mov [sa_8001sf.exe+E21E0],ebp { (0) }
		//util::MemoryMove(timerAddr3, "\x89\x2D\xE0\x21\x4E\x00", 6);
		//sa_8001sf.exe+A914 - EB 0C                 - jmp sa_8001sf.exe+A922 //最重要的
		//sa_8001sf.exe+A914 - 75 0C                 - jne sa_8001sf.exe+A922 //原始
		util::MemoryMove(mainLoopAddr, "\xEB\x0C", 2);
		g_enableBattleDialog = true;

	}
	else
	{
		//util::MemoryMove(addr, "\x90\x90\x90\x90\x90", 5);
		//util::MemoryMove(addr2, "\x90\x90\x90\x90\x90\x90", 6);
		//util::MemoryMove(addrSoundCharOpen, "\x90\x90\x90\x90\x90", 5);
		//util::MemoryMove(addrSoundCharOpen2, "\x90\x90\x90\x90\x90", 5);
		//util::MemoryMove(addrSoundCharClose, "\x90\x90\x90\x90\x90", 5);
		//util::MemoryMove(addrSoundCharClose2, "\x90\x90\x90\x90\x90", 5);
		//*timerAddr = 0;
		//util::MemoryMove(timerAddr2, "\x90\x90\x90\x90\x90\x90", 6);
		//util::MemoryMove(timerAddr3, "\x90\x90\x90\x90\x90\x90", 6);
		util::MemoryMove(mainLoopAddr, "\x90\x90", 2);
		g_enableBattleDialog = false;
	}
}

//顯示特效
void GameService::WM_EnableEffect(bool enable)
{
	DWORD effectAddr = CONVERT_GAMEVAR<DWORD>(0x434DD);
	DWORD effectAddr2 = CONVERT_GAMEVAR<DWORD>(0x482F0);
	DWORD effectAddr3 = CONVERT_GAMEVAR<DWORD>(0x48DE6);
	DWORD effectAddr4 = CONVERT_GAMEVAR<DWORD>(0x49029);
	DWORD effectAddr5 = CONVERT_GAMEVAR<DWORD>(0x7BDF2);
	if (enable)
	{
		//sa_8001sf.exe+434DD - C7 05 F0 0D 63 04 C8 00 00 00 - mov [sa_8001sf.exe+4230DF0],000000C8 { (3),200 }
		util::MemoryMove(effectAddr, "\xC7\x05\xF0\x0D\x63\x04\xC8\x00\x00\x00", 10);


		///*
		//	sa_8001sf.exe+482F0 - 0F8C FA020000         - jl sa_8001sf.exe+485F0
		//	sa_8001sf.exe+482F6 - 8B 76 28              - mov esi,[esi+28]
		//	sa_8001sf.exe+482F9 - 83 FE FF              - cmp esi,-01 { 255 }
		//	sa_8001sf.exe+482FC - 75 12                 - jne sa_8001sf.exe+48310

		//*/
		//util::MemoryMove(effectAddr2, "\x0F\x8C\xFA\x02\x00\x00\x8B\x76\x28\x83\xFE\xFF\x75\x12", 14);
		// 
		// 
		//sa_8001sf.exe+48DE6 - 7C 42                 - jl sa_8001sf.exe+48E2A
		util::MemoryMove(effectAddr3, "\x7C\x42", 2);
		//sa_8001sf.exe+49029 - 8B 4D 34              - mov ecx,[ebp+34]
		//sa_8001sf.exe + 4902C - 3B C1 - cmp eax, ecx
		util::MemoryMove(effectAddr4, "\x8B\x4D\x34\x3B\xC1", 5);
		//sa_8001sf.exe+7BDF2 - 0F87 20010000         - ja sa_8001sf.exe+7BF18
		util::MemoryMove(effectAddr5, "\x0F\x87\x20\x01\x00\x00", 6);

	}
	else
	{
		//sa_8001sf.exe+434DD - EB 08                 - jmp sa_8001sf.exe+434E7
		util::MemoryMove(effectAddr, "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90", 10);


		///*
		//sa_8001sf.exe+482F0 - EB 04                 - jmp sa_8001sf.exe+482F6
		//sa_8001sf.exe+482F2 - FA                    - cli
		//sa_8001sf.exe+482F3 - 02 00                 - add al,[eax]
		//sa_8001sf.exe+482F5 - 00 8B 762883FE        - add [ebx-sa_8001sf.exe+13CD78A],cl
		//sa_8001sf.exe+482FB - FF 75 12              - push [ebp+12]

		//*/
		//util::MemoryMove(effectAddr2, "\xEB\x04\xFA\x02\x00\x00\x00\x8B\x76\x28\x83\xFE\xFF\x75\x12", 14);


		//sa_8001sf.exe+48DE6 - EB 42                 - jmp sa_8001sf.exe+48E2A
		util::MemoryMove(effectAddr3, "\xEB\x42", 2);
		//sa_8001sf.exe+49029 - 3D 00000000           - cmp eax,00000000 { 0 }
		util::MemoryMove(effectAddr4, "\x3D\x00\x00\x00\x00", 5);
		/*
		sa_8001sf.exe+7BDF2 - 90                    - nop
		sa_8001sf.exe+7BDF3 - E9 20010000           - jmp sa_8001sf.exe+7BF18
		*/
		util::MemoryMove(effectAddr5, "\x90\xE9\x20\x01\x00\x00", 6);
	}
}

//顯示人物
void GameService::WM_EnablePlayerShow(bool enable)
{
	DWORD playerShowAddr = CONVERT_GAMEVAR<DWORD>(0xEA30);
	DWORD playerShowAddr2 = CONVERT_GAMEVAR<DWORD>(0xEFD0);
	DWORD playerShowAddr3 = CONVERT_GAMEVAR<DWORD>(0xF180);
	if (enable)
	{
		//sa_8001sf.exe+EA30 - 51                    - push ecx
		util::MemoryMove(playerShowAddr, "\x51", 1);
		//sa_8001sf.exe+EFD0 - 8B 44 24 04           - mov eax,[esp+04]
		util::MemoryMove(playerShowAddr2, "\x8B\x44\x24\x04", 4);
		//sa_8001sf.exe+F180 - 8B 44 24 04           - mov eax,[esp+04]
		util::MemoryMove(playerShowAddr3, "\x8B\x44\x24\x04", 4);
	}
	else
	{
		//sa_8001sf.exe+EA30 - C3                    - ret 
		util::MemoryMove(playerShowAddr, "\xC3", 1);
		//sa_8001sf.exe+EFD0 - C3                    - ret 
		util::MemoryMove(playerShowAddr2, "\xC3\x90\x90\x90", 4);
		//sa_8001sf.exe+F180 - C3                    - ret
		util::MemoryMove(playerShowAddr3, "\xC3\x90\x90\x90", 4);
	}
}

//鎖定時間
void GameService::WM_SetTimeLock(bool enable, int time)
{
	DWORD timerefreshAddr = CONVERT_GAMEVAR<DWORD>(0x1E6D0);
	if (!enable)
	{
		//sa_8001sf.exe+1E6D0 - 56                    - push esi
		util::MemoryMove(timerefreshAddr, "\x56", 1);

	}
	else
	{
		if (time >= 0 && time <= 4)
		{
			int* pcurrentTime = CONVERT_GAMEVAR<int*>(0x14CD00);
			int* pa = CONVERT_GAMEVAR<int*>(0x14CD68);
			int* pb = CONVERT_GAMEVAR<int*>(0x14D170);
			int* pc = CONVERT_GAMEVAR<int*>(0x1AB740);
			int* pd = CONVERT_GAMEVAR<int*>(0x1AB74C);

			constexpr int set[5 * 5] = {
				832, 3, 0, 0,  0,
				64, 0, 1, 256, 1,
				320, 1, 2, 512, 2,
				576, 2, 3, 768, 3,
				832, 3, 0, 1024, 0
			};

			//sa_8001sf.exe+1E6D0 - C3                    - ret 
			util::MemoryMove(timerefreshAddr, "\xC3", 1);
			*pcurrentTime = set[time * 5 + 0];
			*pa = set[time * 5 + 1];
			*pb = set[time * 5 + 2];
			*pc = set[time * 5 + 3];
			*pd = set[time * 5 + 4];
		}
	}
}

//打開聲音
void GameService::WM_EnableSound(bool enable)
{
	int* pBackgroundMusic = CONVERT_GAMEVAR<int*>(0xD36E0);
	int* pSoundEffect = CONVERT_GAMEVAR<int*>(0xD36DC);
	auto pSetSound = CONVERT_GAMEVAR<void(__cdecl*)()>(0x880F0);
	if (enable)
	{
		*pBackgroundMusic = currentMusic_;
		*pSoundEffect = currentSound_;
		pSetSound();
	}
	else
	{
		currentMusic_ = *pBackgroundMusic;
		currentSound_ = *pSoundEffect;
		*pBackgroundMusic = 0;
		*pSoundEffect = 0;
		pSetSound();
	}
}

//鎖定畫面
void GameService::WM_EnableImageLock(bool enable)
{
	DWORD pImageLock = CONVERT_GAMEVAR<DWORD>(0x42A7B);
	if (!enable)
	{
		/*
		sa_8001sf.exe+42A7B - E8 F0 FD FF FF           - call sa_8001sf.exe+42870
		sa_8001sf.exe+42A80 - 8B 15 44 A6 56 04        - mov edx,[sa_8001sf.exe+416A644] { (1280.00) }
		sa_8001sf.exe+42A86 - A1 48 A6 56 04           - mov eax,[sa_8001sf.exe+416A648] { (896.00) }
		sa_8001sf.exe+42A8B - 89 15 98 29 58 04        - mov [sa_8001sf.exe+4182998],edx { (1280.00) }
		sa_8001sf.exe+42A91 - A3 94 29 58 04           - mov [sa_8001sf.exe+4182994],eax { (896.00) }
		*/
		util::MemoryMove(pImageLock, "\xE8\xF0\xFD\xFF\xFF\x8B\x15\x44\xA6\x56\x04\xA1\x48\xA6\x56\x04\x89\x15\x98\x29\x58\x04\xA3\x94\x29\x58\x04", 27);
	}
	else
	{
		//27 nop
		util::MemoryMove(pImageLock, "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90", 27);
	}
}

//橫衝直撞
void GameService::WM_EnablePassWall(bool enable)
{
	DWORD pPassWall = CONVERT_GAMEVAR<DWORD>(0x42051);
	if (!enable)
	{
		//sa_8001sf.exe+42051 - 66 83 3C 55 749B5604 01 - cmp word ptr [edx*2+sa_8001sf.exe+4169B74],01 { 1 }
		util::MemoryMove(pPassWall, "\x66\x83\x3C\x55\x74\x9B\x56\x04\x01", 9);
	}
	else
	{
		//sa_8001sf.exe+42051 - ret nop...
		util::MemoryMove(pPassWall, "\xC3\x90\x90\x90\x90\x90\x90\x90\x90", 9);
	}
}

void GameService::WM_EnableFastWalk(bool enable)
{
	DWORD pFastWalk = CONVERT_GAMEVAR<DWORD>(0x42EB8);
	if (!enable)
	{
		//sa_8001sf.exe+42EB8 - 75 33                 - jne sa_8001sf.exe+42EED
		util::MemoryMove(pFastWalk, "\x75\x33", 2);
	}
	else
	{
		util::MemoryMove(pFastWalk, "\x90\x90", 2);
	}
}

void GameService::WM_SetBoostSpeed(bool enable, int speed)
{
	DWORD pBoostSpeed = CONVERT_GAMEVAR<DWORD>(0x1DEE4);
	int* pSpeed = CONVERT_GAMEVAR<int*>(0xAB7CC);
	if (!enable || (0 == speed))
	{
		//sa_8001sf.exe+1DEE4 - 83 F9 02              - cmp ecx,02 { 2 }
		*pSpeed = 14;
		util::MemoryMove(pBoostSpeed, "\x83\xF9\x02", 3);
	}
	else
	{
		if (speed >= 1 && speed <= 14)
		{
			*pSpeed = 14 - speed + 1;
			//sa_8001sf.exe+1DEE4 - 83 F9 0E              - cmp ecx,0E { 14 }
			util::MemoryMove(pBoostSpeed, "\x83\xF9\x0E", 3);
		}
	}
}

void GameService::WM_EnableMoveLock(bool enable)
{
	DWORD pMoveLock = CONVERT_GAMEVAR<DWORD>(0x42773);
	if (!enable)
	{
		//00442773 - 77 2A                 - ja 0044279F
		util::MemoryMove(pMoveLock, "\x77\x2A", 2);

	}
	else
	{
		//sa_8001sf.exe+42773 - EB 2A                 - jmp sa_8001sf.exe+4279F
		util::MemoryMove(pMoveLock, "\xEB\x2A", 2);
	}
}

void GameService::WM_Announce(char* str, int color)
{
	auto pAnnounce = CONVERT_GAMEVAR<void(_cdecl*)(char*, int)>(0x115C0);
	pAnnounce(str, color);
}

void GameService::WM_Move(int x, int y)
{
	/*
	sa_8001sf.exe+4277A - 8B 15 38A65604        - mov edx,[sa_8001sf.exe+416A638] { (98) }
	sa_8001sf.exe+42780 - A3 54B74B00           - mov [sa_8001sf.exe+BB754],eax { (580410503) }
	sa_8001sf.exe+42785 - A1 3CA65604           - mov eax,[sa_8001sf.exe+416A63C] { (0.01) }
	sa_8001sf.exe+4278A - 89 15 4C025604        - mov [sa_8001sf.exe+416024C],edx { (100) }
	sa_8001sf.exe+42790 - A3 50025604           - mov [sa_8001sf.exe+4160250],eax { (100) }
	sa_8001sf.exe+42795 - C7 05 E0295804 01000000 - mov [sa_8001sf.exe+41829E0],00000001 { (0),1 }

	*/

	int* goalX = CONVERT_GAMEVAR<int*>(0x416024C);
	int* goalY = CONVERT_GAMEVAR<int*>(0x4160250);
	int* walking = CONVERT_GAMEVAR<int*>(0x41829E0);

	*goalX = x;
	*goalY = y;
	*walking = 1;
}

////接收伺服器發來已經送出戰鬥封包則切換標誌
//void GameService::WM_ServerNotifyBattleCommandEnd()
//{
//	/*
//	sa_8001sf.exe+4608 - 8B 0D CC214E00        - mov ecx,[sa_8001sf.exe+E21CC] { (0) }
//	sa_8001sf.exe+460F - 8B 04 8D 28E4EB04     - mov eax,[ecx*4+sa_8001sf.exe+4ABE428]
//	sa_8001sf.exe+4616 - B9 40000000           - mov ecx,00000040 { 64 }
//	sa_8001sf.exe+461B - 8B B0 A0000000        - mov esi,[eax+000000A0]
//	sa_8001sf.exe+4621 - 0B F1                 - or esi,ecx
//	sa_8001sf.exe+4623 - 89 B0 A0000000        - mov [eax+000000A0],esi
//	sa_8001sf.exe+4629 - 8B 15 CC214E00        - mov edx,[sa_8001sf.exe+E21CC] { (0) }
//	sa_8001sf.exe+4631 - 8B 04 95 3CE4EB04     - mov eax,[edx*4+sa_8001sf.exe+4ABE43C]
//	sa_8001sf.exe+4639 - 8B 90 A0000000        - mov edx,[eax+000000A0]
//	sa_8001sf.exe+463F - 0B D1                 - or edx,ecx
//	sa_8001sf.exe+4641 - 89 90 A0000000        - mov [eax+000000A0],edx
//	sa_8001sf.exe+4647 - A1 F00D6304           - mov eax,[sa_8001sf.exe+4230DF0] { (3) }
//	sa_8001sf.exe+464C - 40                    - inc eax
//	sa_8001sf.exe+464D - A3 F00D6304           - mov [sa_8001sf.exe+4230DF0],eax { (3) }
//	*/
//
//	__asm
//	{
//		mov ecx, [0x04E21CC]
//		mov eax, [ecx * 0x4 + 0x4EBE428]
//		mov esi, [eax + 0x00000A0]
//
//		mov ecx, 0x0000040
//		or esi, ecx
//		mov[eax + 0x00000A0], esi
//
//		mov edx, [0x04E21CC]
//		mov eax, [edx * 0x4 + 0x4EBE43C]
//		mov edx, [eax + 0x00000A0]
//
//		or edx, ecx
//		mov[eax + 0x00000A0], edx
//
//		mov eax, [0x4630DF0]
//		inc eax
//		mov ecx, 0x4630DF0
//		mov[ecx], eax
//	}
//
//	//auto update = [](int* addr)
//	//{
//	//	int* pE21CC = CONVERT_GAMEVAR<int*>(0xE21CC);
//	//	int E21CCValue = *pE21CC;
//	//	int value = E21CCValue;
//
//	//	int* ptr = ((E21CCValue) * 4 + addr);
//	//	int* temp = (ptr + 0xA0);
//	//	(*temp) |= 0x40;
//	//};
//
//
//	//g_battleNotifyHasSend = false;
//
//	//update(CONVERT_GAMEVAR<int*>(0x4ABE428));
//	//update(CONVERT_GAMEVAR<int*>(0x4ABE43C));
//
//	//int* addr4230DF0 = CONVERT_GAMEVAR<int*>(0x4230DF0);
//	//(*addr4230DF0)++;
//}