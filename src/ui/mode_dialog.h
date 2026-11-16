#ifndef WINTOOLS_UI_MODE_DIALOG_H
#define WINTOOLS_UI_MODE_DIALOG_H

#include <QDialog>

class QRadioButton;

class ModeDialog : public QDialog
{
    Q_OBJECT

public:
    enum class Mode
    {
        Normal,
        Admin
    };

    explicit ModeDialog(QWidget *parent = nullptr);

    Mode selectedMode() const;

private:
    QRadioButton *m_normalRadio;
    QRadioButton *m_adminRadio;
};

#endif // WINTOOLS_UI_MODE_DIALOG_H
