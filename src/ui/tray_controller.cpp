#include "ui/tray_controller.h"

#include "platform/win/icon_factory.h"

#include <QAction>
#include <QMenu>
#include <QSystemTrayIcon>

TrayController::TrayController(QObject *parent)
    : QObject(parent)
{
    m_tray = new QSystemTrayIcon(IconFactory::applicationIcon(), this);
    m_tray->setToolTip(QStringLiteral("WinTools 启动台"));

    m_menu = new QMenu();
    m_menu->addAction(QStringLiteral("显示主窗口"), this, &TrayController::showRequested);

    m_groupAction = m_menu->addAction(QStringLiteral("打开组"));
    m_groupMenu = new QMenu(m_menu);
    m_groupAction->setMenu(m_groupMenu);

    m_menu->addSeparator();
    m_autostartAction = m_menu->addAction(QStringLiteral("开机自启"));
    m_autostartAction->setCheckable(true);
    connect(m_autostartAction, &QAction::toggled, this, [this](bool checked) {
        if (!m_updatingAutostart) {
            emit autostartToggleRequested(checked);
        }
    });

    m_menu->addSeparator();
    m_menu->addAction(QStringLiteral("退出"), this, &TrayController::quitRequested);

    m_tray->setContextMenu(m_menu);
    connect(m_tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick) {
            emit showRequested();
        }
    });
    connect(m_tray, &QSystemTrayIcon::messageClicked, this, &TrayController::messageClicked);

    m_tray->show();
}

TrayController::~TrayController()
{
    delete m_menu;
}

void TrayController::setGroups(const QVector<Group> &groups)
{
    m_groups = groups;
    rebuildGroupMenu();
}

void TrayController::rebuildGroupMenu()
{
    m_groupMenu->clear();
    if (m_groups.isEmpty()) {
        m_groupMenu->addAction(QStringLiteral("（无组）"))->setEnabled(false);
        return;
    }
    for (int index = 0; index < m_groups.size(); ++index) {
        const QString name = m_groups.at(index).name;
        QAction *action = m_groupMenu->addAction(name);
        connect(action, &QAction::triggered, this, [this, index]() {
            emit groupLaunchRequested(index);
        });
    }
}

void TrayController::setAutostartChecked(bool checked)
{
    m_updatingAutostart = true;
    m_autostartAction->setChecked(checked);
    m_updatingAutostart = false;
}

bool TrayController::isAutostartChecked() const
{
    return m_autostartAction->isChecked();
}

void TrayController::showMessage(const QString &title, const QString &text)
{
    if (!m_tray->isVisible()) {
        m_tray->show();
    }
    m_tray->showMessage(title, text, QSystemTrayIcon::Information, 5000);
}
