#include "platform/win/shell_launcher.h"

#include "platform/win/win_utils.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>

#include <string>

namespace {

bool isCmdProgram(const QString &program)
{
    return QFileInfo(program).fileName().compare(QStringLiteral("cmd.exe"),
                                                 Qt::CaseInsensitive) == 0;
}

// QProcess::startDetached 在 GUI 进程中会带上 CREATE_NO_WINDOW，导致 cmd 没有可见窗口。
// 命令行条目需要自己的控制台窗口，因此直接用 CreateProcessW 并指定 CREATE_NEW_CONSOLE。
bool startConsoleCommand(const QStringList &arguments, const QString &workingDirectory,
                         QString *error)
{
    if (arguments.size() < 2) {
        if (error) {
            *error = QStringLiteral("命令行参数不完整");
        }
        return false;
    }
    const QString flag = arguments.at(0);
    if (flag != QLatin1String("/c") && flag != QLatin1String("/k")) {
        if (error) {
            *error = QStringLiteral("不支持的命令行模式：%1").arg(flag);
        }
        return false;
    }

    QString commandProcessor =
        QProcessEnvironment::systemEnvironment().value(QStringLiteral("ComSpec"));
    if (commandProcessor.isEmpty()) {
        commandProcessor = QStringLiteral("cmd.exe");
    }

    const QString commandLine = QStringLiteral("\"%1\" %2 %3")
                                    .arg(QDir::toNativeSeparators(commandProcessor),
                                         flag,
                                         arguments.at(1));
    std::wstring nativeCommandLine = commandLine.toStdWString();
    const std::wstring nativeWorkingDir = workingDirectory.toStdWString();

    STARTUPINFOW startupInfo = {};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo = {};

    const BOOL ok = CreateProcessW(
        nullptr,
        &nativeCommandLine[0],
        nullptr,
        nullptr,
        FALSE,
        CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT,
        nullptr,
        nativeWorkingDir.empty() ? nullptr : nativeWorkingDir.c_str(),
        &startupInfo,
        &processInfo);
    if (!ok) {
        if (error) {
            *error = QStringLiteral("无法启动命令行：%1").arg(winErrorMessage(GetLastError()));
        }
        return false;
    }

    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
    return true;
}

} // namespace

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

    if (isCmdProgram(program) && !arguments.isEmpty()) {
        return startConsoleCommand(arguments, workingDirectory, error);
    }

    if (QProcess::startDetached(program, arguments, workingDirectory)) {
        return true;
    }
    if (error) {
        *error = QStringLiteral("无法启动：%1").arg(program);
    }
    return false;
}
