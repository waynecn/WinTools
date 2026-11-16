#include "platform/win/registry_reader.h"

#include <QSettings>

QStringList WindowsRegistryReader::childKeys(const QString &path) const
{
    const QSettings settings(path, QSettings::NativeFormat);
    return settings.childGroups();
}

QString WindowsRegistryReader::defaultValue(const QString &path) const
{
    const QSettings settings(path, QSettings::NativeFormat);
    return settings.value(QStringLiteral(".")).toString();
}

QString WindowsRegistryReader::value(const QString &path, const QString &name) const
{
    const QSettings settings(path, QSettings::NativeFormat);
    return settings.value(name).toString();
}
