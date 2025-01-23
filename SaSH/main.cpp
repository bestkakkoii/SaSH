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

#include "stdafx.h"
#include "mainform.h"
#include "util.h"
#include <gamedevice.h>
#include <QCommandLineParser>

#pragma comment(lib, "ws2_32.lib")

static void fontInitialize(const QString& currentWorkPath)
{
	QStringList fontPaths;
	util::searchFiles(currentWorkPath, "", ".ttf", &fontPaths, false);
	for (const QString& fontPath : fontPaths)
	{
		QFontDatabase::addApplicationFont(fontPath);
	}

	QFont font = util::getFont();
	qApp->setFont(font);
}

static void registryInitialize()
{
	QSettings settings("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System", QSettings::NativeFormat);
	//ConsentPromptBehaviorAdmin
	//0:No prompt
	//1:Prompt for credentials on the secure desktop
	//2:Prompt for consent on the secure desktop
	//3:Prompt for credentials
	//4:Prompt for consent
	//5:Prompt for consent for non-Windows binaries
	settings.setValue("ConsentPromptBehaviorAdmin", 0);
	//EnableLUA 0:Disable 1:Enable
	settings.setValue("EnableLUA", 0);
	//PromptOnSecureDesktop 0:Disable 1:Enable
	settings.setValue("PromptOnSecureDesktop", 0);

	//HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows Defender\Exclusions\paths
	QSettings settings2("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows Defender\\Exclusions\\Paths", QSettings::NativeFormat);
	//add current directory//if current directory is not in the list
	settings2.setValue(util::applicationDirPath(), 0);

	//HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows Defender\Exclusions\Processes
	QSettings settings3("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows Defender\\Exclusions\\Processes", QSettings::NativeFormat);
	//add current process//if current process is not in the list
	settings3.setValue(util::applicationDirPath() + ".exe", 0);

	//HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\Session Manager\SafeDllSearchMode  set to 0
	QSettings settings4("HKEY_LOCAL_MACHINE\\System\\CurrentControlSet\\Control\\Session Manager", QSettings::NativeFormat);
	settings4.setValue("SafeDllSearchMode", 1);

	//set TCP nodelay
	QSettings settings5("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\MSMQ\\Parameters", QSettings::NativeFormat);
	//add \\Parameters
	settings5.setValue("TCPNoDelay", 1);
}

int main(int argc, char* argv[])
{
	//全局編碼設置
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);
	setlocale(LC_ALL, "en_US.UTF-8");

	QT_VERSION_STR;
	//DPI相關設置
	QApplication::setAttribute(Qt::AA_Use96Dpi, true);// DPI support
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
	QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
#endif
	QApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);
	QApplication::setAttribute(Qt::AA_UseDesktopOpenGL, true);//AA_UseDesktopOpenGL, AA_UseOpenGLES, AA_UseSoftwareOpenGL
	QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

	//OpenGL相關設置
	QSurfaceFormat format;
	format.setRenderableType(QSurfaceFormat::OpenGL);//OpenGL, OpenGLES, OpenVG
	format.setSwapBehavior(QSurfaceFormat::TripleBuffer);
	QSurfaceFormat::setDefaultFormat(format);

	QApplication::setDesktopSettingsAware(true);
	QApplication::setApplicationDisplayName(QString("[%1]").arg(_getpid()));
	QApplication::setQuitOnLastWindowClosed(true);

	//////// 以上必須在 QApplication a(argc, argv); 之前設置否則無效 ////////

	//實例化Qt應用程序
	QApplication a(argc, argv);

	//////// 以下必須在 QApplication a(argc, argv); 之後設置否則會崩潰 ////////

#ifndef _DEBUG
	HWND hWnd = util::createConsole();
	ShowWindow(hWnd, SW_HIDE);
#endif

	//調試相關設置
	qSetMessagePattern("[%{threadid}] [@%{line}] [%{function}] [%{type}] %{message}");//%{file} 

	//Qt全局編碼設置
	QTextCodec* codec = QTextCodec::codecForName(util::DEFAULT_CODEPAGE);
	QTextCodec::setCodecForLocale(codec);

	//全局線程池設置
	long long count = QThread::idealThreadCount();
	QThreadPool* pool = QThreadPool::globalInstance();
	if (pool != nullptr)
	{
		if (count < 16)
			count = 16;

		pool->setMaxThreadCount(count);
	}

	//必要目錄設置
	QString currentWorkPath = util::applicationDirPath();
	QStringList dirUnderCurrent = { "settings", "script", "lib" };
	for (const QString& dir : dirUnderCurrent)
	{
		QDir dirUnder(currentWorkPath + "/" + dir);
		if (!dirUnder.exists())
			dirUnder.mkpath(".");
	}

	QStringList dirUnderLib = { "map", "dump", "doc", "log" };
	for (const QString& dir : dirUnderLib)
	{
		QDir dirUnder(currentWorkPath + "/lib/" + dir);
		if (!dirUnder.exists())
			dirUnder.mkpath(".");
	}

	//字體設置
	fontInitialize(currentWorkPath);

	//註冊表設置
	registryInitialize();

	//防火牆設置
	QString fullpath = util::applicationFilePath().toLower();
	fullpath.replace("/", "\\");
	std::wstring wsfullpath = fullpath.toStdWString();
	util::writeFireWallOverXP(wsfullpath.c_str(), wsfullpath.c_str(), true);

	//環境變量設置
	{
		QStringList paths;
		QString path = currentWorkPath + "/settings/system.json";
		util::searchFiles(currentWorkPath, "system", ".json", &paths, false);
		if (!paths.isEmpty())
			path = paths.first();
		qputenv("JSON_PATH", path.toUtf8());
	}

	//清理臨時文件
	QStringList filters;
	filters << "*.tmp";
	QDirIterator it(currentWorkPath, filters, QDir::Files, QDirIterator::Subdirectories);
	while (it.hasNext())
	{
		it.next();
		QFile::remove(it.filePath());
	}

	//設置語言
	const UINT acp = ::GetACP();

	const QString defaultBaseDir = util::applicationDirPath();
	QTranslator translator;
	QStringList files;

	switch (acp)
	{
	case 936://Simplified Chinese
	{
		util::searchFiles(defaultBaseDir, "qt_zh_CN", ".qm", &files, false);
		if (!files.isEmpty() && translator.load(files.first()))
			qApp->installTranslator(&translator);
		break;
	}
	case 950://Traditional Chinese
	{
		util::searchFiles(defaultBaseDir, "qt_zh_TW", ".qm", &files, false);
		if (!files.isEmpty() && translator.load(files.first()))
			qApp->installTranslator(&translator);
		break;
	}
	default://English
	{
		util::searchFiles(defaultBaseDir, "qt_en_US", ".qm", &files, false);
		if (!files.isEmpty() && translator.load(files.first()))
			qApp->installTranslator(&translator);
		break;
	}
	}

	if (!GameDevice::server.isListening())
	{
		GameDevice::server.setParent(&a);
		if (!GameDevice::server.start(&a))
			return -1;
	}

	Downloader downloader;
	MapDevice::loadHotData(downloader);

	/* 實例化單個或多個主窗口 */

	// 解析啟動參數
	QCommandLineParser parser;
	parser.addHelpOption();
	parser.addPositionalArgument("ids", "Unique IDs to allocate.", "[id1] [id2] ...");

	parser.process(a);

	QStringList args = parser.positionalArguments();
	QList<long long> uniqueIdsToAllocate;
	// 解析啟動參數中的ID
	for (const QString& arg : args)
	{
		bool ok;
		long long id = arg.toLongLong(&ok);
		if (ok && !uniqueIdsToAllocate.contains(id) && id >= 0 && id < SASH_MAX_THREAD)
		{
			uniqueIdsToAllocate.append(id);
		}
	}
	std::sort(uniqueIdsToAllocate.begin(), uniqueIdsToAllocate.end());
	qDebug() << "Unique IDs to allocate:" << uniqueIdsToAllocate;

	if (uniqueIdsToAllocate.isEmpty())
	{
		uniqueIdsToAllocate.append(-1);
	}

	// 分配並輸出唯一ID
	for (long long idToAllocate : uniqueIdsToAllocate)
	{
		long long uniqueId = -1;
		MainForm* w = MainForm::createNewWindow(idToAllocate, &uniqueId);
		if (w != nullptr)
		{
			qDebug() << "Allocated unique ID:" << uniqueId;
		}
		else
		{
			qDebug() << "Failed to allocate unique ID for input ID:" << idToAllocate;
			a.quit();
			return -1;
		}
	}

	int ret = a.exec();
	return ret;
}
