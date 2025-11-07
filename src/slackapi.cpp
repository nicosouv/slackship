#include "slackapi.h"
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QDebug>

const QString SlackAPI::API_BASE_URL = "https://slack.com/api/";

SlackAPI::SlackAPI(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_webSocketClient(new WebSocketClient(this))
    , m_isAuthenticated(false)
{
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &SlackAPI::handleNetworkReply);

    connect(m_webSocketClient, &WebSocketClient::messageReceived,
            this, &SlackAPI::handleWebSocketMessage);
    connect(m_webSocketClient, &WebSocketClient::error,
            this, &SlackAPI::handleWebSocketError);
}

SlackAPI::~SlackAPI()
{
}

void SlackAPI::authenticate(const QString &token)
{
    m_token = token;
    emit tokenChanged();

    qDebug() << "=== AUTHENTICATE CALLED ===";
    qDebug() << "Token length:" << m_token.length();

    // Test authentication with auth.test endpoint
    QUrl url(API_BASE_URL + "auth.test");
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_token).toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    qDebug() << "Sending auth.test request to:" << url.toString();
    m_networkManager->get(request);
}

void SlackAPI::logout()
{
    m_token.clear();
    m_workspaceName.clear();
    m_currentUserId.clear();
    m_isAuthenticated = false;

    disconnectWebSocket();

    emit tokenChanged();
    emit authenticationChanged();
}

void SlackAPI::fetchConversations()
{
    QJsonObject params;
    params["types"] = "public_channel,private_channel,mpim,im";
    params["limit"] = 200;

    makeApiRequest("conversations.list", params);
}

void SlackAPI::fetchConversationHistory(const QString &channelId, int limit)
{
    QJsonObject params;
    params["channel"] = channelId;
    params["limit"] = limit;

    makeApiRequest("conversations.history", params);
}

void SlackAPI::joinConversation(const QString &channelId)
{
    QJsonObject params;
    params["channel"] = channelId;

    makeApiRequest("conversations.join", params);
}

void SlackAPI::leaveConversation(const QString &channelId)
{
    QJsonObject params;
    params["channel"] = channelId;

    makeApiRequest("conversations.leave", params);
}

void SlackAPI::sendMessage(const QString &channelId, const QString &text)
{
    QJsonObject params;
    params["channel"] = channelId;
    params["text"] = text;

    makeApiRequest("chat.postMessage", params);
}

void SlackAPI::updateMessage(const QString &channelId, const QString &ts, const QString &text)
{
    QJsonObject params;
    params["channel"] = channelId;
    params["ts"] = ts;
    params["text"] = text;

    makeApiRequest("chat.update", params);
}

void SlackAPI::deleteMessage(const QString &channelId, const QString &ts)
{
    QJsonObject params;
    params["channel"] = channelId;
    params["ts"] = ts;

    makeApiRequest("chat.delete", params);
}

void SlackAPI::addReaction(const QString &channelId, const QString &ts, const QString &emoji)
{
    QJsonObject params;
    params["channel"] = channelId;
    params["timestamp"] = ts;
    params["name"] = emoji;

    makeApiRequest("reactions.add", params);
}

void SlackAPI::removeReaction(const QString &channelId, const QString &ts, const QString &emoji)
{
    QJsonObject params;
    params["channel"] = channelId;
    params["timestamp"] = ts;
    params["name"] = emoji;

    makeApiRequest("reactions.remove", params);
}

void SlackAPI::fetchUsers()
{
    makeApiRequest("users.list");
}

void SlackAPI::fetchUserInfo(const QString &userId)
{
    QJsonObject params;
    params["user"] = userId;

    makeApiRequest("users.info", params);
}

void SlackAPI::connectWebSocket()
{
    if (m_token.isEmpty()) {
        qWarning() << "Cannot connect WebSocket: not authenticated";
        return;
    }

    // First get the WebSocket URL from Slack
    makeApiRequest("apps.connections.open");
}

void SlackAPI::disconnectWebSocket()
{
    m_webSocketClient->disconnect();
}

void SlackAPI::handleNetworkReply(QNetworkReply *reply)
{
    reply->deleteLater();

    qDebug() << "=== NETWORK REPLY RECEIVED ===";
    qDebug() << "URL:" << reply->url().toString();
    qDebug() << "Error:" << reply->error();

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Network error:" << reply->errorString();
        emit networkError(reply->errorString());
        return;
    }

    QByteArray data = reply->readAll();
    qDebug() << "Response data:" << data;

    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (!doc.isObject()) {
        qDebug() << "Response is not a JSON object";
        emit apiError("Invalid response from Slack API");
        return;
    }

    QJsonObject response = doc.object();
    qDebug() << "Response 'ok' field:" << response["ok"].toBool();

    if (!response["ok"].toBool()) {
        QString error = response["error"].toString();
        qDebug() << "API error:" << error;
        emit apiError(error);
        return;
    }

    // Extract endpoint from reply URL
    QString endpoint = reply->url().path();
    qDebug() << "Full path:" << endpoint;
    endpoint.remove("/api/");
    qDebug() << "Extracted endpoint:" << endpoint;

    processApiResponse(endpoint, response);
}

void SlackAPI::handleWebSocketMessage(const QJsonObject &message)
{
    QString type = message["type"].toString();

    if (type == "message") {
        emit messageReceived(message);
    } else if (type == "message_changed") {
        emit messageUpdated(message);
    } else if (type == "message_deleted") {
        QString channelId = message["channel"].toString();
        QString ts = message["deleted_ts"].toString();
        emit messageDeleted(channelId, ts);
    } else if (type == "reaction_added") {
        emit reactionAdded(message);
    } else if (type == "reaction_removed") {
        emit reactionRemoved(message);
    }
}

void SlackAPI::handleWebSocketError(const QString &error)
{
    qWarning() << "WebSocket error:" << error;
    emit networkError(error);
}

void SlackAPI::makeApiRequest(const QString &endpoint, const QJsonObject &params)
{
    if (m_token.isEmpty() && endpoint != "auth.test") {
        emit apiError("Not authenticated");
        return;
    }

    QUrl url(API_BASE_URL + endpoint);
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_token).toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Endpoints that require POST with JSON body
    bool requiresPost = endpoint.startsWith("chat.") ||
                       endpoint.startsWith("files.") ||
                       endpoint.startsWith("reactions.");

    if (requiresPost) {
        // POST request with JSON body
        QJsonDocument doc(params);
        QByteArray jsonData = doc.toJson();
        qDebug() << "Sending POST request to" << endpoint << "with body:" << jsonData;
        m_networkManager->post(request, jsonData);
    } else {
        // GET request with query parameters
        if (!params.isEmpty()) {
            QUrlQuery query;
            for (auto it = params.begin(); it != params.end(); ++it) {
                query.addQueryItem(it.key(), it.value().toVariant().toString());
            }
            url.setQuery(query);
            request.setUrl(url);
        }
        m_networkManager->get(request);
    }
}

void SlackAPI::processApiResponse(const QString &endpoint, const QJsonObject &response)
{
    qDebug() << "=== PROCESS API RESPONSE ===";
    qDebug() << "Endpoint:" << endpoint;
    qDebug() << "Checking if endpoint == 'auth.test':" << (endpoint == "auth.test");

    if (endpoint == "auth.test") {
        qDebug() << "AUTH.TEST response received!";
        qDebug() << "user_id:" << response["user_id"].toString();
        qDebug() << "team:" << response["team"].toString();

        m_isAuthenticated = true;
        m_currentUserId = response["user_id"].toString();
        m_workspaceName = response["team"].toString();

        qDebug() << "Setting m_isAuthenticated to true";
        qDebug() << "Emitting authenticationChanged signal";

        emit authenticationChanged();
        emit workspaceChanged();
        emit currentUserChanged();

        // After authentication, connect WebSocket
        connectWebSocket();

    } else if (endpoint == "conversations.list") {
        QJsonArray conversations = response["channels"].toArray();
        emit conversationsReceived(conversations);

    } else if (endpoint == "conversations.history") {
        QJsonArray messages = response["messages"].toArray();
        qDebug() << "CONVERSATIONS.HISTORY: Emitting messagesReceived signal with" << messages.count() << "messages";
        emit messagesReceived(messages);

    } else if (endpoint == "users.list") {
        QJsonArray users = response["members"].toArray();
        emit usersReceived(users);

    } else if (endpoint == "users.info") {
        QJsonObject user = response["user"].toObject();
        emit userInfoReceived(user);

    } else if (endpoint == "apps.connections.open") {
        QString wsUrl = response["url"].toString();
        if (!wsUrl.isEmpty()) {
            m_webSocketClient->connectToUrl(wsUrl);
        }
    }
}
