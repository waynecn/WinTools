#include "platform/win/win_utils.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>

#include <QDir>
#include <QProcess>

QString winErrorMessage(unsigned long code)
{
    switch (code) {
    case 2:
        return QStringLiteral("找不到文件");
    case 3:
        return QStringLiteral("找不到路径");
    case 5:
        return QStringLiteral("拒绝访问");
    case 26:
        return QStringLiteral("无法共享文件");
    case 27:
        return QStringLiteral("文件关联不完整");
    case 28:
        return QStringLiteral("DDE 操作超时");
    case 29:
        return QStringLiteral("DDE 操作失败");
    case 31:
        return QStringLiteral("没有关联的应用程序");
    case 32:
        return QStringLiteral("找不到所需的 DLL");
    case 1223:
        return QStringLiteral("用户取消了操作");
    default:
        break;
    }

    LPWSTR buffer = nullptr;
    const DWORD length = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
            | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, static_cast<DWORD>(code), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);

    QString message;
    if (buffer && length > 0) {
        message = QString::fromWCharArray(buffer, static_cast<int>(length)).trimmed();
        LocalFree(buffer);
    }
    if (message.isEmpty()) {
        message = QStringLiteral("错误码 %1").arg(code);
    }
    return message;
}

bool shellOpenTarget(const QString &target, QString *error)
{
    if (error) {
        error->clear();
    }
    if (target.trimmed().isEmpty()) {
        if (error) {
            *error = QStringLiteral("目标为空");
        }
        return false;
    }

    const QString file = target.contains(QLatin1String("://"))
        ? target
        : QDir::toNativeSeparators(target);
    const std::wstring nativeFile = file.toStdWString();

    SHELLEXECUTEINFOW info = {};
    info.cbSize = sizeof(info);
    info.fMask = SEE_MASK_NOASYNC | SEE_MASK_FLAG_NO_UI;
    info.lpVerb = L"open";
    info.lpFile = nativeFile.c_str();
    info.nShow = SW_SHOWNORMAL;

    if (!ShellExecuteExW(&info)) {
        const DWORD code = GetLastError();
        if (error) {
            *error = winErrorMessage(code);
        }
        return false;
    }
    return true;
}

bool shellRunElevatedWait(const QString &file, const QString &parameters,
                          int *exitCode, QString *error)
{
    if (error) {
        error->clear();
    }
    const std::wstring nativeFile = QDir::toNativeSeparators(file).toStdWString();
    const std::wstring nativeParams = parameters.toStdWString();

    SHELLEXECUTEINFOW info = {};
    info.cbSize = sizeof(info);
    info.fMask = SEE_MASK_NOASYNC | SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
    info.lpVerb = L"runas";
    info.lpFile = nativeFile.c_str();
    info.lpParameters = nativeParams.empty() ? nullptr : nativeParams.c_str();
    info.nShow = SW_HIDE;

    if (!ShellExecuteExW(&info)) {
        const DWORD code = GetLastError();
        if (error) {
            *error = winErrorMessage(code);
        }
        return false;
    }
    if (!info.hProcess) {
        if (error) {
            *error = QStringLiteral("未能获取提权进程句柄");
        }
        return false;
    }

    bool ok = true;
    const DWORD waitResult = WaitForSingleObject(info.hProcess, 60000);
    if (waitResult == WAIT_TIMEOUT) {
        if (error) {
            *error = QStringLiteral("等待提权命令超时");
        }
        ok = false;
    } else {
        DWORD code = 0;
        if (GetExitCodeProcess(info.hProcess, &code)) {
            if (exitCode) {
                *exitCode = static_cast<int>(code);
            }
        } else if (error) {
            *error = QStringLiteral("无法获取提权命令退出码");
            ok = false;
        }
    }
    CloseHandle(info.hProcess);
    return ok;
}

bool currentProcessElevated()
{
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        return false;
    }
    TOKEN_ELEVATION elevation = {};
    DWORD returned = 0;
    const BOOL ok = GetTokenInformation(token, TokenElevation, &elevation,
                                        sizeof(elevation), &returned);
    CloseHandle(token);
    return ok && elevation.TokenIsElevated != 0;
}

bool runProcessHidden(const QString &program, const QStringList &arguments,
                      int *exitCode, QString *error)
{
    if (error) {
        error->clear();
    }
    QProcess process;
    process.setCreateProcessArgumentsModifier(
        [](QProcess::CreateProcessArguments *args) { args->flags |= CREATE_NO_WINDOW; });
    process.start(program, arguments);
    if (!process.waitForStarted(5000)) {
        if (error) {
            *error = QStringLiteral("无法启动 %1：%2").arg(program, process.errorString());
        }
        return false;
    }
    if (!process.waitForFinished(20000)) {
        process.kill();
        process.waitForFinished(1000);
        if (error) {
            *error = QStringLiteral("%1 执行超时").arg(program);
        }
        return false;
    }
    if (exitCode) {
        *exitCode = process.exitCode();
    }
    return true;
}
