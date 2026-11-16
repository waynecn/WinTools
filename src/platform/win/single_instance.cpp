#include "platform/win/single_instance.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLockFile>

SingleInstance::SingleInstance(QObject *parent)
    : QObject(parent)
{
    const QByteArray hash = QCryptographicHash::hash(
        QCoreApplication::applicationDirPath().toUtf8(), QCryptographicHash::Sha1);
    m_serverName = QStringLiteral("WinTools-") + QString::fromLatin1(hash.toHex());
    m_lockPath = QDir::temp().filePath(m_serverName + QStringLiteral(".lock"));
}

SingleInstance::~SingleInstance()
{
    release();
}

bool SingleInstance::acquire(QString *error)
{
    if (error) {
        error->clear();
    }
    if (m_lock) {
        return true;
    }

    m_lock = new QLockFile(m_lockPath);
    m_lock->setStaleLockTime(30000);
    if (!m_lock->tryLock(0)) {
        delete m_lock;
        m_lock = nullptr;
        if (error) {
            *error = QStringLiteral("已有一个 WinTools 实例在运行");
        }
        return false;
    }

    // 拿到了锁，说明之前没有活着的实例；清掉可能残留的旧 socket 名称。
    QLocalServer::removeServer(m_serverName);
    m_server = new QLocalServer(this);
    if (!m_server->listen(m_serverName)) {
        if (error) {
            *error = m_server->errorString();
        }
        release();
        return false;
    }
    connect(m_server, &QLocalServer::newConnection, this, &SingleInstance::handleNewConnection);
    return true;
}

void SingleInstance::release()
{
    if (m_server) {
        m_server->close();
        delete m_server;
        m_server = nullptr;
    }
    if (m_lock) {
        m_lock->unlock();
        delete m_lock;
        m_lock = nullptr;
    }
}

bool SingleInstance::notifyExisting(int timeoutMs)
{
    QLocalSocket socket;
    socket.connectToServer(m_serverName);
    if (!socket.waitForConnected(timeoutMs)) {
        return false;
    }
    socket.write("show");
    socket.waitForBytesWritten(500);
    socket.disconnectFromServer();
    return true;
}

void SingleInstance::handleNewConnection()
{
    while (m_server && m_server->hasPendingConnections()) {
        QLocalSocket *socket = m_server->nextPendingConnection();
        if (socket) {
            connect(socket, &QLocalSocket::disconnected, socket, &QLocalSocket::deleteLater);
            socket->readAll();
        }
        emit activationRequested();
    }
}
