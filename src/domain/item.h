#ifndef WINTOOLS_DOMAIN_ITEM_H
#define WINTOOLS_DOMAIN_ITEM_H

#include <QJsonObject>
#include <QString>
#include <QUuid>

enum class ItemType
{
    Directory,
    Application,
    File,
    Url,
    Command
};

enum class BrowserMode
{
    SystemDefault,
    Detected,
    Custom
};

struct BrowserRef
{
    BrowserMode mode = BrowserMode::SystemDefault;
    QString programPath;
};

struct Item
{
    QUuid id;
    ItemType type = ItemType::Directory;
    QString name;
    QString path;
    QString url;
    BrowserRef browser;
    QString command;
    QString workingDir;
    bool keepConsole = false;

    static Item make(ItemType type);

    QJsonObject toJson() const;
    static bool fromJson(const QJsonObject &object, Item *item, QString *error = nullptr);

    QString targetText() const;
    QString suggestedName() const;
};

QString itemTypeKey(ItemType type);
QString itemTypeLabel(ItemType type);
bool itemTypeFromKey(const QString &key, ItemType *type);

QString browserModeKey(BrowserMode mode);
bool browserModeFromKey(const QString &key, BrowserMode *mode);

#endif // WINTOOLS_DOMAIN_ITEM_H
