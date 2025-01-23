#ifndef STDAFX_H_
#define STDAFX_H_
#pragma execution_character_set("utf-8")
#include "sdkddkver.h" // This is the SDK version header file.
//#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <windows.h>
#include <psapi.h>
#include <iostream>
#include <stack>
#include <cstring>


#define MINT_USE_SEPARATE_NAMESPACE // If you wonder to use separate namespace, please define the following macro.
#include <MINT/MINT.h>

#include "Detours/detours.h"

#include <Ntdef.h>
#include <comdef.h>
#include <Wbemidl.h>

// Qt
#include <QtCore/QCoreApplication>
#include <QDebug>
#include <QCommandLineParser>
#include <QFile>
#include <QDir>
#include <QUrl>
#include <QThread>
#include <QElapsedTimer>
#include <QTextCodec>
#include <QRegularExpression>
#include <QHash>
#include <QVariant>
#include <QQueue>
#include <QCache>
#include <QMetaMethod>
#include <QMetaObject>
#include <QReadWriteLock>
#include <QPointer>
#include <QScopedPointer>
#include <QSharedPointer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QPoint>
#include <QTextCodec>

#include <QtConcurrent/QtConcurrent>
#include <QNetworkProxy>


#include <tool.h>
#include <timer.h>

// custom
#include <binary.h>
#include <global.h>
#include "sahook/game/include/gamegloabl.h"
#include <usermessage.h>

#endif // STDAFX_H_