#include "dbusadaptor.h"
#include <QDebug>

DBusAdaptor::DBusAdaptor(SlackShipDaemon *daemon)
    : QDBusAbstractAdaptor(daemon)
    , m_daemon(daemon)
{
    // Connect daemon signals to D-Bus signals
    connect(m_daemon, &SlackShipDaemon::newMessageReceived,
            this, &DBusAdaptor::NewMessageReceived);
    connect(m_daemon, &SlackShipDaemon::unreadCountChanged,
            this, &DBusAdaptor::UnreadCountChanged);
    connect(m_daemon, &SlackShipDaemon::connectionStateChanged,
            this, &DBusAdaptor::ConnectionStateChanged);
    connect(m_daemon, &SlackShipDaemon::syncCompleted,
            this, &DBusAdaptor::SyncCompleted);
    connect(m_daemon, &SlackShipDaemon::userTyping,
            this, &DBusAdaptor::UserTyping);

    qDebug() << "D-Bus adaptor created";
}

void DBusAdaptor::SyncNow()
{
    qDebug() << "D-Bus: SyncNow called";
    m_daemon->syncNow();
}

void DBusAdaptor::SetWorkspace(const QString &workspaceId)
{
    qDebug() << "D-Bus: SetWorkspace called:" << workspaceId;
    m_daemon->setWorkspace(workspaceId);
}

void DBusAdaptor::MarkChannelAsRead(const QString &channelId)
{
    qDebug() << "D-Bus: MarkChannelAsRead called:" << channelId;
    m_daemon->markChannelAsRead(channelId);
}

void DBusAdaptor::SendMessage(const QString &channelId, const QString &text)
{
    qDebug() << "D-Bus: SendMessage called:" << channelId;
    m_daemon->sendMessageFromUI(channelId, text);
}

bool DBusAdaptor::IsConnected()
{
    return m_daemon->isConnected();
}

int DBusAdaptor::GetUnreadCount()
{
    return m_daemon->getTotalUnreadCount();
}
