#ifndef WINTOOLS_UI_SETTINGS_DIALOG_H
#define WINTOOLS_UI_SETTINGS_DIALOG_H

#include "domain/config.h"

#include <QDialog>

class AutostartManager;
class QCheckBox;
class QComboBox;
class QLabel;

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    SettingsDialog(AutostartManager *manager, const Settings &settings,
                   const QString &logDirectory, QWidget *parent = nullptr);

    Settings result() const;

protected:
    void accept() override;

private:
    void refreshFromSystem();
    void updateModeEnabled();

    AutostartManager *m_manager;
    Settings m_settings;
    QString m_logDirectory;
    QCheckBox *m_autostartCheck;
    QComboBox *m_modeCombo;
    QLabel *m_hintLabel;
};

#endif // WINTOOLS_UI_SETTINGS_DIALOG_H
