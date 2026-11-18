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

// 命令行条目的窗口策略：
// - cmd /k（保留窗口）：创建可见的新控制台；
// - cmd /c（不保留）：后台执行，不显示控制台。
//   git-bash.exe 这类常驻启动器会让 cmd 一直等待，如果创建可见控制台就会留下空黑窗。
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
    const bool keepWindow = flag == QLatin1String("/k");

    QString commandProcessor =
        QProcessEnvironment::systemEnvironment().value(QStringLiteral("ComSpec"));
    if (commandProcessor.isEmpty()) {
        commandProcessor = QStringLiteral("cmd.exe");
    }

    // 用 /d 跳过 AutoRun，/s 强制按“去掉首尾引号”的规则解析；
    // 再把用户命令行整体包一层引号，避免 cmd 吃掉命令内部的引号
    // （例如 "C:\Program Files\app.exe" --arg）。
    const QString commandLine = QStringLiteral("\"%1\" /d /s %2 \"%3\"")
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
        (keepWindow ? CREATE_NEW_CONSOLE : CREATE_NO_WINDOW) | CREATE_UNICODE_ENVIRONMENT,
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
