
#ifdef SADLL_EXPORTS
#define SADLL_API __declspec(dllexport)
#else
#define SADLL_API __declspec(dllimport)
#endif

#ifndef DISABLE_COPY
#define DISABLE_COPY(Class) \
    Class(const Class &) = delete;\
    Class &operator=(const Class &) = delete;
#endif
#ifndef DISABLE_MOVE
#define DISABLE_MOVE(Class) \
    Class(Class &&) = delete; \
    Class &operator=(Class &&) = delete;
#endif
#ifndef DISABLE_COPY_MOVE
#define DISABLE_COPY_MOVE(Class) \
    DISABLE_COPY(Class) \
    DISABLE_MOVE(Class) \
public:\
	static Class& get_instance() {\
		static Class instance;\
		return instance;\
	}
#endif
#ifndef STATICINS
#define STATICINS(Class) Class& g_##Class = Class::get_instance()
#endif

namespace util
{
	enum UserMessage
	{
		kUserMessage = WM_USER + 0x1024,
		kInitialize,
		kUninitialize,
		kGetModule,
		kSendPacket,

		//
		kEnableEffect,
		kEnablePlayerShow,
		kSetTimeLock,
		kEnableSound,
		kEnableImageLock,
		kEnablePassWall,
		kEnableFastWalk,
		kSetBoostSpeed,
		kEnableMoveLock,
		kMuteSound,
		kEnableBattleDialog,
		kSetGameStatus,
		kSetBLockPacket,
		kBattleTimeExtend,

		//Action
		kSendAnnounce,
		kSetMove,
		kDistoryDialog,
		kCleanChatHistory,
	};

	using handle_data = struct
	{
		unsigned long process_id;
		HWND window_handle;
	};

	template<class T, class T2>
	void __stdcall MemoryMove(T dis, T2* src, size_t size)
	{
		DWORD dwOldProtect = 0;
		VirtualProtect((void*)dis, size, PAGE_EXECUTE_READWRITE, &dwOldProtect);
		memcpy((void*)dis, (void*)src, size);
		VirtualProtect((void*)dis, size, dwOldProtect, &dwOldProtect);
	}

	BOOL WINAPI IsCurrentWindow(const HWND& handle)
	{
		if ((GetWindow(handle, GW_OWNER) == NULL) && (IsWindowVisible(handle)))
			return TRUE;
		else
			return FALSE;
	}

	BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam)
	{
		handle_data& data = *(handle_data*)lParam;
		unsigned long process_id = 0;
		GetWindowThreadProcessId(handle, &process_id);
		if (data.process_id != process_id || !IsCurrentWindow(handle))
			return TRUE;
		data.window_handle = handle;
		return FALSE;
	}

	HWND WINAPI FindCurrentWindow(const unsigned long& process_id)
	{
		handle_data data = {};
		data.process_id = process_id;
		data.window_handle = 0;
		EnumWindows(EnumWindowsCallback, (LPARAM)&data);
		return data.window_handle;
	}

	HWND WINAPI GetCurrentWindowHandle()
	{
		unsigned long process_id = GetCurrentProcessId();
		HWND hwnd = NULL;
		while (!hwnd) { hwnd = FindCurrentWindow(process_id); }
		return hwnd;
	}

}

class SyncClient;
class GameService
{
	DISABLE_COPY_MOVE(GameService);
	GameService() = default;

public:
	~GameService() = default;
	void initialize(unsigned short port);
	void uninitialize();

public:
	void WM_EnableEffect(bool enable);
	void WM_EnablePlayerShow(bool enable);
	void WM_SetTimeLock(bool enable, int time);
	void WM_EnableSound(bool enable);
	void WM_EnableImageLock(bool enable);
	void WM_EnablePassWall(bool enable);
	void WM_EnableFastWalk(bool enable);
	void WM_SetBoostSpeed(bool enable, int speed);
	void WM_EnableMoveLock(bool enable);
	void WM_MuteSound(bool enable);
	void WM_BattleTimeExtend(bool enable);
	void WM_EnableBattleDialog(bool enable);
	void WM_SetGameStatus(int status);

	void WM_Announce(char* str, int color);
	void WM_Move(int x, int y);
	void WM_DistoryDialog();
	void WM_CleanChatHistory();

	void WM_SetBLockPacket(bool enable) { isBLockPacket_ = enable; }
public://g-var
	int* g_sockfd = nullptr;
	int* g_world_status = nullptr;
	int* g_game_status = nullptr;
	bool g_muteSound = false;
	bool g_enableBattleDialog = false;

public://hook
	SOCKET WSAAPI New_socket(int af, int type, int protocol);

	int WSAAPI New_WSARecv(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags,
		LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);
	int WSAAPI New_closesocket(SOCKET s);

	int WSAAPI New_send(SOCKET s, const char* buf, int len, int flags);
	int WSAAPI New_recv(SOCKET s, char* buf, int len, int flags);

	void __cdecl New_PlaySound(int a, int b, int c);
	void __cdecl New_BattleProc();
	void __cdecl New_BattleCommandReady();

	//Sleep
	using pfnSleep = void(WINAPI*)(DWORD dwMilliseconds);
	pfnSleep pSleep = nullptr;

	using pfnsocket = SOCKET(WSAAPI*)(int af, int type, int protocol);
	pfnsocket psocket = nullptr;

	int(WSAAPI* pWSARecv)(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags,
		LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine) = nullptr;

	using pfnclosesocket = int(WSAAPI*)(SOCKET s);
	pfnclosesocket pclosesocket = nullptr;

	using pfnrecv = int(WSAAPI*)(SOCKET s, char* buf, int len, int flags);
	pfnrecv precv = nullptr;

	using pfnsend = int(WSAAPI*)(SOCKET s, const char* buf, int len, int flags);
	pfnsend psend = nullptr;

	using pfnPlaySound = void(__cdecl*)(int, int, int);
	pfnPlaySound pPlaySound = nullptr;

	using pfnBattleProc = void(__cdecl*)();
	pfnBattleProc pBattleProc = nullptr;

	using pfnBattleCommandReady = void(__cdecl*)();
	pfnBattleCommandReady pBattleCommandReady = nullptr;
private:
	std::atomic_bool isInitialized_ = false;
	std::unique_ptr<SyncClient> syncClient_ = nullptr;

	int currentMusic_ = 15;
	int currentSound_ = 15;
	bool isBLockPacket_ = false;
};