#ifndef WINTOOLS_DOMAIN_PATH_RESOLVER_H
#define WINTOOLS_DOMAIN_PATH_RESOLVER_H

#include "domain/interfaces.h"
#include "domain/item.h"

#include <QString>

enum class PathStatus
{
    Valid,
    Missing,
    WrongType
};

class PathResolver
{
public:
    PathResolver(const IEnvironment *environment, const IFileSystem *fileSystem);

    QString expand(const QString &value) const;
    PathStatus classify(const Item &item) const;

    // 用于比较两个路径是否指向同一目标（Windows 下不区分大小写）。
    QString normalized(const QString &path) const;
    bool samePath(const QString &left, const QString &right) const;

    static QString pathStatusText(PathStatus status);

private:
    const IEnvironment *m_environment;
    const IFileSystem *m_fileSystem;
};

#endif // WINTOOLS_DOMAIN_PATH_RESOLVER_H
