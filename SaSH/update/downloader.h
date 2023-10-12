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

class Downloader : public QWidget
{
	Q_OBJECT
public:
	explicit Downloader(QWidget* parent = nullptr);

	virtual ~Downloader();

	static bool checkUpdate(QString* current, QString* ptext, QString* pformated);

	QString oneShotDownload(const QString& url);

protected:
	virtual void showEvent(QShowEvent* event) override;

private:
	QProgressBar* createProgressBar(__int64 startY);
	/////////////////////////////////////////////
	Q_INVOKABLE void start();

	void overwriteCurrentExecutable(QByteArray ba);

	void downloadAndUncompress(const QString& url, const QString& targetDir);

	bool download(const QString& url, QByteArray* pbyteArray);

	bool downloadFile(const QString& url, const QString& filename);

	bool compress(const QString& source, const QString& destination);
	bool uncompress(const QString& source, const QString& destination);

signals:
	void labelTextChanged(const QString& text);
	void progressReset(__int64 value);

private slots:
	void onDownloadProgress(__int64 bytesReceived, __int64 bytesTotal);
	void onDownloadFinished();
	void onLabelTextChanged(const QString& text);
	void onProgressBarReset(__int64 value);
	void onErrorOccurred(QNetworkReply::NetworkError code);

private:
	Ui::DownloaderClass ui;

	const __int64 pid_ = _getpid();
	const QString szCurrentDirectory_;
	const QString szCurrentDotExe_;
	const QString szCurrentDotExeAsDotTmp_;
	const QString szSysTmpDir_;

	QString szDownloadedFileName_ = "\0";
	QString rcPath_ = "\0";
	QString szTmpDot7zFile_ = "\0";

private:
	bool isMain = false;
	qreal currentProgress_ = 0.0;
	QProgressBar* progressBar = nullptr;
	std::unique_ptr<QNetworkAccessManager> networkManager_;
	QNetworkReply* reply_ = nullptr;
};
