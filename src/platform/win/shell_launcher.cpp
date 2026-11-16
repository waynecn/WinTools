#include "platform/win/shell_launcher.h"

#include "platform/win/win_utils.h"

#include <QProcess>

bool ShellLauncher::shellOpen(const QString &target, QString *error)
{
    return shellOpenTarget(target, error);
}

bool ShellLauncher::startDetached(const QString &program, const QStringList &arguments,
                                  const QString &workingDirectory, QString *error)
{
    if (error) {
        error->clear();
    }
    if (program.trimmed().isEmpty()) {
        if (error) {
            *error = QStringLiteral("启动程序为空");
        }
        return false;
    }
    if (QProcess::startDetached(program, arguments, workingDirectory)) {
        return true;
    }
    if (error) {
        *error = QStringLiteral("无法启动：%1").arg(program);
    }
    return false;
}
