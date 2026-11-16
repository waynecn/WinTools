#ifndef WINTOOLS_UI_MAIN_WINDOW_H
#define WINTOOLS_UI_MAIN_WINDOW_H

#include "domain/browser_catalog.h"
#include "domain/config.h"
#include "domain/config_store.h"
#include "domain/launch_planner.h"
#include "domain/launch_runner.h"
#include "domain/logger.h"
#include "domain/path_resolver.h"
#include "platform/win/autostart_manager.h"
#include "platform/win/process_enumerator.h"
#include "platform/win/registry_reader.h"
#include "platform/win/shell_launcher.h"
#include "platform/win/system_environment.h"
#include "ui/launch_result_dialog.h"

#include <QMainWindow>
#include <QPointer>
#include <QUuid>
#include <QUrl>
#include <QVector>

class QCheckBox;
class QLabel;
class QPushButton;
class QTimer;
class GroupListWidget;
class ItemTableWidget;
class TrayController;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(ConfigStore *store, const Config &config, bool elevated, Logger *logger,
               QWidget *parent = nullptr);
    ~MainWindow() override;

    void showFromTray();
    void notifyRunningInTray();
    void prepareQuit();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void setupUi();
    void setupTray();

    int currentGroupIndex() const;
    void refreshGroups();
    void refreshItems();

    void addItem();
    void editItemAt(int row);
    void deleteItemAt(int row);
    void relocateItemAt(int row);
    void launchItemAtRow(int row);

    void createGroup();
    void renameCurrentGroup();
    void deleteCurrentGroup();
    void moveGroup(int from, int to);
    void moveItem(int from, int to);

    void addDroppedFiles(const QList<QUrl> &urls);

    void launchCurrentGroup();
    void launchGroupAt(int index);
    void launchGroup(const Group &group);
    void handleResults(const QString &actionName, const QVector<LaunchResult> &results);
    void showLastResults();

    void showGroupContextMenu(const QPoint &pos);
    void showItemContextMenu(const QPoint &pos);

    void openSettings();
    void applyAutostart(bool enabled);
    void syncAutostartFromSystem(bool updateConfig);

    void scheduleSave();
    void saveNow();

    ConfigStore *m_store;
    Config m_config;
    bool m_elevated;
    Logger *m_logger;
    QUuid m_currentGroupId;

    SystemEnvironment m_environment;
    SystemFileSystem m_fileSystem;
    PathResolver m_resolver;
    ProcessEnumerator m_processEnumerator;
    ShellLauncher m_shellLauncher;
    LaunchPlanner m_planner;
    LaunchRunner m_runner;
    WindowsRegistryReader m_registryReader;
    BrowserCatalog m_browserCatalog;
    AutostartManager m_autostartManager;

    TrayController *m_tray;
    QTimer *m_saveTimer;
    QPointer<LaunchResultDialog> m_resultDialog;
    QVector<LaunchResult> m_lastResults;
    QString m_lastActionName;
    bool m_saveFailedShown;
    bool m_quitting;
    bool m_trayNotified;

    GroupListWidget *m_groupList;
    ItemTableWidget *m_itemTable;
    QPushButton *m_launchButton;
    QCheckBox *m_forceCheck;
    QLabel *m_adminBanner;
};

#endif // WINTOOLS_UI_MAIN_WINDOW_H
