#include "notificationmanager.h"
#include "settings/appsettings.h"
#include <QDebug>
#include <QTime>

NotificationManager::NotificationManager(QObject *parent)
    : QObject(parent)
    , m_enabled(true)
    , m_appSettings(nullptr)
{
}

void NotificationManager::setAppSettings(AppSettings *settings)
{
    m_appSettings = settings;
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

    // Check if we're in Do Not Disturb period
    if (isInDoNotDisturbPeriod()) {
        qDebug() << "DND active - suppressing message notification";
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

    // Check if we're in Do Not Disturb period
    if (isInDoNotDisturbPeriod()) {
        qDebug() << "DND active - suppressing mention notification";
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
    qDebug() << "=== NOTIFICATION ACTION INVOKED ===";
    qDebug() << "Action:" << action;

    Notification *notification = qobject_cast<Notification*>(sender());
    if (notification) {
        qDebug() << "Notification object valid";
        QString channelId = notification->property("channelId").toString();
        qDebug() << "Channel ID from property:" << channelId;

        if (!channelId.isEmpty()) {
            qDebug() << "Emitting notificationClicked signal with channelId:" << channelId;
            emit notificationClicked(channelId);
        } else {
            qDebug() << "WARNING: channelId is empty!";
        }
    } else {
        qDebug() << "WARNING: notification object is null!";
    }

    qDebug() << "=== END NOTIFICATION ACTION ===";
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
    // IMPORTANT: Use the .desktop file name (without .desktop extension)
    // This allows Sailfish OS to open the app when notification is clicked
    notification->setAppName("harbour-lagoon");
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

    // IMPORTANT: Store channelId as a Qt property so we can retrieve it in handleActionInvoked
    notification->setProperty("channelId", channelId);

    // Set up remote action for notification click
    // This uses DBus to call openChannel when the notification is clicked
    QVariantList remoteActionArguments;
    remoteActionArguments.append(channelId);

    qDebug() << "Setting remote action for channel:" << channelId;
    notification->setRemoteAction(
        Notification::remoteAction(
            "default",                  // action name
            "openChannel",             // display name
            "org.lagoon.slackship",    // DBus service name
            "/org/lagoon/slackship",   // DBus object path
            "org.lagoon.slackship",    // DBus interface
            "openChannel",             // DBus method to call
            remoteActionArguments      // Arguments to pass
        )
    );

    // Set urgency (higher for mentions)
    if (isMention) {
        notification->setUrgency(Notification::Critical);
    } else {
        notification->setUrgency(Notification::Normal);
    }

    // Set timestamp (current time - could be message timestamp if available)
    notification->setTimestamp(QDateTime::currentDateTime());

    // Connect signals
    // The notification is automatically clickable when appName matches the .desktop file
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

bool NotificationManager::isInDoNotDisturbPeriod() const
{
    // If no settings or DND is disabled, return false
    if (!m_appSettings || !m_appSettings->dndEnabled()) {
        return false;
    }

    QTime now = QTime::currentTime();
    QTime startTime(m_appSettings->dndStartHour(), m_appSettings->dndStartMinute());
    QTime endTime(m_appSettings->dndEndHour(), m_appSettings->dndEndMinute());

    // Handle case where DND period crosses midnight (e.g., 22:00 to 08:00)
    if (startTime > endTime) {
        // DND is active if current time is after start OR before end
        return now >= startTime || now < endTime;
    } else {
        // Normal case: DND is active if current time is between start and end
        return now >= startTime && now < endTime;
    }
}
