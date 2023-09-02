/*
				GNU GENERAL PUBLIC LICENSE
				   Version 2, June 1991
COPYRIGHT (C) Bestkakkoii 2023 All Rights Reserved.
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
#include "qdownloader.h"
#include "curldownload.h"
#include <gdatetime.h>
#include "util.h"

#include <cpr/cpr.h>

//Qt Private
#include <QtGui/private/qzipreader_p.h>
#include <QtGui/private/qzipwriter_p.h>

#ifdef _DEBUG
#pragma comment(lib, "cpr-d.lib")
#pragma comment(lib, "libcurl-d.lib")
#pragma comment(lib, "libcrypto32MDd.lib")
#pragma comment(lib, "libssl32MDd.lib")
#else
#pragma comment(lib, "cpr.lib")
#pragma comment(lib, "libcurl.lib")
#pragma comment(lib, "libcrypto32MD.lib")
#pragma comment(lib, "libssl32MD.lib")
#endif


static std::vector<QProgressBar*> g_vProgressBar;
static QMutex g_mutex;

static std::vector<pfnProgressFunc> g_vpfnProgressFunc;
//constexpr int g_nProcessPrecision = 10;
qreal g_current[MAX_DOWNLOAD_THREAD] = {};
constexpr const char* URL = "https://www.lovesa.cc/SaSH/update/SaSH.7z";
constexpr const char* sz7zEXE_URL = "https://www.lovesa.cc/SaSH/update/7z.exe";
constexpr const char* sz7zDLL_URL = "https://www.lovesa.cc/SaSH/update/7z.dll";
constexpr const char* doc_URL = "https://gitee.com/Bestkakkoii/sash/wikis/pages/export?type=markdown&doc_id=4046472";
constexpr const char* SHA512_7ZEXE = "b46137ff657348f40a74bb63b93c0662bab69ea05f3ef420ea76e6cebb1a3c865194516785c457faa8b819a52c570996fbcfd8a420db83aef7f6136b66412f32";
constexpr const char* SHA512_7ZDLL = "908060f90cfe88aee09c89b37421bc8d755bfc3a9b9539573188d00066fb074c2ef5ca882b8eacc8e15c62efab10f21e0ee09d07e4990c831f8e79a1ff48ff9b";
constexpr const char* kBackupfileNameFormat = "sash_yyyyMMdd";
constexpr const char* kBackupfileName1 = "sash_backup_%1.7z";
constexpr const char* kBackupfileName2 = "sash_backup_%1_%2.7z";
constexpr const char* kBackupExecuteFile = "SaSH.exe";
constexpr const char* kBackupExecuteFileTmp = "SaSH.tmp";
constexpr const char* kDefaultClosingProcessName = "sa_8001.exe";
static const QStringList preBackupFileNames = { kBackupExecuteFile, "sadll.dll", "settings" };
std::atomic_int DO_NOT_SHOW = false;
constexpr int SHADOW_WIDTH = 10;
constexpr int MAX_BAR_HEIGHT = 20;
constexpr int MAX_BAR_SEP_LEN = 10;
constexpr int MAX_GIF_MOVE_WIDTH = 920;
constexpr int PROGRESS_BAR_BEGIN_Y = 85;

QString QDownloader::Sha3_512(const QString& fileNamePath) const
{
	QFile theFile(fileNamePath);
	if (!theFile.exists())
		return "\0";
	theFile.open(QIODevice::ReadOnly);
	const QByteArray ba = QCryptographicHash::hash(theFile.readAll(), QCryptographicHash::Sha3_512);
	theFile.close();
	const QString result = ba.toHex().constData();
	return result;
}

QString g_etag;
constexpr int UPDATE_TIME_MIN = 5 * 60;
bool QDownloader::checkUpdate(QString* current, QString* ptext)
{
	QString exeFileName = QCoreApplication::applicationFilePath();

	{
		util::Config config(qgetenv("JSON_PATH"));
		g_etag = config.read<QString>("System", "Update", "ETag");
		g_etag.replace("\"", "");
	}

	QUrl zipUrl(URL);

	cpr::Header header;
	header.insert({ "If-None-Match", g_etag.toUtf8().constData() });
	std::string zipUrlUtf8 = zipUrl.toString().toUtf8().constData();
	cpr::Response response = cpr::Head(cpr::Url(zipUrlUtf8), header);

	bool bret = false;
	do
	{
		if (response.error.code != cpr::ErrorCode::OK)
		{
			qDebug() << "HTTP request failed: " << response.error.message.c_str();
			break;
		}

		// Get modify time from server
		QDateTime zipModified;
		QDateTime exeModified;
		QLocale locale(QLocale::English);
		bool skipModifyTimeCheck = false;
		if (response.header.count("ETag"))
		{
			QString newEtag = QString::fromUtf8(response.header["ETag"].c_str());
			newEtag.replace("\"", "");
			if ((!newEtag.isEmpty() && !g_etag.isEmpty() && newEtag != g_etag) || g_etag.isEmpty())
			{
				qDebug() << "New version available!" << newEtag;
				g_etag = newEtag;
				bret = true;
				skipModifyTimeCheck = true;
			}
			else
			{
				qDebug() << "No new version available." << newEtag;
			}
		}

		if (response.header.count("Last-Modified"))
		{
			QString lastModifiedStr = QString::fromUtf8(response.header["Last-Modified"].c_str());
			//Tue, 18 Apr 2023 01:01:06 GMT
			qDebug() << "SaSH 7z file last modified time:" << lastModifiedStr;


			QDateTime gmtTime = locale.toDateTime(lastModifiedStr, "ddd, dd MMM yyyy hh:mm:ss 'GMT'");
			//補-8小時
			zipModified = gmtTime.addSecs(8ll * 60ll * 60ll);

			qDebug() << "SaSH 7z file modified time:" << zipModified.toString("yyyy-MM-dd hh:mm:ss");
			if (!zipModified.isValid())
			{
				qDebug() << "Failed to convert date string to QDateTime object.";
				break;
			}
			else
			{
				if (ptext)
					*ptext = zipModified.toString("yyyy-MM-dd hh:mm:ss");
			}
		}
		else
		{
			qWarning() << "No Last-Modified header available.";
			break;
		}

		util::buildDateTime(&exeModified);
		qDebug() << "SaSH.exe file modified time:" << exeModified.toString("yyyy-MM-dd hh:mm:ss");

		if (current != nullptr)
			*current = exeModified.toString("yyyy-MM-dd hh:mm:ss");

		if (skipModifyTimeCheck)
			break;

		int timeDiffInSeconds = exeModified.secsTo(zipModified);

		// Check if the remote file is newer than the local file
		if (timeDiffInSeconds > UPDATE_TIME_MIN)
		{
			if (zipModified > exeModified)
			{
				qDebug() << "SaSH.7z newer than SaSH.exe";
				bret = true;
			}
			else
			{
				qDebug() << "SaSH.exe newer than SaSH.7z, time diff:" << timeDiffInSeconds;
			}
		}
		else if (timeDiffInSeconds >= 0)
		{
			qDebug() << "SaSH.exe and SaSH.7z are the same, time diff:" << timeDiffInSeconds;
		}
		else
		{
			qDebug() << "SaSH.exe newer than SaSH.7z, time diff:" << timeDiffInSeconds;
		}

	} while (false);

	return bret;
}

QDownloader::QDownloader(QWidget* parent)
	: QWidget(parent)
	, szCurrentDirectory_(util::applicationDirPath() + "/")
	, szCurrentDotExe_(szCurrentDirectory_ + kBackupExecuteFile)
	, szCurrentDotExeAsDotTmp_(szCurrentDirectory_ + kBackupExecuteFileTmp)
	, sz7zDotExe_(szCurrentDirectory_ + "7z.exe")
	, sz7zDotDll_(szCurrentDirectory_ + "7z.dll")
	, szSysTmpDir_(QDir::tempPath())
{
	ui.setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose);
	setAttribute(Qt::WA_TranslucentBackground);
	setWindowFlags(this->windowFlags() | Qt::FramelessWindowHint);
	::SetWindowLongW((HWND)winId() \
		, GWL_EXSTYLE, GetWindowLong((HWND)winId(), GWL_EXSTYLE) ^ WS_EX_LAYERED);
	::SetLayeredWindowAttributes((HWND)winId(), NULL, 0, LWA_ALPHA);

	//install font
	QFontDatabase::addApplicationFont(util::applicationDirPath() + "/JoysticMonospace.ttf");
	QFont font("JoysticMonospace", 9);
	setFont(font);

	QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect(ui.widget);
	// 陰影偏移
	shadowEffect->setOffset(0, 0);
	// 陰影顏色;
	shadowEffect->setColor(Qt::black);
	// 陰影半徑;
	shadowEffect->setBlurRadius(SHADOW_WIDTH);
	// 給窗口設置上當前的陰影效果;
	this->setGraphicsEffect(shadowEffect);

	QMovie* movie = new QMovie("://image/jimmy.gif");
	movie->setObjectName("movieLoading");
	movie->setScaledSize(QSize(44, 72));
	ui.label->setMovie(movie);
	movie->start();


	int n = PROGRESS_BAR_BEGIN_Y;
	for (int i = 0; i < MAX_DOWNLOAD_THREAD; ++i)
	{
		g_vProgressBar.push_back(createProgressBar(n));
		n += MAX_BAR_HEIGHT + MAX_BAR_SEP_LEN;
	}


	resetProgress(0);

	g_vpfnProgressFunc.push_back(&onProgress<0>);
	g_vpfnProgressFunc.push_back(&onProgress<1>);
	g_vpfnProgressFunc.push_back(&onProgress<2>);
	g_vpfnProgressFunc.push_back(&onProgress<3>);

	rcPath_ = QString("%1/%2/").arg(szSysTmpDir_).arg(pid_);
	QDir dir(rcPath_);
	if (!dir.exists())
		dir.mkpath(rcPath_);

	//從網址中擷取檔案名稱
	QString szFileName = URL;
	szDownloadedFileName_ = szFileName.mid(szFileName.lastIndexOf('/') + 1);
	szTmpDot7zFile_ = rcPath_ + szDownloadedFileName_;// %Temp%/pid/SaSH.7z

	synchronizer_.setCancelOnWait(false);
	resetProgress(0);
}

QDownloader::~QDownloader()
{
	for (QTimer& it : timer_)
	{
		it.stop();
	}
	resetProgress(0);
	g_vProgressBar.clear();
	g_vpfnProgressFunc.clear();
}

void QDownloader::showEvent(QShowEvent* event)
{
	//首次show
	static bool bFirstShow = true;
	if (bFirstShow)
	{
		bFirstShow = false;
		//原本的show
		QWidget::showEvent(event);
		QApplication::processEvents();

		for (int i = 0; i < MAX_DOWNLOAD_THREAD; ++i)
		{
			connect(&timer_[i], &QTimer::timeout, this,
				[i]()->void
				{
					if (DO_NOT_SHOW.load(std::memory_order_acquire))
					{
						QCoreApplication::processEvents();
						return;
					}

					int percent = qFloor(g_current[i]);
					if (g_vProgressBar[i]->value() != percent)
					{
						g_vProgressBar[i]->setValue(percent);
						g_vProgressBar[i]->repaint();
					}
					QCoreApplication::processEvents();
				}
			);
			timer_[i].start(100);
		}

		connect(&labelTimer_, &QTimer::timeout, this,
			[this]()->void
			{
				//是否全部100%
				qreal sum = 0.0;
				for (const qreal& it : g_current)
				{
					sum += it;
				}
				//計算總和百分比 label按照比例向右移動 總距離為 900 當前進度為 100% x = 900
				qreal percent = sum / static_cast<qreal>(MAX_DOWNLOAD_THREAD);
				ui.label->move(MAX_GIF_MOVE_WIDTH * (percent + 1.0) / 100, 10);
				QCoreApplication::processEvents();

				if (sum < 100.0 * static_cast<qreal>(MAX_DOWNLOAD_THREAD))
				{
					return;
				}

				for (QTimer& it : timer_)
				{
					it.stop();
				}

				overwriteCurrentExecutable();
			}
		);
		labelTimer_.start(100);

		this->start();
	}
}

void QDownloader::start()
{

	QFuture<void>future = QtConcurrent::run([this]()->void
		{
			QString mdFullPath = QString("%1/lib/doc").arg(util::applicationDirPath());
			downloadAndExtractZip(doc_URL, mdFullPath);
		});

	future.waitForFinished();



	synchronizer_.addFuture(QtConcurrent::run([this]() { asyncDownloadFile(URL, rcPath_, szDownloadedFileName_); }));
}

QProgressBar* QDownloader::createProgressBar(int startY)
{
	QProgressBar* pProgressBar = (new QProgressBar(ui.widget));
	constexpr const char* cstyle = R"(
		QProgressBar{
			font:8pt "Joystix Monospace";
			text-align:center;
	
			color: #A6B8E0;
			border-radius:5px;
			border:0px solid #E8EDF2;
			background-color: #5E73A8;
			border-color: rgb(180, 180, 180);
		}
		QProgressBar:chunk{
			border-radius:5px;
			background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:0, stop:0 rgba(252, 136, 201, 220), stop:1 rgba(171, 86, 254, 255));
		}
	)";

	pProgressBar->setStyleSheet(cstyle);
	//width 1000 height 18
	pProgressBar->setGeometry(10, startY, 1000, MAX_BAR_HEIGHT);
	pProgressBar->setMinimum(0);
	pProgressBar->setMaximum(100);
	pProgressBar->setValue(0);
	ui.label_3->move(10, startY + MAX_BAR_HEIGHT + MAX_BAR_SEP_LEN);
	this->resize(1038, startY + ((MAX_BAR_HEIGHT + MAX_BAR_SEP_LEN) + 30) + 18);
	return pProgressBar;
}


void QDownloader::resetProgress(int value)
{
	for (int j = 0; j < MAX_DOWNLOAD_THREAD; ++j)
	{
		g_current[j] = static_cast<qreal>(value);
		g_vProgressBar[j]->setMinimum(0);
		g_vProgressBar[j]->setMaximum(100);
		g_vProgressBar[j]->setValue(value);
		QCoreApplication::processEvents();
	}
}

void CreateAndRunBat(const QString& path, const QString& data)
{
	const QString batfile = QString("%1/%2.bat").arg(path).arg(QDateTime::currentDateTime().toString(kBackupfileNameFormat));
	QFile file(batfile);
	if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
	{
		QTextStream out(&file);
		out.setCodec("UTF-8");
		out << data;
		file.flush();
		file.close();
		ShellExecuteW(NULL, L"open", (LPCWSTR)batfile.utf16(), NULL, NULL, SW_HIDE);
	}
}

//(源文件目錄路徑，目的文件目錄，文件存在是否覆蓋)
bool copyDirectory(const QString& srcPath, const QString& dstPath, bool coverFileIfExist)
{
	QDir srcDir(srcPath);
	QDir dstDir(dstPath);
	if (!dstDir.exists())
	{ //目的文件目錄不存在則創建文件目錄
		if (!dstDir.mkdir(dstDir.absolutePath()))
			return false;
	}
	QFileInfoList fileInfoList = srcDir.entryInfoList();
	for (const QFileInfo& fileInfo : fileInfoList)
	{
		if (fileInfo.fileName() == "." || fileInfo.fileName() == "..")
			continue;

		if (fileInfo.isDir())
		{    // 當為目錄時，遞歸的進行copy 
			if (!copyDirectory(fileInfo.filePath(), dstDir.filePath(fileInfo.fileName()), coverFileIfExist))
				return false;
		}
		else
		{            //當允許覆蓋操作時，將舊文件進行刪除操作 
			if (coverFileIfExist && dstDir.exists(fileInfo.fileName()))
			{
				dstDir.remove(fileInfo.fileName());
			}
			/// 進行文件copy
			if (!QFile::copy(fileInfo.filePath(), dstDir.filePath(fileInfo.fileName())))
			{
				return false;
			}
		}
	}
	return true;
}

//(源文件文件路徑，目的文件路徑，文件存在是否覆蓋)
bool copyFile(const QString& qsrcPath, const QString& qdstPath, bool coverFileIfExist)
{
	QString srcPath = qsrcPath;
	QString dstPath = qdstPath;
	srcPath.replace("\\", "/");
	dstPath.replace("\\", "/");
	if (srcPath == dstPath)
	{
		return true;
	}

	if (!QFile::exists(srcPath))
	{  //源文件不存在
		return false;
	}

	if (QFile::exists(dstPath))
	{
		if (coverFileIfExist)
		{
			QFile::remove(dstPath);
		}
	}

	if (!QFile::copy(srcPath, dstPath))
	{
		return false;
	}
	return true;
}

void QDownloader::overwriteCurrentExecutable()
{
	static bool ALREADY_RUN = false;
	if (ALREADY_RUN) return;
	ALREADY_RUN = true;
	synchronizer_.waitForFinished();

	resetProgress(100);

	QFile tmpFile(szCurrentDotExeAsDotTmp_);
	if (tmpFile.exists())
		tmpFile.remove();

	QFile Dot7zEXE(sz7zDotExe_);
	QFile Dot7zDLL(sz7zDotDll_);
	DO_NOT_SHOW.store(true, std::memory_order_release);

	QString shaexe = "\0";
	if (!Dot7zEXE.exists() || (Dot7zEXE.exists() && ((shaexe = Sha3_512(sz7zDotExe_)) != SHA512_7ZEXE)))
	{
		if (shaexe != SHA512_7ZEXE)
			Dot7zEXE.remove();

		//下載 7z.exe
		QCoreApplication::processEvents();

		QFuture<void> f = QtConcurrent::run([this]() { asyncDownloadFile(sz7zEXE_URL, szCurrentDirectory_, "7z.exe"); });
		f.waitForFinished();
		QCoreApplication::processEvents();
	}

	QString shadll = "\0";
	if (!Dot7zDLL.exists() || (Dot7zDLL.exists() && ((shadll = Sha3_512(sz7zDotDll_)) != SHA512_7ZDLL)))
	{
		if (shadll != SHA512_7ZDLL)
			Dot7zDLL.remove();

		//下載 7z.dll
		QCoreApplication::processEvents();
		QFuture<void> f = QtConcurrent::run([this]() { asyncDownloadFile(sz7zDLL_URL, szCurrentDirectory_, "7z.dll"); });
		f.waitForFinished();
		QCoreApplication::processEvents();
	}
	DO_NOT_SHOW.store(false, std::memory_order_release);

	resetProgress(100);

	ui.label_3->setText("BACKUPING...");
	QCoreApplication::processEvents();
	//將當前目錄下所有文件壓縮(7z)成備份，檔案名稱為  SaSH_backup_當前日期時間 
	constexpr auto buildDateTime = []()
	{
		QString d = util::buildDateTime(nullptr);
		return QString("v1.0.%1").arg(d).replace(":", "");
	};
	QString szBackup7zFileName = QString(kBackupfileName1).arg(buildDateTime());
	QString szBackup7zFilePath = QString("%1%2").arg(rcPath_).arg(szBackup7zFileName);
	QProcess    m_7zip; QStringList zipargs;

	//在rcpath 創建一個 backup
	QDir dir(rcPath_);
	if (!dir.exists("backup"))
		dir.mkdir("backup");

	const QString szBackupDir = QString("%1backup/").arg(rcPath_);
	//列表中的文件/目錄 從szCurrentDirectory_ 複製(覆蓋) 到 rc的 backup
	for (const auto& fileName : preBackupFileNames)
	{
		QFileInfo fileInfo(szCurrentDirectory_ + fileName);
		if (fileInfo.isFile())
		{
			copyFile(szCurrentDirectory_ + fileName, szBackupDir + fileName, true);
		}
		else if (fileInfo.isDir())
		{
			copyDirectory(szCurrentDirectory_ + fileName, szBackupDir + fileName, true);
		}
	}

	//如果存在則刪除
	QString szBackup7zNewFilePath = QString("%1%2").arg(szCurrentDirectory_).arg(szBackup7zFileName);
	int n = 0;
	while (QFile::exists(szBackup7zNewFilePath)) //_2 _3 _ 4
	{
		szBackup7zNewFilePath = QString("%1%2").arg(szCurrentDirectory_).arg(QString(kBackupfileName2).arg(buildDateTime()).arg(++n));
	}


	//zip all files under szCurrentDirectory_ lzma2
	zipargs << "a" << szBackup7zFilePath << szBackupDir
		<< "-mx=9" << "-m0=lzma2" << "-ms" << "-mmt";
	m_7zip.start(sz7zDotExe_, zipargs);
	m_7zip.waitForFinished();
	//move to current
	QElapsedTimer timer; timer.start();
	if (QFile::exists(szBackup7zFilePath))
	{
		while (!QFile::rename(szBackup7zFilePath, szBackup7zNewFilePath))
		{
			QCoreApplication::processEvents();
			QThread::msleep(100);
			if (timer.hasExpired(30000ll))
			{
				break;
			}
		}
	}

	QFile::rename(szCurrentDotExe_, szCurrentDotExeAsDotTmp_);// ./SaSH.exe to ./SaSH.tmp

	//close all .exe
	QProcess kill;
	kill.start("taskkill", QStringList() << "/f" << "/im" << kDefaultClosingProcessName);
	kill.waitForFinished();

	//x:eXtract with full paths用文件的完整路徑解壓至當前目錄或指定目錄
	//-o (Set Output Directory)
	//-aoa 直接覆蓋現有文件，而沒有任何提示
	ui.label_3->setText("REPLACING...");
	QCoreApplication::processEvents();
	QProcess    m_7z; QStringList args;
	args << "x" << rcPath_ + szDownloadedFileName_ << "-o" + szCurrentDirectory_ << "-aoa";
	m_7z.setWorkingDirectory(szCurrentDirectory_);
	m_7z.start(sz7zDotExe_, args, QIODevice::ReadWrite);
	if (!m_7z.waitForFinished())
	{
		qDebug() << "7z.exe finish failed";
		close();
		return;
	}

	ui.label_3->setText("FINISHED! READY TO RESTART!");
	QCoreApplication::processEvents();
	constexpr int delay = 5;
	// rcpath/date.bat
	QString bat;
	bat += "@echo off\r\n";
	bat = QString("SET pid=%1\r\n:loop\r\n").arg(pid_);
	bat += "tasklist /nh /fi \"pid eq %pid%\"|find /i \"%pid%\" > nul\r\n";
	bat += "if %errorlevel%==0 (\r\n";
	bat += "ping -n 2 127.0.0.1 > nul\r\n";
	bat += "goto loop )\r\n";
	bat += QString("ping -n %1 127.0.0.1 > nul\r\n").arg(delay + 1);
	bat += "taskkill -f -im SaSH.exe\r\n";
	bat += "taskkill -f -im sa_8001.exe\r\n";
	bat += "cd /d " + szCurrentDirectory_ + "\r\n";  // cd to directory
	bat += "del /f /q ./*.tmp\r\n"; //刪除.tmp
	bat += "start " + szCurrentDotExe_ + "\r\n";
	bat += QString("Rd /s /q \"%1\"\r\n").arg(rcPath_);
	bat += "del %0";
	bat += "exit\r\n";
	CreateAndRunBat(szSysTmpDir_, bat);

	{
		util::Config config(qgetenv("JSON_PATH"));
		config.write("System", "Update", "ETag", g_etag);
	}
	QCoreApplication::quit();
}

bool QDownloader::asyncDownloadFile(const QString& szUrl, const QString& dir, const QString& szSaveFileName)
{
	QString strUrl = szUrl;

	if (strUrl.length())
	{
		QSharedPointer<CurlDownload> cur(q_check_ptr(new CurlDownload()));
		cur->setProgressFunPtrs(g_vpfnProgressFunc);
		cur->downLoad(MAX_DOWNLOAD_THREAD, strUrl.toUtf8().constData(), dir.toUtf8().constData(), szSaveFileName.toUtf8().constData());
		return true;
	}
	return false;
}

void QDownloader::setProgressValue(int i, qreal totalToDownload, qreal nowDownloaded, qreal, qreal)
{
	if (totalToDownload > 0)
	{
		qreal percentage = (nowDownloaded / totalToDownload * 100.0);
		//if (percentage % g_nProcessPrecision == 0)
		{
			if (g_current[i] != percentage)
			{
				qDebug() << "g_percent[" << i << "] = " << percentage;
				g_current[i] = percentage;
			}
		}
	}
}

template <int Index>
int QDownloader::onProgress(void* clientp, qint64 totalToDownload, qint64 nowDownloaded, qint64 totalToUpLoad, qint64 nowUpLoaded)
{
	QDownloader* downloader = static_cast<QDownloader*>(clientp);
	downloader->setProgressValue(Index, totalToDownload, nowDownloaded, totalToUpLoad, nowUpLoaded);
	return 0;
}

bool downloadFile(const std::string& url, const std::string& filename)
{
	static cpr::cpr_off_t s_totalSize = 0;
	std::string tmp_filename = filename;
	std::ofstream of(tmp_filename, std::ios::binary | std::ios::app);
	auto pos = of.tellp();

	cpr::Url cpr_url{url};
	cpr::Session s;
	s.SetUrl(cpr_url);
	s.SetHeader(cpr::Header{{"Accept-Encoding", "gzip"}});

	auto fileLength = s.GetDownloadFileLength();
	s.SetRange(cpr::Range{pos, fileLength - 1});

	cpr::Response response = s.Download(of);
	s_totalSize += response.downloaded_bytes;

	if (s_totalSize >= fileLength)
	{
		s_totalSize = 0;
		of.flush();
		of.close();
		return true;
	}

	of.flush();
	of.close();
	return false;
}

void extractZip(const QString& savepath, const QString& filepath)
{
	QDir dir(savepath);
	if (!dir.exists())
	{
		dir.mkpath(savepath);
	}

	bool unzipok = false;
	QZipReader zipreader(filepath);
	unzipok = zipreader.extractAll(savepath);

	for (int i = 0; i < zipreader.fileInfoList().size(); ++i)
	{
		QStringList paths = zipreader.fileInfoList().at(i).filePath.split("/");
		paths.removeLast();
		QString path = paths.join("/");
		QDir subdir(savepath + "/" + path);
		if (!subdir.exists())
			dir.mkpath(QString::fromLocal8Bit("%1").arg(savepath + "/" + path));

		QFile file(savepath + "/" + zipreader.fileInfoList().at(i).filePath);
		file.open(QIODevice::WriteOnly);

		QByteArray dt = zipreader.fileInfoList().at(i).filePath.toUtf8();
		QString strtemp = QString::fromLocal8Bit(dt);

		QByteArray array = zipreader.fileData(strtemp);
		file.write(array);
		file.close();


	}
}

void QDownloader::downloadAndExtractZip(const QString& url, const QString& targetDir)
{
	constexpr const char* zipFile = "sash.zip";

	// 建立下載檔案的暫存目錄
	QString tempDir = targetDir;
	QDir dir(tempDir);
	if (!dir.exists())
		dir.mkpath(tempDir);
	QString filePath = tempDir + "/" + zipFile;
	QFile file(filePath);
	if (file.exists())
		file.remove();

	std::string surl = url.toUtf8().constData();

	std::string filename = filePath.toUtf8().constData();
	bool success = false;
	while (!success)
	{
		success = downloadFile(surl, filename);
	}

	// 刪除目標目錄下的所有文件夾和文件，除了壓縮檔
	QDir target(targetDir);
	QStringList targetFiles = target.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
	for (const QString& targetFile : targetFiles)
	{
		QString targetFilePath = targetDir + "/" + targetFile;
		if (targetFile != zipFile)
		{
			if (QFileInfo(targetFilePath).isDir())
			{
				QDir(targetFilePath).removeRecursively();
			}
			else
			{
				QFile::remove(targetFilePath);
			}
		}
	}


	extractZip(targetDir, filePath);
	//QZipReader reader(filePath);
	//reader.extractAll(targetDir);
}

//void QDownloader::downloadAndExtractZip(const QString& url, const QString& targetDir) const
//{
//	constexpr const char* zipFile = "sash.zip";
//
//	// 建立下載檔案的暫存目錄
//	QString tempDir = targetDir;
//	QDir dir(tempDir);
//	if (!dir.exists())
//		dir.mkpath(tempDir);
//
//	QNetworkAccessManager manager;
//	QNetworkRequest request(url);
//	QNetworkReply* reply = manager.get(request);
//
//	QEventLoop eventLoop;
//	QObject::connect(reply, &QNetworkReply::finished, [&]()
//		{
//			QString filePath = tempDir + "/" + zipFile;
//			QFile file(filePath);
//			if (file.exists())
//				file.remove();
//
//			do
//			{
//				// 下載完成
//				if (reply->error() == QNetworkReply::NoError)
//				{
//					// 儲存下載的檔案
//					if (!file.open(QIODevice::WriteOnly))
//						break;
//
//					file.write(reply->readAll());
//					file.flush();
//					file.close();
//					qDebug() << "檔案下載完成：" << filePath;
//
//					// 刪除目標目錄下的所有文件夾和文件，除了壓縮檔
//					QDir target(targetDir);
//					QStringList targetFiles = target.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
//					for (const QString& targetFile : targetFiles)
//					{
//						QString targetFilePath = targetDir + "/" + targetFile;
//						if (targetFile != zipFile)
//						{
//							if (QFileInfo(targetFilePath).isDir())
//							{
//								QDir(targetFilePath).removeRecursively();
//							}
//							else
//							{
//								QFile::remove(targetFilePath);
//							}
//						}
//					}
//					QZipReader reader(filePath);
//					reader.extractAll(targetDir);
//
//				}
//				else
//				{
//					qDebug() << "下載時發生錯誤:" << reply->errorString();
//				}
//			} while (false);
//			// 清理下載的回應物件
//
//			// 刪除下載的壓縮檔案
//			file.remove();
//			reply->deleteLater();
//			eventLoop.quit();
//		});
//
//	eventLoop.exec(); // 等待下載和解壓縮完成
//}
