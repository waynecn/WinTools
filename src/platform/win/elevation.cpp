#include "platform/win/elevation.h"

#include "platform/win/win_utils.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>

#include <QCoreApplication>
#include <QDir>

bool isCurrentProcessElevated()
{
    return currentProcessElevated();
}

bool relaunchElevated(const QStringList &arguments, QString *error)
{
    if (error) {
        error->clear();
    }

    const std::wstring executable = QCoreApplication::applicationFilePath().toStdWString();
    QString parameterText;
    for (const QString &argument : arguments) {
        if (argument.contains(QLatin1Char(' '))) {
            parameterText += QStringLiteral("\"%1\" ").arg(argument);
        } else {
            parameterText += argument + QLatin1Char(' ');
        }
    }
    parameterText = parameterText.trimmed();
    const std::wstring parameters = parameterText.toStdWString();
    const std::wstring workingDir = QCoreApplication::applicationDirPath().toStdWString();

    SHELLEXECUTEINFOW info = {};
    info.cbSize = sizeof(info);
    info.fMask = SEE_MASK_NOASYNC | SEE_MASK_FLAG_NO_UI;
    info.lpVerb = L"runas";
    info.lpFile = executable.c_str();
    info.lpParameters = parameters.empty() ? nullptr : parameters.c_str();
    info.lpDirectory = workingDir.c_str();
    info.nShow = SW_SHOWNORMAL;

    if (!ShellExecuteExW(&info)) {
        if (error) {
            *error = winErrorMessage(GetLastError());
        }
        return false;
    }
    return true;
}
