#ifndef WINTOOLS_UI_ITEM_DIALOG_H
#define WINTOOLS_UI_ITEM_DIALOG_H

#include "domain/browser_catalog.h"
#include "domain/item.h"

#include <QDialog>
#include <QVector>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QPushButton;
class QStackedWidget;
class QWidget;

class ItemDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ItemDialog(const QVector<BrowserInfo> &browsers, QWidget *parent = nullptr);

    void setItem(const Item &item);
    Item item() const;

protected:
    void accept() override;

private:
    ItemType currentType() const;
    void updateTypePage();
    void updateBrowserRows();
    void browseTargetPath();
    void browseBrowserPath();
    void browseWorkingDirectory();

    QVector<BrowserInfo> m_browsers;
    Item m_item;

    QLineEdit *m_nameEdit;
    QComboBox *m_typeCombo;
    QStackedWidget *m_stack;

    QLineEdit *m_pathEdit;
    QPushButton *m_pathBrowse;

    QLineEdit *m_urlEdit;
    QComboBox *m_browserCombo;
    QWidget *m_browserPathRow;
    QLineEdit *m_browserPathEdit;

    QLineEdit *m_commandEdit;
    QLineEdit *m_workingDirEdit;
    QCheckBox *m_keepConsoleCheck;
};

#endif // WINTOOLS_UI_ITEM_DIALOG_H
