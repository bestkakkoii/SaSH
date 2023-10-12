module;
#include <QObject>
#include <QContiguousCache>
#include <QMutex>
#include <QDir>
#include <QDateTime>
#include <QRegularExpression>
#include <atomic>
#include "indexer.h"

export module Logger;

import Utility;
import Scoped;

export class Logger : public Indexer
{
public:
	explicit Logger(__int64 index)
		: Indexer(index)
	{

	}

private:
	void writeLog(const QString& logText)
	{
		if (!isInitialized_.load(std::memory_order_acquire))
			return;
		QString text = logText;
		format(text);
		QByteArray data = text.toUtf8();
		data.append("\n");
		file_.write(data);
	}

	void emitLog(const QContiguousCache<QString>& logText)
	{
		if (!isInitialized_.load(std::memory_order_acquire))
			return;
		QStringList log;
		for (__int64 i = 0; i < logText.size(); ++i)
			writeLog(logText.at(i));

		nextBuffer_.clear();
	}

	void swapBuffersAndWrite()
	{
		if (!isInitialized_.load(std::memory_order_acquire))
			return;

		currentBuffer_.swap(nextBuffer_);
		emitLog(nextBuffer_);
	}

public:
	virtual ~Logger()
	{
		if (file_.isOpen())
		{
			swapBuffersAndWrite();
			file_.flush();
			file_.close();
		}
	}

private:
	void format(QString& logText, bool isFileName = false) const
	{
		QStringList logTextList = logText.split("@@@@@@@@@@", Qt::SkipEmptyParts);
		static const QRegularExpression re(R"(%\((\w+)\))");
		QRegularExpressionMatchIterator i = re.globalMatch(logFormat_);
		QString newLogText;
		if (!isFileName)
		{
			newLogText = logFormat_;
			newLogText.replace("%(date)", QDate::currentDate().toString("yyyy-MM-dd"));
			newLogText.replace("%(time)", QTime::currentTime().toString("hh:mm:ss"));
			newLogText.replace("%(message)", logTextList.value(0));

			if (logTextList.size() > 1)
				newLogText.replace("%(line)", logTextList.value(1));
			else
				newLogText.replace("%(line)", "");
		}
		else
		{
			newLogText = logText;
			newLogText.replace("%(date)", QDate::currentDate().toString("yyyyMMdd"));
			newLogText.replace("%(time)", QTime::currentTime().toString("hh_mm_ss"));
		}

		logText = newLogText;
	}

	bool openFile(const QString& logFileName)
	{
		if (file_.isOpen())
		{
			swapBuffersAndWrite();
			file_.flush();
			file_.close();
		}

		QString fileName = logFileName;
		format(fileName, true);
		if (!fileName.endsWith(".log"))
			fileName.append(".log");

		QDir logDir(util::applicationDirPath() + QString("/lib/log")); // Change to your desired log directory
		if (!logDir.exists())
			logDir.mkpath(util::applicationDirPath());

		fileName = logDir.absoluteFilePath(fileName);

		file_.setFileName(fileName);

		if (file_.openWriteAppend())
		{
			QString text = QString("========== [info] [index:%1] [datetime:%2] ==========\n").arg(getIndex()).arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
			file_.write(text.toUtf8());
			isInitialized_.store(true, std::memory_order_release);
			return true;
		}

		isInitialized_.store(false, std::memory_order_release);
		return false;
	}

public:
	bool initialize(const QString& logFileName, int bufferSize = 1024, const QString& logFormat = "[%(date) %(time)] | [@%(line)] | %(message)")
	{
		QMutexLocker locker(&mutex_);
		bufferSize_ = bufferSize;
		logFormat_ = logFormat;
		currentBuffer_.clear();
		nextBuffer_.clear();

		currentBuffer_.setCapacity(bufferSize_);
		nextBuffer_.setCapacity(bufferSize_);

		return openFile(logFileName);
	}

	void write(const QString& message, __int64 line = -1)
	{
		if (!isInitialized_.load(std::memory_order_acquire))
			return;

		QMutexLocker locker(&mutex_);

		QString newMessage = QString("%1@@@@@@@@@@%2").arg(message).arg(line);

		if (bufferSize_ == 0)
		{
			writeLog(newMessage);
			return;
		}
		else if (message.size() > bufferSize_)
		{
			bufferSize_ = message.size();
			currentBuffer_.setCapacity(bufferSize_);
			nextBuffer_.setCapacity(bufferSize_);
		}

		if (static_cast<__int64>(currentBuffer_.size()) + static_cast<__int64>(message.size()) > bufferSize_)
			swapBuffersAndWrite();

		currentBuffer_.append(newMessage);
	}

	int size() const
	{
		QMutexLocker locker(&mutex_);
		return currentBuffer_.size();
	}

	void flush()
	{
		QMutexLocker locker(&mutex_);
		swapBuffersAndWrite();
	}

	bool isEmpty() const
	{
		QMutexLocker locker(&mutex_);
		return currentBuffer_.isEmpty() && nextBuffer_.isEmpty();
	}

	bool isFull() const
	{
		QMutexLocker locker(&mutex_);
		return currentBuffer_.size() >= bufferSize_;
	}

	bool isOpen() const
	{
		return isInitialized_.load(std::memory_order_acquire);
	}

	void close()
	{
		QMutexLocker locker(&mutex_);
		if (file_.isOpen())
		{
			swapBuffersAndWrite();
			file_.flush();
			file_.close();
			isInitialized_.store(false, std::memory_order_release);
		}
	}

private:
	mutable QMutex mutex_;
	std::atomic<bool> isInitialized_;
	ScopedFile file_;
	__int64 bufferSize_ = 1024;
	QString logFormat_;
	QContiguousCache<QString> currentBuffer_;
	QContiguousCache<QString> nextBuffer_;
};

//int main(int argc, char** argv)
//{
//	QCoreApplication app(argc, argv);
//
//	Logger& logger = Logger::instance();
//	logger.initialize("mylog.log", 1024, "%(date) %(time) %(message)");
//
//	// Example usage:
//	logger.write("This is a log message.");
//
//	return app.exec();
//}