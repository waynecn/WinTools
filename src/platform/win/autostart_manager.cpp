#include "platform/win/autostart_manager.h"

#include "platform/win/win_utils.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>

namespace {

const QString kRunKeyPath =
    QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run");
const QString kRunValueName = QStringLiteral("WinTools");

QString xmlEscape(QString value)
{
    value.replace(QLatin1Char('&'), QStringLiteral("&amp;"));
    value.replace(QLatin1Char('<'), QStringLiteral("&lt;"));
    value.replace(QLatin1Char('>'), QStringLiteral("&gt;"));
    value.replace(QLatin1Char('"'), QStringLiteral("&quot;"));
    value.replace(QLatin1Char('\''), QStringLiteral("&apos;"));
    return value;
}

QString quoteParameter(const QString &value)
{
    if (!value.contains(QLatin1Char(' '))) {
        return value;
    }
    QString escaped = value;
    escaped.replace(QLatin1Char('"'), QStringLiteral("\\\""));
    return QLatin1Char('"') + escaped + QLatin1Char('"');
}

} // namespace

AutostartManager::AutostartManager(QString executablePath)
    : m_executablePath(std::move(executablePath))
{
}

QString AutostartManager::executablePath() const
{
    return m_executablePath;
}

QString AutostartManager::taskName()
{
    return QStringLiteral("WinToolsLauncher");
}

AutostartManager::State AutostartManager::query() const
{
    State state;
    const QSettings settings(kRunKeyPath, QSettings::NativeFormat);
    state.runKey = settings.contains(kRunValueName);
    state.task = taskExists();
    return state;
}

bool AutostartManager::apply(bool enabled, const QString &mode, QString *error)
{
    if (error) {
        error->clear();
    }

    if (!enabled) {
        writeRunKey(false);
        if (taskExists() && !deleteTask(error)) {
            return false;
        }
        return true;
    }

    if (mode == QLatin1String("admin")) {
        writeRunKey(false);
        if (taskExists()) {
            return true;
        }
        return createTask(error);
    }

    if (taskExists() && !deleteTask(error)) {
        return false;
    }
    writeRunKey(true);
    return true;
}

bool AutostartManager::taskExists() const
{
    int exitCode = -1;
    QString error;
    if (!runProcessHidden(QStringLiteral("schtasks.exe"),
                          {QStringLiteral("/Query"), QStringLiteral("/TN"), taskName()},
                          &exitCode, &error)) {
        return false;
    }
    return exitCode == 0;
}

bool AutostartManager::createTask(QString *error)
{
    const QString xml = buildTaskXml();
    const QString tempPath = QDir::temp().filePath(
        QStringLiteral("wintools-task-%1.xml").arg(QCoreApplication::applicationPid()));

    QFile file(tempPath);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error) {
            *error = QStringLiteral("无法写入计划任务 XML：%1").arg(file.errorString());
        }
        return false;
    }
    QByteArray data;
    data.append(static_cast<char>(0xFF));
    data.append(static_cast<char>(0xFE));
    const ushort *units = reinterpret_cast<const ushort *>(xml.utf16());
    data.append(reinterpret_cast<const char *>(units), xml.size() * 2);
    file.write(data);
    file.close();

    int exitCode = -1;
    const bool ran = runSchtasks({QStringLiteral("/Create"),
                                  QStringLiteral("/TN"), taskName(),
                                  QStringLiteral("/XML"), tempPath,
                                  QStringLiteral("/F")},
                                 &exitCode, error);
    QFile::remove(tempPath);

    if (!ran) {
        return false;
    }
    if (exitCode != 0) {
        if (error) {
            *error = QStringLiteral("创建计划任务失败（schtasks 退出码 %1）").arg(exitCode);
        }
        return false;
    }
    return true;
}

bool AutostartManager::deleteTask(QString *error)
{
    int exitCode = -1;
    const bool ran = runSchtasks({QStringLiteral("/Delete"),
                                  QStringLiteral("/TN"), taskName(),
                                  QStringLiteral("/F")},
                                 &exitCode, error);
    if (!ran) {
        return false;
    }
    if (exitCode != 0) {
        if (error) {
            *error = QStringLiteral("删除计划任务失败（schtasks 退出码 %1）").arg(exitCode);
        }
        return false;
    }
    return true;
}

void AutostartManager::writeRunKey(bool enabled)
{
    QSettings settings(kRunKeyPath, QSettings::NativeFormat);
    if (enabled) {
        const QString command =
            QStringLiteral("\"%1\" --autostart").arg(QDir::toNativeSeparators(m_executablePath));
        settings.setValue(kRunValueName, command);
    } else {
        settings.remove(kRunValueName);
    }
    settings.sync();
}

QString AutostartManager::buildTaskXml() const
{
    const QString executable = xmlEscape(QDir::toNativeSeparators(m_executablePath));
    const QString workingDir = xmlEscape(
        QDir::toNativeSeparators(QFileInfo(m_executablePath).absolutePath()));

    return QStringLiteral(
               "<?xml version=\"1.0\" encoding=\"UTF-16\"?>\n"
               "<Task version=\"1.2\" xmlns=\"http://schemas.microsoft.com/windows/2004/02/mit/task\">\n"
               "  <RegistrationInfo><Description>WinTools Launcher autostart</Description></RegistrationInfo>\n"
               "  <Triggers><LogonTrigger><Enabled>true</Enabled></LogonTrigger></Triggers>\n"
               "  <Principals><Principal id=\"Author\"><LogonType>InteractiveToken</LogonType>"
               "<RunLevel>HighestAvailable</RunLevel></Principal></Principals>\n"
               "  <Settings><MultipleInstancesPolicy>IgnoreNew</MultipleInstancesPolicy>"
               "<DisallowStartIfOnBatteries>false</DisallowStartIfOnBatteries>"
               "<StopIfGoingOnBatteries>false</StopIfGoingOnBatteries>"
               "<AllowHardTerminate>true</AllowHardTerminate>"
               "<StartWhenAvailable>false</StartWhenAvailable>"
               "<RunOnlyIfNetworkAvailable>false</RunOnlyIfNetworkAvailable>"
               "<IdleSettings><StopOnIdleEnd>false</StopOnIdleEnd>"
               "<RestartOnIdle>false</RestartOnIdle></IdleSettings>"
               "<AllowStartOnDemand>true</AllowStartOnDemand><Enabled>true</Enabled>"
               "<Hidden>false</Hidden><RunOnlyIfIdle>false</RunOnlyIfIdle>"
               "<WakeToRun>false</WakeToRun><ExecutionTimeLimit>PT0S</ExecutionTimeLimit>"
               "<Priority>7</Priority></Settings>\n"
               "  <Actions Context=\"Author\"><Exec><Command>%1</Command>"
               "<Arguments>--autostart</Arguments><WorkingDirectory>%2</WorkingDirectory></Exec></Actions>\n"
               "</Task>\n")
        .arg(executable, workingDir);
}

bool AutostartManager::runSchtasks(const QStringList &arguments, int *exitCode,
                                   QString *error) const
{
    if (currentProcessElevated()) {
        return runProcessHidden(QStringLiteral("schtasks.exe"), arguments, exitCode, error);
    }

    QString parameters;
    for (const QString &argument : arguments) {
        if (!parameters.isEmpty()) {
            parameters += QLatin1Char(' ');
        }
        parameters += quoteParameter(argument);
    }
    return shellRunElevatedWait(QStringLiteral("schtasks.exe"), parameters, exitCode, error);
}
