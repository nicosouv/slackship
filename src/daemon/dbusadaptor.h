#ifndef DBUSADAPTOR_H
#define DBUSADAPTOR_H

#include <QDBusAbstractAdaptor>
#include <QDBusVariant>
#include "slackshipdaemon.h"

class DBusAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.harbour.lagoon")

public:
    explicit DBusAdaptor(SlackShipDaemon *daemon);

public slots:
    // Methods exposed via D-Bus
    void SyncNow();
    void SetWorkspace(const QString &workspaceId);
    void MarkChannelAsRead(const QString &channelId);
    void SendMessage(const QString &channelId, const QString &text);

    // Status methods
    bool IsConnected();
    int GetUnreadCount();

signals:
    // Signals exposed via D-Bus
    void NewMessageReceived(const QString &channelId, const QString &messageJson);
    void UnreadCountChanged(int totalUnread);
    void ConnectionStateChanged(bool connected);
    void SyncCompleted();
    void UserTyping(const QString &channelId, const QString &userId);

private:
    SlackShipDaemon *m_daemon;
};

#endif // DBUSADAPTOR_H
