#ifndef WINTOOLS_UI_LAUNCH_RESULT_DIALOG_H
#define WINTOOLS_UI_LAUNCH_RESULT_DIALOG_H

#include "domain/launch_runner.h"

#include <QDialog>
#include <QVector>

class LaunchResultDialog : public QDialog
{
    Q_OBJECT

public:
    LaunchResultDialog(const QString &title, const QVector<LaunchResult> &results,
                       QWidget *parent = nullptr);
};

#endif // WINTOOLS_UI_LAUNCH_RESULT_DIALOG_H
