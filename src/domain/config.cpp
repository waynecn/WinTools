#include "domain/config.h"

#include <QJsonArray>

QJsonObject Settings::toJson() const
{
    QJsonObject object;
    object.insert(QStringLiteral("autostart"), autostart);
    object.insert(QStringLiteral("autostartMode"), autostartMode);
    return object;
}

Settings Settings::fromJson(const QJsonObject &object)
{
    Settings settings;
    settings.autostart = object.value(QStringLiteral("autostart")).toBool(false);
    const QString mode = object.value(QStringLiteral("autostartMode")).toString();
    if (mode == QLatin1String("admin")) {
        settings.autostartMode = QStringLiteral("admin");
    } else {
        settings.autostartMode = QStringLiteral("normal");
    }
    return settings;
}

Group Group::makeDefault()
{
    Group group;
    group.id = QUuid::createUuid();
    group.name = QStringLiteral("默认");
    return group;
}

QJsonObject Group::toJson() const
{
    QJsonArray itemArray;
    for (const Item &item : items) {
        itemArray.append(item.toJson());
    }

    QJsonObject object;
    object.insert(QStringLiteral("id"), id.toString(QUuid::WithoutBraces));
    object.insert(QStringLiteral("name"), name);
    object.insert(QStringLiteral("items"), itemArray);
    return object;
}

bool Group::fromJson(const QJsonObject &object, Group *group, QString *error)
{
    if (!group) {
        return false;
    }
    Group result;
    const QString idText = object.value(QStringLiteral("id")).toString();
    result.id = idText.isEmpty() ? QUuid::createUuid() : QUuid(idText);
    if (result.id.isNull()) {
        result.id = QUuid::createUuid();
    }
    result.name = object.value(QStringLiteral("name")).toString();
    if (result.name.isEmpty()) {
        result.name = QStringLiteral("未命名组");
    }

    if (!object.value(QStringLiteral("items")).isArray()) {
        if (error) {
            *error = QStringLiteral("组 %1 的条目列表无效").arg(result.name);
        }
        return false;
    }
    const QJsonArray itemArray = object.value(QStringLiteral("items")).toArray();
    for (const QJsonValue &value : itemArray) {
        if (!value.isObject()) {
            if (error) {
                *error = QStringLiteral("组 %1 包含无效条目").arg(result.name);
            }
            return false;
        }
        Item item;
        if (!Item::fromJson(value.toObject(), &item, error)) {
            return false;
        }
        result.items.append(item);
    }

    *group = result;
    return true;
}

Config Config::makeDefault()
{
    Config config;
    config.schemaVersion = kCurrentSchemaVersion;
    config.groups.append(Group::makeDefault());
    return config;
}

QJsonObject Config::toJson() const
{
    QJsonArray groupArray;
    for (const Group &group : groups) {
        groupArray.append(group.toJson());
    }

    QJsonObject object;
    object.insert(QStringLiteral("schemaVersion"), schemaVersion);
    object.insert(QStringLiteral("groups"), groupArray);
    object.insert(QStringLiteral("settings"), settings.toJson());
    return object;
}

bool Config::fromJson(const QJsonObject &object, Config *config, QString *error)
{
    if (!config) {
        return false;
    }
    if (!object.value(QStringLiteral("schemaVersion")).isDouble()) {
        if (error) {
            *error = QStringLiteral("缺少或无效的 schemaVersion");
        }
        return false;
    }
    if (!object.value(QStringLiteral("groups")).isArray()) {
        if (error) {
            *error = QStringLiteral("缺少或无效的 groups 数组");
        }
        return false;
    }

    Config result;
    result.schemaVersion = object.value(QStringLiteral("schemaVersion")).toInt();
    result.settings = Settings::fromJson(object.value(QStringLiteral("settings")).toObject());

    const QJsonArray groupArray = object.value(QStringLiteral("groups")).toArray();
    for (const QJsonValue &value : groupArray) {
        if (!value.isObject()) {
            if (error) {
                *error = QStringLiteral("groups 中包含非对象元素");
            }
            return false;
        }
        Group group;
        if (!Group::fromJson(value.toObject(), &group, error)) {
            return false;
        }
        result.groups.append(group);
    }

    *config = result;
    return true;
}
