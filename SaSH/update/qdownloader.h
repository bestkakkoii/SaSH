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
	static bool checkUpdate(QString* ptext);
protected:
	void showEvent(QShowEvent* event) override;

private:
	QProgressBar* CreateProgressBar(int startY);
	/////////////////////////////////////////////
	void ResetProgress(int value);
	void OverwriteCurrentExecutable();
	bool AsyncDownloadFile(const QString& szUrl, const QString& dir, const QString& szSaveFileName);
	QTimer m_labelTimer;
	QTimer m_timer[MAX_DOWNLOAD_THREAD];
	static void SetProgressValue(int i, double totalToDownload, double nowDownloaded, double totalToUpLoad, double nowUpLoaded);
	static int OnProgress(void* ptr, double totalToDownload, double nowDownloaded, double totalToUpLoad, double nowUpLoaded);
	static int OnProgress_2(void* ptr, double totalToDownload, double nowDownloaded, double totalToUpLoad, double nowUpLoaded);
	static int OnProgress_3(void* ptr, double totalToDownload, double nowDownloaded, double totalToUpLoad, double nowUpLoaded);
	static int OnProgress_4(void* ptr, double totalToDownload, double nowDownloaded, double totalToUpLoad, double nowUpLoaded);

	QString Sha3_512(const QString& fileNamePath) const;

private:
	Ui::QDownloaderClass ui;

	const int m_pid = QCoreApplication::applicationPid();
	const QString m_szCurrentDirectory;
	const QString m_szCurrentDotExe;
	const QString m_szCurrentDotExeAsDotTmp;
	const QString m_sz7zDotExe;
	const QString m_sz7zDotDll;
	const QString m_szSysTmpDir;

	QString m_szDownloadedFileName = "\0";
	QString m_rcPath = "\0";
	QString m_szTmpDot7zFile = "\0";

	QFutureSynchronizer<void> m_synchronizer;


};
