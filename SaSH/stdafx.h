#pragma once
#if _MSC_VER >= 1600 
#pragma execution_character_set("utf-8") 
#endif

#ifndef QT_USE_QSTRINGBUILDER
#define QT_USE_QSTRINGBUILDER
#endif

#ifndef QT_NO_CAST_FROM_BYTEARRAY
#define QT_NO_CAST_FROM_BYTEARRAY
#endif

#ifndef QT_NO_CAST_TO_ASCII
#define QT_NO_CAST_TO_ASCII
#endif

//#ifdef QT_NO_DEBUG
//#define QT_NO_DEBUG_OUTPUT
//#define QT_NO_INFO_OUTPUT
//#endif

#if defined __cplusplus
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// default
#include <sdkddkver.h>
#include <WinSock2.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <sstream>
#include <cassert>
#include <cmath>
#include <ctime>
#include <tlhelp32.h>
#include <netcon.h>
#include <netfw.h>
#include <iomanip>
#include <psapi.h>
#include <chrono>
#include <thread>
#include <commdlg.h>
#if _MSVC_LANG > 201703L
#include <ranges>
#endif
#if _MSVC_LANG >= 201703L
#include <memory_resource>
#endif
#include <random>
#include <algorithm>
#include <fstream>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <unordered_map>
#include <dbghelp.h>
#include <iphlpapi.h>
#include <tchar.h>
#include <functional>
#include <limits>
#include <type_traits> 
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// QT header
#include <QtGlobal>
#include <QMetaEnum>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostInfo>
#include <QHostAddress>

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QHttpMultiPart>

#include <QLocalServer>
#include <QLocalSocket>

#include <QDockWidget>
#include <QSystemTrayIcon>
#include <QSysInfo>
#include <QDebug>
#include <QClipboard>
#include <QObject>
#include <QMetaType>
#include <QtWidgets/QApplication>
#include <QCoreApplication>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QDesktopWidget>
#endif
#include <QInputDialog>
#include <QWidget>
#include <QtWidgets/QMainWindow>
#include <QGraphicsDropShadowEffect>
#include <QMenuBar>
#include <QMenu>
#include <QFileDialog>
#include <QDialog>
#include <QLabel>
#include <QWidgetAction>
#include <QListView>
#include <QTableWidget>
#include <QListWidget>
#include <QComboBox>
#include <QGroupBox>
#include <QSpinBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QRadioButton>
#include <QTextEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplashScreen>
#include <QMovie>
#include <QStackedWidget>
#include <QDesktopServices>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QGridLayout>
//#include <QSoundEffect>
#include <QMessageBox>
#include <QScreen>
#include <QDateTime>
#include <QPushButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QProgressBar>
#include <QScrollBar>


#include <QStringListModel>
#include <QTreeWidgetItem>
#include <QTemporaryFile>
#include <QUrlQuery>
#include <QHeaderView>
#include <QToolTip>
#include <QTextBrowser>
#include <QOpenGLPaintDevice>

// other
#include <QEvent>
#include <QShowEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QEventLoop>
#ifdef USE_DM
#include <ActiveQt\QAxBase.h>
#include <ActiveQt\QAxObject.h>
#endif
#include <QtCore\qmath.h>
#include <QReadWriteLock>
#include <QLockFile>
#include <QOperatingSystemVersion>
#include <QDate>
#include <QWaitCondition>
#include <QRandomGenerator>
#include <QElapsedTimer>
#include <QTimer>
#include <QRegularExpression>
#include <QSettings>
#include <QMutex>
#include <QSemaphore>
#include <QProcess>
#include <QCryptographicHash>
//#include <QDomNodeList>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QScreen>
#include <QFontDataBase>
#include <QLibrary>
#include <QColor>
#include <QPainterPath>
// io
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QDir>
#include <QIODevice>
#include <QTextCursor>
#include <QTextStream>
#include <QTextCodec>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QGraphicsSvgItem>
#else
#include <QtSvgWidgets/QGraphicsSvgItem>
#ifdef _DEBUG
#pragma comment(lib, "Qt6SvgWidgetsd.lib")
#else
#pragma comment(lib, "Qt6SvgWidgets.lib")
#endif
#endif

// container
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QPair>
#include <QVector>
#include <QList>
#include <QMap>
#include <QHash>
#include <QStack>
#include <QPoint>
#include <QQueue>
#include <QMultiHash>
#include <QSet>
// smart pointer
//#include <QPointer>
//#include <QWeakPointer>
//#include <QSharedPointer>
// concurrent
#include <QtConcurrentRun>
#include <QtConcurrent/QtConcurrent>
#include <QFuture>
#include <QFutureSynchronizer>
// thread
#include <QThread>
#include <QThreadPool>

#include <QCryptographicHash>
#include <QStyleFactory>
#include <QGraphicsView>
#include <QGraphicsTextItem>
#include <QGraphicsSceneEvent>
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 3rd parties
#include <VLD/vld.h>

#ifndef SOL_ALL_SAFETIES_ON
#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>
#endif

#ifndef MINT_USE_SEPARATE_NAMESPACE
#define MINT_USE_SEPARATE_NAMESPACE
#include <MINT/MINT.h>
#endif

#include <3rdparty/zip.h>
#include <3rdparty/unzip.h>

//custom
#include "script/crypto.h"
#include <model/indexer.h>
#include <model/scopedhandle.h>
#include <model/combobox.h>
#include <model/listview.h>
#include <model/treewidgetitem.h>
#include <model/signaldispatcher.h>
#include <model/fastlabel.h>
#include <model/customtitlebar.h>
#include <model/mapglwidget.h>
#include <model/progressbar.h>
#include <model/pushButton.h>
#include <model/qthumbnailform.h>
#include <model/qthumbnailwidget.h>
#include <model/codeeditor.h>
#include <model/treewidget.h>
#include <model/treewidgetitem.h>
#include <model/tablewidget.h>

#include <model/safe.h>

#include <net/descrypt.h>
#include <net/macchanger.h>

//#include "update/curldownload.h"

#include "map/astardevice.h"
#include <usermessage.h>
#include "model/builddatetime.h"
#include "interfacer.h"


#include "globalmicro.h"
#include "net/database.h"



#endif // __cplusplus
