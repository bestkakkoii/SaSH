#include "stdafx.h"
#include <version.h>

// 靜態成員變量初始化
QString Version::version_ = QString("%1.%2.%3")
.arg(Version::majorVersion)
.arg(Version::minorVersion)
.arg(Version::generateBuildNumber());

// 將日期和時間轉換為 ISO 格式
QString Version::generateBuildNumber()
{
	static const QLocale locale(QLocale::English);
	QString dateStr = QString("%1 %2").arg(QString(compileDate).replace("  ", " 0")).arg(compileTime);
	QDateTime dateTime = locale.toDateTime(dateStr, "MMM d yyyy HH:mm:ss");
	if (!dateTime.isValid())
	{
		dateTime = locale.toDateTime(dateStr, "MMM dd yyyy HH:mm:ss");
	}

	// 設置時區
	QTimeZone timeZone = QTimeZone::systemTimeZone();
	dateTime.setTimeZone(timeZone);

	QTimeZone utcTimeZone(QTimeZone::utc());
	dateTime = dateTime.toTimeZone(utcTimeZone);

	// 通過數學計算生成年月和時分編號
	QString buildNumber = QString("%1%2.%3%4%5")
		.arg(dateTime.date().year())
		.arg(dateTime.date().month(), 2, 10, QChar('0'))
		.arg(dateTime.date().day(), 2, 10, QChar('0'))
		.arg(dateTime.time().hour(), 2, 10, QChar('0'))
		.arg(dateTime.time().minute(), 2, 10, QChar('0'));

	return buildNumber;
}

QString Version::generate()
{
	return version_;
}