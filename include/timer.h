#ifndef TIMER_H_
#define TIMER_H_

#include <QElapsedTimer>

class Timer
{
public:
    Timer();
    virtual ~Timer();

    void restart();

    qint64 elapsed() const;

    bool hasExpired(qint64 timeout) const;

private:
    QElapsedTimer timer_;
};

#endif // TIMER_H_