#ifndef SLACKAPI_H
#define SLACKAPI_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>

#include "websocketclient.h"

class SlackAPI : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isAuthenticated READ isAuthenticated NOTIFY authenticationChanged)
    Q_PROPERTY(QString workspaceName READ workspaceName NOTIFY workspaceChanged)
    Q_PROPERTY(QString currentUserId READ currentUserId NOTIFY currentUserChanged)
    Q_PROPERTY(QString token READ token NOTIFY tokenChanged)

public:
    explicit SlackAPI(QObject *parent = nullptr);
    ~SlackAPI();

    bool isAuthenticated() const { return m_isAuthenticated; }
    QString workspaceName() const { return m_workspaceName; }
    QString currentUserId() const { return m_currentUserId; }
    QString token() const { return m_token; }

public slots:
    // Authentication
    void authenticate(const QString &token);
    void logout();

    // Conversations
    void fetchConversations();
    void fetchConversationHistory(const QString &channelId, int limit = 50);
    void joinConversation(const QString &channelId);
    void leaveConversation(const QString &channelId);

    // Messages
    void sendMessage(const QString &channelId, const QString &text);
    void updateMessage(const QString &channelId, const QString &ts, const QString &text);
    void deleteMessage(const QString &channelId, const QString &ts);

    // Reactions
    void addReaction(const QString &channelId, const QString &ts, const QString &emoji);
    void removeReaction(const QString &channelId, const QString &ts, const QString &emoji);

    // Users
    void fetchUsers();
    void fetchUserInfo(const QString &userId);

    // Real-time connection
    void connectWebSocket();
    void disconnectWebSocket();

signals:
    // Authentication signals
    void authenticationChanged();
    void authenticationError(const QString &error);
    void workspaceChanged();
    void currentUserChanged();
    void tokenChanged();

    // Data signals
    void conversationsReceived(const QJsonArray &conversations);
    void messagesReceived(const QJsonArray &messages);
    void usersReceived(const QJsonArray &users);
    void userInfoReceived(const QJsonObject &userInfo);

    // Real-time signals
    void messageReceived(const QJsonObject &message);
    void messageUpdated(const QJsonObject &message);
    void messageDeleted(const QString &channelId, const QString &ts);
    void reactionAdded(const QJsonObject &reaction);
    void reactionRemoved(const QJsonObject &reaction);

    // Error signals
    void networkError(const QString &error);
    void apiError(const QString &error);

private slots:
    void handleNetworkReply(QNetworkReply *reply);
    void handleWebSocketMessage(const QJsonObject &message);
    void handleWebSocketError(const QString &error);

private:
    void makeApiRequest(const QString &endpoint, const QJsonObject &params = QJsonObject());
    void processApiResponse(const QString &endpoint, const QJsonObject &response);

    QNetworkAccessManager *m_networkManager;
    WebSocketClient *m_webSocketClient;

    QString m_token;
    QString m_workspaceName;
    QString m_currentUserId;
    bool m_isAuthenticated;

    static const QString API_BASE_URL;
};

#endif // SLACKAPI_H
