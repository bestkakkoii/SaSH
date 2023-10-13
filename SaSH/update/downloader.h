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
#include <QObject>
#include <QApplication>
#include <QDir>
#include <QString>
#include <QProgressDialog>

class ProgressDialog : public QProgressDialog
{
	Q_OBJECT
public:
	explicit ProgressDialog(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags())
		: QProgressDialog(parent, flags)
	{
		setWindowModality(Qt::WindowModal);
		setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
		setWindowTitle(tr("Downloading..."));
		setLabelText(tr("Downloading..."));
		setMinimum(0);
		setMaximum(100);
		setValue(0);
		setFixedWidth(1200);

		setMinimumDuration(0);
		setAutoClose(false);
		setAutoReset(false);
		setAttribute(Qt::WA_DeleteOnClose);

		setCancelButtonText(tr("Cancel"));

		QRect screenRect = QGuiApplication::primaryScreen()->geometry();
		int x = (screenRect.width() - width()) / 2;
		int y = (screenRect.height() - height()) / 2;
		move(x, y);
	}

	~ProgressDialog()
	{
		qDebug() << "ProgressDialog::~ProgressDialog()";
	}

public slots:
	void onProgressReset(qint64 value)
	{
		reset();
		setMaximum(100);
		setValue(value);
		QCoreApplication::processEvents();
	}
};

class Downloader : public QObject
{
	Q_OBJECT
public:
	enum Mode
	{
		Async = 0,
		Sync = 1
	};

	enum Source
	{
		SaSHServer,
		GiteeWiki,
		GiteeMapData,
	};

	Downloader();

	virtual ~Downloader();

	QString read();

	bool write(const QString& fileName);

	bool start(Source sourceType, QVariant* pvar = nullptr);

	ProgressDialog* progressDialog_ = nullptr;
public:
	static bool checkUpdate(QString* current, QString* ptext, QString* pformated);

	static bool getHeader(const QUrl& url, QHash<QString, QString>* pheaders);

private:
	void overwriteCurrentExecutable();

	bool download(const QUrl& url, Mode mode = Sync);

signals:
	void labelTextChanged(const QString& text);
	void progressReset(qint64 value);

private slots:
	void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void onDownloadFinished();
	void onErrorOccurred(QNetworkReply::NetworkError code);
	void onCanceled();

private:
	const qint64 pid_ = _getpid();
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
	std::unique_ptr<QNetworkAccessManager> networkManager_;
	QNetworkReply* reply_ = nullptr;
	QByteArray buffer_;
	std::function<void()> callback_;
};
