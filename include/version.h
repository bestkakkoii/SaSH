#ifndef VERSION_H_
#define VERSION_H_

#include <QString>
#include <QDateTime>

class Version
{
public:
    static QString generate();

private:
    // 主版本號、次版本號、補丁號
    // 主版本號：當有重大功能更新或改變時增加
    static constexpr qint64 majorVersion = 4;

    // 次版本號：當有新功能增加，但不影響舊功能時增加
    static constexpr qint64 minorVersion = 0;

    // 補丁號：當有錯誤修正或小幅改進時增加
    static constexpr qint64 patchVersion = 0;

    // 獲取編譯時間的字符串
    static constexpr const char* compileDate = __DATE__;
    static constexpr const char* compileTime = __TIME__;

    // 將日期和時間轉換為 ISO 格式
    static QString generateBuildNumber();

    // 將版本號和編譯時間組合成最終版本號
    static QString version_;

private:
    Q_DISABLE_COPY_MOVE(Version);
};

#endif // VERSION_H_
