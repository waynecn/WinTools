#ifndef WINTOOLS_DOMAIN_CONFIG_H
#define WINTOOLS_DOMAIN_CONFIG_H

#include "domain/item.h"

#include <QJsonObject>
#include <QString>
#include <QVector>

static constexpr int kCurrentSchemaVersion = 1;

struct Settings
{
    bool autostart = false;
    QString autostartMode = QStringLiteral("normal");

    QJsonObject toJson() const;
    static Settings fromJson(const QJsonObject &object);
};

struct Group
{
    QUuid id;
    QString name;
    QVector<Item> items;

    static Group makeDefault();
    QJsonObject toJson() const;
    static bool fromJson(const QJsonObject &object, Group *group, QString *error = nullptr);
};

struct Config
{
    int schemaVersion = kCurrentSchemaVersion;
    QVector<Group> groups;
    Settings settings;

    static Config makeDefault();
    QJsonObject toJson() const;
    static bool fromJson(const QJsonObject &object, Config *config, QString *error = nullptr);
};

#endif // WINTOOLS_DOMAIN_CONFIG_H
