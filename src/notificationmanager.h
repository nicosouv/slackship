#ifndef NOTIFICATIONMANAGER_H
#define NOTIFICATIONMANAGER_H

#include <QObject>
#include <notification.h>
#include <QJsonObject>

class AppSettings;

class NotificationManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)

public:
    explicit NotificationManager(QObject *parent = nullptr);
    ~NotificationManager();

    void setAppSettings(AppSettings *settings);

    bool enabled() const { return m_enabled; }
    void setEnabled(bool enabled);

public slots:
    void showMessageNotification(const QString &channelName,
                                 const QString &userName,
                                 const QString &messageText,
                                 const QString &channelId);

    void showMentionNotification(const QString &channelName,
                                const QString &userName,
                                const QString &messageText,
                                const QString &channelId);

    void clearNotifications();
    void clearChannelNotifications(const QString &channelId);

signals:
    void enabledChanged();
    void notificationClicked(const QString &channelId);

private slots:
    void handleNotificationClosed(uint reason);
    void handleActionInvoked(const QString &action);

private:
    void showNotification(const QString &summary,
                         const QString &body,
                         const QString &channelId,
                         bool isMention = false);
    bool isInDoNotDisturbPeriod() const;

    bool m_enabled;
    QHash<QString, Notification*> m_activeNotifications;
    AppSettings *m_appSettings;
};

#endif // NOTIFICATIONMANAGER_H
