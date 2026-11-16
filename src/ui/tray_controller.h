#ifndef WINTOOLS_UI_TRAY_CONTROLLER_H
#define WINTOOLS_UI_TRAY_CONTROLLER_H

#include "domain/config.h"

#include <QObject>
#include <QVector>

class QAction;
class QMenu;
class QSystemTrayIcon;

class TrayController : public QObject
{
    Q_OBJECT

public:
    explicit TrayController(QObject *parent = nullptr);
    ~TrayController() override;

    void setGroups(const QVector<Group> &groups);
    void setAutostartChecked(bool checked);
    bool isAutostartChecked() const;
    void showMessage(const QString &title, const QString &text);

signals:
    void showRequested();
    void groupLaunchRequested(int groupIndex);
    void quitRequested();
    void autostartToggleRequested(bool enabled);
    void messageClicked();

private:
    void rebuildGroupMenu();

    QSystemTrayIcon *m_tray;
    QMenu *m_menu;
    QMenu *m_groupMenu;
    QAction *m_groupAction;
    QAction *m_autostartAction;
    QVector<Group> m_groups;
    bool m_updatingAutostart = false;
};

#endif // WINTOOLS_UI_TRAY_CONTROLLER_H
