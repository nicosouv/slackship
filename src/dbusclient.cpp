#include "dbusclient.h"
#include <QDBusReply>
#include <QDBusConnectionInterface>
#include <QProcess>
#include <QTimer>
#include <QDebug>

DBusClient::DBusClient(QObject *parent)
    : QObject(parent)
    , m_interface(nullptr)
    , m_isDaemonRunning(false)
    , m_isConnected(false)
    , m_unreadCount(0)
{
    // Create D-Bus interface
    m_interface = new QDBusInterface("org.harbour.lagoon",
                                     "/org/harbour/lagoon",
                                     "org.harbour.lagoon",
                                     QDBusConnection::sessionBus(),
                                     this);

    // Check if daemon is already running
    checkDaemonStatus();

    // Connect to signals
    if (m_isDaemonRunning) {
        connectToSignals();
    }
}

DBusClient::~DBusClient()
{
}

void DBusClient::syncNow()
{
    if (!m_isDaemonRunning) {
        qWarning() << "Daemon not running, cannot sync";
        return;
    }

    qDebug() << "Calling SyncNow via D-Bus";
    m_interface->call("SyncNow");
}

void DBusClient::setWorkspace(const QString &workspaceId)
{
    if (!m_isDaemonRunning) {
        qWarning() << "Daemon not running, cannot set workspace";
        return;
    }

    qDebug() << "Calling SetWorkspace via D-Bus:" << workspaceId;
    m_interface->call("SetWorkspace", workspaceId);
}

void DBusClient::markChannelAsRead(const QString &channelId)
{
    if (!m_isDaemonRunning) {
        qWarning() << "Daemon not running, cannot mark as read";
        return;
    }

    qDebug() << "Calling MarkChannelAsRead via D-Bus:" << channelId;
    m_interface->call("MarkChannelAsRead", channelId);
}

void DBusClient::sendMessage(const QString &channelId, const QString &text)
{
    if (!m_isDaemonRunning) {
        qWarning() << "Daemon not running, cannot send message";
        return;
    }

    qDebug() << "Calling SendMessage via D-Bus:" << channelId;
    m_interface->call("SendMessage", channelId, text);
}

void DBusClient::startDaemon()
{
    qDebug() << "Starting daemon via systemd...";

    QProcess process;
    process.start("systemctl", QStringList() << "--user" << "start" << "harbour-lagoon-daemon.service");
    process.waitForFinished();

    if (process.exitCode() == 0) {
        qDebug() << "Daemon started successfully";
        // Wait a bit for daemon to initialize
        QTimer::singleShot(1000, this, SLOT(checkDaemonStatus()));
    } else {
        qWarning() << "Failed to start daemon:" << process.readAllStandardError();
    }
}

void DBusClient::checkDaemonStatus()
{
    QDBusConnectionInterface *interface = QDBusConnection::sessionBus().interface();
    bool running = interface->isServiceRegistered("org.harbour.lagoon");

    if (running != m_isDaemonRunning) {
        m_isDaemonRunning = running;
        emit daemonStatusChanged();

        if (m_isDaemonRunning) {
            qDebug() << "Daemon is running";
            connectToSignals();

            // Get initial state
            QDBusReply<bool> connectedReply = m_interface->call("IsConnected");
            if (connectedReply.isValid()) {
                m_isConnected = connectedReply.value();
                emit connectionStateChanged(m_isConnected);
            }

            QDBusReply<int> unreadReply = m_interface->call("GetUnreadCount");
            if (unreadReply.isValid()) {
                m_unreadCount = unreadReply.value();
                emit unreadCountChanged(m_unreadCount);
            }
        } else {
            qDebug() << "Daemon is not running";
        }
    }
}

void DBusClient::handleNewMessageReceived(const QString &channelId, const QString &messageJson)
{
    qDebug() << "UI received new message for channel:" << channelId;
    emit newMessageReceived(channelId, messageJson);
}

void DBusClient::handleUnreadCountChanged(int totalUnread)
{
    qDebug() << "UI received unread count update:" << totalUnread;
    m_unreadCount = totalUnread;
    emit unreadCountChanged(totalUnread);
}

void DBusClient::handleConnectionStateChanged(bool connected)
{
    qDebug() << "UI received connection state change:" << connected;
    m_isConnected = connected;
    emit connectionStateChanged(connected);
}

void DBusClient::handleSyncCompleted()
{
    qDebug() << "UI received sync completed signal";
    emit syncCompleted();
}

void DBusClient::connectToSignals()
{
    // Connect to daemon signals via D-Bus
    QDBusConnection::sessionBus().connect(
        "org.harbour.lagoon",
        "/org/harbour/lagoon",
        "org.harbour.lagoon",
        "NewMessageReceived",
        this,
        SLOT(handleNewMessageReceived(QString,QString))
    );

    QDBusConnection::sessionBus().connect(
        "org.harbour.lagoon",
        "/org/harbour/lagoon",
        "org.harbour.lagoon",
        "UnreadCountChanged",
        this,
        SLOT(handleUnreadCountChanged(int))
    );

    QDBusConnection::sessionBus().connect(
        "org.harbour.lagoon",
        "/org/harbour/lagoon",
        "org.harbour.lagoon",
        "ConnectionStateChanged",
        this,
        SLOT(handleConnectionStateChanged(bool))
    );

    QDBusConnection::sessionBus().connect(
        "org.harbour.lagoon",
        "/org/harbour/lagoon",
        "org.harbour.lagoon",
        "SyncCompleted",
        this,
        SLOT(handleSyncCompleted())
    );

    qDebug() << "Connected to daemon D-Bus signals";
}
