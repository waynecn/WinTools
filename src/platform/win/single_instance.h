#ifndef WINTOOLS_PLATFORM_SINGLE_INSTANCE_H
#define WINTOOLS_PLATFORM_SINGLE_INSTANCE_H

#include <QObject>
#include <QString>

class QLockFile;
class QLocalServer;

class SingleInstance : public QObject
{
    Q_OBJECT

public:
    explicit SingleInstance(QObject *parent = nullptr);
    ~SingleInstance() override;

    bool acquire(QString *error);
    void release();

    // 通知已运行实例显示窗口。失败通常意味着权限级别不同（UIPI 阻止连接）。
    bool notifyExisting(int timeoutMs = 2000);

signals:
    void activationRequested();

private slots:
    void handleNewConnection();

private:
    QString m_serverName;
    QString m_lockPath;
    QLockFile *m_lock = nullptr;
    QLocalServer *m_server = nullptr;
};

#endif // WINTOOLS_PLATFORM_SINGLE_INSTANCE_H
