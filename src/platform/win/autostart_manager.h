#ifndef WINTOOLS_PLATFORM_AUTOSTART_MANAGER_H
#define WINTOOLS_PLATFORM_AUTOSTART_MANAGER_H

#include <QString>
#include <QStringList>

class AutostartManager
{
public:
    struct State
    {
        bool runKey = false;
        bool task = false;

        bool any() const { return runKey || task; }
    };

    explicit AutostartManager(QString executablePath);

    QString executablePath() const;
    State query() const;

    // mode: "normal" 或 "admin"。
    bool apply(bool enabled, const QString &mode, QString *error);

    static QString taskName();

private:
    bool taskExists() const;
    bool createTask(QString *error);
    bool deleteTask(QString *error);
    void writeRunKey(bool enabled);
    QString buildTaskXml() const;
    bool runSchtasks(const QStringList &arguments, int *exitCode, QString *error) const;

    QString m_executablePath;
};

#endif // WINTOOLS_PLATFORM_AUTOSTART_MANAGER_H
