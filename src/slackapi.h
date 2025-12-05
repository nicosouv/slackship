#ifndef SLACKAPI_H
#define SLACKAPI_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QHash>
#include <QSet>
#include <QSettings>

#include "websocketclient.h"

class SlackAPI : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isAuthenticated READ isAuthenticated NOTIFY authenticationChanged)
    Q_PROPERTY(QString workspaceName READ workspaceName NOTIFY workspaceChanged)
    Q_PROPERTY(QString teamId READ teamId NOTIFY teamIdChanged)
    Q_PROPERTY(QString currentUserId READ currentUserId NOTIFY currentUserChanged)
    Q_PROPERTY(QString token READ token NOTIFY tokenChanged)
    Q_PROPERTY(bool autoRefresh READ autoRefresh WRITE setAutoRefresh NOTIFY autoRefreshChanged)
    Q_PROPERTY(int refreshInterval READ refreshInterval WRITE setRefreshInterval NOTIFY refreshIntervalChanged)
    Q_PROPERTY(qint64 sessionBandwidthBytes READ sessionBandwidthBytes NOTIFY sessionBandwidthBytesChanged)
    Q_PROPERTY(qint64 sessionStartTime READ sessionStartTime NOTIFY sessionStartTimeChanged)

public:
    explicit SlackAPI(QObject *parent = nullptr);
    ~SlackAPI();

    bool isAuthenticated() const { return m_isAuthenticated; }
    QString workspaceName() const { return m_workspaceName; }
    QString teamId() const { return m_teamId; }
    QString currentUserId() const { return m_currentUserId; }
    QString token() const { return m_token; }
    bool autoRefresh() const { return m_autoRefresh; }
    void setAutoRefresh(bool enabled);
    int refreshInterval() const { return m_refreshInterval; }
    void setRefreshInterval(int seconds);
    qint64 sessionBandwidthBytes() const { return m_sessionBandwidthBytes; }
    qint64 sessionStartTime() const { return m_sessionStartTime; }

public slots:
    // Authentication
    void authenticate(const QString &token);
    void logout();

    // Conversations
    void fetchConversations();
    void fetchConversationUnreads(const QStringList &channelIds);  // Fetch unread counts for channels
    Q_INVOKABLE bool isChannelLoading(const QString &channelId) const;  // Check if channel is loading unread info
    void fetchConversationHistory(const QString &channelId, int limit = 50);
    void fetchConversationInfo(const QString &channelId);
    void fetchAllPublicChannels();  // Fetch all public channels (for browsing/joining)
    void joinConversation(const QString &channelId);
    void leaveConversation(const QString &channelId);
    void openDirectMessage(const QString &userId);  // Open or create a DM with a user
    void markConversationRead(const QString &channelId, const QString &timestamp);  // Mark conversation as read up to timestamp

    // Messages
    void sendMessage(const QString &channelId, const QString &text);
    void sendThreadReply(const QString &channelId, const QString &threadTs, const QString &text);
    void updateMessage(const QString &channelId, const QString &ts, const QString &text);
    void deleteMessage(const QString &channelId, const QString &ts);
    void fetchThreadReplies(const QString &channelId, const QString &threadTs);

    // Reactions
    void addReaction(const QString &channelId, const QString &ts, const QString &emoji);
    void removeReaction(const QString &channelId, const QString &ts, const QString &emoji);

    // Pins
    void fetchPins(const QString &channelId);
    void addPin(const QString &channelId, const QString &timestamp);
    void removePin(const QString &channelId, const QString &timestamp);

    // Bookmarks
    void fetchBookmarks(const QString &channelId);

    // Emoji
    void fetchCustomEmoji();

    // Users
    void fetchUsers();
    void fetchUserInfo(const QString &userId);
    void fetchUserProfile(const QString &userId);  // Enhanced profile with status

    // Search
    void searchMessages(const QString &query);

    // Real-time connection
    void connectWebSocket();
    void disconnectWebSocket();

signals:
    // Authentication signals
    void authenticationChanged();
    void authenticationError(const QString &error);
    void workspaceChanged();
    void teamIdChanged();
    void currentUserChanged();
    void tokenChanged();

    // Data signals
    void conversationsReceived(const QJsonArray &conversations);
    void conversationInfoReceived(const QJsonObject &channelInfo);
    void conversationLeft(const QString &channelId);
    void directMessageOpened(const QString &channelId, const QString &userId);
    void publicChannelsReceived(const QJsonArray &channels);
    void messagesReceived(const QJsonArray &messages);
    void threadRepliesReceived(const QJsonArray &replies);
    void usersReceived(const QJsonArray &users);
    void userInfoReceived(const QJsonObject &userInfo);
    void userProfileReceived(const QJsonObject &profile);
    void searchResultsReceived(const QJsonObject &results);
    void pinsReceived(const QString &channelId, const QJsonArray &pins);
    void bookmarksReceived(const QString &channelId, const QJsonArray &bookmarks);
    void customEmojiReceived(const QJsonObject &emoji);

    // Real-time signals
    void messageReceived(const QJsonObject &message);
    void messageUpdated(const QJsonObject &message);
    void messageDeleted(const QString &channelId, const QString &ts);
    void reactionAdded(const QJsonObject &reaction);
    void reactionRemoved(const QJsonObject &reaction);
    void userTyping(const QString &typingChannelId, const QString &typingUserId);

    // Error signals
    void networkError(const QString &error);
    void apiError(const QString &error);

    // Polling signals
    void autoRefreshChanged();
    void refreshIntervalChanged();
    void newUnreadMessages(const QString &channelId, int newCount, int totalUnread);
    void conversationUnreadReceived(const QString &channelId, int unreadCount, qint64 lastMessageTime);
    void channelLoadingChanged(const QString &channelId, bool isLoading);

    // Bandwidth signals
    void sessionBandwidthBytesChanged();
    void bandwidthBytesAdded(qint64 bytes);  // For updating total in AppSettings
    void sessionStartTimeChanged();

private slots:
    void handleNetworkReply(QNetworkReply *reply);
    void handleWebSocketMessage(const QJsonObject &message);
    void handleWebSocketError(const QString &error);
    void handleRefreshTimer();

private:
    QNetworkReply* makeApiRequest(const QString &endpoint, const QJsonObject &params = QJsonObject());
    void processApiResponse(const QString &endpoint, const QJsonObject &response, QNetworkReply *reply);
    void trackBandwidth(qint64 bytes);
    void fetchSingleMessage(const QString &channelId, const QString &timestamp);
    void processNextUnreadBatch();

    QNetworkAccessManager *m_networkManager;
    WebSocketClient *m_webSocketClient;

    QString m_token;
    QString m_workspaceName;
    QString m_teamId;
    QString m_currentUserId;
    bool m_isAuthenticated;

    // Auto-refresh / polling
    QTimer *m_refreshTimer;
    bool m_autoRefresh;
    int m_refreshInterval;  // in seconds
    QHash<QString, int> m_lastUnreadCounts;  // channelId -> unread count

    // Unread fetching
    QStringList m_pendingUnreadFetches;  // Channels pending unread fetch
    QSet<QString> m_loadingChannels;  // Channels currently loading unread info
    QHash<QString, QPair<int, qint64>> m_unreadResults;  // channelId -> (unreadCount, lastMessageTime)

    // Bandwidth tracking
    qint64 m_sessionBandwidthBytes;  // Bytes used in current session
    qint64 m_sessionStartTime;  // QDateTime::currentMSecsSinceEpoch() when session started

    static const QString API_BASE_URL;
};

#endif // SLACKAPI_H
