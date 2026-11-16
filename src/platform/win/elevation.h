#ifndef WINTOOLS_PLATFORM_ELEVATION_H
#define WINTOOLS_PLATFORM_ELEVATION_H

#include <QString>
#include <QStringList>

bool isCurrentProcessElevated();

// 以管理员权限重启当前 exe，arguments 为追加参数（通常包含 --elevated）。
bool relaunchElevated(const QStringList &arguments, QString *error);

#endif // WINTOOLS_PLATFORM_ELEVATION_H
