#include "domain/logger.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QTextStream>

Logger::Logger(QString directory, const IClock *clock)
    : m_directory(std::move(directory))
    , m_clock(clock)
{
}

QString Logger::directory() const
{
    return m_directory;
}

QString Logger::currentLogPath() const
{
    return logPathFor(m_clock ? m_clock->now() : QDateTime::currentDateTime());
}

QString Logger::logPathFor(const QDateTime &time) const
{
    return QDir(m_directory).filePath(logFileNameFor(time));
}

QString Logger::logFileNameFor(const QDateTime &time)
{
    return time.toString(QStringLiteral("yyyy-MM-dd")) + QStringLiteral(".log");
}

void Logger::log(const QString &message)
{
    if (!QDir().mkpath(m_directory)) {
        return;
    }
    QFile file(currentLogPath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        return;
    }
    const QDateTime now = m_clock ? m_clock->now() : QDateTime::currentDateTime();
    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    stream << QLatin1Char('[') << now.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")) << QLatin1String("] ")
           << message << QLatin1Char('\n');
}

void Logger::pruneOldLogs(int keep)
{
    if (keep < 1) {
        keep = 1;
    }
    QDir dir(m_directory);
    if (!dir.exists()) {
        return;
    }
    QStringList files = dir.entryList({QStringLiteral("*.log")}, QDir::Files, QDir::Name);
    while (files.size() > keep) {
        const QString oldest = files.takeFirst();
        QFile::remove(dir.filePath(oldest));
    }
}
