#include "domain/config_store.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

ConfigStore::ConfigStore(QString configPath)
    : m_path(std::move(configPath))
{
}

QString ConfigStore::configPath() const
{
    return m_path;
}

QString ConfigStore::backupPath() const
{
    return m_path + QStringLiteral(".bak");
}

QString ConfigStore::directoryPath() const
{
    return QFileInfo(m_path).absolutePath();
}

ConfigStore::LoadResult ConfigStore::load()
{
    LoadResult result;

    if (!QFileInfo::exists(m_path)) {
        result.config = Config::makeDefault();
        result.status = LoadStatus::CreatedDefault;
        result.ok = true;
        QString error;
        if (!writeFile(result.config, false, &error)) {
            result.message = QStringLiteral("初始化配置保存失败：%1").arg(error);
        }
        return result;
    }

    Config config;
    QString parseError;
    if (!parseFile(m_path, &config, &parseError)) {
        Config backup;
        QString backupError;
        if (parseFile(backupPath(), &backup, &backupError)
            && backup.schemaVersion <= kCurrentSchemaVersion) {
            preserveCorruptFile();
            if (backup.groups.isEmpty()) {
                backup.groups.append(Group::makeDefault());
            }
            QString writeError;
            writeFile(backup, false, &writeError);
            result.config = backup;
            result.status = LoadStatus::RecoveredFromBackup;
            result.ok = true;
            result.message = QStringLiteral("主配置文件损坏，已从备份恢复。");
            return result;
        }

        preserveCorruptFile();
        Config defaults = Config::makeDefault();
        QString writeError;
        writeFile(defaults, false, &writeError);
        result.config = defaults;
        result.status = LoadStatus::ResetAfterCorruption;
        result.ok = true;
        result.message = QStringLiteral("配置文件损坏且备份不可用，已重置为默认配置；原文件已保留。");
        return result;
    }

    if (config.schemaVersion > kCurrentSchemaVersion) {
        result.config = config;
        result.status = LoadStatus::RefusedNewerVersion;
        result.ok = false;
        result.message = QStringLiteral("配置文件由更高版本的 WinTools 创建（schemaVersion %1），当前程序无法加载。")
                             .arg(config.schemaVersion);
        return result;
    }

    bool needsWrite = false;
    if (config.schemaVersion < kCurrentSchemaVersion) {
        // v1 是目前唯一版本；为后续迁移预留分支。
        config.schemaVersion = kCurrentSchemaVersion;
        needsWrite = true;
        result.status = LoadStatus::Migrated;
        result.message = QStringLiteral("配置已升级到新版本。");
    }
    if (config.groups.isEmpty()) {
        config.groups.append(Group::makeDefault());
        needsWrite = true;
    }

    if (needsWrite) {
        QString error;
        writeFile(config, true, &error);
        if (!error.isEmpty()) {
            result.message += QStringLiteral(" 保存失败：%1").arg(error);
        }
    } else {
        result.status = LoadStatus::Loaded;
    }

    result.config = config;
    result.ok = true;
    return result;
}

bool ConfigStore::save(const Config &config, QString *error)
{
    return writeFile(config, true, error);
}

bool ConfigStore::saveWithoutBackup(const Config &config, QString *error)
{
    return writeFile(config, false, error);
}

bool ConfigStore::writeFile(const Config &config, bool updateBackup, QString *error)
{
    if (error) {
        error->clear();
    }

    const QFileInfo info(m_path);
    QDir dir = info.absoluteDir();
    if (!dir.exists() && !QDir().mkpath(dir.absolutePath())) {
        if (error) {
            *error = QStringLiteral("无法创建配置目录：%1").arg(dir.absolutePath());
        }
        return false;
    }

    if (updateBackup && QFileInfo::exists(m_path)) {
        QFile::remove(backupPath());
        // 备份失败不阻止主文件写入，避免因备份问题导致配置丢失。
        QFile::copy(m_path, backupPath());
    }

    QSaveFile file(m_path);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }
    const QByteArray data = QJsonDocument(config.toJson()).toJson(QJsonDocument::Indented);
    if (file.write(data) != data.size()) {
        if (error) {
            *error = file.errorString();
        }
        file.cancelWriting();
        return false;
    }
    if (!file.commit()) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }
    return true;
}

bool ConfigStore::parseFile(const QString &path, Config *config, QString *error) const
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = QStringLiteral("无法读取 %1：%2").arg(path, file.errorString());
        }
        return false;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error) {
            *error = QStringLiteral("%1 不是有效的 JSON 对象：%2").arg(path, parseError.errorString());
        }
        return false;
    }
    return Config::fromJson(document.object(), config, error);
}

void ConfigStore::preserveCorruptFile()
{
    if (!QFileInfo::exists(m_path)) {
        return;
    }
    QFile::copy(m_path, corruptPath());
}

QString ConfigStore::corruptPath() const
{
    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMddHHmmsszzz"));
    return QDir(directoryPath()).filePath(QStringLiteral("config.corrupt-%1.json").arg(stamp));
}
