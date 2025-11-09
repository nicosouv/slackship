#ifndef SLACKSHIPDAEMON_H
#define SLACKSHIPDAEMON_H

#include <QObject>
#include <QTimer>
#include "../slackapi.h"
#include "../websocketclient.h"
#include "../notificationmanager.h"
#include "../workspacemanager.h"
#include "../cache/cachemanager.h"

class SlackShipDaemon : public QObject
{
    Q_OBJECT

public:
    explicit SlackShipDaemon(QObject *parent = nullptr);
    ~SlackShipDaemon();

    bool initialize();
    void start();
    void stop();

    // Status getters
    bool isConnected() const { return m_isConnected; }
    int getTotalUnreadCount() const;

public slots:
    // D-Bus exposed methods
    void syncNow();
    void setWorkspace(const QString &workspaceId);
    void markChannelAsRead(const QString &channelId);
    void sendMessageFromUI(const QString &channelId, const QString &text);

signals:
    // D-Bus signals to UI
    void newMessageReceived(const QString &channelId, const QString &messageJson);
    void unreadCountChanged(int totalUnread);
    void connectionStateChanged(bool connected);
    void syncCompleted();
    void userTyping(const QString &channelId, const QString &userId);

private slots:
    // WebSocket handlers
    void handleWebSocketMessage(const QJsonObject &message);
    void handleWebSocketDisconnected();
    void handleWebSocketError(const QString &error);

    // Periodic sync
    void performPeriodicSync();
    void reconnectWebSocket();

    // Notification handlers
    void handleNotificationClicked(const QString &channelId);

private:
    void connectToWorkspace(const QString &token);
    void processIncomingMessage(const QJsonObject &message);
    void updateUnreadCounts();
    void saveMessageToCache(const QJsonObject &message);
    int calculateTotalUnread();

    SlackAPI *m_slackAPI;
    WebSocketClient *m_webSocketClient;
    NotificationManager *m_notificationManager;
    WorkspaceManager *m_workspaceManager;
    CacheManager *m_cacheManager;

    QTimer *m_syncTimer;
    QTimer *m_reconnectTimer;

    bool m_isRunning;
    bool m_isConnected;
    QString m_currentWorkspaceId;
    QHash<QString, int> m_unreadCounts;

    static const int SYNC_INTERVAL = 300000; // 5 minutes
    static const int RECONNECT_INTERVAL = 30000; // 30 seconds
};

#endif // SLACKSHIPDAEMON_H
