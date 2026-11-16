#include "domain/item.h"

#include <QFileInfo>
#include <QUrl>

Item Item::make(ItemType type)
{
    Item item;
    item.id = QUuid::createUuid();
    item.type = type;
    return item;
}

QJsonObject Item::toJson() const
{
    QJsonObject browserObject;
    browserObject.insert(QStringLiteral("mode"), browserModeKey(browser.mode));
    browserObject.insert(QStringLiteral("path"), browser.programPath);

    QJsonObject object;
    object.insert(QStringLiteral("id"), id.toString(QUuid::WithoutBraces));
    object.insert(QStringLiteral("type"), itemTypeKey(type));
    object.insert(QStringLiteral("name"), name);
    object.insert(QStringLiteral("path"), path);
    object.insert(QStringLiteral("url"), url);
    object.insert(QStringLiteral("browser"), browserObject);
    object.insert(QStringLiteral("command"), command);
    object.insert(QStringLiteral("workingDir"), workingDir);
    object.insert(QStringLiteral("keepConsole"), keepConsole);
    return object;
}

bool Item::fromJson(const QJsonObject &object, Item *item, QString *error)
{
    if (!item) {
        return false;
    }
    const QString typeKey = object.value(QStringLiteral("type")).toString();
    ItemType type = ItemType::Directory;
    if (!itemTypeFromKey(typeKey, &type)) {
        if (error) {
            *error = QStringLiteral("未知的条目类型: %1").arg(typeKey);
        }
        return false;
    }

    Item result;
    result.type = type;
    const QString idText = object.value(QStringLiteral("id")).toString();
    result.id = idText.isEmpty() ? QUuid::createUuid() : QUuid(idText);
    if (result.id.isNull()) {
        result.id = QUuid::createUuid();
    }
    result.name = object.value(QStringLiteral("name")).toString();
    result.path = object.value(QStringLiteral("path")).toString();
    result.url = object.value(QStringLiteral("url")).toString();
    result.command = object.value(QStringLiteral("command")).toString();
    result.workingDir = object.value(QStringLiteral("workingDir")).toString();
    result.keepConsole = object.value(QStringLiteral("keepConsole")).toBool(false);

    const QJsonObject browserObject = object.value(QStringLiteral("browser")).toObject();
    BrowserMode browserMode = BrowserMode::SystemDefault;
    browserModeFromKey(
        browserObject.value(QStringLiteral("mode")).toString(QStringLiteral("default")),
        &browserMode);
    result.browser.mode = browserMode;
    result.browser.programPath = browserObject.value(QStringLiteral("path")).toString();

    if (result.name.isEmpty()) {
        result.name = result.suggestedName();
    }
    *item = result;
    return true;
}

QString Item::targetText() const
{
    switch (type) {
    case ItemType::Directory:
    case ItemType::Application:
    case ItemType::File:
        return path;
    case ItemType::Url:
        return url;
    case ItemType::Command:
        return command;
    }
    return QString();
}

QString Item::suggestedName() const
{
    switch (type) {
    case ItemType::Directory:
    case ItemType::Application:
    case ItemType::File: {
        const QFileInfo info(path);
        if (!info.fileName().isEmpty()) {
            return info.fileName();
        }
        return path;
    }
    case ItemType::Url: {
        const QUrl parsed(url);
        const QString host = parsed.host();
        return host.isEmpty() ? url : host;
    }
    case ItemType::Command: {
        const QString trimmed = command.trimmed();
        const int space = trimmed.indexOf(QLatin1Char(' '));
        return space < 0 ? trimmed : trimmed.left(space);
    }
    }
    return QString();
}

QString itemTypeKey(ItemType type)
{
    switch (type) {
    case ItemType::Directory:
        return QStringLiteral("directory");
    case ItemType::Application:
        return QStringLiteral("application");
    case ItemType::File:
        return QStringLiteral("file");
    case ItemType::Url:
        return QStringLiteral("url");
    case ItemType::Command:
        return QStringLiteral("command");
    }
    return QStringLiteral("directory");
}

QString itemTypeLabel(ItemType type)
{
    switch (type) {
    case ItemType::Directory:
        return QStringLiteral("目录");
    case ItemType::Application:
        return QStringLiteral("应用");
    case ItemType::File:
        return QStringLiteral("文件");
    case ItemType::Url:
        return QStringLiteral("URL");
    case ItemType::Command:
        return QStringLiteral("命令行");
    }
    return QString();
}

bool itemTypeFromKey(const QString &key, ItemType *type)
{
    if (!type) {
        return false;
    }
    if (key == QLatin1String("directory")) {
        *type = ItemType::Directory;
    } else if (key == QLatin1String("application")) {
        *type = ItemType::Application;
    } else if (key == QLatin1String("file")) {
        *type = ItemType::File;
    } else if (key == QLatin1String("url")) {
        *type = ItemType::Url;
    } else if (key == QLatin1String("command")) {
        *type = ItemType::Command;
    } else {
        return false;
    }
    return true;
}

QString browserModeKey(BrowserMode mode)
{
    switch (mode) {
    case BrowserMode::SystemDefault:
        return QStringLiteral("default");
    case BrowserMode::Detected:
        return QStringLiteral("detected");
    case BrowserMode::Custom:
        return QStringLiteral("custom");
    }
    return QStringLiteral("default");
}

bool browserModeFromKey(const QString &key, BrowserMode *mode)
{
    if (!mode) {
        return false;
    }
    if (key == QLatin1String("default")) {
        *mode = BrowserMode::SystemDefault;
    } else if (key == QLatin1String("detected")) {
        *mode = BrowserMode::Detected;
    } else if (key == QLatin1String("custom")) {
        *mode = BrowserMode::Custom;
    } else {
        return false;
    }
    return true;
}
