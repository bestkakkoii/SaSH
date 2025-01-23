#ifndef ANSICOLORIZER_H_
#define ANSICOLORIZER_H_

#include <QString>
#include <QTextStream>
#include <QCoreApplication>
#include <QDateTime>

namespace ansi
{
    class Colorizer
    {
    public:
        static Colorizer& instance()
        {
            static Colorizer instance;
            return instance;
        }

        QString formatLine(const QString& label, const QString& value);

    private:
        Colorizer();
        virtual ~Colorizer();

        static void qtMessageHandler(QtMsgType type, const QMessageLogContext&, const QString& msg);

    private:
        bool enableANSIColors();
        QString highlightEscapeSequences(const QString& input, bool& wasYellow);
        QString replaceSeparators(const QString& input, const QString& separator, const QString& color, bool& wasYellow);


    private:
        Q_DISABLE_COPY_MOVE(Colorizer);
    };
}


#endif // ANSICOLORIZER_H_