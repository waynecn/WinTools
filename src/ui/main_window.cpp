#include "ui/main_window.h"

#include "platform/win/icon_factory.h"
#include "ui/drag_widgets.h"
#include "ui/item_dialog.h"
#include "ui/launch_result_dialog.h"
#include "ui/settings_dialog.h"
#include "ui/tray_controller.h"

#include <QAction>
#include <QCheckBox>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStatusBar>
#include <QStyle>
#include <QTableWidgetItem>
#include <QTimer>
#include <QToolBar>
#include <QUrl>
#include <QVBoxLayout>

MainWindow::MainWindow(ConfigStore *store, const Config &config, bool elevated, Logger *logger,
                       QWidget *parent)
    : QMainWindow(parent)
    , m_store(store)
    , m_config(config)
    , m_elevated(elevated)
    , m_logger(logger)
    , m_environment()
    , m_fileSystem()
    , m_resolver(&m_environment, &m_fileSystem)
    , m_processEnumerator()
    , m_shellLauncher()
    , m_planner(&m_resolver, &m_processEnumerator, QDir::homePath())
    , m_runner(&m_shellLauncher, logger)
    , m_registryReader()
    , m_browserCatalog(&m_registryReader, &m_fileSystem)
    , m_autostartManager(QCoreApplication::applicationFilePath())
    , m_tray(nullptr)
    , m_saveTimer(nullptr)
    , m_saveFailedShown(false)
    , m_quitting(false)
    , m_trayNotified(false)
    , m_groupList(nullptr)
    , m_itemTable(nullptr)
    , m_launchButton(nullptr)
    , m_forceCheck(nullptr)
    , m_adminBanner(nullptr)
{
    setWindowIcon(IconFactory::applicationIcon());
    setupUi();
    setupTray();

    if (m_logger) {
        m_logger->pruneOldLogs(30);
    }

    refreshGroups();
    syncAutostartFromSystem(true);
}

MainWindow::~MainWindow()
{
    // 退出路径上不再弹窗，避免在事件循环结束后出现模态框。
    if (m_store) {
        QString error;
        m_store->save(m_config, &error);
    }
}

void MainWindow::setupUi()
{
    setWindowTitle(QStringLiteral("WinTools 启动台"));
    resize(920, 580);

    QToolBar *toolbar = addToolBar(QStringLiteral("主工具栏"));
    toolbar->setMovable(false);
    QAction *addAction = toolbar->addAction(QStringLiteral("添加条目"));
    QAction *settingsAction = toolbar->addAction(QStringLiteral("设置"));
    connect(addAction, &QAction::triggered, this, [this]() { addItem(); });
    connect(settingsAction, &QAction::triggered, this, [this]() { openSettings(); });

    auto *central = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(6, 6, 6, 6);

    m_adminBanner = new QLabel(
        QStringLiteral("管理员模式：拖拽添加不可用，请使用「添加条目」按钮。"), central);
    m_adminBanner->setStyleSheet(QStringLiteral(
        "background:#fff3cd; color:#664d03; padding:6px; border:1px solid #ffe69c;"));
    m_adminBanner->setVisible(m_elevated);
    mainLayout->addWidget(m_adminBanner);

    auto *splitter = new QSplitter(Qt::Horizontal, central);

    auto *left = new QWidget(splitter);
    auto *leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    auto *groupLabel = new QLabel(QStringLiteral("组"), left);
    leftLayout->addWidget(groupLabel);
    m_groupList = new GroupListWidget(left);
    m_groupList->setContextMenuPolicy(Qt::CustomContextMenu);
    leftLayout->addWidget(m_groupList);

    auto *groupButtons = new QHBoxLayout();
    auto *addGroupButton = new QPushButton(QStringLiteral("新建"), left);
    auto *renameGroupButton = new QPushButton(QStringLiteral("重命名"), left);
    auto *deleteGroupButton = new QPushButton(QStringLiteral("删除"), left);
    groupButtons->addWidget(addGroupButton);
    groupButtons->addWidget(renameGroupButton);
    groupButtons->addWidget(deleteGroupButton);
    leftLayout->addLayout(groupButtons);

    auto *right = new QWidget(splitter);
    auto *rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    auto *launchRow = new QHBoxLayout();
    m_launchButton = new QPushButton(QStringLiteral("打开本组"), right);
    m_forceCheck = new QCheckBox(QStringLiteral("强制重开已运行应用"), right);
    launchRow->addWidget(m_launchButton);
    launchRow->addWidget(m_forceCheck);
    launchRow->addStretch();
    rightLayout->addLayout(launchRow);

    m_itemTable = new ItemTableWidget(right);
    m_itemTable->setColumnCount(3);
    m_itemTable->setHorizontalHeaderLabels(
        {QStringLiteral("名称"), QStringLiteral("类型"), QStringLiteral("目标")});
    m_itemTable->verticalHeader()->setVisible(false);
    m_itemTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_itemTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_itemTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_itemTable->setAlternatingRowColors(true);
    m_itemTable->horizontalHeader()->setStretchLastSection(true);
    m_itemTable->setColumnWidth(0, 190);
    m_itemTable->setColumnWidth(1, 80);
    m_itemTable->setContextMenuPolicy(Qt::CustomContextMenu);
    m_itemTable->setExternalDropEnabled(!m_elevated);
    rightLayout->addWidget(m_itemTable);

    splitter->addWidget(left);
    splitter->addWidget(right);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({240, 680});
    mainLayout->addWidget(splitter);
    setCentralWidget(central);

    statusBar()->showMessage(QStringLiteral("就绪"));

    m_saveTimer = new QTimer(this);
    m_saveTimer->setSingleShot(true);
    m_saveTimer->setInterval(300);
    connect(m_saveTimer, &QTimer::timeout, this, &MainWindow::saveNow);

    connect(m_groupList, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row >= 0 && row < m_config.groups.size()) {
            m_currentGroupId = m_config.groups.at(row).id;
            refreshItems();
        }
    });
    connect(m_groupList, &QWidget::customContextMenuRequested,
            this, &MainWindow::showGroupContextMenu);
    connect(m_groupList, &GroupListWidget::rowsMoveRequested, this, &MainWindow::moveGroup);

    connect(m_itemTable, &QWidget::customContextMenuRequested,
            this, &MainWindow::showItemContextMenu);
    connect(m_itemTable, &ItemTableWidget::rowsMoveRequested, this, &MainWindow::moveItem);
    connect(m_itemTable, &ItemTableWidget::filesDropped, this, &MainWindow::addDroppedFiles);
    connect(m_itemTable, &QTableWidget::cellDoubleClicked, this,
            [this](int row, int) { launchItemAtRow(row); });

    connect(m_launchButton, &QPushButton::clicked, this, &MainWindow::launchCurrentGroup);

    connect(addGroupButton, &QPushButton::clicked, this, &MainWindow::createGroup);
    connect(renameGroupButton, &QPushButton::clicked, this, &MainWindow::renameCurrentGroup);
    connect(deleteGroupButton, &QPushButton::clicked, this, &MainWindow::deleteCurrentGroup);
}

void MainWindow::setupTray()
{
    m_tray = new TrayController(this);
    m_tray->setGroups(m_config.groups);

    connect(m_tray, &TrayController::showRequested, this, &MainWindow::showFromTray);
    connect(m_tray, &TrayController::groupLaunchRequested, this, &MainWindow::launchGroupAt);
    connect(m_tray, &TrayController::quitRequested, this, [this]() {
        prepareQuit();
        qApp->quit();
    });
    connect(m_tray, &TrayController::autostartToggleRequested, this, &MainWindow::applyAutostart);
    connect(m_tray, &TrayController::messageClicked, this, &MainWindow::showLastResults);
}

int MainWindow::currentGroupIndex() const
{
    for (int i = 0; i < m_config.groups.size(); ++i) {
        if (m_config.groups.at(i).id == m_currentGroupId) {
            return i;
        }
    }
    if (!m_config.groups.isEmpty()) {
        return 0;
    }
    return -1;
}

void MainWindow::refreshGroups()
{
    const QUuid previous = m_currentGroupId;
    int row = 0;
    {
        const QSignalBlocker blocker(m_groupList);
        m_groupList->clear();
        for (const Group &group : m_config.groups) {
            m_groupList->addItem(group.name);
        }
        if (!m_config.groups.isEmpty()) {
            for (int i = 0; i < m_config.groups.size(); ++i) {
                if (m_config.groups.at(i).id == previous) {
                    row = i;
                    break;
                }
            }
            m_groupList->setCurrentRow(row);
            m_currentGroupId = m_config.groups.at(row).id;
        } else {
            m_currentGroupId = QUuid();
        }
    }

    m_tray->setGroups(m_config.groups);
    m_launchButton->setEnabled(!m_config.groups.isEmpty());
    refreshItems();
}

void MainWindow::refreshItems()
{
    m_itemTable->clearContents();
    m_itemTable->setRowCount(0);

    const int groupIndex = currentGroupIndex();
    if (groupIndex < 0) {
        return;
    }

    const Group &group = m_config.groups.at(groupIndex);
    m_itemTable->setRowCount(group.items.size());
    const QIcon warningIcon = style()->standardIcon(QStyle::SP_MessageBoxWarning);
    const QColor invalidColor(140, 140, 140);

    for (int row = 0; row < group.items.size(); ++row) {
        const Item &item = group.items.at(row);
        auto *nameItem = new QTableWidgetItem(item.name);
        auto *typeItem = new QTableWidgetItem(itemTypeLabel(item.type));
        auto *targetItem = new QTableWidgetItem(item.targetText());

        const PathStatus status = m_resolver.classify(item);
        if (status != PathStatus::Valid) {
            const QString reason = PathResolver::pathStatusText(status);
            nameItem->setIcon(warningIcon);
            nameItem->setForeground(invalidColor);
            typeItem->setForeground(invalidColor);
            targetItem->setForeground(invalidColor);
            nameItem->setToolTip(reason);
            targetItem->setToolTip(reason);
        } else {
            targetItem->setToolTip(m_resolver.expand(item.targetText()));
        }

        m_itemTable->setItem(row, 0, nameItem);
        m_itemTable->setItem(row, 1, typeItem);
        m_itemTable->setItem(row, 2, targetItem);
    }
}

void MainWindow::addItem()
{
    const int groupIndex = currentGroupIndex();
    if (groupIndex < 0) {
        return;
    }

    QStringList labels;
    for (int i = static_cast<int>(ItemType::Directory);
         i <= static_cast<int>(ItemType::Command); ++i) {
        labels << itemTypeLabel(static_cast<ItemType>(i));
    }
    bool ok = false;
    const QString chosen = QInputDialog::getItem(this, QStringLiteral("添加条目"),
                                                 QStringLiteral("选择条目类型："), labels, 0,
                                                 false, &ok);
    if (!ok) {
        return;
    }
    const int typeIndex = labels.indexOf(chosen);
    if (typeIndex < 0) {
        return;
    }

    Item item = Item::make(static_cast<ItemType>(typeIndex));
    ItemDialog dialog(m_browserCatalog.detect(), this);
    dialog.setWindowTitle(QStringLiteral("添加条目"));
    dialog.setItem(item);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    m_config.groups[groupIndex].items.append(dialog.item());
    refreshItems();
    scheduleSave();
}

void MainWindow::editItemAt(int row)
{
    const int groupIndex = currentGroupIndex();
    if (groupIndex < 0 || row < 0 || row >= m_config.groups.at(groupIndex).items.size()) {
        return;
    }
    ItemDialog dialog(m_browserCatalog.detect(), this);
    dialog.setWindowTitle(QStringLiteral("编辑条目"));
    dialog.setItem(m_config.groups.at(groupIndex).items.at(row));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    m_config.groups[groupIndex].items[row] = dialog.item();
    refreshItems();
    scheduleSave();
}

void MainWindow::deleteItemAt(int row)
{
    const int groupIndex = currentGroupIndex();
    if (groupIndex < 0 || row < 0 || row >= m_config.groups.at(groupIndex).items.size()) {
        return;
    }
    const Item &item = m_config.groups.at(groupIndex).items.at(row);
    if (QMessageBox::question(this, QStringLiteral("删除条目"),
                              QStringLiteral("确定删除条目「%1」吗？").arg(item.name))
        != QMessageBox::Yes) {
        return;
    }
    m_config.groups[groupIndex].items.removeAt(row);
    refreshItems();
    scheduleSave();
}

void MainWindow::relocateItemAt(int row)
{
    const int groupIndex = currentGroupIndex();
    if (groupIndex < 0 || row < 0 || row >= m_config.groups.at(groupIndex).items.size()) {
        return;
    }
    Item &item = m_config.groups[groupIndex].items[row];
    QString selected;
    if (item.type == ItemType::Directory) {
        selected = QFileDialog::getExistingDirectory(this, QStringLiteral("重新定位目录"),
                                                     item.path);
    } else if (item.type == ItemType::Application) {
        selected = QFileDialog::getOpenFileName(
            this, QStringLiteral("重新定位应用程序"), item.path,
            QStringLiteral("可执行文件 (*.exe);;所有文件 (*)"));
    } else {
        selected = QFileDialog::getOpenFileName(this, QStringLiteral("重新定位文件"), item.path);
    }
    if (selected.isEmpty()) {
        return;
    }
    item.path = QDir::toNativeSeparators(selected);
    refreshItems();
    scheduleSave();
}

void MainWindow::launchItemAtRow(int row)
{
    const int groupIndex = currentGroupIndex();
    if (groupIndex < 0 || row < 0 || row >= m_config.groups.at(groupIndex).items.size()) {
        return;
    }
    const Item item = m_config.groups.at(groupIndex).items.at(row);
    const LaunchPlan plan = m_planner.plan(item, m_forceCheck->isChecked());
    const QString actionName = QStringLiteral("打开条目「%1」").arg(item.name);
    const QVector<LaunchResult> results = m_runner.run(plan, actionName);
    handleResults(actionName, results);
}

void MainWindow::createGroup()
{
    bool ok = false;
    const QString name = QInputDialog::getText(this, QStringLiteral("新建组"),
                                               QStringLiteral("组名称："), QLineEdit::Normal,
                                               QStringLiteral("新组"), &ok)
                             .trimmed();
    if (!ok || name.isEmpty()) {
        return;
    }
    Group group = Group::makeDefault();
    group.name = name;
    m_config.groups.append(group);
    m_currentGroupId = group.id;
    refreshGroups();
    scheduleSave();
}

void MainWindow::renameCurrentGroup()
{
    const int groupIndex = currentGroupIndex();
    if (groupIndex < 0) {
        return;
    }
    bool ok = false;
    const QString name = QInputDialog::getText(
                             this, QStringLiteral("重命名组"), QStringLiteral("组名称："),
                             QLineEdit::Normal, m_config.groups.at(groupIndex).name, &ok)
                             .trimmed();
    if (!ok || name.isEmpty()) {
        return;
    }
    m_config.groups[groupIndex].name = name;
    refreshGroups();
    scheduleSave();
}

void MainWindow::deleteCurrentGroup()
{
    const int groupIndex = currentGroupIndex();
    if (groupIndex < 0) {
        return;
    }
    if (m_config.groups.size() <= 1) {
        QMessageBox::information(this, QStringLiteral("删除组"),
                                 QStringLiteral("至少需要保留一个组。"));
        return;
    }
    const Group &group = m_config.groups.at(groupIndex);
    if (QMessageBox::question(this, QStringLiteral("删除组"),
                              QStringLiteral("确定删除组「%1」吗？").arg(group.name))
        != QMessageBox::Yes) {
        return;
    }
    if (!group.items.isEmpty()) {
        if (QMessageBox::question(
                this, QStringLiteral("删除组"),
                QStringLiteral("该组包含 %1 个条目，删除后不可恢复。确认删除？")
                    .arg(group.items.size()))
            != QMessageBox::Yes) {
            return;
        }
    }
    m_config.groups.removeAt(groupIndex);
    const int nextIndex = qMin(groupIndex, m_config.groups.size() - 1);
    m_currentGroupId = m_config.groups.at(nextIndex).id;
    refreshGroups();
    scheduleSave();
}

void MainWindow::moveGroup(int from, int to)
{
    if (from < 0 || from >= m_config.groups.size() || to < 0 || to > m_config.groups.size()) {
        return;
    }
    Group group = m_config.groups.takeAt(from);
    if (to > m_config.groups.size()) {
        to = m_config.groups.size();
    }
    m_config.groups.insert(to, group);
    refreshGroups();
    scheduleSave();
}

void MainWindow::moveItem(int from, int to)
{
    const int groupIndex = currentGroupIndex();
    if (groupIndex < 0) {
        return;
    }
    QVector<Item> &items = m_config.groups[groupIndex].items;
    if (from < 0 || from >= items.size() || to < 0 || to > items.size()) {
        return;
    }
    Item item = items.takeAt(from);
    if (to > items.size()) {
        to = items.size();
    }
    items.insert(to, item);
    refreshItems();
    scheduleSave();
}

void MainWindow::addDroppedFiles(const QList<QUrl> &urls)
{
    if (m_elevated) {
        return;
    }
    const int groupIndex = currentGroupIndex();
    if (groupIndex < 0) {
        return;
    }
    int added = 0;
    for (const QUrl &url : urls) {
        if (!url.isLocalFile()) {
            continue;
        }
        const QFileInfo info(url.toLocalFile());
        if (!info.exists()) {
            continue;
        }
        Item item = Item::make(info.isDir() ? ItemType::Directory : ItemType::File);
        item.path = QDir::toNativeSeparators(info.absoluteFilePath());
        item.name = info.fileName();
        m_config.groups[groupIndex].items.append(item);
        ++added;
    }
    if (added > 0) {
        refreshItems();
        scheduleSave();
    }
}

void MainWindow::launchCurrentGroup()
{
    const int groupIndex = currentGroupIndex();
    if (groupIndex < 0) {
        return;
    }
    launchGroup(m_config.groups.at(groupIndex));
}

void MainWindow::launchGroupAt(int index)
{
    if (index < 0 || index >= m_config.groups.size()) {
        return;
    }
    m_currentGroupId = m_config.groups.at(index).id;
    refreshGroups();
    launchGroup(m_config.groups.at(index));
}

void MainWindow::launchGroup(const Group &group)
{
    if (group.items.isEmpty()) {
        statusBar()->showMessage(QStringLiteral("「%1」组没有条目").arg(group.name), 8000);
        m_tray->showMessage(QStringLiteral("WinTools"),
                            QStringLiteral("「%1」组没有条目").arg(group.name));
        return;
    }
    const LaunchPlan plan = m_planner.plan(group, m_forceCheck->isChecked());
    const QString actionName = QStringLiteral("打开组「%1」").arg(group.name);
    const QVector<LaunchResult> results = m_runner.run(plan, actionName);
    handleResults(actionName, results);
}

void MainWindow::handleResults(const QString &actionName, const QVector<LaunchResult> &results)
{
    int success = 0;
    int skipped = 0;
    int failed = 0;
    int invalid = 0;
    for (const LaunchResult &result : results) {
        switch (result.status) {
        case LaunchStatus::Success:
            ++success;
            break;
        case LaunchStatus::Skipped:
            ++skipped;
            break;
        case LaunchStatus::Failed:
            ++failed;
            break;
        case LaunchStatus::Invalid:
            ++invalid;
            break;
        }
    }

    const QString summary = QStringLiteral("成功 %1，跳过 %2，失败 %3，无效 %4")
                                .arg(success)
                                .arg(skipped)
                                .arg(failed)
                                .arg(invalid);
    statusBar()->showMessage(actionName + QStringLiteral("：") + summary, 8000);
    m_tray->showMessage(actionName, summary);

    m_lastResults = results;
    m_lastActionName = actionName;
}

void MainWindow::showLastResults()
{
    if (m_lastResults.isEmpty()) {
        return;
    }
    if (m_resultDialog) {
        m_resultDialog->close();
    }
    auto *dialog = new LaunchResultDialog(m_lastActionName, m_lastResults, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    m_resultDialog = dialog;
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}

void MainWindow::showGroupContextMenu(const QPoint &pos)
{
    const QModelIndex index = m_groupList->indexAt(pos);
    if (!index.isValid()) {
        return;
    }
    m_groupList->setCurrentRow(index.row());

    QMenu menu(this);
    QAction *renameAction = menu.addAction(QStringLiteral("重命名"));
    QAction *deleteAction = menu.addAction(QStringLiteral("删除"));
    QAction *chosen = menu.exec(m_groupList->viewport()->mapToGlobal(pos));
    if (chosen == renameAction) {
        renameCurrentGroup();
    } else if (chosen == deleteAction) {
        deleteCurrentGroup();
    }
}

void MainWindow::showItemContextMenu(const QPoint &pos)
{
    const int row = m_itemTable->rowAt(pos.y());
    if (row < 0) {
        return;
    }
    const int groupIndex = currentGroupIndex();
    if (groupIndex < 0 || row >= m_config.groups.at(groupIndex).items.size()) {
        return;
    }

    const Item &item = m_config.groups.at(groupIndex).items.at(row);
    QMenu menu(this);
    QAction *openAction = menu.addAction(QStringLiteral("打开"));
    QAction *editAction = menu.addAction(QStringLiteral("编辑"));
    QAction *relocateAction = menu.addAction(QStringLiteral("重新定位"));
    QAction *deleteAction = menu.addAction(QStringLiteral("删除"));
    relocateAction->setEnabled(item.type == ItemType::Directory
                               || item.type == ItemType::Application
                               || item.type == ItemType::File);

    QAction *chosen = menu.exec(m_itemTable->viewport()->mapToGlobal(pos));
    if (chosen == openAction) {
        launchItemAtRow(row);
    } else if (chosen == editAction) {
        editItemAt(row);
    } else if (chosen == relocateAction) {
        relocateItemAt(row);
    } else if (chosen == deleteAction) {
        deleteItemAt(row);
    }
}

void MainWindow::openSettings()
{
    SettingsDialog dialog(&m_autostartManager, m_config.settings,
                          m_logger ? m_logger->directory() : QString(), this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    m_config.settings = dialog.result();
    scheduleSave();
    syncAutostartFromSystem(false);
}

void MainWindow::applyAutostart(bool enabled)
{
    QString mode = m_config.settings.autostartMode;
    if (mode.isEmpty()) {
        mode = QStringLiteral("normal");
    }

    QString error;
    if (!m_autostartManager.apply(enabled, mode, &error)) {
        QMessageBox::warning(this, QStringLiteral("开机自启"), error);
        if (m_logger) {
            m_logger->log(QStringLiteral("开机自启设置失败：%1").arg(error));
        }
        syncAutostartFromSystem(false);
        return;
    }

    m_config.settings.autostart = enabled;
    scheduleSave();
    syncAutostartFromSystem(false);
}

void MainWindow::syncAutostartFromSystem(bool updateConfig)
{
    const AutostartManager::State state = m_autostartManager.query();
    const bool enabled = state.any();
    const QString mode = state.task ? QStringLiteral("admin") : QStringLiteral("normal");
    m_tray->setAutostartChecked(enabled);

    if (updateConfig
        && (m_config.settings.autostart != enabled
            || m_config.settings.autostartMode != mode)) {
        m_config.settings.autostart = enabled;
        m_config.settings.autostartMode = mode;
        scheduleSave();
    }
}

void MainWindow::scheduleSave()
{
    if (m_saveTimer) {
        m_saveTimer->start();
    }
}

void MainWindow::saveNow()
{
    if (m_saveTimer) {
        m_saveTimer->stop();
    }
    QString error;
    if (!m_store->save(m_config, &error)) {
        if (m_logger) {
            m_logger->log(QStringLiteral("保存配置失败：%1").arg(error));
        }
        if (!m_saveFailedShown) {
            m_saveFailedShown = true;
            QMessageBox::warning(this, QStringLiteral("保存配置"),
                                 QStringLiteral("配置保存失败：%1").arg(error));
        }
    }
}

void MainWindow::showFromTray()
{
    showNormal();
    raise();
    activateWindow();
}

void MainWindow::notifyRunningInTray()
{
    m_tray->showMessage(QStringLiteral("WinTools"), QStringLiteral("已在系统托盘运行。"));
}

void MainWindow::prepareQuit()
{
    m_quitting = true;
    saveNow();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_quitting) {
        saveNow();
        event->accept();
        return;
    }
    event->ignore();
    hide();
    if (!m_trayNotified) {
        m_trayNotified = true;
        m_tray->showMessage(QStringLiteral("WinTools"),
                            QStringLiteral("已最小化到托盘，双击托盘图标可重新打开。"));
    }
}
