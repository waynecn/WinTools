#ifndef WINTOOLS_DOMAIN_LOGGER_H
#define WINTOOLS_DOMAIN_LOGGER_H

#include "domain/interfaces.h"

#include <QDateTime>
#include <QString>

class SystemClock : public IClock
{
public:
    QDateTime now() const override { return QDateTime::currentDateTime(); }
};

class Logger
{
public:
    Logger(QString directory, const IClock *clock);

    QString directory() const;
    QString currentLogPath() const;
    QString logPathFor(const QDateTime &time) const;

    void log(const QString &message);
    void pruneOldLogs(int keep = 30);

    static QString logFileNameFor(const QDateTime &time);

private:
    QString m_directory;
    const IClock *m_clock;
};

#endif // WINTOOLS_DOMAIN_LOGGER_H
