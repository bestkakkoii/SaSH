#pragma once

#include <QWidget>
#include "ui_qdownloader.h"
#include <QTimer>
#include <QFutureSynchronizer>
#include <QDir>
#include <QProgressBar>
#include <QObject>
#include <QString>
#include <QCoreApplication>

constexpr size_t MAX_DOWNLOAD_THREAD = 4u;

class QDownloader : public QWidget
{
	Q_OBJECT

public:
	QDownloader(QWidget* parent = nullptr);

	virtual ~QDownloader();

	void start();

	static bool checkUpdate(QString* current, QString* ptext);

protected:
	void showEvent(QShowEvent* event) override;

private:
	QProgressBar* createProgressBar(int startY);
	/////////////////////////////////////////////

	void resetProgress(int value);
	void overwriteCurrentExecutable();
	bool asyncDownloadFile(const QString& szUrl, const QString& dir, const QString& szSaveFileName);

	static void setProgressValue(int i, double totalToDownload, double nowDownloaded, double totalToUpLoad, double nowUpLoaded);
	static int onProgress(void* ptr, double totalToDownload, double nowDownloaded, double totalToUpLoad, double nowUpLoaded);
	static int onProgress_2(void* ptr, double totalToDownload, double nowDownloaded, double totalToUpLoad, double nowUpLoaded);
	static int onProgress_3(void* ptr, double totalToDownload, double nowDownloaded, double totalToUpLoad, double nowUpLoaded);
	static int onProgress_4(void* ptr, double totalToDownload, double nowDownloaded, double totalToUpLoad, double nowUpLoaded);

	QString Sha3_512(const QString& fileNamePath) const;
	void QDownloader::downloadAndExtractZip(const QString& url, const QString& targetDir);
private:
	Ui::QDownloaderClass ui;

	QTimer labelTimer_;
	QTimer timer_[MAX_DOWNLOAD_THREAD];

	const int pid_ = QCoreApplication::applicationPid();
	const QString szCurrentDirectory_;
	const QString szCurrentDotExe_;
	const QString szCurrentDotExeAsDotTmp_;
	const QString sz7zDotExe_;
	const QString sz7zDotDll_;
	const QString szSysTmpDir_;

	QString szDownloadedFileName_ = "\0";
	QString rcPath_ = "\0";
	QString szTmpDot7zFile_ = "\0";

	QFutureSynchronizer<void> synchronizer_;
};
