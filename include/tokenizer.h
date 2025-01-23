#ifndef TOKENIZER_H_
#define TOKENIZER_H_

#include <QString>
#include <QStringView>
#include <QChar>

class Tokenizer
{
public:
    Tokenizer(QStringView str, QChar delimiter);

    Tokenizer(QStringView str, QStringView delimiter);

    virtual ~Tokenizer() = default;

    QString string(qint64 index) const;

    qint64 integer(qint64 index, qint64 base = 10) const;

    qint32 integer32(qint64 index, qint64 base = 10) const;

    bool isEnd(qint64 index) const;

    qint64 size() const;

private:
    QString tokenize(qint64 index) const;

private:
    QStringView sourceStr_;
    QChar delimiter_;
    mutable qint64 sizeCache_ = -1; // 缓存大小结果
};


#endif // TOKENIZER