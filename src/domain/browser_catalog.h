#ifndef WINTOOLS_DOMAIN_BROWSER_CATALOG_H
#define WINTOOLS_DOMAIN_BROWSER_CATALOG_H

#include "domain/interfaces.h"

#include <QString>
#include <QVector>

struct BrowserInfo
{
    QString name;
    QString executablePath;
};

class BrowserCatalog
{
public:
    BrowserCatalog(const IRegistry *registry, const IFileSystem *fileSystem);

    QVector<BrowserInfo> detect() const;

    static QString extractExecutable(const QString &commandLine);

private:
    const IRegistry *m_registry;
    const IFileSystem *m_fileSystem;
};

#endif // WINTOOLS_DOMAIN_BROWSER_CATALOG_H
