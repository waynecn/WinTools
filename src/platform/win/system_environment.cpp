#include "platform/win/system_environment.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <userenv.h>

#include <QDir>
#include <QFileInfo>

#include <string>

QString SystemEnvironment::expand(const QString &value) const
{
    if (value.isEmpty()) {
        return value;
    }
    const std::wstring input = value.toStdWString();
    const DWORD required = ExpandEnvironmentStringsW(input.c_str(), nullptr, 0);
    if (required == 0) {
        return value;
    }
    std::wstring output(required, L'\0');
    const DWORD written = ExpandEnvironmentStringsW(input.c_str(), &output[0], required);
    if (written == 0 || written > required) {
        return value;
    }
    output.resize(written - 1);
    return QString::fromStdWString(output);
}

bool SystemFileSystem::existsFile(const QString &path) const
{
    return QFileInfo(path).isFile();
}

bool SystemFileSystem::existsDir(const QString &path) const
{
    return QFileInfo(path).isDir();
}

QString SystemFileSystem::canonicalPath(const QString &path) const
{
    const QFileInfo info(path);
    const QString canonical = info.canonicalFilePath();
    if (!canonical.isEmpty()) {
        return QDir::cleanPath(canonical);
    }
    return QDir::cleanPath(info.absoluteFilePath());
}
