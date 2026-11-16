#ifndef WINTOOLS_PLATFORM_WIN_UTILS_H
#define WINTOOLS_PLATFORM_WIN_UTILS_H

#include <QString>
#include <QStringList>

QString winErrorMessage(unsigned long code);

bool shellOpenTarget(const QString &target, QString *error);

// 以管理员权限运行并等待结束，用于 schtasks 等需要提权的系统命令。
bool shellRunElevatedWait(const QString &file, const QString &parameters,
                          int *exitCode, QString *error);

bool currentProcessElevated();

bool runProcessHidden(const QString &program, const QStringList &arguments,
                      int *exitCode, QString *error);

#endif // WINTOOLS_PLATFORM_WIN_UTILS_H
