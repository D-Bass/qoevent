#ifndef DISPLAYTIME_H
#define DISPLAYTIME_H
#include <QCoreApplication>

struct DisplayTime
{
  static QString hour_min_sec(const uint32_t val)
  {
    return QString("%1:%2:%3")
        .arg(val/3600,2,10,QChar('0'))
        .arg(val%3600/60,2,10,QChar('0'))
        .arg(val%3600%60,2,10,QChar('0'));
  }
  static QString min_sec(const uint32_t val)
  {
    return QString("%1:%2")
        .arg(val/60,2,10,QChar('0'))
        .arg(val%60,2,10,QChar('0'));
  }
};

#endif // DISPLAYTIME_H
