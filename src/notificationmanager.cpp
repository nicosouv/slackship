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
    qDebug() << "=== SHOW NOTIFICATION CALLED ===";
    qDebug() << "Summary:" << summary;
    qDebug() << "Body:" << body;
    qDebug() << "Channel ID:" << channelId;
    qDebug() << "Is Mention:" << isMention;
    qDebug() << "Enabled:" << m_enabled;

    if (!m_enabled) {
        qDebug() << "Notifications disabled, returning early";
        return;
    }

    // Clear existing notification for this channel
    clearChannelNotifications(channelId);

    // Create new notification
    Notification *notification = new Notification(this);

    // Category for instant messaging
    notification->setCategory("x-nemo.messaging.im");
    notification->setAppName("Lagoon");
    notification->setAppIcon("harbour-lagoon");

    // Main notification content
    notification->setSummary(summary);
    notification->setBody(body);

    // Preview banner (shown immediately)
    notification->setPreviewSummary(summary);
    notification->setPreviewBody(body);

    // Item count for grouping (1 for now, could be extended later)
    notification->setItemCount(1);

    // CRITICAL SAILFISH OS HINTS - Based on Fernschreiber
    // Without these, notifications may not display properly
    notification->setHintValue("x-nemo-visibility", "public");  // Make notification visible
    notification->setHintValue("x-nemo-priority", 120);         // Messaging app priority
    notification->setHintValue("suppress-sound", false);        // Allow sound
    notification->setHintValue("x-nemo-display-on", true);      // Wake display
    notification->setHintValue("x-nemo-vibrate", true);         // Allow vibration

    // Store channel ID as hint (for restoration support)
    notification->setHintValue("x-slackship-channel-id", channelId);

    // Set urgency (higher for mentions)
    if (isMention) {
        notification->setUrgency(Notification::Critical);
    } else {
        notification->setUrgency(Notification::Normal);
    }

    // Set timestamp (current time - could be message timestamp if available)
    notification->setTimestamp(QDateTime::currentDateTime());

    // Add remote action for opening the channel - USING PROPER D-BUS FORMAT
    QVariantList args;
    args.append(channelId);

    notification->setRemoteAction(Notification::remoteAction(
        "default",                      // action name
        "openChannel",                  // method name
        "harbour.lagoon",               // D-Bus service (match app name)
        "/harbour/lagoon",              // object path
        "harbour.lagoon.Main",          // interface
        "openChannel",                  // method
        args));                         // arguments (channel ID)

    // Connect signals
    connect(notification, &Notification::closed,
            this, &NotificationManager::handleNotificationClosed);
    connect(notification, &Notification::actionInvoked,
            this, &NotificationManager::handleActionInvoked);

    // Publish notification
    qDebug() << "About to call notification->publish()";
    notification->publish();
    qDebug() << "notification->publish() called successfully";

    // Store for later reference
    m_activeNotifications.insert(channelId, notification);

    qDebug() << "Notification published:" << summary << "-" << body;
    qDebug() << "=== END SHOW NOTIFICATION ===";
}
