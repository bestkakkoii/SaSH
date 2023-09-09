#pragma once

#if _MSC_VER >= 1600 
#pragma execution_character_set("utf-8") 
#endif

namespace compile
{
	constexpr const char* global_date(__DATE__);
	constexpr const char* global_time(__TIME__);

	static QDateTime g_buildDate;

	static QString initializeBuildDateTime();

	static QString g_buildDateTime = initializeBuildDateTime();

	static QString initializeBuildDateTime()
	{
		QString dateTimeStr(global_date);
		dateTimeStr.replace("  ", " 0");// 注意" "是兩個空格，用於日期為單數時需要轉成“空格+0”

		// 創建 QDateTime 對象以解析日期部分
		const QLocale l(QLocale::English);
		const QDateTime d(l.toDateTime(dateTimeStr, "MMM dd yyyy"));

		// 獲取日期對應的時區
		const QTimeZone timeZone = QTimeZone::systemTimeZone();

		// 判斷是否在夏令時期間
		const bool isDST = timeZone.isDaylightTime(d);

		// 構建日期時間字符串
		const QString str(QString("%1 %2").arg(d.toString("yyyy-MM-dd")).arg(global_time));

		QDateTime dt(QDateTime::fromString(str, "yyyy-MM-dd hh:mm:ss"));

		constexpr qint64 baseTime = 60ll * 60ll;

		// 根據夏令時調整時間
		if (isDST)
			dt = dt.addSecs(15ll * baseTime); // 考慮夏令時變化和 -15 小時
		else
			dt = dt.addSecs(15ll * baseTime); // 考慮 -16 小時

		g_buildDate = dt;

		return dt.toString("yyyyMMdd-hh:mm:ss");
	}

	inline QString buildDateTime(QDateTime* date)
	{
		if (date != nullptr && g_buildDate.isValid())
			*date = g_buildDate;
		return g_buildDateTime;
	}
}
