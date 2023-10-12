#pragma once
enum InterfaceMessage
{
	kInterfaceMessage = WM_USER + 2048,
	kRunScript,			// kInterfaceMessage + 1
	kStopScript,		// kInterfaceMessage + 2
	kRunFile,			// kInterfaceMessage + 3
	kStopFile,			// kInterfaceMessage + 4
	kRunGame,			// kInterfaceMessage + 5
	kCloseGame,			// kInterfaceMessage + 6
	kGetGameState,		// kInterfaceMessage + 7
	kScriptState,		// kInterfaceMessage + 8
	kMultiFunction,		// kInterfaceMessage + 9
	kSortWindow,		// kInterfaceMessage + 10
	kThumbnail,			// kInterfaceMessage + 11
	kOpenNewWindow,		// kInterfaceMessage + 12
	kGetGamePid,		// kInterfaceMessage + 13
	kGetGameHwnd, 	    // kInterfaceMessage + 14
	kGetCurrentId, 	    // kInterfaceMessage + 15
	kSetAutoLogin, 	    // kInterfaceMessage + 16
	kLoadSettings, 	    // kInterfaceMessage + 17
};

enum InterfaceFunctionType
{
	WindowNone = 0,		//無
	WindowInfo,			//信息窗口
	WindowMap,			//地圖窗口
	WindowScript,		//腳本窗口
	SelectServerList,   //選服列表
	SelectProcessList,  //選進程列表
	ToolTrayShow,		//系統托盤顯示
	HideGame,			//隱藏遊戲
};

typedef struct tagLoginInfo
{
	int server = 0;
	int subserver = 0;
	int position = 0;
	char* username = nullptr;
	char* password = nullptr;
}LoginInfo;

class InterfaceSender : public QObject {
	Q_OBJECT

public:
	explicit InterfaceSender(HWND targetWindow) : targetWindow_(targetWindow) {}

	__int64 RunScript(__int64 windowID, const QString& scriptCode) {
		DWORD_PTR result = 0;
		if (SendMessageTimeoutW(targetWindow_, kRunScript, windowID, (LPARAM)scriptCode.toUtf8().constData(), SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, 10000u, &result) != 0) {
			return static_cast<__int64>(result);
		}
		return -1;
	}

	__int64 StopScript(__int64 windowID) {
		DWORD_PTR result = 0;
		if (SendMessageTimeoutW(targetWindow_, kStopScript, windowID, 0, SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, 10000u, &result) != 0) {
			return static_cast<__int64>(result);
		}
		return -1;
	}

	__int64 RunFile(__int64 windowID, const QString& filePath) {
		DWORD_PTR result = 0;
		if (SendMessageTimeoutW(targetWindow_, kRunFile, windowID, (LPARAM)filePath.toUtf8().constData(), SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, 10000u, &result) != 0) {
			return static_cast<__int64>(result);
		}
		return -1;
	}

	__int64 StopFile(__int64 windowID) {
		DWORD_PTR result = 0;
		if (SendMessageTimeoutW(targetWindow_, kStopFile, windowID, 0, SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, 10000u, &result) != 0) {
			return static_cast<__int64>(result);
		}
		return -1;
	}

	__int64 RunGame(__int64 windowID) {
		DWORD_PTR result = 0;
		if (SendMessageTimeoutW(targetWindow_, kRunGame, windowID, 0, SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, 10000u, &result) != 0) {
			return static_cast<__int64>(result);
		}
		return -1;
	}

	__int64 CloseGame(__int64 windowID) {
		DWORD_PTR result = 0;
		if (SendMessageTimeoutW(targetWindow_, kCloseGame, windowID, 0, SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, 10000u, &result) != 0) {
			return static_cast<__int64>(result);
		}
		return -1;
	}

	__int64 GetGameState(__int64 windowID) {
		DWORD_PTR result = 0;
		if (SendMessageTimeoutW(targetWindow_, kGetGameState, windowID, 0, SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, 10000u, &result) != 0) {
			return static_cast<__int64>(result);
		}
		return -1;
	}

	__int64 GetScriptState(__int64 windowID) {
		DWORD_PTR result = 0;
		if (SendMessageTimeoutW(targetWindow_, kScriptState, windowID, 0, SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, 10000u, &result) != 0) {
			return static_cast<__int64>(result);
		}
		return -1;
	}

	__int64 MultiFunction(__int64 windowID, __int64 functionType, __int64 lParam) {
		__int64 nLParam = (functionType & 0xffff) | ((lParam & 0xffff) << 16);
		DWORD_PTR result = 0;
		if (SendMessageTimeoutW(targetWindow_, kMultiFunction, windowID, nLParam, SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, 10000u, &result) != 0) {
			return static_cast<__int64>(result);
		}
		return -1;
	}

	__int64 SortWindow(bool alignLeft, const QString& windowList) {
		__int64 wParam = alignLeft ? 0 : 1;
		DWORD_PTR result = 0;
		if (SendMessageTimeoutW(targetWindow_, kSortWindow, wParam, (LPARAM)windowList.toUtf8().constData(), SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, 10000u, &result) != 0) {
			return static_cast<__int64>(result);
		}
		return -1;
	}

	__int64 Thumbnail(const QString& thumbnailInfo) {
		DWORD_PTR result = 0;
		if (SendMessageTimeoutW(targetWindow_, kThumbnail, 0, (LPARAM)thumbnailInfo.toUtf8().constData(), SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, 10000u, &result) != 0) {
			return static_cast<__int64>(result);
		}
		return -1;
	}

	__int64 OpenNewWindow(__int64 windowID) {
		DWORD_PTR result = 0;
		if (SendMessageTimeoutW(targetWindow_, kOpenNewWindow, windowID, 0, SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, 10000u, &result) != 0) {
			return static_cast<__int64>(result);
		}
		return -1;
	}

	__int64 GetGamePid(__int64 windowID) {
		DWORD_PTR result = 0;
		if (SendMessageTimeoutW(targetWindow_, kGetGamePid, windowID, 0, SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, 10000u, &result) != 0) {
			return static_cast<__int64>(result);
		}
		return -1;
	}

	__int64 GetGameHwnd(__int64 windowID) {
		DWORD_PTR result = 0;
		if (SendMessageTimeoutW(targetWindow_, kGetGameHwnd, windowID, 0, SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, 10000u, &result) != 0) {
			return static_cast<__int64>(result);
		}
		return -1;
	}

	__int64 GetCurrentId(__int64 windowID) {
		DWORD_PTR result = 0;
		if (SendMessageTimeoutW(targetWindow_, kGetCurrentId, windowID, 0, SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, 10000u, &result) != 0) {
			return static_cast<__int64>(result);
		}
		return -1;
	}

	__int64 SetAutoLogin(__int64 windowID, __int64 server, __int64 subserver, __int64 position, const QString& acccount, const QString& password)
	{
		DWORD_PTR result = 0;
		LoginInfo info;
		info.server = server;
		info.subserver = subserver;
		info.position = position;

		QByteArray baAccount = acccount.toUtf8();
		QByteArray baPassword = password.toUtf8();

		info.username = baAccount.data();
		info.password = baPassword.data();

		if (SendMessageTimeoutW(targetWindow_, kSetAutoLogin, windowID, (LPARAM)&info, SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, 10000u, &result) != 0)
		{
			return static_cast<__int64>(result);
		}

		return -1;
	}

	__int64 LoadSettings(__int64 windowID, const QString& jsonConfigPath) {
		DWORD_PTR result = 0;
		if (SendMessageTimeoutW(targetWindow_, kLoadSettings, windowID, (LPARAM)jsonConfigPath.toUtf8().constData(), SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, 10000u, &result) != 0) {
			return static_cast<__int64>(result);
		}
		return -1;
	}
private:
	HWND targetWindow_;
};
