#pragma once
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
	static Class& getInstance() {\
		static Class *instance = new Class();\
		return *instance;\
	}
#endif

#ifdef _DEBUG
#else
#define _NDEBUG
#endif

namespace util
{
	enum UserMessage
	{
		kUserMessage = WM_USER + 0x1024,
		kConnectionOK,
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
		kSetBlockPacket,
		kBattleTimeExtend,
		kEnableOptimize,

		//Action
		kSendAnnounce,
		kSetMove,
		kDistoryDialog,
		kCleanChatHistory,
		kCreateDialog,
	};

	using handle_data = struct
	{
		unsigned long process_id;
		HWND window_handle;
	};

	inline bool IsCurrentWindow(const HWND handle)
	{
		if ((GetWindow(handle, GW_OWNER) == nullptr) && (IsWindowVisible(handle)))
			return true;
		else
			return false;
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

	HWND FindCurrentWindow(const unsigned long process_id)
	{
		handle_data data = {};
		data.process_id = process_id;
		data.window_handle = 0;
		EnumWindows(EnumWindowsCallback, reinterpret_cast<LPARAM>(&data));
		return data.window_handle;
	}

	HWND GetCurrentWindowHandle()
	{
		unsigned long process_id = GetCurrentProcessId();
		HWND hwnd = nullptr;
		while (nullptr == hwnd)
		{
			hwnd = FindCurrentWindow(process_id);
		}
		return hwnd;
	}

	template<class T, class T2>
	inline void MemoryMove(T dis, T2* src, size_t size)
	{
		DWORD dwOldProtect = 0;
		VirtualProtect((void*)dis, size, PAGE_EXECUTE_READWRITE, &dwOldProtect);
		memcpy((void*)dis, (void*)src, size);
		VirtualProtect((void*)dis, size, dwOldProtect, &dwOldProtect);
	}

	template <class ToType, class FromType>
	void __stdcall getFuncAddr(ToType* addr, FromType f)
	{
		union
		{
			FromType _f;
			ToType   _t;
		}ut{};

		ut._f = f;

		*addr = ut._t;
	}

	/// <summary>
	/// HOOK函數
	/// </summary>
	/// <typeparam name="T">跳轉的全局或類函數或某個地址段</typeparam>
	/// <typeparam name="T2">BYTE數組</typeparam>
	/// <param name="pfnHookFunc">要 CALL 或 JMP 的函數或地址 </param>
	/// <param name="bOri">要寫入HOOK的地址</param>
	/// <param name="bOld">用於保存原始數據的BYTE數組，取決於寫入地址原始匯編占多大</param>
	/// <param name="bNew">用於寫入跳轉或CALL的BYTE數組要預先填好需要填充的0x90或0xE8 0xE9</param>
	/// <param name="nByteSize">bOld bNew數組的大小</param>
	/// <param name="offest">有時候跳轉目標地址前面可能會有其他東西會需要跳過則需要偏移，大部分時候為 0 </param>
	/// <returns></returns>
	template<class T, class T2>
	void __stdcall detour(T pfnHookFunc, DWORD bOri, T2* bOld, T2* bNew, const size_t nByteSize, const DWORD offest)
	{
		DWORD hookfunAddr = 0;
		getFuncAddr(&hookfunAddr, pfnHookFunc);//獲取函數HOOK目標函數指針(地址)
		DWORD dwOffset = (hookfunAddr + offest) - (DWORD)bOri - nByteSize;//計算偏移
		MemoryMove((DWORD)&bNew[nByteSize - 4], &dwOffset, sizeof(dwOffset));//將計算出的結果寫到緩存 CALL XXXXXXX 或 JMP XXXXXXX
		MemoryMove((DWORD)bOld, (void*)bOri, nByteSize);//將原始內容保存到bOld (之後需要還原時可用)
		MemoryMove((DWORD)bOri, bNew, nByteSize);//將緩存內的東西寫到要HOOK的地方(跳轉到hook函數 或調用hook函數)
	}

	/// <summary>
	/// 取消HOOK
	/// </summary>
	/// <typeparam name="T">BYTE數組</typeparam>
	/// <param name="ori">要還原的地址段</param>
	/// <param name="oldBytes">備份用的BYTE數組指針</param>
	/// <param name="size">BYTE數組大小</param>
	/// <returns></returns>
	template<class T>
	void  __stdcall undetour(T ori, BYTE* oldBytes, SIZE_T size)
	{
		MemoryMove(ori, oldBytes, size);
	}
}

#define USE_ASYNC_TCP

#ifdef USE_ASYNC_TCP
class AsyncClient;
#else
class SyncClient;
#endif

class GameService
{
	DISABLE_COPY_MOVE(GameService);
	GameService() = default;

public:
	virtual ~GameService() = default;
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
	void WM_SetOptimize(bool enable);

	void WM_Announce(char* str, int color);
	void WM_Move(int x, int y);
	void WM_DistoryDialog();
	void WM_CleanChatHistory();
	void WM_CreateDialog(int button, const char* data);

	inline void WM_SetBLockPacket(bool enable) { IS_ENCOUNT_BLOCK_FLAG = enable; }

public://g-var
	int* g_sockfd = nullptr;

private:
	int* g_world_status = nullptr;
	int* g_game_status = nullptr;

	bool IS_BATTLE_PROC_FLAG = false;
	bool IS_TIME_LOCK_FLAG = false;
	bool IS_SOUND_MUTE_FLAG = false;
	bool IS_ENCOUNT_BLOCK_FLAG = false;

	bool IS_MOVE_LOCK = false;

public://hook
	SOCKET WSAAPI New_socket(int af, int type, int protocol);

	int WSAAPI New_closesocket(SOCKET s);

	int WSAAPI New_send(SOCKET s, const char* buf, int len, int flags);

	int WSAAPI New_recv(SOCKET s, char* buf, int len, int flags);

	BOOL WINAPI New_SetWindowTextA(HWND hWnd, LPCSTR lpString);

	DWORD WINAPI New_GetTickCount();

	BOOL WINAPI New_QueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount);

	DWORD WINAPI New_TimeGetTime();

	void WINAPI New_Sleep(DWORD dwMilliseconds);

	void __cdecl New_PlaySound(int a, int b, int c);
	void __cdecl New_BattleProc();
	void __cdecl New_BattleCommandReady();
	void __cdecl New_TimeProc(int fd);
	void __cdecl New_lssproto_EN_recv(int fd, int result, int field);
	void __cdecl New_lssproto_WN_send(int fd, int x, int y, int seqno, int objindex, int select, const char* data);
	void __cdecl New_lssproto_TK_send(int fd, int x, int y, const char* message, int color, int area);
	//setwindowtexta
	using pfnSetWindowTextA = BOOL(__stdcall*)(HWND hWnd, LPCSTR lpString);
	pfnSetWindowTextA pSetWindowTextA = nullptr;

	using pfnsocket = SOCKET(__stdcall*)(int af, int type, int protocol);
	pfnsocket psocket = nullptr;

	using pfnclosesocket = int(__stdcall*)(SOCKET s);
	pfnclosesocket pclosesocket = nullptr;

	using pfnrecv = int(__stdcall*)(SOCKET s, char* buf, int len, int flags);
	pfnrecv precv = nullptr;

	using pfnsend = int(__stdcall*)(SOCKET s, const char* buf, int len, int flags);
	pfnsend psend = nullptr;

	//DWORD WINAPI New_GetTickCount();
	using pfnGetTickCount = DWORD(__stdcall*)();
	pfnGetTickCount pGetTickCount = nullptr;

	using pfnTimeGetTime = DWORD(__stdcall*)();
	pfnTimeGetTime pTimeGetTime = nullptr;

	using pfnSleep = void(__stdcall*)(DWORD dwMilliseconds);
	pfnSleep pSleep = nullptr;

	//BOOL WINAPI New_QueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount);
	using pfnQueryPerformanceCounter = BOOL(__stdcall*)(LARGE_INTEGER*);
	pfnQueryPerformanceCounter pQueryPerformanceCounter = nullptr;

	using pfnPlaySound = void(__cdecl*)(int, int, int);
	pfnPlaySound pPlaySound = nullptr;

	using pfnBattleProc = void(__cdecl*)();
	pfnBattleProc pBattleProc = nullptr;

	DWORD* pBattleCommandReady = nullptr;

	using pfnTimeProc = void(__cdecl*)(int);
	pfnTimeProc pTimeProc = nullptr;

	using pfnLssproto_EN_recv = void(__cdecl*)(int, int, int);
	pfnLssproto_EN_recv pLssproto_EN_recv = nullptr;

	using pfnLssproto_WN_send = void(__cdecl*)(int, int, int, int, int, int, const char*);
	pfnLssproto_WN_send pLssproto_WN_send = nullptr;

	using pfnLssproto_TK_send = void(__cdecl*)(int, int, int, const char*, int, int);
	pfnLssproto_TK_send pLssproto_TK_send = nullptr;

private:
	void hideModule(HMODULE hLibrary);
	void Send(const std::string& text);
private:
	std::atomic_bool isInitialized_ = false;

#ifdef USE_ASYNC_TCP
	std::unique_ptr<AsyncClient> asyncClient_ = nullptr;
#else
	std::unique_ptr<SyncClient> syncClient_ = nullptr;
#endif

	BYTE oldBattleCommandReadyByte[6] = {}; //保存舊數據用於還原

	int currentMusic_ = 15;
	int currentSound_ = 15;

	DWORD speedBoostValue = 1;
};