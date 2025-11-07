#include "notificationmanager.h"
#include <QDebug>

NotificationManager::NotificationManager(QObject *parent)
    : QObject(parent)
    , m_enabled(true)
{
}

NotificationManager::~NotificationManager()
{
    clearNotifications();
}

void NotificationManager::setEnabled(bool enabled)
{
    if (m_enabled != enabled) {
        m_enabled = enabled;
        emit enabledChanged();

        if (!m_enabled) {
            clearNotifications();
        }
    }
}

void NotificationManager::showMessageNotification(const QString &channelName,
                                                  const QString &userName,
                                                  const QString &messageText,
                                                  const QString &channelId)
{
    if (!m_enabled) {
        return;
    }

    QString summary = QString("%1 in #%2").arg(userName, channelName);
    showNotification(summary, messageText, channelId, false);
}

void NotificationManager::showMentionNotification(const QString &channelName,
                                                 const QString &userName,
                                                 const QString &messageText,
                                                 const QString &channelId)
{
    if (!m_enabled) {
        return;
    }

    QString summary = QString("@Mention from %1 in #%2").arg(userName, channelName);
    showNotification(summary, messageText, channelId, true);
}

void NotificationManager::clearNotifications()
{
    for (Notification *notification : m_activeNotifications.values()) {
        notification->close();
        notification->deleteLater();
    }
    m_activeNotifications.clear();
}

void NotificationManager::clearChannelNotifications(const QString &channelId)
{
    if (m_activeNotifications.contains(channelId)) {
        Notification *notification = m_activeNotifications.value(channelId);
        notification->close();
        notification->deleteLater();
        m_activeNotifications.remove(channelId);
    }
}

void NotificationManager::handleNotificationClosed(uint reason)
{
    Q_UNUSED(reason);

    Notification *notification = qobject_cast<Notification*>(sender());
    if (notification) {
        QString channelId = notification->property("channelId").toString();
        m_activeNotifications.remove(channelId);
        notification->deleteLater();
    }
}

void NotificationManager::handleActionInvoked(const QString &action)
{
    Notification *notification = qobject_cast<Notification*>(sender());
    if (notification && action == "default") {
        QString channelId = notification->property("channelId").toString();
        emit notificationClicked(channelId);
    }
}

void NotificationManager::showNotification(const QString &summary,
                                          const QString &body,
                                          const QString &channelId,
                                          bool isMention)
{
    // Clear existing notification for this channel
    clearChannelNotifications(channelId);

    // Create new notification
    Notification *notification = new Notification(this);
    notification->setCategory("x-nemo.messaging.im");
    notification->setAppName("SlackShip");
    notification->setSummary(summary);
    notification->setBody(body);
    notification->setProperty("channelId", channelId);

    // Set icon
    notification->setAppIcon("harbour-lagoon");

    // Set urgency (higher for mentions)
    if (isMention) {
        notification->setUrgency(Notification::Critical);
    } else {
        notification->setUrgency(Notification::Normal);
    }

    // Set timestamp
    notification->setTimestamp(QDateTime::currentDateTime());

    // Connect signals
    connect(notification, &Notification::closed,
            this, &NotificationManager::handleNotificationClosed);
    connect(notification, &Notification::actionInvoked,
            this, &NotificationManager::handleActionInvoked);

    // Publish notification
    notification->publish();

    // Store for later reference
    m_activeNotifications.insert(channelId, notification);

    qDebug() << "Notification shown:" << summary << "-" << body;
}
