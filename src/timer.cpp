#include "stdafx.h"
#include <timer.h>

Timer::Timer()
{
    timer_.start();
}

Timer::~Timer()
{

}

void Timer::restart()
{
    timer_.restart();
}

qint64 Timer::elapsed() const
{
    return timer_.elapsed();
}

bool Timer::hasExpired(qint64 timeout) const
{
    return timer_.hasExpired(timeout);
}