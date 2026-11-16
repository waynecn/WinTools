#ifndef WINTOOLS_PLATFORM_SHELL_LAUNCHER_H
#define WINTOOLS_PLATFORM_SHELL_LAUNCHER_H

#include "domain/interfaces.h"

class ShellLauncher : public ILauncher
{
public:
    bool shellOpen(const QString &target, QString *error) override;
    bool startDetached(const QString &program, const QStringList &arguments,
                       const QString &workingDirectory, QString *error) override;
};

#endif // WINTOOLS_PLATFORM_SHELL_LAUNCHER_H
