#include "ui/settings_dialog.h"

#include "platform/win/autostart_manager.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QDir>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(AutostartManager *manager, const Settings &settings,
                               const QString &logDirectory, QWidget *parent)
    : QDialog(parent)
    , m_manager(manager)
    , m_settings(settings)
    , m_logDirectory(logDirectory)
{
    setWindowTitle(QStringLiteral("设置"));
    resize(460, 220);

    auto *layout = new QVBoxLayout(this);
    auto *form = new QFormLayout();

    m_autostartCheck = new QCheckBox(QStringLiteral("开机自启"), this);
    form->addRow(QString(), m_autostartCheck);

    m_modeCombo = new QComboBox(this);
    m_modeCombo->addItem(QStringLiteral("普通权限"), QStringLiteral("normal"));
    m_modeCombo->addItem(QStringLiteral("管理员权限（计划任务）"), QStringLiteral("admin"));
    form->addRow(QStringLiteral("自启时模式："), m_modeCombo);

    auto *logRow = new QHBoxLayout();
    auto *openLogsButton = new QPushButton(QStringLiteral("打开日志目录"), this);
    logRow->addWidget(openLogsButton);
    logRow->addStretch();
    form->addRow(QStringLiteral("日志："), logRow);

    layout->addLayout(form);

    m_hintLabel = new QLabel(this);
    m_hintLabel->setWordWrap(true);
    m_hintLabel->setStyleSheet(QStringLiteral("color:#666;"));
    layout->addWidget(m_hintLabel);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
    connect(m_autostartCheck, &QCheckBox::toggled, this, &SettingsDialog::updateModeEnabled);
    connect(openLogsButton, &QPushButton::clicked, this, [this]() {
        QDir().mkpath(m_logDirectory);
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_logDirectory));
    });

    refreshFromSystem();
}

Settings SettingsDialog::result() const
{
    return m_settings;
}

void SettingsDialog::refreshFromSystem()
{
    const AutostartManager::State state = m_manager->query();
    m_autostartCheck->setChecked(state.any());
    m_modeCombo->setCurrentIndex(state.task ? 1 : 0);
    if (state.task) {
        m_hintLabel->setText(QStringLiteral(
            "当前通过计划任务以管理员权限自启（登录时不弹 UAC）。"));
    } else if (state.runKey) {
        m_hintLabel->setText(QStringLiteral("当前通过注册表 Run 键以普通权限自启。"));
    } else {
        m_hintLabel->setText(QStringLiteral("开机自启当前未启用。"));
    }
    updateModeEnabled();
}

void SettingsDialog::updateModeEnabled()
{
    m_modeCombo->setEnabled(m_autostartCheck->isChecked());
}

void SettingsDialog::accept()
{
    const bool enabled = m_autostartCheck->isChecked();
    const QString mode = m_modeCombo->currentData().toString();

    QString error;
    if (!m_manager->apply(enabled, mode, &error)) {
        QMessageBox::warning(this, QStringLiteral("开机自启"), error);
        refreshFromSystem();
        return;
    }

    m_settings.autostart = enabled;
    m_settings.autostartMode = mode;
    QDialog::accept();
}
