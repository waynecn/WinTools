#ifndef WINTOOLS_PLATFORM_SYSTEM_ENVIRONMENT_H
#define WINTOOLS_PLATFORM_SYSTEM_ENVIRONMENT_H

#include "domain/interfaces.h"

class SystemEnvironment : public IEnvironment
{
public:
    QString expand(const QString &value) const override;
};

class SystemFileSystem : public IFileSystem
{
public:
    bool existsFile(const QString &path) const override;
    bool existsDir(const QString &path) const override;
    QString canonicalPath(const QString &path) const override;
};

#endif // WINTOOLS_PLATFORM_SYSTEM_ENVIRONMENT_H
