#ifndef WINTOOLS_DOMAIN_CONFIG_STORE_H
#define WINTOOLS_DOMAIN_CONFIG_STORE_H

#include "domain/config.h"

#include <QString>

class ConfigStore
{
public:
    enum class LoadStatus
    {
        Loaded,
        CreatedDefault,
        Migrated,
        RecoveredFromBackup,
        ResetAfterCorruption,
        RefusedNewerVersion
    };

    struct LoadResult
    {
        Config config;
        LoadStatus status = LoadStatus::Loaded;
        QString message;
        bool ok = true;
    };

    explicit ConfigStore(QString configPath);

    LoadResult load();
    bool save(const Config &config, QString *error);
    bool saveWithoutBackup(const Config &config, QString *error);

    QString configPath() const;
    QString backupPath() const;
    QString directoryPath() const;

private:
    bool writeFile(const Config &config, bool updateBackup, QString *error);
    bool parseFile(const QString &path, Config *config, QString *error) const;
    void preserveCorruptFile();
    QString corruptPath() const;

    QString m_path;
};

#endif // WINTOOLS_DOMAIN_CONFIG_STORE_H
