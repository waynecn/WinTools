#ifndef WINTOOLS_PLATFORM_REGISTRY_READER_H
#define WINTOOLS_PLATFORM_REGISTRY_READER_H

#include "domain/interfaces.h"

class WindowsRegistryReader : public IRegistry
{
public:
    QStringList childKeys(const QString &path) const override;
    QString defaultValue(const QString &path) const override;
    QString value(const QString &path, const QString &name) const override;
};

#endif // WINTOOLS_PLATFORM_REGISTRY_READER_H
