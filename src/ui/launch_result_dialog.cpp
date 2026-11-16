#include "ui/launch_result_dialog.h"

#include <QAbstractItemView>
#include <QColor>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QTableWidget>
#include <QVBoxLayout>

LaunchResultDialog::LaunchResultDialog(const QString &title, const QVector<LaunchResult> &results,
                                       QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(title);
    resize(680, 360);

    auto *layout = new QVBoxLayout(this);
    auto *table = new QTableWidget(results.size(), 3, this);
    table->setHorizontalHeaderLabels(
        {QStringLiteral("条目"), QStringLiteral("状态"), QStringLiteral("详情")});
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->horizontalHeader()->setStretchLastSection(true);

    for (int row = 0; row < results.size(); ++row) {
        const LaunchResult &result = results.at(row);
        auto *nameItem = new QTableWidgetItem(result.name);
        auto *statusItem = new QTableWidgetItem(launchStatusLabel(result.status));
        auto *detailItem = new QTableWidgetItem(result.message);
        detailItem->setToolTip(result.target + QLatin1Char('\n') + result.message);

        QColor color;
        switch (result.status) {
        case LaunchStatus::Success:
            color = QColor(30, 120, 60);
            break;
        case LaunchStatus::Skipped:
            color = QColor(140, 110, 20);
            break;
        case LaunchStatus::Failed:
            color = QColor(180, 40, 40);
            break;
        case LaunchStatus::Invalid:
            color = QColor(120, 120, 120);
            break;
        }
        nameItem->setForeground(color);
        statusItem->setForeground(color);

        table->setItem(row, 0, nameItem);
        table->setItem(row, 1, statusItem);
        table->setItem(row, 2, detailItem);
    }
    table->resizeColumnsToContents();

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);

    layout->addWidget(table);
    layout->addWidget(buttons);
}
