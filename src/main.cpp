#include "domain/config_store.h"
#include "domain/logger.h"
#include "platform/win/elevation.h"
#include "platform/win/icon_factory.h"
#include "platform/win/single_instance.h"
#include "ui/main_window.h"
#include "ui/mode_dialog.h"

#include <QApplication>
#include <QMessageBox>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("WinTools"));
    QApplication::setApplicationName(QStringLiteral("WinTools"));
    QApplication::setApplicationVersion(QStringLiteral("1.0.0"));
    QApplication::setQuitOnLastWindowClosed(false);
    QApplication::setWindowIcon(IconFactory::applicationIcon());

    bool elevatedFlag = false;
    bool autostartFlag = false;
    for (int i = 1; i < argc; ++i) {
        const QString argument = QString::fromLocal8Bit(argv[i]);
        if (argument == QLatin1String("--elevated")) {
            elevatedFlag = true;
        } else if (argument == QLatin1String("--autostart")) {
            autostartFlag = true;
        }
    }

    SingleInstance instance;
    QString instanceError;
    if (!instance.acquire(&instanceError)) {
        if (!instance.notifyExisting()) {
            QMessageBox::information(
                nullptr, QStringLiteral("WinTools"),
                QStringLiteral("已有 WinTools 实例在运行（可能以管理员身份）。"));
        }
        return 0;
    }

    const bool alreadyElevated = isCurrentProcessElevated();
    if (!elevatedFlag && !autostartFlag && !alreadyElevated) {
        ModeDialog dialog;
        if (dialog.exec() != QDialog::Accepted) {
            return 0;
        }
        if (dialog.selectedMode() == ModeDialog::Mode::Admin) {
            instance.release();
            QString error;
            if (!relaunchElevated({QStringLiteral("--elevated")}, &error)) {
                QMessageBox::critical(
                    nullptr, QStringLiteral("WinTools"),
                    QStringLiteral("无法以管理员身份启动：%1").arg(error));
            }
            return 0;
        }
    }

    const QString baseDir = QCoreApplication::applicationDirPath();
    ConfigStore store(baseDir + QStringLiteral("/config.json"));
    const ConfigStore::LoadResult loaded = store.load();
    if (!loaded.ok) {
        QMessageBox::critical(nullptr, QStringLiteral("WinTools"), loaded.message);
        return 0;
    }
    if (!loaded.message.isEmpty()) {
        QMessageBox::warning(nullptr, QStringLiteral("WinTools"), loaded.message);
    }

    SystemClock clock;
    Logger logger(baseDir + QStringLiteral("/logs"), &clock);
    logger.pruneOldLogs(30);

    MainWindow window(&store, loaded.config, alreadyElevated, &logger);
    QObject::connect(&instance, &SingleInstance::activationRequested,
                     &window, &MainWindow::showFromTray);

    if (autostartFlag) {
        window.hide();
        QTimer::singleShot(0, &window, &MainWindow::notifyRunningInTray);
    } else {
        window.show();
    }

    return app.exec();
}
