#include "ui/mode_dialog.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

ModeDialog::ModeDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("选择运行模式"));
    setModal(true);

    auto *layout = new QVBoxLayout(this);
    auto *label = new QLabel(QStringLiteral("请选择本次运行的权限模式："), this);
    layout->addWidget(label);

    m_normalRadio = new QRadioButton(QStringLiteral("普通模式"), this);
    m_adminRadio = new QRadioButton(QStringLiteral("管理员模式（会弹出 UAC 提示）"), this);
    m_normalRadio->setChecked(true);
    layout->addWidget(m_normalRadio);
    layout->addWidget(m_adminRadio);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("启动"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("退出"));
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    resize(360, sizeHint().height());
}

ModeDialog::Mode ModeDialog::selectedMode() const
{
    return m_adminRadio->isChecked() ? Mode::Admin : Mode::Normal;
}
