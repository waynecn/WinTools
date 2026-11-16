#ifndef WINTOOLS_DOMAIN_INTERFACES_H
#define WINTOOLS_DOMAIN_INTERFACES_H

#include <QDateTime>
#include <QString>
#include <QStringList>

// 领域层依赖的能力接口。具体实现位于 platform/win，测试中使用假实现。
class IEnvironment
{
public:
    virtual ~IEnvironment() = default;
    virtual QString expand(const QString &value) const = 0;
};

class IFileSystem
{
public:
    virtual ~IFileSystem() = default;
    virtual bool existsFile(const QString &path) const = 0;
    virtual bool existsDir(const QString &path) const = 0;
    virtual QString canonicalPath(const QString &path) const = 0;
};

class IProcessEnumerator
{
public:
    virtual ~IProcessEnumerator() = default;
    virtual QStringList runningExecutablePaths() const = 0;
};

class ILauncher
{
public:
    virtual ~ILauncher() = default;
    virtual bool shellOpen(const QString &target, QString *error) = 0;
    virtual bool startDetached(const QString &program, const QStringList &arguments,
                               const QString &workingDirectory, QString *error) = 0;
};

class IClock
{
public:
    virtual ~IClock() = default;
    virtual QDateTime now() const = 0;
};

class IRegistry
{
public:
    virtual ~IRegistry() = default;
    virtual QStringList childKeys(const QString &path) const = 0;
    virtual QString defaultValue(const QString &path) const = 0;
    virtual QString value(const QString &path, const QString &name) const = 0;
};

#endif // WINTOOLS_DOMAIN_INTERFACES_H
