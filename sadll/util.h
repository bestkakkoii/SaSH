#ifndef UTIL_H
#define UTIL_H

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
		kEnableCharShow,
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
		kEnableWindowHide,

		//Action
		kSendAnnounce,
		kSetMove,
		kDistoryDialog,
		kCleanChatHistory,
		kCreateDialog,
	};

	using handle_data = struct
	{
		DWORD dwProcessId;
		HWND hWnd;
	};

	inline BOOL WINAPI IsCurrentWindow(const HWND hWnd)
	{
		if ((nullptr == GetWindow(hWnd, GW_OWNER)) && (TRUE == IsWindowVisible(hWnd)))
			return TRUE;
		else
			return FALSE;
	}

	inline BOOL CALLBACK EnumWindowsCallback(HWND hWnd, LPARAM lParam)
	{
		handle_data& data = *reinterpret_cast<handle_data*>(lParam);
		DWORD process_id = 0UL;

		GetWindowThreadProcessId(hWnd, &process_id);

		if ((data.dwProcessId != process_id) || (FALSE == IsCurrentWindow(hWnd)))
			return TRUE;

		data.hWnd = hWnd;
		return FALSE;
	}

	inline HWND WINAPI FindCurrentWindow(DWORD dwProcessId)
	{
		handle_data data = {};
		data.dwProcessId = dwProcessId;
		data.hWnd = nullptr;

		EnumWindows(EnumWindowsCallback, reinterpret_cast<LPARAM>(&data));

		return data.hWnd;
	}

	inline HWND WINAPI GetCurrentWindowHandle()
	{
		DWORD process_id = GetCurrentProcessId();
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
		DWORD dwOldProtect = 0UL;
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
		DWORD hookfunAddr = 0UL;
		getFuncAddr(&hookfunAddr, pfnHookFunc);//獲取函數HOOK目標函數指針(地址)
		DWORD dwOffset = (hookfunAddr + offest) - (DWORD)bOri - nByteSize;//計算偏移

		MemoryMove((DWORD)&bNew[nByteSize - 4u], &dwOffset, sizeof(dwOffset));//將計算出的結果寫到緩存 CALL XXXXXXX 或 JMP XXXXXXX

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

#endif //UTIL_H