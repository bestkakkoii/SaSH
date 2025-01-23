#ifndef PROCESSMANAGER_H_
#define PROCESSMANAGER_H_

#include <QCoreApplication>
#include <QProcess>
#include <QDebug>
#include <QObject>
#include <QTextStream>
#include <windows.h>
#include <tlhelp32.h>
#include <QPointer>
#include <QWebSocket>
#include <QThread>

#include <tool.h>
#include <timer.h>

class ProcessManager : public QObject
{
	Q_OBJECT

public:
	explicit ProcessManager(const tool::JsonReply& reply);
	virtual ~ProcessManager();

	qint64 processId() const;

	qint64 elapsed();

signals:
	void started(tool::JsonReply reply);

	void failed();

	void processFinished(qint64 processId);


public:
	void handleStartUp();

private slots:
	void handleProcessStarted();

	void handleProcessError(QProcess::ProcessError error);

	void handleOutput();

	void handleError();

	void handleProcessFinished(qint32 exitCode, QProcess::ExitStatus exitStatus);

private:


private:
	tool::JsonReply reply_;
	Timer timer_;
	qint64 processId_ = 0;
	QString conversationId_;
	QPointer<QProcess> process_ = nullptr;
	static QStringList commandLine_;
};

#endif // PROCESSMANAGER_H_