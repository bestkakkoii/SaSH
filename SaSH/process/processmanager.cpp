#include "stdafx.h"
#include "processmanager.h"

#include <remote_memory.h>

QStringList ProcessManager::commandLine_;

namespace
{
	static const QStringList makeGameCommand()
	{
		long long nRealBin = 138;
		long long nAdrnBin = 138;
		long long nSprBin = 116;
		long long nSprAdrnBin = 116;
		long long nRealTrue = 13;
		long long nAdrnTrue = 5;
		long long nEncode = 0;

		bool canSave = false;

		util::Config config(QString("%1|%2").arg(__FUNCTION__).arg(__LINE__));
		long long tmp = config.read<long long>("System", "Command", "realbin");
		if (tmp)
			nRealBin = tmp;
		else
			canSave = true;

		tmp = config.read<long long>("System", "Command", "adrnbin");
		if (tmp)
			nAdrnBin = tmp;
		else
			canSave = true;

		tmp = config.read<long long>("System", "Command", "sprbin");
		if (tmp)
			nSprBin = tmp;
		else
			canSave = true;

		tmp = config.read<long long>("System", "Command", "spradrnbin");
		if (tmp)
			nSprAdrnBin = tmp;
		tmp = config.read<long long>("System", "Command", "realtrue");
		if (tmp)
			nRealTrue = tmp;
		else
			canSave = true;

		tmp = config.read<long long>("System", "Command", "adrntrue");
		if (tmp)
			nAdrnTrue = tmp;
		else
			canSave = true;

		tmp = config.read<long long>("System", "Command", "encode");
		if (tmp)
			nEncode = tmp;
		else
			canSave = true;

		QString customCommand = config.read<QString>("System", "Command", "custom");

		auto mkcmd = [](const QString& sec, long long value)->QString
			{
				return QString("%1:%2").arg(sec).arg(value);
			};

		QStringList commandList;
		//啟動參數
		//updated realbin:138 adrnbin:138 sprbin:116 spradrnbin:116 adrntrue:5 realtrue:13 encode:0 windowmode
		if (customCommand.isEmpty())
		{
			commandList.append("sa_8001.exe");
			commandList.append("updated");
			commandList.append(mkcmd("realbin", nRealBin));
			commandList.append(mkcmd("adrnbin", nAdrnBin));
			commandList.append(mkcmd("sprbin", nSprBin));
			commandList.append(mkcmd("spradrnbin", nSprAdrnBin));
			commandList.append(mkcmd("realtrue", nRealTrue));
			commandList.append(mkcmd("adrntrue", nAdrnTrue));
			commandList.append(mkcmd("encode", nEncode));
			commandList.append("windowmode");
		}
		else
		{
			commandList.append(customCommand);
		}

		return commandList;
	}

}

ProcessManager::ProcessManager(const tool::JsonReply& reply)
	: QObject(nullptr)
	, reply_(reply)
	, conversationId_(reply_.conversationId())
{
	// constexpr const char* command = R"(updated IP:160:1.bluecg.net:8080 IP:161:2.bluecg.net:8080 IP:162:3.bluecg.net:8080 IP:163:4.bluecg.net:8080 IP:164:5.bluecg.net:8080 IP:165:6.bluecg.net:8080 IP:166:7.bluecg.net:8080 IP:167:8.bluecg.net:8080 IP:0:car1.bluecg.net:443 IP:1:2.bluecg.net:443 IP:2:3.bluecg.net:443 IP:3:4.bluecg.net:443 IP:4:5.bluecg.net:443 IP:5:6.bluecg.net:443 IP:6:7.bluecg.net:443 IP:7:8.bluecg.net:443 IP:80:lion1.bluecg.net:8081 IP:81:lion2.bluecg.net:8081 IP:82:lion3.bluecg.net:8081 IP:83:lion4.bluecg.net:8081 IP:84:lion5.bluecg.net:8081 IP:420:ruby1.bluecg.net:80 IP:421:ruby2.bluecg.net:80 IP:422:ruby3.bluecg.net:80 IP:423:ruby4.bluecg.net:80 IP:424:ruby5.bluecg.net:80 IP:140:bear1.bluecg.net:22 IP:141:bear2.bluecg.net:22 IP:142:bear3.bluecg.net:22 IP:143:bear4.bluecg.net:22 IP:144:cno3.bluecg.net:22 IP:40:gemini1.bluecg.net:9045 IP:41:gemini2.bluecg.net:443 IP:42:gemini3.bluecg.net:443 IP:43:gemini4.bluecg.net:443 IP:480:comi1.bluecg.net:9047 IP:481:comi2.bluecg.net:31 IP:482:comi3.bluecg.net:31 IP:483:comi4.bluecg.net:31 IP:484:comi5.bluecg.net:31 graphicbin:66 graphicinfobin:66 animebin:4 animeinfobin:4 graphicbinex:5 graphicinfobinex:5 animebinex:1 animeinfobinex:1 graphicbinv3:19 graphicinfobinv3:19 animebinv3:8 animeinfobinv3:8 graphicbin_puk2:2 graphicinfobin_puk2:2 animebin_puk2:4 animeinfobin_puk2:4 graphicbin_puk3:1 graphicinfobin_puk3:1 animebin_puk3:2 animeinfobin_puk3:2 coordinatebinv3:10 coordinateinfobinv3:10 graphicbin_joy:54 graphicinfobin_joy:54 animebin_joy:34 animeinfobin_joy:34 graphicinfobin_joy_ex:9 graphicbin_joy_ex:9 animebin_joy_ex:9 animebin_joy_ex:9 graphicinfobin_joy_ch:1 graphicbin_joy_ch:1 animebin_joy_ch:1 animebin_joy_ch:1 windowmode B2G windowmode:1    3Ddevice:1 GACGS   YZM:BlueLogin PZX:y67AtsSnwaZ8M3wwfHx8fDA= )";

	commandLine_ = makeGameCommand();
}

ProcessManager::~ProcessManager()
{
	if (nullptr != process_)
	{
		process_->deleteLater();
		process_ = nullptr;
	}
}

qint64 ProcessManager::processId() const
{
	return processId_;
}

qint64 ProcessManager::elapsed()
{
	return timer_.elapsed();
}

void ProcessManager::handleStartUp()
{
	process_ = new QProcess(this);
	if (nullptr == process_)
	{
		qCritical().noquote() << "Failed to create game process";
		return;
	}


	connect(process_, &QProcess::errorOccurred, this, &ProcessManager::handleProcessError, Qt::QueuedConnection);

	process_->setWorkingDirectory(QDir::currentPath());
	process_->setReadChannel(QProcess::StandardOutput);
	process_->setProcessChannelMode(QProcess::MergedChannels);

	process_->start("sa_8001.exe", commandLine_);
	if (process_->waitForStarted())
	{
		handleProcessStarted();
	}
}

void ProcessManager::handleProcessStarted()
{
	connect(process_, &QProcess::readyReadStandardOutput, this, &ProcessManager::handleOutput, Qt::QueuedConnection);
	connect(process_, &QProcess::readyReadStandardError, this, &ProcessManager::handleError, Qt::QueuedConnection);
	connect(process_, QOverload<qint32, QProcess::ExitStatus>::of(&QProcess::finished), this, &ProcessManager::handleProcessFinished, Qt::QueuedConnection);

	processId_ = process_->processId();
	qDebug().noquote() << QString("Game process started: {\"processId\":%1}").arg(processId_);

	reply_.setProcessId(processId_);
	reply_.insertParameter("elapsed", timer_.elapsed());

	emit started(reply_);

	timer_.restart();
}

void ProcessManager::handleProcessError(QProcess::ProcessError error)
{
	qCritical().noquote() << "Failed to start game process, error:" << error << ", errorString:" << process_->errorString();
	reply_.setAction("response");
	reply_.setMessage(process_->errorString());
	reply_.setStatus(false);
	reply_.setProcessId(processId_);
	reply_.insertParameter("error", static_cast<quint64>(error));

	emit failed();

	if (nullptr == process_)
	{
		return;
	}

	process_->deleteLater();
	process_ = nullptr;
}

void ProcessManager::handleOutput()
{
	if (nullptr == process_)
	{
		return;
	}

	while (process_->canReadLine())
	{
		qDebug().noquote() << "[DLL]:" << QString::fromUtf8(process_->readLine());
	}
}

void ProcessManager::handleError()
{
	if (nullptr == process_)
	{
		return;
	}

	while (process_->canReadLine())
	{
		qDebug().noquote() << QString::fromUtf8(process_->readLine());
	}
}

void ProcessManager::handleProcessFinished(qint32 exitCode, QProcess::ExitStatus exitStatus)
{
	QMetaEnum metaEnum = QMetaEnum::fromType<QProcess::ExitStatus>();
	QString message = QString("%1(%2)").arg(metaEnum.valueToKey(exitStatus)).arg(static_cast<quint64>(exitStatus));

	if (0 == exitCode)
	{
		qDebug().noquote() << "Game process finished, exitCode:" << static_cast<quint64>(exitCode) << ", exitStatus:" << message;
	}
	else
	{
		qCritical().noquote() << "Game process finished with an error, exitCode:" << static_cast<quint64>(exitCode) << ", exitStatus:" << message;
	}

	reply_.setAction("notify");
	reply_.setMessage(message);
	reply_.setStatus(true);
	reply_.setCommand("closeGame");
	reply_.insertParameter("exitCode", static_cast<quint64>(exitCode));
	reply_.insertParameter("exitStatus", static_cast<quint64>(exitStatus));
	reply_.insertParameter("elapsed", timer_.elapsed());

	if (!process_.isNull())
	{
		process_->deleteLater();
		process_ = nullptr;
	}

	emit processFinished(processId_);
}