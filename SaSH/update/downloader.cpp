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

constexpr const char* SASH_WIKI_URL = "https://gitee.com/Bestkakkoii/sash/wikis/pages/export?type=markdown&doc_id=4046472";
constexpr const char* SASH_MAPDATA_URL = "https://gitee.com/Bestkakkoii/sash/raw/master/mapdata.lua";
#ifdef _WIN64
constexpr const char* SASH_UPDATE_URL = "https://www.lovesa.cc/SaSH/update/SaSHx64.zip";
constexpr const char* kBackupfileName1 = "sash_x64_backup_%1.zip";
constexpr const char* kBackupfileName2 = "sash_x64_backup_%1_%2.zip";
constexpr const char* kBackupExecuteFileTmp = "SaSH_x64.tmp";
#else
constexpr const char* SASH_UPDATE_URL = "https://www.lovesa.cc/SaSH/update/SaSHx86.zip";
constexpr const char* kBackupfileName1 = "sash_x86_backup_%1.zip";
constexpr const char* kBackupfileName2 = "sash_x86_backup_%1_%2.zip";
constexpr const char* kBackupExecuteFileTmp = "SaSH_x86.tmp";
#endif
static const QStringList preBackupFileNames = { util::applicationName(), QString(SASH_INJECT_DLLNAME) + ".dll", "settings", "script" };

QString g_etag;
constexpr long long UPDATE_TIME_MIN = 5 * 60;

void setHeader(QNetworkRequest* prequest)
{
	if (prequest == nullptr)
		return;

	QSslConfiguration sslConf = prequest->sslConfiguration();
	sslConf.setPeerVerifyMode(QSslSocket::VerifyNone);
	sslConf.setProtocol(QSsl::TlsV1_2OrLater);
	prequest->setSslConfiguration(sslConf);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	prequest->setAttribute(QNetworkRequest::Http2AllowedAttribute, true);
	prequest->setAttribute(QNetworkRequest::Http2WasUsedAttribute, true);
	prequest->setAttribute(QNetworkRequest::Http2CleartextAllowedAttribute, true);
#endif

	prequest->setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/107.0.0.0 Safari/537.36 Edg/107.0.1418.35");
	prequest->setRawHeader("authority", "www.lovesa.cc");
	prequest->setRawHeader("Method", "GET");
	prequest->setRawHeader("Scheme", "https");
	//prequest->setRawHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9");
	prequest->setRawHeader("Accept", "*/*");
	prequest->setRawHeader("Accept-Encoding", "*");
	prequest->setRawHeader("accept-language", "zh-TW,zh;q=0.9,en;q=0.8,en-GB;q=0.7,en-US;q=0.6,zh-CN;q=0.5");
	prequest->setRawHeader("Cache-Control", "max-age=0");
	prequest->setRawHeader("DNT", "1");
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
	long long i = 0;
	bool bIsDir;
	do
	{
		QFileInfo fileInfo = list.value(i);
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

bool compress(Downloader* d, const QString& source, const QString& destination)
{
	QVector<QPair<QString, QString>> list;
	if (!util::enumAllFiles(source, "", &list))
	{
		qDebug() << "enumAllFile failed";
		d->progressDialog_->setLabelText("enumAllFile failed");
		return  false;
	}

	QString newDestination = destination;
	newDestination.replace("/", "\\");

	std::wstring wdestination = newDestination.toStdWString();

	zipper::HZIP hz = zipper::CreateZip(wdestination.c_str(), 0);
	if (hz == nullptr)
		return false;

	long long totalSize = list.size();
	d->progressDialog_->reset();
	d->progressDialog_->setMaximum(totalSize);
	long long currentSize = 0;
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

		zipper::ZRESULT ret = zipper::ZipAdd(hz, wfilename.c_str(), wfullpath.c_str());
		if (ret != zipper::ZR_OK)
		{
			qDebug() << "ZipAdd failed" << szFullPath;
			d->progressDialog_->setLabelText("ZipAdd failed: " + szFullPath);
		}
		d->progressDialog_->setValue(currentSize);
		QApplication::processEvents();
		++currentSize;
		QThread::msleep(1);
	}

	zipper::CloseZipZ(hz);

	return true;
}

bool uncompress(Downloader* d, const QString& source, const QString& destination)
{
	QString newSource = source;
	newSource.replace("/", "\\");
	std::wstring wsource = newSource.toStdWString();

	zipper::HZIP hz = zipper::OpenZip(wsource.c_str(), 0);
	if (hz == nullptr)
		return false;

	QString newDestination = destination;
	newDestination.replace("/", "\\");

	std::wstring wdestination = newDestination.toStdWString();

	zipper::ZRESULT ret = zipper::SetUnzipBaseDir(hz, wdestination.c_str());
	if (ret != zipper::ZR_OK)
	{
		qDebug() << "SetUnzipBaseDir failed" << newDestination;
		d->progressDialog_->setLabelText("SetUnzipBaseDir failed: " + newDestination);
		zipper::CloseZipU(hz);
		return false;
	}

	zipper::ZIPENTRY ze = {};
	ret = GetZipItem(hz, -1, &ze);
	if (ret != zipper::ZR_OK)
	{
		qDebug() << "GetZipItem failed" << ze.index;
		d->progressDialog_->setLabelText("GetZipItem failed with index: " + QString::number(ze.index));
		zipper::CloseZipU(hz);
		return false;
	}

	long long numitems = ze.index;
	d->progressDialog_->reset();
	d->progressDialog_->setMaximum(numitems);
	long long currentSize = 0;
	for (long long zi = 0; zi < numitems; zi++)
	{
		ret = GetZipItem(hz, zi, &ze);
		if (ret != zipper::ZR_OK)
		{
			qDebug() << "GetZipItem failed" << zi;
			d->progressDialog_->setLabelText("GetZipItem failed with index: " + QString::number(zi));
			continue;
		}

		ret = UnzipItem(hz, zi, ze.name);
		if (ret != zipper::ZR_OK)
		{
			qDebug() << "UnzipItem failed" << zi;
			d->progressDialog_->setLabelText("UnzipItem failed with index: " + QString::number(zi));
			continue;
		}

		d->progressDialog_->setValue(currentSize);
		QApplication::processEvents();
		++currentSize;
		QThread::msleep(1);
	}

	zipper::CloseZipU(hz);

	return true;
}

QString Sha3_512(const QString& fileNamePath)
{
	util::ScopedFile theFile(fileNamePath);
	if (!theFile.exists())
		return "\0";

	if (!theFile.openRead())
		return "";

	const QByteArray ba = QCryptographicHash::hash(theFile.readAll(), QCryptographicHash::Sha3_512);
	const QString result = ba.toHex().constData();
	return result;
}

bool Downloader::getHeader(const QUrl& url, QHash<QString, QString>* pheaders)
{
	QNetworkAccessManager manager;
	QNetworkRequest request(url);

	setHeader(&request);

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
		//print all 
		QByteArrayList headers = reply->rawHeaderList();
		qDebug() << headers.size();
		for (const QByteArray& header : headers)
		{
			qDebug() << QString::fromUtf8(header) << ":" << QString::fromUtf8(reply->rawHeader(header));
			pheaders->insert(QString::fromUtf8(header).toLower(), QString::fromUtf8(reply->rawHeader(header)));
		}

		bret = true;
	}

	reply->deleteLater();

	return bret;
}

bool Downloader::checkUpdate(QString* current, QString* ptext, QString* pformated)
{
	{
		util::Config config(QString("%1|%2").arg(__FUNCTION__).arg(__LINE__));
		g_etag = config.read<QString>("System", "Update", "ETag");
	}

	QString exeFileName = QCoreApplication::applicationFilePath();

	QUrl zipUrl(SASH_UPDATE_URL); // 将 "URL" 替换为实际的下载地址

	QHash<QString, QString> headers;
	if (!getHeader(zipUrl, &headers))
		return false;

	bool bret = false;
	// 获取修改时间
	QDateTime zipModified;
	QDateTime exeModified;

	if (headers.contains("etag"))
	{
		QString newEtag = headers.value("etag");
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

	if (headers.contains("last-modified"))
	{
		QString lastModifiedStr = headers.value("last-modified");

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

	long long timeDiffInSeconds = exeModified.secsTo(zipModified);
	QString szTimeDiff = util::formatSeconds(std::abs(timeDiffInSeconds));
	if (pformated != nullptr)
	{
		*pformated = szTimeDiff;
	}

	if (!bret)
	{
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

	return bret;
}

Downloader::Downloader()
	: szCurrentDirectory_(util::applicationDirPath() + "/")
	, szCurrentDotExe_(szCurrentDirectory_ + util::applicationName())
	, szCurrentDotExeAsDotTmp_(szCurrentDirectory_ + kBackupExecuteFileTmp)
	, szSysTmpDir_(QDir::tempPath())
{
	//connect(this, &Downloader::labelTextChanged, this, &Downloader::onLabelTextChanged, Qt::QueuedConnection);
	//connect(this, &Downloader::progressReset, this, &Downloader::onProgressBarReset, Qt::QueuedConnection);

	rcPath_ = QString("%1/%2/").arg(szSysTmpDir_).arg(pid_);
	QDir dir(rcPath_);
	if (!dir.exists())
		dir.mkpath(rcPath_);

	//從網址中擷取檔案名稱
	QString szFileName = SASH_UPDATE_URL;
	szDownloadedFileName_ = szFileName.mid(szFileName.lastIndexOf('/') + 1);
	szTmpDot7zFile_ = rcPath_ + szDownloadedFileName_;// %Temp%/pid/SaSH.7z

	networkManager_.reset(q_check_ptr(new QNetworkAccessManager(this)));
	sash_assume(networkManager_ != nullptr);

	emit progressReset(0);
}

Downloader::~Downloader()
{
	if (networkManager_ != nullptr)
		networkManager_->deleteLater();

	if (reply_ != nullptr)
	{
		reply_->deleteLater();
		reply_ = nullptr;
	}
}

bool Downloader::download(const QUrl& url, Mode mode)
{
	bool bret = false;
	do
	{
		if (networkManager_ == nullptr)
			break;

		networkManager_->clearAccessCache();

		QNetworkRequest request(url);
		setHeader(&request);

		try
		{
			reply_ = networkManager_->get(request);
		}
		catch (...)
		{
			qDebug() << "Failed to create request.";
			emit labelTextChanged("Failed to create request.");
			break;
		}

		if (reply_ == nullptr)
		{
			qDebug() << "Failed to create request.";
			emit labelTextChanged("Failed to create request.");
			break;
		}

		connect(reply_, &QNetworkReply::downloadProgress, this, &Downloader::onDownloadProgress, Qt::QueuedConnection);
		connect(reply_, &QNetworkReply::errorOccurred, this, &Downloader::onErrorOccurred);
		if (mode == Async)
		{
			connect(reply_, &QNetworkReply::finished, this, &Downloader::onDownloadFinished);
			bret = true;
			break;
		}
		else if (mode == Sync)
		{
			QEventLoop loop;
			QTimer timer;
			connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
			connect(reply_, &QNetworkReply::finished, &loop, &QEventLoop::quit);
			timer.singleShot(30000, &loop, &QEventLoop::quit);
			loop.exec();

			if (nullptr == reply_)
			{
				qDebug() << "Failed to download file: " << url.toString();
				emit labelTextChanged("Failed to download file: " + url.toString());
				break;
			}

			if (reply_->error() != QNetworkReply::NoError)
			{
				qDebug() << "Failed to download file: " << reply_->errorString();
				emit labelTextChanged("Failed to download file: " + reply_->errorString());
			}
			else
			{
				buffer_ = reply_->readAll();
				bret = true;
				break;
			}
		}

	} while (false);

	if (mode == Sync)
	{
		if (reply_ != nullptr)
		{
			reply_->deleteLater();
			reply_ = nullptr;
		}
	}

	return bret;
}

void Downloader::onDownloadProgress(long long bytesReceived, long long bytesTotal)
{
	if (bytesTotal <= 0)
		return;

	double percent = static_cast<double>((bytesReceived * 100.0) / bytesTotal);
	currentProgress_ = static_cast<qreal>(percent);
	if (progressDialog_ != nullptr)
	{
		progressDialog_->setValue(bytesReceived);
		progressDialog_->setMaximum(bytesTotal);
		QCoreApplication::processEvents();
	}
}

void Downloader::onDownloadFinished()
{
	if (reply_ == nullptr)
		return;

	if (reply_->error() == QNetworkReply::NoError)
	{
		buffer_ = reply_->readAll();

	}

	reply_->deleteLater();
	reply_ = nullptr;

	if (callback_ != nullptr)
		callback_();

	if (progressDialog_ != nullptr)
	{
		progressDialog_->onProgressReset(100);
		connect(this, &Downloader::progressReset, progressDialog_, &ProgressDialog::onProgressReset, Qt::QueuedConnection);
		connect(this, &Downloader::labelTextChanged, progressDialog_, &ProgressDialog::setLabelText, Qt::QueuedConnection);
	}
}

void Downloader::onErrorOccurred(QNetworkReply::NetworkError)
{
	if (reply_ != nullptr)
	{
		QString errorString = reply_->errorString();
		qDebug() << "Network error:" << errorString;
		emit labelTextChanged("Network error:" + errorString);
		reply_->deleteLater();
		reply_ = nullptr;
	}

	if (progressDialog_ != nullptr)
	{
		progressDialog_->reset();
		progressDialog_ = nullptr;
	}
}

void Downloader::onCanceled()
{
	qDebug() << "Download canceled.";
	if (reply_ != nullptr)
	{
		reply_->abort();
	}

	if (progressDialog_ != nullptr)
	{
		progressDialog_->reset();
		progressDialog_->close();
		progressDialog_ = nullptr;
	}
}

QString Downloader::read()
{
	if (buffer_.isEmpty())
		return "";

	QString sz = QString::fromUtf8(buffer_);
	buffer_.clear();
	return sz;
}

bool Downloader::write(const QString& fileName)
{
	if (buffer_.isEmpty())
		return false;

	util::ScopedFile file(fileName);
	if (!file.openWriteNew())
		return false;

	file.write(buffer_);
	return true;
}

bool Downloader::start(Source sourceType, QVariant* pvar)
{
	QUrl url;
	Mode mode = Async;
	switch (sourceType)
	{
	case SaSHServer:
		url = QUrl(SASH_UPDATE_URL);
		callback_ = std::bind(&Downloader::overwriteCurrentExecutable, this);
		progressDialog_ = q_check_ptr(new ProgressDialog);
		sash_assume(progressDialog_ != nullptr);
		connect(progressDialog_, &ProgressDialog::canceled, this, &Downloader::onCanceled);
		break;
	case GiteeWiki:
		url = QUrl(SASH_WIKI_URL);
		mode = Sync;
		break;
	case GiteeMapData:
		url = QUrl(SASH_MAPDATA_URL);
		mode = Sync;
		break;
	}

	if (url.isEmpty())
		return false;

	if (!download(url, mode))
		return false;

	if (mode == Async)
	{
		return progressDialog_->exec() == QDialog::Accepted;
	}
	else
	{
		if (pvar != nullptr)
			*pvar = read();
		return true;
	}

	return true;
}

void Downloader::overwriteCurrentExecutable()
{
	{
		//將更新數據寫入至rc文件
		progressDialog_->onProgressReset(0);
		progressDialog_->setLabelText(QString("write downloaded date to %1").arg(rcPath_ + szDownloadedFileName_));
		if (!write(rcPath_ + szDownloadedFileName_))
		{
			qDebug() << "Downloaded file saved to" << QString(rcPath_ + szDownloadedFileName_);
			progressDialog_->setLabelText("downloaded file saved to" + QString(rcPath_ + szDownloadedFileName_));
			return;
		}
		progressDialog_->onProgressReset(100);
	}

	{
		//移除當前進程目錄下的tmp文件
		progressDialog_->onProgressReset(0);
		QFile tmpFile(szCurrentDotExeAsDotTmp_);
		if (tmpFile.exists())
			tmpFile.remove();
		progressDialog_->onProgressReset(100);
	}

	{
		//備份文件
		progressDialog_->onProgressReset(0);
		progressDialog_->setLabelText("backuping...");

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
		progressDialog_->reset();
		progressDialog_->setMaximum(preBackupFileNames.size());
		long long currentSize = 0;
		for (const auto& fileName : preBackupFileNames)
		{
			QFileInfo fileInfo(szCurrentDirectory_ + fileName);
			if (fileInfo.isFile())
			{
				copyFile(szCurrentDirectory_ + fileName, szBackupDir + fileName, true);
				progressDialog_->setLabelText(QString("backuping %1").arg(fileName));
				progressDialog_->setValue(currentSize);
			}
			else if (fileInfo.isDir())
			{
				copyDirectory(szCurrentDirectory_ + fileName, szBackupDir + fileName, true);
				progressDialog_->setLabelText(QString("backuping %1").arg(fileName));
				progressDialog_->setValue(currentSize);
			}
			++currentSize;
			QThread::msleep(1);
		}

		QString szBackup7zFileName = QString(kBackupfileName1).arg(buildDateTime());
		QString szBackup7zFilePath = QString("%1%2").arg(rcPath_).arg(szBackup7zFileName);
		QString szBackup7zNewFilePath = QString("%1%2").arg(szCurrentDirectory_).arg(szBackup7zFileName);
		long long n = 0;
		while (QFile::exists(szBackup7zNewFilePath)) //_2 _3 _4..increase until name is not duplicate
		{
			szBackup7zNewFilePath = QString("%1%2").arg(szCurrentDirectory_).arg(QString(kBackupfileName2).arg(buildDateTime()).arg(++n));
			QThread::msleep(1);
		}

		progressDialog_->onProgressReset(0);
		progressDialog_->setLabelText(QString("compressing %1 to %2").arg(szCurrentDirectory_).arg(szBackup7zFilePath));
		compress(this, szBackupDir, szBackup7zFilePath);

		//move to current
		{
			util::timer timer;
			if (QFile::exists(szBackup7zFilePath))
			{
				while (!QFile::rename(szBackup7zFilePath, szBackup7zNewFilePath))
				{
					QThread::msleep(1000);
					if (timer.hasExpired(30000))
					{
						break;
					}
				}
			}
		}
	}

	// ./SaSH.exe to ./SaSH.tmp
	progressDialog_->onProgressReset(0);
	progressDialog_->setLabelText(QString("rename %1 to %2").arg(szCurrentDotExe_).arg(szCurrentDotExeAsDotTmp_));
	QFile::rename(szCurrentDotExe_, szCurrentDotExeAsDotTmp_);

	{
		//close all .exe that has sadll.dll module
		QVector<long long> processes;
		if (mem::enumProcess(&processes, QString(SASH_INJECT_DLLNAME) + ".dll"))
		{
			progressDialog_->reset();
			progressDialog_->setMaximum(processes.size());
			long long currentSize = 0;
			for (long long pid : processes)
			{
				ScopedHandle hProcess(pid);
				if (hProcess.isValid())
				{
					MINT::NtTerminateProcess(hProcess, 0);
					progressDialog_->setLabelText(QString("close process that has %1.dll module, pid: %2").arg(SASH_INJECT_DLLNAME).arg(pid));
					progressDialog_->setValue(currentSize);
					++currentSize;
					QThread::msleep(1);
				}
			}
		}
	}

	progressDialog_->setLabelText(QString("replace %1 with %2").arg(szCurrentDotExeAsDotTmp_).arg(rcPath_ + szDownloadedFileName_));
	QApplication::processEvents();

	//uncompress sash.7z to current directory
	progressDialog_->setLabelText(QString("uncompress %1 to %2").arg(rcPath_ + szDownloadedFileName_).arg(szCurrentDirectory_));
	uncompress(this, rcPath_ + szDownloadedFileName_, szCurrentDirectory_.chopped(1));

	progressDialog_->setLabelText("congratulations! the update is complete!");
	QApplication::processEvents();

	//移除整個rc
	progressDialog_->setLabelText("removing " + rcPath_);
	QDir d(rcPath_);
	d.removeRecursively();

	constexpr long long delay = 1;
	{
		// write and async run bat file
		progressDialog_->setLabelText("setting up restart process...");
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
		progressDialog_->setLabelText("recording new ETag...");
		util::Config config(QString("%1|%2").arg(__FUNCTION__).arg(__LINE__));
		config.write("System", "Update", "ETag", g_etag);
	}

	progressDialog_->onProgressReset(0);
	progressDialog_->setMaximum(5);
	for (long long i = 5; i > 0; --i)
	{
		progressDialog_->setLabelText(QString("restart SaSH.exe in %1 seconds...").arg(i));
		progressDialog_->setValue(5 - i);
		QThread::msleep(1000);
	}
	MINT::NtTerminateProcess(GetCurrentProcess(), 0);
}