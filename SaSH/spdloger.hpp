#pragma once
//spdlog
#define SPDLOG_TRACE_ON
#define SPDLOG_DEBUG_ON
#define SPDLOG_INFO_ON
#define SPDLOG_WCHAR_FILENAMES
#define SPDLOG_WCHAR_TO_UTF8_SUPPORT

#include <QObject>

#include <QCoreApplication>
#include <QDir>
#include <QDateTime>
#include <string>
#include <sstream>
#include <iostream>
#include "spdlog/spdlog.h"

#include "spdlog/sinks/rotating_file_sink.h"
//#include "spdlog/logger.h"
#include "spdlog/async.h"

#include "injector.h"

//#include "spdlog/sinks/stdout_sinks.h"
//#include "spdlog/sinks/stdout_color_sinks.h"
//#include "spdlog/common.h"

typedef enum
{
	SPD_INFO,
	SPD_WARN,
	SPD_ERROR,
	SPD_DEBUG,
	SPD_CRITICAL,
	SPD_TRACE,
}SPD_LOGTYPE;

constexpr int GLOBAL_LOG_ID = 1000;

inline void SPD_INIT(const std::wstring& name, const std::wstring& fileName)
{
	try
	{
		QString path(QString("%1/log/").arg(QCoreApplication::applicationDirPath()));
		QDir dir(path);
		if (!dir.exists())
		{
			dir.mkdir(path);
		}

		QString qname = QString::fromStdWString(name);
		std::string sname = qname.toStdString();
		if (spdlog::get(sname))
			return;

		constexpr int max_size = 1024 * 1024 * 100;
		constexpr int max_files = 10;

		std::shared_ptr<spdlog::sinks::rotating_file_sink_mt> rotating_file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(fileName.c_str(), max_size, max_files);



		std::shared_ptr<spdlog::logger> log = std::make_shared<spdlog::logger>(sname, rotating_file_sink);
		spdlog::initialize_logger(log);
		log->set_level(spdlog::level::trace);
		log->flush_on(spdlog::level::info);
	}
	catch (...)
	{

	}
}

inline void SPD_INIT(int index)
{
	try
	{
		QString name("\0");
		if (index != GLOBAL_LOG_ID)
			name = QString("SaSH{ID_%1}").arg(index);
		else
			name = QString("SaSH{Global}");
		std::wstring sname = name.toStdWString();

		QDateTime time = QDateTime::currentDateTime();
		QString timeStr = time.toString("yyyyMMdd");
		QString fileName = QString("%1/log/%2%3.log").arg(QCoreApplication::applicationDirPath()).arg(name).arg(timeStr).toLower();
		QFile file(fileName);
		if (file.exists())
			file.remove();

		std::wstring sfileName = fileName.toStdWString();
		SPD_INIT(sname, sfileName);
	}
	catch (...)
	{

	}
}

inline QString SPD_INIT(const QString& filename)
{
	try
	{
		QString name = QString("SaSH{%1}").arg(filename);
		std::wstring sname = name.toStdWString();
		QString fileName = "\0";
		//file name format: year_month_day_hours_ID_index
		QDateTime time = QDateTime::currentDateTime();
		QString timeStr = time.toString("yyyyMMdd");
		fileName = QString("%1/log/%2%3.log").arg(QCoreApplication::applicationDirPath()).arg(name).arg(timeStr).toLower();
		std::wstring sfileName = fileName.toStdWString();
		SPD_INIT(sname, sfileName);
		return name;
	}
	catch (...)
	{

	}
	return QString("\0");
}

inline void SPD_CLOSE(const std::string& name)
{
	try
	{
		if (name.empty()) return;
		std::shared_ptr<spdlog::logger> log = spdlog::get(name);
		if (log)
		{
			log->flush();
			log.reset();
		}
		spdlog::drop(name);
	}
	catch (...)
	{

	}
}

inline void SPD_CLOSE(int index)
{
	try
	{
		QString name("\0");
		if (index != GLOBAL_LOG_ID)
			name = QString("SaSH{ID_%1}").arg(index);
		else
			name = QString("SaSH{Global}");
		std::string sname = name.toStdString();
		SPD_CLOSE(sname);
	}
	catch (...)
	{

	}
}

inline void SPD_LOG(int index, const QString& msg, SPD_LOGTYPE logtype = SPD_INFO)
{
	Injector& injector = Injector::getInstance();
	if (!injector.getEnableHash(util::kScriptDebugModeEnable))
		return;


	try
	{
		QString name("\0");
		if (index != GLOBAL_LOG_ID)
			name = QString("BlueCgHP{ID_%1}").arg(index);
		else
			name = QString("BlueCgHP{Global}");
		std::string sname = name.toStdString();

		QString newMessage = QString("[%1] %2").arg(GetCurrentThreadId()).arg(msg);
		std::wstring smsg = newMessage.toStdWString();
		std::shared_ptr<spdlog::logger> log = spdlog::get(sname);
		if (log)
		{
			switch (logtype)
			{
			case SPD_INFO:
				log->info(smsg);
				break;
			case SPD_WARN:
				log->warn(smsg);
				break;
			case SPD_ERROR:
				log->error(smsg);
				break;
			case SPD_DEBUG:
				log->debug(smsg);
				break;
			case SPD_CRITICAL:
				log->critical(smsg);
				break;
			case SPD_TRACE:
				log->trace(smsg);
				break;
			}
		}
	}
	catch (...)
	{

	}
}

inline void SPD_LOG(const QString& name, const QString& msg, SPD_LOGTYPE logtype = SPD_DEBUG)
{
	Injector& injector = Injector::getInstance();
	if (!injector.getEnableHash(util::kScriptDebugModeEnable))
		return;

	try
	{
		std::string sname = name.toStdString();

		QString newMessage = QString("[%1] %2").arg(GetCurrentThreadId()).arg(msg);
		std::wstring smsg = newMessage.toStdWString();
		std::shared_ptr<spdlog::logger> log = spdlog::get(sname);
		if (log)
		{
			switch (logtype)
			{
			case SPD_INFO:
				log->info(smsg);
				break;
			case SPD_WARN:
				log->warn(smsg);
				break;
			case SPD_ERROR:
				log->error(smsg);
				break;
			case SPD_DEBUG:
				log->debug(smsg);
				break;
			case SPD_CRITICAL:
				log->critical(smsg);
				break;
			case SPD_TRACE:
				log->trace(smsg);
				break;
			}
		}
	}
	catch (...)
	{

	}
}

inline void SPD_LOG(const std::string& name, const std::string& msg, SPD_LOGTYPE logtype = SPD_DEBUG)
{
	Injector& injector = Injector::getInstance();
	if (!injector.getEnableHash(util::kScriptDebugModeEnable))
		return;

	try
	{
		std::shared_ptr<spdlog::logger> log = spdlog::get(name);
		if (log)
		{
			QString qmsg = QString("[%1] %2").arg(GetCurrentThreadId()).arg(QString::fromStdString(msg));
			std::wstring smsg = qmsg.toStdWString();
			switch (logtype)
			{
			case SPD_INFO:
				log->info(smsg);
				break;
			case SPD_WARN:
				log->warn(smsg);
				break;
			case SPD_ERROR:
				log->error(smsg);
				break;
			case SPD_DEBUG:
				log->debug(smsg);
				break;
			}
		}
	}
	catch (...)
	{

	}
}