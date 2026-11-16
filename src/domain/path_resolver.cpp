#include "domain/path_resolver.h"

#include <QDir>

PathResolver::PathResolver(const IEnvironment *environment, const IFileSystem *fileSystem)
    : m_environment(environment)
    , m_fileSystem(fileSystem)
{
}

QString PathResolver::expand(const QString &value) const
{
    if (!m_environment || value.isEmpty()) {
        return value;
    }
    return m_environment->expand(value);
}

PathStatus PathResolver::classify(const Item &item) const
{
    switch (item.type) {
    case ItemType::Directory: {
        const QString path = expand(item.path);
        if (path.trimmed().isEmpty()) {
            return PathStatus::Missing;
        }
        if (m_fileSystem && m_fileSystem->existsDir(path)) {
            return PathStatus::Valid;
        }
        if (m_fileSystem && m_fileSystem->existsFile(path)) {
            return PathStatus::WrongType;
        }
        return PathStatus::Missing;
    }
    case ItemType::Application:
    case ItemType::File: {
        const QString path = expand(item.path);
        if (path.trimmed().isEmpty()) {
            return PathStatus::Missing;
        }
        if (m_fileSystem && m_fileSystem->existsFile(path)) {
            return PathStatus::Valid;
        }
        if (m_fileSystem && m_fileSystem->existsDir(path)) {
            return PathStatus::WrongType;
        }
        return PathStatus::Missing;
    }
    case ItemType::Url:
        return item.url.trimmed().isEmpty() ? PathStatus::Missing : PathStatus::Valid;
    case ItemType::Command: {
        if (item.command.trimmed().isEmpty()) {
            return PathStatus::Missing;
        }
        if (item.workingDir.trimmed().isEmpty()) {
            return PathStatus::Valid;
        }
        const QString workingDir = expand(item.workingDir);
        if (m_fileSystem && m_fileSystem->existsDir(workingDir)) {
            return PathStatus::Valid;
        }
        if (m_fileSystem && m_fileSystem->existsFile(workingDir)) {
            return PathStatus::WrongType;
        }
        return PathStatus::Missing;
    }
    }
    return PathStatus::Missing;
}

QString PathResolver::normalized(const QString &path) const
{
    QString result;
    if (m_fileSystem) {
        result = m_fileSystem->canonicalPath(path);
    }
    if (result.isEmpty()) {
        result = QDir::cleanPath(path);
    } else {
        result = QDir::cleanPath(result);
    }
#ifdef Q_OS_WIN
    result = result.toLower();
#endif
    return result;
}

bool PathResolver::samePath(const QString &left, const QString &right) const
{
    return normalized(left) == normalized(right);
}

QString PathResolver::pathStatusText(PathStatus status)
{
    switch (status) {
    case PathStatus::Valid:
        return QStringLiteral("有效");
    case PathStatus::Missing:
        return QStringLiteral("目标不存在");
    case PathStatus::WrongType:
        return QStringLiteral("目标类型不匹配");
    }
    return QString();
}
