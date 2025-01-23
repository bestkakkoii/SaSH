#include "stdafx.h"
#include <QCoreApplication>
#include <QDebug>
#include <QObject>
#include <iostream>
#include "sahook/websocket/include/websocketclient.h"
#include "sahook/game/include/gameservice.h"

class QtThread : public QThread
{
public:
	QtThread() = default;
	virtual ~QtThread()
	{
		if (isRunning())
		{
			quit();
			wait();
		}
	}

protected:
	virtual void run() override
	{
		web::SocketClient& webSocketClient = web::SocketClient::instance();

		game::Service& service = game::Service::instance();

		HWND windowId = tool::getCurrentWindowHandle();
		service.setMainWindow(windowId);
		webSocketClient.setWindowId(windowId);
		service.setWindowProcEnable(true);

		QObject::connect(&webSocketClient, &web::SocketClient::connected, &service, &game::Service::handleServerConnected, Qt::QueuedConnection);
		QObject::connect(&webSocketClient, &web::SocketClient::messageReceived, &service, &game::Service::handleServerMessage, Qt::QueuedConnection);
		QObject::connect(&service, &game::Service::sendWebSocketBinaryMessage, &webSocketClient, &web::SocketClient::sendBinaryMessage, Qt::QueuedConnection);
		QObject::connect(&service, &game::Service::sendWebSocketTextMessage, &webSocketClient, &web::SocketClient::sendTextMessage, Qt::QueuedConnection);

		webSocketClient.connectToServer(QUrl("ws://localhost:12345"));
		exec();
	}
};
QtThread qtThread;

// message handler
static void qtMessageHandler(QtMsgType, const QMessageLogContext&, const QString& msg)
{
	std::cout << msg.toUtf8().toStdString() << std::endl;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID)
{
	static bool isHooked = false;
	static int argc = 0;
	static char* argv[] = { nullptr };

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		if (isHooked)
		{
			break;
		}

		isHooked = true;

		QCoreApplication* app = QCoreApplication::instance();
		if (nullptr == app)
		{
			app = new QCoreApplication(argc, argv);
		}

		QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
		SetConsoleCP(CP_UTF8);
		SetConsoleOutputCP(CP_UTF8);
		qInstallMessageHandler(qtMessageHandler);

		qtThread.start();

		DisableThreadLibraryCalls(hModule);
		break;
	}
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	{
		break;
	}
	case DLL_PROCESS_DETACH:
	{
		if (!isHooked)
		{
			break;
		}

		QCoreApplication* app = QCoreApplication::instance();
		if (nullptr != app)
		{
			qtThread.quit();
			qtThread.wait();
			delete app;
		}
		break;
	}
	}
	return TRUE;
}
