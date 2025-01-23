#include "stdafx.h"
#include <ansicolorizer.h>

namespace ansi
{
    constexpr const char* RESET = "\033[0m"; // 重置
    constexpr const char* RED = "\033[31m"; // 紅色
    constexpr const char* GREEN = "\033[32m"; // 綠色
    constexpr const char* YELLOW = "\033[33m"; // 黃色
    constexpr const char* BLUE = "\033[34m"; // 藍色
    constexpr const char* MAGENTA = "\033[35m"; // 紫色
    constexpr const char* CYAN = "\033[36m"; // 青色
    constexpr const char* WHITE = "\033[37m"; // 白色
    constexpr const char* LIGHT_BLACK = "\033[90m"; // 亮黑（灰）
    constexpr const char* LIGHT_RED = "\033[91m"; // 亮紅
    constexpr const char* LIGHT_GREEN = "\033[92m"; // 亮綠
    constexpr const char* LIGHT_YELLOW = "\033[93m"; // 亮黃
    constexpr const char* LIGHT_BLUE = "\033[94m"; // 亮藍
    constexpr const char* LIGHT_MAGENTA = "\033[95m"; // 亮品紅
    constexpr const char* LIGHT_CYAN = "\033[96m"; // 亮青
    constexpr const char* LIGHT_WHITE = "\033[97m"; // 亮白
    constexpr const char* BOLD = "\033[1m"; // 粗體

    // 高亮數字
    static const QStringList preHightlightNumberKeys = {
        "\"pid\"", "\"commandId\"", "\"timestamp\"", "\"processId\"",
        "\"type\"", "\"uid\"", "\"code\"", "\"count\"", "\"sqltime\"", "\"groupId\"", "\"categoryId\"",
        "\"level\"", "\"imageId\"", "\"tradeCredits\"", "\"adminId\"", "\"credits\"", "\"status\"", "\"groupExpirationTime\"",
        "\"integer0\"", "\"integer1\"", "\"integer2\"", "\"integer3\"", "\"integer4\"", "\"integer5\"",
        "\"integer6\"", "\"integer7\"", "\"integer8\"", "\"integer9\"", "\"integer10\"", "\"integer11\"",
        "\"integer12\"", "\"integer13\"", "\"integer14\"", "\"integer15\""
    };

    // 高亮鍵名
    static const QStringList preHighlightkeys = {
         "\"action\"", "\"data\"", "\"status\"", "\"timestamp\"", "\"subAction\"", "\"conversationId\"",
         "\"command\"", "\"commandId\"", "\"message\"", "\"processId\"", "\"username\"","\"phone\"", "\"qq\"", "\"connectionName\"",
         "\"parameters\"", "\"field\"", "\"fields\"", "\"tableName\"", "\"conditions\"", "\"type\"", "\"notice\"", "\"groupExpirationTime\"",
         "\"source\"", "\"destination\"", "\"uid\"", "\"code\"", "\"count\"", "\"sqltime\"", "\"tradeCredits\"", "\"password\"",
         "\"categoryId\"", "\"categoryName\"", "\"credits\"", "\"isFromCache\"", "\"expirationDate\"", "\"groupId\"", "\"groupName\"",
         "\"results\"", "\"hash\"", "\"imageId\"", "\"name\"", "\"level\"", "\"memo\"", "\"adminId\"", "\"cardKey\"", "\"version\"",
         "\"updateUrl\"", "\"email\"", "\"wechat\"",
         "\"string0\"", "\"string1\"", "\"string2\"", "\"string3\"", "\"string4\"", "\"string5\"",
         "\"string6\"", "\"string7\"", "\"string8\"", "\"string9\"", "\"string10\"", "\"string11\"",
         "\"string12\"", "\"string13\"", "\"string14\"", "\"string15\"",
         "\"integer0\"", "\"integer1\"", "\"integer2\"", "\"integer3\"", "\"integer4\"", "\"integer5\"",
         "\"integer6\"", "\"integer7\"", "\"integer8\"", "\"integer9\"", "\"integer10\"", "\"integer11\"",
         "\"integer12\"", "\"integer13\"", "\"integer14\"", "\"integer15\""
    };
}

ansi::Colorizer::Colorizer()
{
    // 設置UTF-8編碼
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

#ifndef DISABLE_DEBUG_CONSOLE
    enableANSIColors();

    qInstallMessageHandler(qtMessageHandler);
#endif

    // 清空控制台
    tool::clrscr();
}

ansi::Colorizer::~Colorizer()
{
}

void ansi::Colorizer::qtMessageHandler(QtMsgType type, const QMessageLogContext&, const QString& msg)
{
    if (type == QtFatalMsg)
    {
        Q_ASSERT_X(false, "qtMessageHandler", qPrintable(msg));
        return;
    }

    if (msg.contains("32400000"))
    {
        qCritical().noquote();
    }

    ansi::Colorizer& colorizer = ansi::Colorizer::instance();

    QString logMessage;
    QString logType;

    if (msg.contains("Commited:"))
    {
        type = QtInfoMsg;
    }

    switch (type)
    {
    case QtDebugMsg:
    {
        logType = QString(u8"%1🟢 Debug   %2").arg(ansi::GREEN).arg(ansi::RESET);
        break;
    }
    case QtInfoMsg:
    {
        logType = QString(u8"%1ℹ️ Info    %2").arg(ansi::CYAN).arg(ansi::RESET);
        break;
    }
    case QtWarningMsg:
    {
        logType = QString(u8"%1⚠️ Warning %2").arg(ansi::YELLOW).arg(ansi::RESET);
        break;
    }
    case QtCriticalMsg:
    {
        logType = QString(u8"%1⛔ Critical%2").arg(ansi::RED).arg(ansi::RESET);
        break;
    }
    case QtFatalMsg:
    {
        logType = QString(u8"%1🚫 Fatal   %2").arg(ansi::LIGHT_RED).arg(ansi::RESET);
        break;
    }
    }

    QString time = QString("%1%2%3").arg(ansi::GREEN).arg(QTime::currentTime().toString("hh:mm:ss.zzz")).arg(ansi::RESET);

    QString newMessage = msg;

    if (QtWarningMsg == type)
    {
        logMessage = QString("%1 | %2 | %3")
            .arg(time)
            .arg(logType)
            .arg(newMessage);

        QStringList logMessages = logMessage.split("\n");
        for (const QString& message : logMessages)
        {
            QString trimmedMessage = message.trimmed();
            QTextStream(stdout) << trimmedMessage << Qt::endl;
        }

        return;
    }

    newMessage.replace("<", QString("%1<%2").arg(ansi::YELLOW).arg(ansi::RESET));
    newMessage.replace(">", QString("%1>%2").arg(ansi::YELLOW).arg(ansi::RESET));

    if (msg.contains("Commited:"))
    {
        newMessage.replace("Commited:", QString("%1# Commited%2:").arg(ansi::LIGHT_MAGENTA).arg(ansi::RESET));
    }



    // 黃色高亮大括號
    newMessage.replace("{", QString("%1{ %2").arg(ansi::YELLOW).arg(ansi::RESET));
    newMessage.replace("}", QString("%1 }%2").arg(ansi::YELLOW).arg(ansi::RESET));

    for (const QString& key : preHightlightNumberKeys)
    {
        // 或 浮點數
        const QRegularExpression reFloat(key + ":\\s*(\\d+\\.\\d+)");
        auto matchFloat = reFloat.match(newMessage);
        if (matchFloat.hasMatch())
        {
            newMessage.replace(reFloat, QString("%1: %2%3%4").arg(key).arg(ansi::GREEN).arg(matchFloat.captured(1)).arg(ansi::RESET));
            continue;
        }

        // 處理數字 或負數
        const QRegularExpression reNum(key + ":\\s*(-?\\d+)");
        auto match = reNum.match(newMessage);
        if (match.hasMatch())
        {
            newMessage.replace(reNum, QString("%1: %2%3%4").arg(key).arg(ansi::GREEN).arg(match.captured(1)).arg(ansi::RESET));
        }

        const QRegularExpression rePlaceHoldNum("#####(\\d+)#####");
        auto matchPlaceHoldNum = rePlaceHoldNum.match(newMessage);
        if (matchPlaceHoldNum.hasMatch())
        {
            newMessage.replace(rePlaceHoldNum, QString("%1%2%3").arg(ansi::GREEN).arg(matchPlaceHoldNum.captured(1)).arg(ansi::RESET));
        }

        const QRegularExpression reHoldNum("::(\\d+)");
        auto matchHoldNum = reHoldNum.match(newMessage);
        if (matchHoldNum.hasMatch())
        {
            newMessage.replace(reHoldNum, QString("::%1%2%3").arg(ansi::GREEN).arg(matchHoldNum.captured(1)).arg(ansi::RESET));
        }
    }

    // 高亮 true 和 false
    newMessage.replace("true", QString("%1true%2").arg(ansi::GREEN).arg(ansi::RESET));
    newMessage.replace("false", QString("%1false%2").arg(ansi::RED).arg(ansi::RESET));

    // 高亮字符串值
    static const QRegularExpression reCommandStr("\"command\":\\s*\"(.+?)\"");
    auto matchCommandStr = reCommandStr.match(newMessage);
    if (matchCommandStr.hasMatch())
    {
        newMessage.replace(reCommandStr, QString("\"command\": %1\"%2\"%3").arg(ansi::YELLOW).arg(matchCommandStr.captured(1)).arg(ansi::RESET));
    }

    static const QRegularExpression reMessageStr("\"message\":\\s*\"(.+?)\"");
    auto matchMessageStr = reMessageStr.match(newMessage);
    if (matchMessageStr.hasMatch())
    {
        newMessage.replace(reMessageStr, QString("\"message\": %1\"%2\"%3").arg(ansi::YELLOW).arg(matchMessageStr.captured(1)).arg(ansi::RESET));
    }


    for (const QString& key : preHighlightkeys)
    {
        newMessage.replace(key, QString("%1%2%3").arg(ansi::CYAN).arg(key).arg(ansi::RESET));
    }

    // 高亮所有字符串值
    static const QRegularExpression reStringValue(":\\s*\"([^\"]+)\"");
    newMessage.replace(reStringValue, QString(": %1\"\\1\"%2").arg(ansi::YELLOW).arg(ansi::RESET));

    // 高亮轉義字符
    bool wasYellow = false;
    newMessage = colorizer.highlightEscapeSequences(newMessage, wasYellow);

    // 高亮分隔符
    newMessage = colorizer.replaceSeparators(newMessage, ":", ansi::RED, wasYellow);
    newMessage = colorizer.replaceSeparators(newMessage, ",", ansi::RED, wasYellow);
    newMessage = colorizer.replaceSeparators(newMessage, "|", ansi::RED, wasYellow);

    logMessage = QString("%1 %4 %2 %4 %3")
        .arg(time)
        .arg(logType)
        .arg(newMessage)
        .arg(QString("%1|%2").arg(ansi::RED).arg(ansi::RESET));

    QStringList logMessages = logMessage.split("\n");
    for (const QString& message : logMessages)
    {
        QString trimmedMessage = message.trimmed();
        //OutputDebugStringW((trimmedMessage + "\n").toStdWString().c_str());
        QTextStream(stdout) << trimmedMessage << Qt::endl;
    }
}

bool ansi::Colorizer::enableANSIColors()
{
    // 獲取標準輸出句柄
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Error: Unable to get console handle" << std::endl;
        return false;
    }

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode))
    {
        std::cerr << "Error: Unable to get console mode" << std::endl;
        return false;
    }

    if (dwMode & ENABLE_VIRTUAL_TERMINAL_PROCESSING)
    {
        return true;
    }

    // 啟用 ANSI 轉義序列支持
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hOut, dwMode)) {
        std::cerr << "Error: Unable to set console mode" << std::endl;
        return false;
    }

    return true;
}

QString ansi::Colorizer::highlightEscapeSequences(const QString& input, bool& wasYellow)
{
    QString output;
    for (qint64 i = 0; i < input.size(); ++i)
    {
        if (input[i] == '\\')
        {
            output += ansi::CYAN;
            while (i < input.size() && input[i] == '\\')
            {
                output += input[i];
                ++i;
            }
            if (i < input.size())
            {
                output += input[i];
            }
            output += ansi::RESET;
            output += wasYellow ? ansi::YELLOW : ansi::WHITE;
        }
        else
        {
            output += input[i];
            if (input[i] == '"')
            {
                wasYellow = !wasYellow;
            }
        }
    }
    return output;
}

QString ansi::Colorizer::replaceSeparators(const QString& input, const QString& separator, const QString& color, bool& wasYellow)
{
    QString output;
    for (qint64 i = 0; i < input.size(); ++i)
    {
        if (input.mid(i, separator.size()) == separator)
        {
            output += color + separator + ansi::RESET;
            i += separator.size() - 1;
            output += wasYellow ? ansi::YELLOW : ansi::WHITE;
        }
        else
        {
            output += input[i];
            if (input[i] == '"')
            {
                wasYellow = !wasYellow;
            }
        }
    }
    return output;
}

QString ansi::Colorizer::formatLine(const QString& label, const QString& value)
{
    QString line = "* " + label + QString("%1%2%3").arg(ansi::CYAN).arg(value).arg(ansi::RESET) + " ";
    qint64 spacesToAdd = 59 - line.length() - 1; // 50 total length, minus 1 for the trailing '*'
    line += QString(spacesToAdd, ' ') + "*";
    return line;
}