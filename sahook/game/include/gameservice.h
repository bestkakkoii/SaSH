#ifndef GAMESERVICE_H_
#define GAMESERVICE_H_

#include <windows.h>
#include <shared_mutex>
#include <QObject>
#include <QScopedPointer>


namespace game
{
	enum WINDOW_MESSAGE
	{
		WM_INITIALIZE = WM_USER + 1,
		WM_UNINITIALIZE,
		WM_SEND_PACKET,
		WM_ANNOUNCE,

		WM_CLOSE_DISCONNECT_DIALOG,
		WM_SET_SAVE_USERNAME_LOOP,
		WM_SET_SELECT_SERVER_LOOP,
		WM_SET_SELECT_SUBSERVER_LOOP,
		WM_SET_SELECT_CHARACTER_LOOP,

		WM_CLOSE_ALL_DIALOG,

		WM_MOVE_CHARACTER,

		WM_TEST,
	};

	class ServicePrivate;

	class Service : public QObject
	{
		Q_OBJECT
	public:
		static Service& instance();

		Service(QObject* parent = nullptr);
		virtual ~Service();

		BOOL __fastcall initialize(int index, HWND parentWId);
		BOOL __fastcall uninitialize();

		void setMainWindow(HWND mainWindow);

		void setWindowProcEnable(bool enable);

		BOOL __fastcall postMessage(UINT Msg, WPARAM wParam, LPARAM lParam, HWND hWnd = nullptr) const;

		BOOL __fastcall sendMessage(UINT Msg, WPARAM wParam, LPARAM lParam, HWND hWnd = nullptr) const;

		LRESULT callOrigicalWndProc(HWND hWnd, UINT message, LPARAM wParam, LPARAM lParam);

	public:
		int __fastcall getWorldStatue() const;
		int __fastcall getGameStatue() const;

	public:
		BOOL __fastcall WM_EnableForwardSend(BOOL enable);
		BOOL __fastcall WM_EnableEffect(BOOL enable);
		BOOL __fastcall WM_EnableCharShow(BOOL enable);
		BOOL __fastcall WM_SetTimeLock(BOOL enable, unsigned int time);
		BOOL __fastcall WM_EnableSound(BOOL enable);
		BOOL __fastcall WM_EnableImageLock(BOOL enable);
		BOOL __fastcall WM_EnablePassWall(BOOL enable);
		BOOL __fastcall WM_EnableFastWalk(BOOL enable);
		BOOL __fastcall WM_SetBoostSpeed(BOOL enable, int speed);
		BOOL __fastcall WM_EnableMoveLock(BOOL enable);
		BOOL __fastcall WM_MuteSound(BOOL enable);
		BOOL __fastcall WM_BattleTimeExtend(BOOL enable);
		BOOL __fastcall WM_EnableBattleDialog(BOOL enable);
		BOOL __fastcall WM_SetGameStatus(int status);
		BOOL __fastcall WM_SetWorldStatus(int status);
		BOOL __fastcall WM_SetOptimize(BOOL enable);
		BOOL __fastcall WM_SetWindowHide(BOOL enable);
		BOOL __fastcall WM_Announce(char* str, int color);
		BOOL __fastcall WM_Move(int x, int y);
		BOOL __fastcall WM_DistoryDialog();
		BOOL __fastcall WM_CleanChatHistory();
		BOOL __fastcall WM_CreateDialog(int type, int button, const char* data);

		BOOL __fastcall WM_SetBlockPacket(BOOL enable);

	public://hook
		SOCKET __fastcall New_socket(int af, int type, int protocol) const;
		int __fastcall New_closesocket(SOCKET s);
		int __fastcall New_send(SOCKET s, const char* buf, int len, int flags);
		int __fastcall New_recv(SOCKET s, char* buf, int len, int flags);
		int __fastcall New_connect(SOCKET s, const struct sockaddr* name, int namelen) const;

		BOOL __fastcall New_SetWindowTextA(HWND hWnd, LPCSTR lpString);
		void __fastcall New_Sleep(DWORD dwMilliseconds);

		void __fastcall New_PlaySound(int a, int b, int c) const;
		void __fastcall New_BattleProc() const;
		void __fastcall New_BattleCommandReady();
		void __fastcall New_TimeProc(int fd) const;
		void __fastcall New_lssproto_EN_recv(int fd, int result, int field) const;
		void __fastcall New_lssproto_B_recv(int fd, char* command) const;
		void __fastcall New_lssproto_WN_send(int fd, int x, int y, int dialogid, int unitid, int select, const char* data);
		void __fastcall New_lssproto_TK_send(int fd, int x, int y, const char* message, int color, int area);
		void __fastcall New_lssproto_W2_send(int fd, int x, int y, const char* dir) const;
		void __fastcall New_CreateDialog(int unk, int type, int button, int unitid, int dialogid, const char* data);

	signals:
		void sendWebSocketBinaryMessage(const QByteArray& message);
		void sendWebSocketTextMessage(const QString& message);

	public slots:
		void handleServerMessage(const QByteArray& message);

		void handleServerConnected();

	private:

		void detours();

		void undetours();

	private:
		static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, LPARAM wParam, LPARAM lParam);

	private:
		Q_DECLARE_PRIVATE(Service);
		Q_DISABLE_COPY_MOVE(Service);
		QScopedPointer<ServicePrivate> d_ptr;
	};
}

#endif // GAMESERVICE_H_