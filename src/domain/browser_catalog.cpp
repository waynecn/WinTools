#include "domain/browser_catalog.h"

#include <QSet>

#include <algorithm>

BrowserCatalog::BrowserCatalog(const IRegistry *registry, const IFileSystem *fileSystem)
    : m_registry(registry)
    , m_fileSystem(fileSystem)
{
}

QVector<BrowserInfo> BrowserCatalog::detect() const
{
    QVector<BrowserInfo> browsers;
    if (!m_registry) {
        return browsers;
    }

    const QStringList roots = {
        QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\Clients\\StartMenuInternet"),
        QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Clients\\StartMenuInternet"),
        QStringLiteral("HKEY_CURRENT_USER\\SOFTWARE\\Clients\\StartMenuInternet")
    };

    QSet<QString> seen;
    for (const QString &root : roots) {
        const QStringList keys = m_registry->childKeys(root);
        for (const QString &key : keys) {
            const QString clientPath = root + QLatin1Char('\\') + key;
            QString name = m_registry->defaultValue(clientPath).trimmed();
            if (name.isEmpty()) {
                name = key;
            }
            const QString command = m_registry->defaultValue(
                clientPath + QStringLiteral("\\shell\\open\\command"));
            QString executable = extractExecutable(command);
            if (executable.isEmpty()) {
                continue;
            }
            if (m_fileSystem && !m_fileSystem->existsFile(executable)) {
                continue;
            }
            const QString normalized = executable.toLower();
            if (seen.contains(normalized)) {
                continue;
            }
            seen.insert(normalized);
            BrowserInfo info;
            info.name = name;
            info.executablePath = executable;
            browsers.append(info);
        }
    }

    std::sort(browsers.begin(), browsers.end(), [](const BrowserInfo &left, const BrowserInfo &right) {
        return left.name.compare(right.name, Qt::CaseInsensitive) < 0;
    });
    return browsers;
}

QString BrowserCatalog::extractExecutable(const QString &commandLine)
{
    const QString trimmed = commandLine.trimmed();
    if (trimmed.isEmpty()) {
        return QString();
    }
    if (trimmed.startsWith(QLatin1Char('"'))) {
        const int closing = trimmed.indexOf(QLatin1Char('"'), 1);
        if (closing <= 1) {
            return QString();
        }
        return trimmed.mid(1, closing - 1).trimmed();
    }
    const int space = trimmed.indexOf(QLatin1Char(' '));
    return (space < 0 ? trimmed : trimmed.left(space)).trimmed();
}
