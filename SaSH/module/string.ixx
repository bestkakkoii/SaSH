module;

#include <Windows.h>
#include <QObject>
#include <QDebug>
#include <QString>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QtCore/QTextCodec>
#else
#include <QtCore5Compat/QTextCodec>
#endif
#include <QByteArray>
#include <QScopedArrayPointer>
#include <QTextStream>

#ifndef SOL_ALL_SAFETIES_ON
#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>
#endif

export module String;
import <type_traits>;
import <string>;
import Global;

export template<typename T>
inline [[nodiscard]] QString toQString(T d, __int64 base = 10)
{
	if constexpr (std::is_same_v<T, double>)
	{
		return QString::number(d, 'f', 5);
	}
	else if constexpr (std::is_same_v<T, float>)
	{
		return QString::number(d, 'f', 5);
	}
	else if constexpr (std::is_same_v<T, bool>)
	{
		return (d) ? QString("true") : QString("false");
	}
	else if constexpr (std::is_same_v<T, wchar_t*>)
	{
		return QString::fromWCharArray(d);
	}
	else if constexpr (std::is_same_v<T, const wchar_t*>)
	{
		return QString::fromWCharArray(d);
	}
	else if constexpr (std::is_same_v<T, char*>)
	{
		return QString::fromUtf8(d);
	}
	else if constexpr (std::is_same_v<T, const char*>)
	{
		return QString::fromUtf8(d);
	}
	else if constexpr (std::is_same_v<T, std::string>)
	{
		return QString::fromUtf8(d.c_str());
	}
	else if constexpr (std::is_same_v<T, __int64>)
	{
		return QString::number(d, base);
	}
	else if constexpr (std::is_same_v<T, quint64>)
	{
		return QString::number(d, base);
	}
	else if constexpr (std::is_same_v<T, int>)
	{
		return QString::number(d, base);
	}
	else if constexpr (std::is_same_v < T, QByteArray>)
	{
		return QString::fromUtf8(d);
	}
	else if constexpr (std::is_same_v < T, sol::object>)
	{
		if (d.is<std::string>())
			return QString::fromUtf8(d.as<std::string>().c_str());
		else
			return "";
	}
	else if constexpr (std::is_integral_v<T>)
	{
		return QString::number(d, base);
	}
	else
	{
		qDebug() << "toQString: unknown type" << typeid(T).name();
		MessageBoxA(NULL, typeid(T).name(), "toQString: unknown type", MB_OK | MB_ICONERROR);
		[[assume(false)]]
		return QString();
	}

}

export inline [[nodiscard]] QString toQUnicode(const char* str, bool trim = true, bool ext = true)
{
	QTextCodec* codec = QTextCodec::codecForName(SASH_DEFAULT_GAME_CODEPAGE);//QTextCodec::codecForMib(2025);//取GB2312解碼器
	QString qstr = codec->toUnicode(str);//先以GB2312解碼轉成UNICODE
	UINT ACP = ::GetACP();
	if (950 == ACP && ext)
	{
#ifndef _DEBUG
		// 繁體系統要轉繁體否則遊戲視窗標題會亂碼(一堆問號字)
		std::wstring wstr = qstr.toStdWString();
		__int64 size = lstrlenW(wstr.c_str());
		QScopedArrayPointer <wchar_t> wbuf(new wchar_t[size + 1]());
		//繁體字碼表映射
		LCMapStringEx(LOCALE_NAME_SYSTEM_DEFAULT, LCMAP_TRADITIONAL_CHINESE, wstr.c_str(), size, wbuf.data(), size, NULL, NULL, NULL);
		qstr = toQString(wbuf.data());
#endif
}
	if (!trim)
		return qstr;
	else
		return qstr.simplified();
}

export inline [[nodiscard]] std::string fromQUnicode(const QString& str, bool keppOrigin = false)
{
	QString qstr = str;
	std::wstring wstr = qstr.toStdWString();
	UINT ACP = ::GetACP();
	if (950 == ACP && !keppOrigin)
	{
		// 繁體系統要轉回簡體體否則遊戲視窗會亂碼
		__int64 size = lstrlenW(wstr.c_str());
		QScopedArrayPointer <wchar_t> wbuf(new wchar_t[size + 1]());
		LCMapStringEx(LOCALE_NAME_SYSTEM_DEFAULT, LCMAP_SIMPLIFIED_CHINESE, wstr.c_str(), size, wbuf.data(), size, NULL, NULL, NULL);
		qstr = toQString(wbuf.data());
	}

	QTextCodec* codec = QTextCodec::codecForName(SASH_DEFAULT_GAME_CODEPAGE);//QTextCodec::codecForMib(2025);//QTextCodec::codecForName("gb2312");
	QByteArray bytes = codec->fromUnicode(qstr);

	std::string s = bytes.data();

	return s;
}

export QString formatMilliseconds(__int64 milliseconds, bool noSpace = false)
{
	__int64 totalSeconds = milliseconds / 1000ll;
	__int64 days = totalSeconds / (24ll * 60ll * 60ll);
	__int64 hours = (totalSeconds % (24ll * 60ll * 60ll)) / (60ll * 60ll);
	__int64 minutes = (totalSeconds % (60ll * 60ll)) / 60ll;
	__int64 seconds = totalSeconds % 60ll;
	__int64 remainingMilliseconds = milliseconds % 1000ll;

	if (!noSpace)
	{
		return QString(QObject::tr("%1 day %2 hour %3 min %4 sec %5 msec"))
			.arg(days).arg(hours).arg(minutes).arg(seconds).arg(remainingMilliseconds);
	}
	else
	{
		return QString(QObject::tr("%1d%2h%3m%4s"))
			.arg(days).arg(hours).arg(minutes).arg(seconds);
	}
}

export QString formatSeconds(__int64 seconds)
{
	__int64 day = seconds / 86400ll;
	__int64 hours = (seconds % 86400ll) / 3600ll;
	__int64 minutes = (seconds % 3600ll) / 60ll;
	__int64 remainingSeconds = seconds % 60ll;

	return QString(QObject::tr("%1 day %2 hour %3 min %4 sec")).arg(day).arg(hours).arg(minutes).arg(remainingSeconds);
};

// 將二進制數據轉換為16進制字符串
export QString byteArrayToHexString(const QByteArray& data)
{
	QString hexString;
	for (char byte : data)
	{
		hexString.append(QString("%1").arg(static_cast<unsigned char>(byte), 2, 16, QChar('0')));
	}
	return hexString;
}

// 將16進制字符串轉換為二進制數據
export QByteArray hexStringToByteArray(const QString& hexString)
{
	QByteArray byteArray;
	for (int i = 0; i < hexString.length(); i += 2)
	{
		bool ok;
		QString byteString = hexString.mid(i, 2);
		char byte = static_cast<char>(byteString.toInt(&ok, 16));
		if (!ok)
		{
			// 處理轉換失敗的情況
			return QByteArray();
		}
		byteArray.append(byte);
	}
	return byteArray;
}

export class TextStream : public QTextStream
{
public:
	explicit TextStream(FILE* file)
		: QTextStream(file)
	{
		init();
	}

	explicit TextStream(QIODevice* device)
		: QTextStream(device)
	{
		init();
	}

private:
	void init()
	{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
		setCodec(SASH_DEFAULT_CODEPAGE);
#else
		setEncoding(QStringConverter::Utf8);
#endif
		setGenerateByteOrderMark(true);

		setAutoDetectUnicode(true);
}
};
