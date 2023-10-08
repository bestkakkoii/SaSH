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
#include "downloader.h"
#include "util.h"

constexpr const char* URL = "https://www.lovesa.cc/SaSH/update/sash.zip";
constexpr const char* doc_URL = "https://gitee.com/Bestkakkoii/sash/wikis/pages/export?type=markdown&doc_id=4046472";
constexpr const char* kBackupfileName1 = "sash_backup_%1.zip";
constexpr const char* kBackupfileName2 = "sash_backup_%1_%2.zip";
constexpr const char* kBackupExecuteFileTmp = "SaSH.tmp";
static const QStringList preBackupFileNames = { util::applicationName(), QString(SASH_INJECT_DLLNAME) + ".dll", "settings", "script", "system.json" };
constexpr qint64 SHADOW_WIDTH = 10;
constexpr qint64 MAX_BAR_HEIGHT = 20;
constexpr qint64 MAX_BAR_SEP_LEN = 10;
constexpr qint64 MAX_GIF_MOVE_WIDTH = 920;
constexpr qint64 PROGRESS_BAR_BEGIN_Y = 85;

QString g_etag;
constexpr qint64 UPDATE_TIME_MIN = 5 * 60;

void setHeader(QNetworkRequest* prequest)
{
	if (prequest == nullptr)
		return;

	QSslConfiguration sslConf = prequest->sslConfiguration();
	sslConf.setPeerVerifyMode(QSslSocket::VerifyNone);
	sslConf.setProtocol(QSsl::TlsV1SslV3);
	prequest->setSslConfiguration(sslConf);

	prequest->setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/107.0.0.0 Safari/537.36 Edg/107.0.1418.35");
	//prequest->setRawHeader("authority", "www.lovesa.cc");
	prequest->setRawHeader("Method", "GET");
	prequest->setRawHeader("Scheme", "https");
	//prequest->setRawHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9");
	prequest->setRawHeader("Accept", "*/*");
	prequest->setRawHeader("Accept-Encoding", "*");
	prequest->setRawHeader("accept-language", "zh-TW,zh;q=0.9,en;q=0.8,en-GB;q=0.7,en-US;q=0.6,zh-CN;q=0.5");
	prequest->setRawHeader("Cache-Control", "max-age=0");
	prequest->setRawHeader("DNT", "1");
	prequest->setRawHeader("Sec-CH-UA", "\"Microsoft Edge\";v=\"107\", \"Chromium\";v=\"107\", \"Not=A?Brand\";v=\"24\"");
	prequest->setRawHeader("Sec-CH-UA-Mobile", "?0");
	prequest->setRawHeader("Sec-CH-UA-Platform", "\"Windows\"");
	prequest->setRawHeader("Sec-Fetch-Dest", "document");
	prequest->setRawHeader("Sec-Fetch-Mode", "navigate");
	prequest->setRawHeader("Sec-Fetch-Site", "none");
	prequest->setRawHeader("Sec-Fetch-User", "?1");
	prequest->setRawHeader("Upgrade-Insecure-Requests", "1");
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

void deleteFile(const QString& targetDir, const QString& file)
{
	QDir target(targetDir);
	QStringList targetFiles = target.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
	for (const QString& targetFile : targetFiles)
	{
		QString targetFilePath = targetDir + "/" + targetFile;
		if (targetFile != file)
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
}

bool enumAllFile(QStringList* pfilePaths, const QString& directory)
{
	QDir dir(directory);
	if (!dir.exists())
	{
		return false;
	}

	dir.setFilter(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
	dir.setSorting(QDir::DirsFirst);

	QFileInfoList list = dir.entryInfoList();
	int i = 0;
	bool bIsDir;
	do
	{
		QFileInfo fileInfo = list.at(i);
		QString strFileName = fileInfo.fileName();
		if (strFileName == "." || strFileName == "..")
		{
			continue;
		}

		bIsDir = fileInfo.isDir();
		if (bIsDir)
		{
			enumAllFile(pfilePaths, fileInfo.filePath());
		}
		else
		{
			pfilePaths->append(fileInfo.filePath());
		}

		i++;
	} while (i < list.size());

	return true;
}

#if 0
bool compress(const QString& source, const QString& destination)
{
	QString preCompressFolder = source;

	QStringList list;
	if (!enumAllFile(&list, preCompressFolder))
	{
		qDebug() << "enumAllFile failed";
		return  false;
	}

	QZipWriter writer(destination);
	writer.setCompressionPolicy(QZipWriter::AlwaysCompress);

	//add files
	for (const QString& filePath : list)
	{
		QFileInfo fileInfo(filePath);
		QString relativePath = fileInfo.filePath().replace(preCompressFolder, "");

		if (fileInfo.isDir())
		{
			writer.addDirectory(relativePath);
			continue;
		}

		util::ScopedFile file(filePath, QIODevice::ReadOnly);
		if (!file.isOpen())
		{
			qDebug() << "open file failed" << filePath;
			continue;
		}

		writer.addFile(relativePath, &file);
	}

	//finish
	writer.close();

	return true;
}

void uncompress(const QString& savepath, const QString& filepath, bool one)
{
	QDir dir(savepath);
	if (!dir.exists())
	{
		dir.mkpath(savepath);
	}

	QZipReader zipreader(filepath);
	QVector<QZipReader::FileInfo> filelist = zipreader.fileInfoList();
	qint64 size = filelist.size();

	for (qint64 i = 0; i < size; ++i)
	{
		QZipReader::FileInfo info = filelist.value(i);

		QStringList paths = info.filePath.split("/");
		paths.removeLast();

		QString path = paths.join("/");
		path.replace("\\", "/");
		path.replace("//", "/");

		QString directory = savepath + "/" + path;
		directory.replace("\\", "/");
		directory.replace("//", "/");

		QDir subdir(directory);
		if (!subdir.exists())
			dir.mkpath(directory);

		QString realSavePath = savepath + "/" + info.filePath;
		realSavePath.replace("\\", "/");
		realSavePath.replace("//", "/");
		if (realSavePath.contains("繁體"))
			continue;

		QFileInfo fileinfo(realSavePath);
		if (fileinfo.suffix().isEmpty() || fileinfo.isDir())
		{
			QDir dir(realSavePath);
			if (!dir.exists())
				dir.mkpath(realSavePath);
			continue;
		}

		QFile testfile(realSavePath);
		if (testfile.exists())
			testfile.remove();

		if (one)
			continue;

		util::ScopedFile file(realSavePath, QIODevice::WriteOnly | QIODevice::Unbuffered | QIODevice::Truncate);
		if (!file.isOpen())
		{
			qDebug() << "Failed to open file for writing: " << realSavePath;
			continue;
		}

		QByteArray arr = zipreader.fileData(info.filePath);
		if (!arr.isEmpty())
			file.write(arr);
	}

	if (one)
	{
		zipreader.extractAll(savepath);
	}
}
#endif


bool Downloader::compress(const QString& source, const QString& destination)
{
	QVector<QPair<QString, QString>> list;
	if (!util::enumAllFiles(source, "", &list))
	{
		qDebug() << "enumAllFile failed";
		emit labelTextChanged("enumAllFile failed");
		return  false;
	}

	QString newDestination = destination;
	newDestination.replace("/", "\\");

	std::wstring wdestination = newDestination.toStdWString();

	HZIP hz = CreateZip(wdestination.c_str(), 0);
	if (hz == nullptr)
		return false;

	for (const QPair<QString, QString>& pair : list)
	{
		QString szFileName = pair.first;
		QString szFullPath = pair.second;
		szFullPath.replace("/", "\\");

		QFileInfo fileInfo(szFullPath);
		QString relativePath = fileInfo.filePath().replace(source, "");
		relativePath.replace("/", "\\");

		std::wstring wfilename = relativePath.toStdWString();
		std::wstring wfullpath = szFullPath.toStdWString();

		ZRESULT ret = ZipAdd(hz, wfilename.c_str(), wfullpath.c_str());
		if (ret != ZR_OK)
		{
			qDebug() << "ZipAdd failed" << szFullPath;
			emit labelTextChanged("ZipAdd failed: " + szFullPath);
		}
	}

	CloseZip(hz);

	return true;
}

bool Downloader::uncompress(const QString& source, const QString& destination)
{
	QString newSource = source;
	newSource.replace("/", "\\");
	std::wstring wsource = newSource.toStdWString();

	HZIP hz = OpenZip(wsource.c_str(), 0);
	if (hz == nullptr)
		return false;

	QString newDestination = destination;
	newDestination.replace("/", "\\");

	std::wstring wdestination = newDestination.toStdWString();

	ZRESULT ret = SetUnzipBaseDir(hz, wdestination.c_str());
	if (ret != ZR_OK)
	{
		qDebug() << "SetUnzipBaseDir failed" << newDestination;
		emit labelTextChanged("SetUnzipBaseDir failed: " + newDestination);
		CloseZip(hz);
		return false;
	}

	ZIPENTRY ze = { 0 };
	ret = GetZipItem(hz, -1, &ze);
	if (ret != ZR_OK)
	{
		qDebug() << "GetZipItem failed" << ze.index;
		emit labelTextChanged("GetZipItem failed with index: " + QString::number(ze.index));
		CloseZip(hz);
		return false;
	}

	int numitems = ze.index;
	for (int zi = 0; zi < numitems; zi++)
	{
		ret = GetZipItem(hz, zi, &ze);
		if (ret != ZR_OK)
		{
			qDebug() << "GetZipItem failed" << zi;
			emit labelTextChanged("GetZipItem failed with index: " + QString::number(zi));
			continue;
		}

		ret = UnzipItem(hz, zi, ze.name);
		if (ret != ZR_OK)
		{
			qDebug() << "UnzipItem failed" << zi;
			emit labelTextChanged("UnzipItem failed with index: " + QString::number(zi));
			continue;
		}
	}

	CloseZip(hz);

	return true;
}

QString Sha3_512(const QString& fileNamePath)
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

bool Downloader::checkUpdate(QString* current, QString* ptext, QString* pformated)
{
	{
		util::Config config(qgetenv("JSON_PATH"));
		g_etag = config.read<QString>("System", "Update", "ETag");
	}

	QString exeFileName = QCoreApplication::applicationFilePath();

	QNetworkAccessManager manager;
	QUrl zipUrl(URL); // 将 "URL" 替换为实际的下载地址
	QNetworkRequest request(zipUrl);

	setHeader(&request);
	if (!g_etag.isEmpty())
	{
		request.setRawHeader("If-None-Match", g_etag.toUtf8());
	}

	QNetworkReply* reply = manager.head(request);
	if (reply == nullptr)
		return false;

	QEventLoop loop;
	QTimer timer;
	connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
	connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
	timer.singleShot(30000, &loop, &QEventLoop::quit);
	loop.exec();

	bool bret = false;

	if (reply->error() != QNetworkReply::NoError)
	{
		qDebug() << "HTTP request failed: " << reply->errorString();
	}
	else
	{
		// 获取修改时间
		QDateTime zipModified;
		QDateTime exeModified;

		if (reply->hasRawHeader("etag"))
		{
			QByteArray etagHeader = reply->rawHeader("etag");
			QString newEtag = QString::fromUtf8(etagHeader);
			newEtag.remove("\"");

			if (!newEtag.isEmpty() && (g_etag.isEmpty() || newEtag != g_etag))
			{
				qDebug() << "New version available!" << newEtag;
				g_etag = newEtag;
				bret = true;
			}
			else
			{
				qDebug() << "No new version available." << newEtag;
			}
		}

		//print all 
		QByteArrayList headers = reply->rawHeaderList();
		qDebug() << headers.size();
		for (const QByteArray& header : headers)
		{
			qDebug() << QString::fromUtf8(header) << ":" << QString::fromUtf8(reply->rawHeader(header));
		}

		if (reply->hasRawHeader("last-modified"))
		{
			QByteArray lastModifiedHeader = reply->rawHeader("last-modified");
			QString lastModifiedStr = QString::fromUtf8(lastModifiedHeader);
			//if (lastModifiedStr.contains("GMT"))
			//{
			//	//lastModifiedStr.replace("GMT", "");
			//}

			//const QHash<QString, qint64> hash = {
			//	{ "Jan", 1 },
			//	{ "Feb", 2 },
			//	{ "Mar", 3 },
			//	{ "Apr", 4 },
			//	{ "May", 5 },
			//	{ "Jun", 6 },
			//	{ "Jul", 7 },
			//	{ "Aug", 8 },
			//	{ "Sep", 9 },
			//	{ "Oct", 10 },
			//	{ "Nov", 11 },
			//	{ "Dec", 12 },
			//	{ "Mon", 100 },
			//	{ "Tue", 200 },
			//	{ "Wed", 300 },
			//	{ "Thu", 400 },
			//	{ "Fri", 500 },
			//	{ "Sat", 600 },
			//	{ "Sun", 700 },
			//};

			//for (const QString& key : hash.keys())
			//{
			//	if (lastModifiedStr.contains(key))
			//	{
			//		qint64 index = hash.value(key);
			//		if (index < 100)
			//			lastModifiedStr.replace(key, QString::number(hash.value(key)));
			//		else
			//			lastModifiedStr.replace(key, "");
			//	}
			//}

			//lastModifiedStr.replace(",", "");
			//lastModifiedStr = lastModifiedStr.simplified();


			// 解析GMT时间格式
			const QLocale l(QLocale::English);
			QDateTime gmtTime(l.toDateTime(lastModifiedStr, "ddd, dd MMM yyyy hh:mm:ss 'GMT'"));
			//QDateTime gmtTime = QDateTime::fromString(lastModifiedStr, "dd MM yyyy hh:mm:ss 'GMT'");//Fri, 06 Oct 2023 15:25:57 GMT
			if (!gmtTime.isValid())
			{
				qDebug() << "Failed to parse date string.";
			}

			//gmtTime = gmtTime.addSecs(8LL * 60LL * 60LL); // 补充时区差
			gmtTime.setTimeSpec(Qt::UTC);
			QTimeZone beijingTimeZone("Asia/Shanghai");
			zipModified = gmtTime.toTimeZone(beijingTimeZone);

			qDebug() << "SaSH 7z file last modified time:" << lastModifiedStr;
			qDebug() << "SaSH 7z file modified time:" << zipModified.toString("yyyy-MM-dd hh:mm:ss");

			if (!zipModified.isValid())
			{
				qDebug() << "Failed to convert date string to QDateTime object.";
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
		}

		compile::buildDateTime(&exeModified);
		qDebug() << "SaSH.exe file modified time:" << exeModified.toString("yyyy-MM-dd hh:mm:ss");

		if (current)
			*current = exeModified.toString("yyyy-MM-dd hh:mm:ss");

		if (!bret)
		{
			qint64 timeDiffInSeconds = exeModified.secsTo(zipModified);
			QString szTimeDiff = util::formatSeconds(std::abs(timeDiffInSeconds));
			if (pformated != nullptr)
			{
				*pformated = szTimeDiff;
			}

			if (timeDiffInSeconds > UPDATE_TIME_MIN)
			{
				if (zipModified > exeModified)
				{
					qDebug() << "SaSH.7z newer than SaSH.exe" << szTimeDiff;
					bret = true;
				}
				else
				{
					qDebug() << "SaSH.exe newer than SaSH.7z, time diff:" << szTimeDiff;
				}
			}
			else if (timeDiffInSeconds >= 0)
			{
				qDebug() << "SaSH.exe and SaSH.7z are almost the same, time diff:" << szTimeDiff;
			}
			else
			{
				qDebug() << "SaSH.exe newer than SaSH.7z, time diff:" << szTimeDiff;
			}
		}
	}

	reply->deleteLater();

	return bret;


}

Downloader::Downloader(QWidget* parent)
	: QWidget(parent)
	, szCurrentDirectory_(util::applicationDirPath() + "/")
	, szCurrentDotExe_(szCurrentDirectory_ + util::applicationName())
	, szCurrentDotExeAsDotTmp_(szCurrentDirectory_ + kBackupExecuteFileTmp)
	, szSysTmpDir_(QDir::tempPath())
{
	ui.setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose);
	setAttribute(Qt::WA_TranslucentBackground);
	setWindowFlags(this->windowFlags() | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
	::SetWindowLongW((HWND)winId() \
		, GWL_EXSTYLE, GetWindowLong((HWND)winId(), GWL_EXSTYLE) ^ WS_EX_LAYERED);
	::SetLayeredWindowAttributes((HWND)winId(), NULL, 0, LWA_ALPHA);

	QFont font = util::getFont();
	font.setFamily("JoysticMonospace");
	font.setPointSize(9);
	setFont(font);

	QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect(ui.widget);
	if (shadowEffect != nullptr)
	{
		// 陰影偏移
		shadowEffect->setOffset(0, 1);
		// 陰影顏色;
		shadowEffect->setColor(Qt::black);
		// 陰影半徑;
		shadowEffect->setBlurRadius(SHADOW_WIDTH);
		// 給窗口設置上當前的陰影效果;
		setGraphicsEffect(shadowEffect);
	}

	QMovie* movie = new QMovie("://image/jimmy.gif");
	if (movie != nullptr)
	{
		movie->setObjectName("movieLoading");
		movie->setScaledSize(QSize(44, 72));
		ui.label->setMovie(movie);
		movie->start();
	}

	connect(this, &Downloader::labelTextChanged, this, &Downloader::onLabelTextChanged, Qt::QueuedConnection);
	connect(this, &Downloader::progressReset, this, &Downloader::onProgressBarReset, Qt::QueuedConnection);

	qint64 n = PROGRESS_BAR_BEGIN_Y;
	progressBar = createProgressBar(n);
	//n += MAX_BAR_HEIGHT + MAX_BAR_SEP_LEN;

	rcPath_ = QString("%1/%2/").arg(szSysTmpDir_).arg(pid_);
	QDir dir(rcPath_);
	if (!dir.exists())
		dir.mkpath(rcPath_);

	//從網址中擷取檔案名稱
	QString szFileName = URL;
	szDownloadedFileName_ = szFileName.mid(szFileName.lastIndexOf('/') + 1);
	szTmpDot7zFile_ = rcPath_ + szDownloadedFileName_;// %Temp%/pid/SaSH.7z

	networkManager_.reset(new QNetworkAccessManager(this));

	emit progressReset(0);
}

Downloader::~Downloader()
{
	if (!networkManager_.isNull())
		networkManager_->deleteLater();

	if (reply_ != nullptr)
		reply_->deleteLater();
}

void Downloader::showEvent(QShowEvent* e)
{
	//首次show
	static bool bFirstShow = true;
	if (bFirstShow)
	{
		bFirstShow = false;
		QMetaObject::invokeMethod(this, "start", Qt::QueuedConnection);
	}
	setAttribute(Qt::WA_Mapped);
	QWidget::showEvent(e);
}

QProgressBar* Downloader::createProgressBar(qint64 startY)
{
	QProgressBar* pProgressBar = (new QProgressBar(ui.widget));
	if (pProgressBar == nullptr)
		return nullptr;

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
	resize(1038, startY + ((MAX_BAR_HEIGHT + MAX_BAR_SEP_LEN) + 30) + ui.label_3->height() + 10);
	return pProgressBar;
}

void Downloader::onLabelTextChanged(const QString& text)
{
	ui.label_3->setUpdatesEnabled(false);
	ui.label_3->setText(text);
	ui.label_3->setUpdatesEnabled(true);
	ui.label_3->update();
	ui.label_3->repaint();
	QApplication::processEvents();
}

void Downloader::onProgressBarReset(qint64 value)
{
	currentProgress_ = static_cast<qreal>(value);
	if (progressBar != nullptr)
		return;
	progressBar->setMinimum(0);
	progressBar->setMaximum(100);
	progressBar->setValue(value);
	progressBar->update();
	progressBar->repaint();
	QApplication::processEvents();
}

bool Downloader::download(const QString& url, QByteArray* pbyteArray)
{
	if (networkManager_.isNull())
		return false;

	networkManager_->clearAccessCache();

	QUrl qurl(url);
	QNetworkRequest request(qurl);
	setHeader(&request);

	try
	{
		reply_ = networkManager_->get(request);
	}
	catch (...)
	{
		qDebug() << "Failed to create request.";
		emit labelTextChanged("Failed to create request.");
		if (reply_ != nullptr)
			reply_->deleteLater();
		return false;
	}

	if (reply_ == nullptr)
	{
		qDebug() << "Failed to create request.";
		emit labelTextChanged("Failed to create request.");
		return false;
	}

	connect(reply_, &QNetworkReply::downloadProgress, this, &Downloader::onDownloadProgress, Qt::QueuedConnection);
	connect(reply_, &QNetworkReply::errorOccurred, this, &Downloader::onErrorOccurred);

	QEventLoop loop;
	QTimer timer;
	connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
	connect(reply_, &QNetworkReply::finished, &loop, &QEventLoop::quit);
	timer.singleShot(30000, &loop, &QEventLoop::quit);
	loop.exec();

	bool bret = false;
	if (reply_->error() != QNetworkReply::NoError)
	{
		qDebug() << "Failed to download file: " << reply_->errorString();
		emit labelTextChanged("Failed to download file: " + reply_->errorString());
	}
	else
	{
		if (pbyteArray != nullptr)
			*pbyteArray = reply_->readAll();

		bret = true;
	}

	reply_->deleteLater();
	reply_ = nullptr;

	return true;
}

bool Downloader::downloadFile(const QString& url, const QString& filename)
{
	if (QFile::exists(filename))
	{
		QFile::remove(filename);
	}

	QByteArray data;
	if (!download(url, &data))
	{
		return false;
	}

	bool bret = false;
	util::ScopedFile file(filename, QIODevice::WriteOnly);
	if (file.isOpen())
	{
		file.write(data);
		bret = true;
	}
	else
	{
		qDebug() << "Failed to open file for writing: " << filename;
		emit labelTextChanged("Failed to open file for writing: " + filename);
	}

	return bret;
}

void Downloader::downloadAndUncompress(const QString& url, const QString& targetDir)
{
	constexpr const char* zipFile = "sash_doc.zip";

	// 建立下載檔案的暫存目錄
	QString tempDir = targetDir;
	QDir dir(tempDir);
	if (!dir.exists())
		dir.mkpath(tempDir);

	QString filePath = tempDir + "/" + zipFile;
	QFile file(filePath);
	if (file.exists())
		file.remove();

	bool success = false;
	while (!success)
	{
		success = downloadFile(url, filePath);
	}

	// 刪除目標目錄下的所有文件夾和文件，除了壓縮檔
	deleteFile(targetDir, zipFile);

	//解壓縮
	uncompress(filePath, targetDir);
}

void Downloader::onErrorOccurred(QNetworkReply::NetworkError code)
{
	qDebug() << "Network error:" << code;
	emit labelTextChanged("Network error:" + QString::number(code));

	if (!isMain)
	{
		qint64 per = progressBar->value() / 10;
		QtConcurrent::run([this, per]()
			{
				for (qint64 i = 10; i >= 0; --i)
				{
					emit labelTextChanged(QString("DOWNLOAD FAIL! %1").arg(i));
					emit progressReset(per * i);
					QThread::msleep(1000);
				}

				MINT::NtTerminateProcess(GetCurrentProcess(), 0);
			});
	}
	else
		emit progressReset(0);

	if (reply_ != nullptr)
	{
		reply_->deleteLater();
		reply_ = nullptr;
	}
}

QString Downloader::oneShotDownload(const QString& url)
{
	QByteArray data;
	if (!download(url, &data))
	{
		return "";
	}

	QString result = util::toQString(data);

	return result;
}

void Downloader::start()
{
	if (networkManager_.isNull())
		return;

	networkManager_->clearAccessCache();

	QUrl qurl(URL);
	QNetworkRequest request(qurl);
	setHeader(&request);

	isMain = true;

	try
	{
		reply_ = networkManager_->get(request);
	}
	catch (...)
	{
		qDebug() << "Failed to create request.";
		emit labelTextChanged("Failed to create request.");
		if (reply_ != nullptr)
			reply_->deleteLater();
		return;
	}

	if (reply_ == nullptr)
	{
		qDebug() << "Failed to create request.";
		emit labelTextChanged("Failed to create request.");
		return;
	}

	connect(reply_, &QNetworkReply::downloadProgress, this, &Downloader::onDownloadProgress);
	connect(reply_, &QNetworkReply::finished, this, &Downloader::onDownloadFinished);
	connect(reply_, &QNetworkReply::errorOccurred, this, &Downloader::onErrorOccurred);
}

void Downloader::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	if (bytesTotal <= 0)
		return;

	double percent = static_cast<double>((bytesReceived * 100.0) / bytesTotal);
	currentProgress_ = static_cast<qreal>(percent);
	if (progressBar != nullptr)
		progressBar->setValue(percent);
	ui.label->move(MAX_GIF_MOVE_WIDTH * (percent + 1.0) / 100, 10);
}

void Downloader::onDownloadFinished()
{
	if (reply_ == nullptr)
		return;

	if (reply_->error() == QNetworkReply::NoError)
	{
		util::ScopedFile file(rcPath_ + szDownloadedFileName_, QIODevice::WriteOnly);
		if (file.isOpen())
		{
			file.write(reply_->readAll());
			qDebug() << "Downloaded file saved to" << file.fileName();
			emit labelTextChanged("Downloaded file saved to" + file.fileName());
			emit progressReset(100);
			isMain = false;
			//QMetaObject::invokeMethod(this, "overwriteCurrentExecutable", Qt::QueuedConnection);
			QtConcurrent::run([this]()
				{
					overwriteCurrentExecutable();
				});
		}
	}

	reply_->deleteLater();
	reply_ = nullptr;
}

void Downloader::overwriteCurrentExecutable()
{
	static bool ALREADY_RUN = false;
	if (ALREADY_RUN)
		return;

	ALREADY_RUN = true;

	emit progressReset(0);

	{
		emit labelTextChanged("DOWNLOAD DOCUMENT...");
		QString mdFullPath = QString("%1/lib/doc").arg(util::applicationDirPath());
		downloadAndUncompress(doc_URL, mdFullPath);
	}

	emit progressReset(0);

	{
		QFile tmpFile(szCurrentDotExeAsDotTmp_);
		if (tmpFile.exists())
			tmpFile.remove();
	}

	emit progressReset(100);

	emit labelTextChanged("BACKUPING...");

	//make a name for backup file
	constexpr auto buildDateTime = []()
	{
		QString d = compile::buildDateTime(nullptr);
		return QString("v1.0.%1").arg(d).replace(":", "");
	};

	//create backup dir in rcpath
	QDir dir(rcPath_);
	if (!dir.exists())
		dir.mkpath(rcPath_);
	dir = QDir(rcPath_ + "backup");
	if (!dir.exists())
		dir.mkdir(rcPath_ + "backup");

	const QString szBackupDir = QString("%1backup/").arg(rcPath_);

	//copy file and directory from szCurrentDirectory_ to rcpath/backup
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

	QString szBackup7zFileName = QString(kBackupfileName1).arg(buildDateTime());
	QString szBackup7zFilePath = QString("%1%2").arg(rcPath_).arg(szBackup7zFileName);
	QString szBackup7zNewFilePath = QString("%1%2").arg(szCurrentDirectory_).arg(szBackup7zFileName);
	qint64 n = 0;
	while (QFile::exists(szBackup7zNewFilePath)) //_2 _3 _4..increase until name is not duplicate
	{
		szBackup7zNewFilePath = QString("%1%2").arg(szCurrentDirectory_).arg(QString(kBackupfileName2).arg(buildDateTime()).arg(++n));
	}

	compress(szBackupDir, szBackup7zFilePath);

	//move to current
	{
		QElapsedTimer timer; timer.start();
		if (QFile::exists(szBackup7zFilePath))
		{
			while (!QFile::rename(szBackup7zFilePath, szBackup7zNewFilePath))
			{
				QThread::msleep(100);
				if (timer.hasExpired(30000ll))
				{
					break;
				}
			}
		}
	}

	QFile::rename(szCurrentDotExe_, szCurrentDotExeAsDotTmp_);// ./SaSH.exe to ./SaSH.tmp

	//close all .exe that has sadll.dll as module
	{
		QVector<qint64> processes;
		if (mem::enumProcess(&processes, QString(SASH_INJECT_DLLNAME) + ".dll"))
		{
			for (qint64 pid : processes)
			{
				ScopedHandle hProcess(pid);
				if (hProcess.isValid())
					MINT::NtTerminateProcess(hProcess, 0);
			}
		}
	}

	emit labelTextChanged("REPLACING...");

	//uncompress sash.7z to current directory
	uncompress(rcPath_ + szDownloadedFileName_, szCurrentDirectory_.chopped(1));

	emit labelTextChanged("FINISHED! READY TO RESTART!");

	{
		// write and async run bat file
		constexpr qint64 delay = 5;
		// rcpath/date.bat
		QString bat;
		bat += "@echo off\r\n";
		bat = QString("SET pid=%1\r\n:loop\r\n").arg(pid_);
		bat += "tasklist /nh /fi \"pid eq %pid%\"|find /i \"%pid%\" > nul\r\n";
		bat += "if %errorlevel%==0 (\r\n";
		bat += "ping -n 2 127.0.0.1 > nul\r\n";
		bat += "goto loop )\r\n";
		bat += QString("ping -n %1 127.0.0.1 > nul\r\n").arg(delay + 1);
		bat += "taskkill -f -im " + util::applicationName() + "\r\n";
		bat += "cd /d " + szCurrentDirectory_ + "\r\n";  // cd to directory
		bat += "del /f /q ./*.tmp\r\n"; //刪除.tmp
		bat += "start " + szCurrentDotExe_ + "\r\n";
		bat += QString("Rd /s /q \"%1\"\r\n").arg(rcPath_);
		bat += "del %0";
		bat += "exit\r\n";
		util::asyncRunBat(szSysTmpDir_, bat);
	}

	{
		util::Config config(qgetenv("JSON_PATH"));
		config.write("System", "Update", "ETag", g_etag);
	}

	MINT::NtTerminateProcess(GetCurrentProcess(), 0);
}