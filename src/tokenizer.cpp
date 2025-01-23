#include "stdafx.h"
#include <tokenizer.h>

Tokenizer::Tokenizer(QStringView str, QChar delimiter)
    : sourceStr_(str)
    , delimiter_(delimiter)
{
}

Tokenizer::Tokenizer(QStringView str, QStringView delimiter)
    : sourceStr_(str)
    , delimiter_(delimiter.at(0))
{
}

QString Tokenizer::string(qint64 index) const
{
    QString result = tokenize(index);
    return result;
}

qint64 Tokenizer::integer(qint64 index, qint64 base) const
{
    bool ok = false;
    qint64 value = string(index).toLongLong(&ok, base);

    return ok ? value : -1;
}

qint32 Tokenizer::integer32(qint64 index, qint64 base) const
{
    bool ok = false;
    qint32 value = string(index).toInt(&ok, base);

    return ok ? value : -1;
}

bool Tokenizer::isEnd(qint64 index) const
{
    return (index + 1) >= size();
}

qint64 Tokenizer::size() const
{
    if (-1 == sizeCache_ || sourceStr_.size() <= 0)
    {
        sizeCache_ = sourceStr_.count(delimiter_) + 1;
    }

    return sizeCache_;
}

QString Tokenizer::tokenize(qint64 index) const
{
    QString src = sourceStr_.toString();
    return src.section(delimiter_, index, index);
}