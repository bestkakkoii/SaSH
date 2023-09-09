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

#pragma once

#include <QWidget>
#include "ui_downloader.h"
#include <QTimer>
#include <QFutureSynchronizer>
#include <QDir>
#include <QProgressBar>
#include <QObject>
#include <QString>
#include <QCoreApplication>

constexpr size_t MAX_DOWNLOAD_THREAD = 4u;

class Downloader : public QWidget
{
	Q_OBJECT
public:
	explicit Downloader(QWidget* parent = nullptr);

	virtual ~Downloader();

	void start();

	static bool checkUpdate(QString* current, QString* ptext);

protected:
	virtual void showEvent(QShowEvent* event) override;

private:
	QProgressBar* createProgressBar(int startY);
	/////////////////////////////////////////////

	void resetProgress(int value);

	void overwriteCurrentExecutable();

	bool asyncDownloadFile(const QString& szUrl, const QString& dir, const QString& szSaveFileName);

	static void setProgressValue(int i, qreal totalToDownload, qreal nowDownloaded, qreal totalToUpLoad, qreal nowUpLoaded);

	template <int Index>
	static int onProgress(void* clientp, qint64 totalToDownload, qint64 nowDownloaded, qint64 totalToUpLoad, qint64 nowUpLoaded);

	QString Sha3_512(const QString& fileNamePath) const;

	void Downloader::downloadAndExtractZip(const QString& url, const QString& targetDir);

private:
	Ui::DownloaderClass ui;

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
