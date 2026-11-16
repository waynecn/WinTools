#include "ui/item_dialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace {

QWidget *pathRow(QLineEdit *edit, QPushButton *button)
{
    auto *row = new QWidget();
    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(edit);
    layout->addWidget(button);
    return row;
}

} // namespace

ItemDialog::ItemDialog(const QVector<BrowserInfo> &browsers, QWidget *parent)
    : QDialog(parent)
    , m_browsers(browsers)
{
    setWindowTitle(QStringLiteral("条目"));
    resize(560, 280);

    auto *layout = new QVBoxLayout(this);
    auto *topForm = new QFormLayout();

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText(QStringLiteral("留空则自动生成"));
    topForm->addRow(QStringLiteral("名称："), m_nameEdit);

    m_typeCombo = new QComboBox(this);
    for (int i = static_cast<int>(ItemType::Directory);
         i <= static_cast<int>(ItemType::Command); ++i) {
        const ItemType type = static_cast<ItemType>(i);
        m_typeCombo->addItem(itemTypeLabel(type), i);
    }
    topForm->addRow(QStringLiteral("类型："), m_typeCombo);
    layout->addLayout(topForm);

    m_stack = new QStackedWidget(this);

    // 页面 0：目录 / 应用 / 文件
    auto *pathPage = new QWidget(this);
    auto *pathForm = new QFormLayout(pathPage);
    m_pathEdit = new QLineEdit(pathPage);
    m_pathBrowse = new QPushButton(QStringLiteral("浏览…"), pathPage);
    pathForm->addRow(QStringLiteral("路径："), pathRow(m_pathEdit, m_pathBrowse));
    auto *pathHint = new QLabel(QStringLiteral("路径支持 %USERPROFILE% 等环境变量。"), pathPage);
    pathHint->setStyleSheet(QStringLiteral("color:#666;"));
    pathForm->addRow(QString(), pathHint);
    m_stack->addWidget(pathPage);

    // 页面 1：URL
    auto *urlPage = new QWidget(this);
    auto *urlForm = new QFormLayout(urlPage);
    m_urlEdit = new QLineEdit(urlPage);
    m_urlEdit->setPlaceholderText(QStringLiteral("https://example.com"));
    urlForm->addRow(QStringLiteral("URL："), m_urlEdit);

    m_browserCombo = new QComboBox(urlPage);
    urlForm->addRow(QStringLiteral("浏览器："), m_browserCombo);

    m_browserPathRow = new QWidget(urlPage);
    auto *browserRowLayout = new QHBoxLayout(m_browserPathRow);
    browserRowLayout->setContentsMargins(0, 0, 0, 0);
    m_browserPathEdit = new QLineEdit(m_browserPathRow);
    auto *browserBrowse = new QPushButton(QStringLiteral("浏览…"), m_browserPathRow);
    browserRowLayout->addWidget(m_browserPathEdit);
    browserRowLayout->addWidget(browserBrowse);
    urlForm->addRow(QStringLiteral("浏览器路径："), m_browserPathRow);
    m_stack->addWidget(urlPage);

    // 页面 2：命令行
    auto *commandPage = new QWidget(this);
    auto *commandForm = new QFormLayout(commandPage);
    m_commandEdit = new QLineEdit(commandPage);
    m_commandEdit->setPlaceholderText(QStringLiteral("例如 npm run dev"));
    commandForm->addRow(QStringLiteral("命令："), m_commandEdit);

    m_workingDirEdit = new QLineEdit(commandPage);
    m_workingDirEdit->setPlaceholderText(QStringLiteral("留空使用用户主目录"));
    auto *workingDirBrowse = new QPushButton(QStringLiteral("浏览…"), commandPage);
    commandForm->addRow(QStringLiteral("工作目录："), pathRow(m_workingDirEdit, workingDirBrowse));

    m_keepConsoleCheck = new QCheckBox(QStringLiteral("保留窗口（cmd /k）"), commandPage);
    commandForm->addRow(QString(), m_keepConsoleCheck);
    m_stack->addWidget(commandPage);

    layout->addWidget(m_stack);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &ItemDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { updateTypePage(); });
    connect(m_pathBrowse, &QPushButton::clicked, this, &ItemDialog::browseTargetPath);
    connect(browserBrowse, &QPushButton::clicked, this, &ItemDialog::browseBrowserPath);
    connect(workingDirBrowse, &QPushButton::clicked, this, &ItemDialog::browseWorkingDirectory);
    connect(m_browserCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { updateBrowserRows(); });

    m_browserCombo->addItem(QStringLiteral("系统默认"), QStringLiteral("default"));
    for (const BrowserInfo &browser : m_browsers) {
        m_browserCombo->addItem(browser.name, browser.executablePath);
    }
    m_browserCombo->addItem(QStringLiteral("自定义…"), QStringLiteral("custom"));

    updateTypePage();
    updateBrowserRows();
}

void ItemDialog::setItem(const Item &item)
{
    m_item = item;
    m_nameEdit->setText(item.name);
    m_typeCombo->setCurrentIndex(static_cast<int>(item.type));
    m_pathEdit->setText(item.path);
    m_urlEdit->setText(item.url);
    m_commandEdit->setText(item.command);
    m_workingDirEdit->setText(item.workingDir);
    m_keepConsoleCheck->setChecked(item.keepConsole);

    int browserIndex = 0;
    if (item.browser.mode != BrowserMode::SystemDefault) {
        browserIndex = -1;
        for (int i = 0; i < m_browserCombo->count(); ++i) {
            if (m_browserCombo->itemData(i).toString() == item.browser.programPath) {
                browserIndex = i;
                break;
            }
        }
        if (browserIndex < 0) {
            browserIndex = m_browserCombo->count() - 1; // 自定义…
            m_browserPathEdit->setText(item.browser.programPath);
        }
    }
    m_browserCombo->setCurrentIndex(browserIndex);

    updateTypePage();
    updateBrowserRows();
}

Item ItemDialog::item() const
{
    return m_item;
}

ItemType ItemDialog::currentType() const
{
    return static_cast<ItemType>(m_typeCombo->currentData().toInt());
}

void ItemDialog::updateTypePage()
{
    switch (currentType()) {
    case ItemType::Directory:
    case ItemType::Application:
    case ItemType::File:
        m_stack->setCurrentIndex(0);
        if (currentType() == ItemType::Directory) {
            m_pathBrowse->setText(QStringLiteral("浏览文件夹…"));
        } else {
            m_pathBrowse->setText(QStringLiteral("浏览文件…"));
        }
        break;
    case ItemType::Url:
        m_stack->setCurrentIndex(1);
        break;
    case ItemType::Command:
        m_stack->setCurrentIndex(2);
        break;
    }
}

void ItemDialog::updateBrowserRows()
{
    const bool custom = m_browserCombo->currentData().toString() == QLatin1String("custom");
    m_browserPathRow->setVisible(custom);
}

void ItemDialog::browseTargetPath()
{
    QString selected;
    if (currentType() == ItemType::Directory) {
        selected = QFileDialog::getExistingDirectory(this, QStringLiteral("选择目录"), m_pathEdit->text());
    } else if (currentType() == ItemType::Application) {
        selected = QFileDialog::getOpenFileName(
            this, QStringLiteral("选择应用程序"), m_pathEdit->text(),
            QStringLiteral("可执行文件 (*.exe);;所有文件 (*)"));
    } else {
        selected = QFileDialog::getOpenFileName(this, QStringLiteral("选择文件"), m_pathEdit->text());
    }
    if (!selected.isEmpty()) {
        m_pathEdit->setText(selected);
    }
}

void ItemDialog::browseBrowserPath()
{
    const QString selected = QFileDialog::getOpenFileName(
        this, QStringLiteral("选择浏览器"), m_browserPathEdit->text(),
        QStringLiteral("可执行文件 (*.exe);;所有文件 (*)"));
    if (!selected.isEmpty()) {
        m_browserPathEdit->setText(selected);
    }
}

void ItemDialog::browseWorkingDirectory()
{
    const QString selected = QFileDialog::getExistingDirectory(
        this, QStringLiteral("选择工作目录"), m_workingDirEdit->text());
    if (!selected.isEmpty()) {
        m_workingDirEdit->setText(selected);
    }
}

void ItemDialog::accept()
{
    const ItemType type = currentType();
    Item result;
    result.id = m_item.id.isNull() ? QUuid::createUuid() : m_item.id;
    result.type = type;
    result.name = m_nameEdit->text().trimmed();

    const QString path = m_pathEdit->text().trimmed();
    if (type == ItemType::Directory || type == ItemType::Application || type == ItemType::File) {
        if (path.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("条目"), QStringLiteral("请填写路径。"));
            m_pathEdit->setFocus();
            return;
        }
        result.path = path;
    } else if (type == ItemType::Url) {
        result.url = m_urlEdit->text().trimmed();
        if (result.url.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("条目"), QStringLiteral("请填写 URL。"));
            m_urlEdit->setFocus();
            return;
        }
        const QString data = m_browserCombo->currentData().toString();
        if (data == QLatin1String("default")) {
            result.browser.mode = BrowserMode::SystemDefault;
        } else if (data == QLatin1String("custom")) {
            result.browser.mode = BrowserMode::Custom;
            result.browser.programPath = m_browserPathEdit->text().trimmed();
            if (result.browser.programPath.isEmpty()) {
                QMessageBox::warning(this, QStringLiteral("条目"), QStringLiteral("请填写浏览器路径。"));
                m_browserPathEdit->setFocus();
                return;
            }
        } else {
            result.browser.mode = BrowserMode::Detected;
            result.browser.programPath = data;
        }
    } else if (type == ItemType::Command) {
        result.command = m_commandEdit->text().trimmed();
        if (result.command.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("条目"), QStringLiteral("请填写命令行。"));
            m_commandEdit->setFocus();
            return;
        }
        result.workingDir = m_workingDirEdit->text().trimmed();
        result.keepConsole = m_keepConsoleCheck->isChecked();
    }

    if (result.name.isEmpty()) {
        result.name = result.suggestedName();
    }
    if (result.name.isEmpty()) {
        result.name = QStringLiteral("未命名条目");
    }

    m_item = result;
    QDialog::accept();
}
