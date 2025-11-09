#include "slackshipdaemon.h"
#include <QDebug>
#include <QJsonDocument>
#include <QDateTime>

SlackShipDaemon::SlackShipDaemon(QObject *parent)
    : QObject(parent)
    , m_slackAPI(new SlackAPI(this))
    , m_webSocketClient(new WebSocketClient(this))
    , m_notificationManager(new NotificationManager(this))
    , m_workspaceManager(new WorkspaceManager(this))
    , m_cacheManager(new CacheManager(this))
    , m_syncTimer(new QTimer(this))
    , m_reconnectTimer(new QTimer(this))
    , m_isRunning(false)
    , m_isConnected(false)
{
    // Setup sync timer
    m_syncTimer->setInterval(SYNC_INTERVAL);
    connect(m_syncTimer, &QTimer::timeout,
            this, &SlackShipDaemon::performPeriodicSync);

    // Setup reconnect timer
    m_reconnectTimer->setInterval(RECONNECT_INTERVAL);
    connect(m_reconnectTimer, &QTimer::timeout,
            this, &SlackShipDaemon::reconnectWebSocket);

    // Connect WebSocket signals
    connect(m_webSocketClient, &WebSocketClient::messageReceived,
            this, &SlackShipDaemon::handleWebSocketMessage);
    connect(m_webSocketClient, &WebSocketClient::disconnected,
            this, &SlackShipDaemon::handleWebSocketDisconnected);
    connect(m_webSocketClient, &WebSocketClient::error,
            this, &SlackShipDaemon::handleWebSocketError);
    connect(m_webSocketClient, &WebSocketClient::connected,
            [this]() {
                m_isConnected = true;
                m_reconnectTimer->stop();
                emit connectionStateChanged(true);
                qDebug() << "Daemon: WebSocket connected";
            });

    // Connect notification signals
    connect(m_notificationManager, &NotificationManager::notificationClicked,
            this, &SlackShipDaemon::handleNotificationClicked);

    qDebug() << "SlackShip Daemon initialized";
}

SlackShipDaemon::~SlackShipDaemon()
{
    stop();
}

bool SlackShipDaemon::initialize()
{
    qDebug() << "Initializing daemon...";

    // Initialize cache
    if (!m_cacheManager->initialize()) {
        qWarning() << "Failed to initialize cache manager";
        return false;
    }

    // Load workspaces
    m_workspaceManager->loadWorkspaces();

    // Connect to current workspace if available
    if (m_workspaceManager->workspaceCount() > 0) {
        QString token = m_workspaceManager->currentWorkspaceToken();
        if (!token.isEmpty()) {
            connectToWorkspace(token);
        }
    }

    return true;
}

void SlackShipDaemon::start()
{
    if (m_isRunning) {
        qDebug() << "Daemon already running";
        return;
    }

    qDebug() << "Starting SlackShip daemon...";
    m_isRunning = true;

    // Start periodic sync
    m_syncTimer->start();

    qDebug() << "Daemon started successfully";
}

void SlackShipDaemon::stop()
{
    if (!m_isRunning) {
        return;
    }

    qDebug() << "Stopping SlackShip daemon...";
    m_isRunning = false;

    // Stop timers
    m_syncTimer->stop();
    m_reconnectTimer->stop();

    // Disconnect WebSocket
    m_webSocketClient->disconnect();

    qDebug() << "Daemon stopped";
}

void SlackShipDaemon::syncNow()
{
    qDebug() << "Manual sync requested";
    performPeriodicSync();
}

void SlackShipDaemon::setWorkspace(const QString &workspaceId)
{
    qDebug() << "Switching to workspace:" << workspaceId;

    // Find workspace and get token
    for (int i = 0; i < m_workspaceManager->workspaceCount(); ++i) {
        QModelIndex idx = m_workspaceManager->index(i, 0);
        QString id = m_workspaceManager->data(idx, WorkspaceManager::IdRole).toString();

        if (id == workspaceId) {
            QString token = m_workspaceManager->data(idx, WorkspaceManager::TokenRole).toString();
            m_currentWorkspaceId = workspaceId;
            connectToWorkspace(token);
            break;
        }
    }
}

void SlackShipDaemon::markChannelAsRead(const QString &channelId)
{
    qDebug() << "Marking channel as read:" << channelId;

    // Clear notifications for this channel
    m_notificationManager->clearChannelNotifications(channelId);

    // Update unread count
    m_unreadCounts[channelId] = 0;
    updateUnreadCounts();
}

void SlackShipDaemon::sendMessageFromUI(const QString &channelId, const QString &text)
{
    qDebug() << "Sending message from UI to channel:" << channelId;
    m_slackAPI->sendMessage(channelId, text);
}

void SlackShipDaemon::handleWebSocketMessage(const QJsonObject &message)
{
    QString type = message["type"].toString();

    qDebug() << "Daemon received WebSocket message, type:" << type;

    if (type == "message") {
        processIncomingMessage(message);
    } else if (type == "message_changed") {
        // Handle message edit
        processIncomingMessage(message);
    } else if (type == "message_deleted") {
        QString channelId = message["channel"].toString();
        emit newMessageReceived(channelId, QJsonDocument(message).toJson());
    } else if (type == "user_typing") {
        QString channelId = message["channel"].toString();
        QString userId = message["user"].toString();
        emit userTyping(channelId, userId);
    }
}

void SlackShipDaemon::handleWebSocketDisconnected()
{
    qDebug() << "Daemon: WebSocket disconnected";
    m_isConnected = false;
    emit connectionStateChanged(false);

    // Start reconnect timer
    if (m_isRunning) {
        m_reconnectTimer->start();
    }
}

void SlackShipDaemon::handleWebSocketError(const QString &error)
{
    qWarning() << "Daemon: WebSocket error:" << error;

    // Try to reconnect
    if (m_isRunning && !m_reconnectTimer->isActive()) {
        m_reconnectTimer->start();
    }
}

void SlackShipDaemon::performPeriodicSync()
{
    if (!m_isConnected) {
        qDebug() << "Skipping sync - not connected";
        return;
    }

    qDebug() << "Performing periodic sync...";

    // Fetch latest conversations
    m_slackAPI->fetchConversations();

    emit syncCompleted();
}

void SlackShipDaemon::reconnectWebSocket()
{
    qDebug() << "Attempting to reconnect WebSocket...";

    if (m_isConnected) {
        m_reconnectTimer->stop();
        return;
    }

    QString token = m_workspaceManager->currentWorkspaceToken();
    if (!token.isEmpty()) {
        connectToWorkspace(token);
    }
}

void SlackShipDaemon::handleNotificationClicked(const QString &channelId)
{
    qDebug() << "Notification clicked for channel:" << channelId;

    // This will be picked up by the UI via D-Bus
    // The UI should open the app and navigate to this channel
}

void SlackShipDaemon::connectToWorkspace(const QString &token)
{
    qDebug() << "Connecting to workspace with token...";

    // Disconnect existing connection
    m_webSocketClient->disconnect();

    // Authenticate with Slack
    m_slackAPI->authenticate(token);

    // Wait for authentication and connect WebSocket
    connect(m_slackAPI, &SlackAPI::authenticationChanged,
            this, [this]() {
        if (m_slackAPI->isAuthenticated()) {
            qDebug() << "Authentication successful, connecting WebSocket...";
            m_slackAPI->connectWebSocket();
        }
    }, Qt::UniqueConnection);
}

void SlackShipDaemon::processIncomingMessage(const QJsonObject &message)
{
    QString channelId = message["channel"].toString();
    QString text = message["text"].toString();
    QString userId = message["user"].toString();
    QString ts = message["ts"].toString();

    qDebug() << "Processing incoming message in channel:" << channelId;

    // Save to cache
    saveMessageToCache(message);

    // Increment unread count for this channel
    m_unreadCounts[channelId] = m_unreadCounts.value(channelId, 0) + 1;

    // Update total unread count
    updateUnreadCounts();

    // Show notification (the NotificationManager is already connected in main.cpp)
    // But we'll trigger it here too for daemon context
    bool isMention = text.contains("@");

    // Get channel name from cache
    QString channelName = channelId; // Default to ID if cache fails
    QJsonObject cachedChannel = m_cacheManager->getCachedConversation(channelId);
    if (!cachedChannel.isEmpty()) {
        channelName = cachedChannel["name"].toString();
        if (channelName.isEmpty()) {
            channelName = channelId;
        }
    }

    if (isMention) {
        m_notificationManager->showMentionNotification(channelName, userId, text, channelId);
    } else {
        m_notificationManager->showMessageNotification(channelName, userId, text, channelId);
    }

    // Emit signal to UI (via D-Bus)
    emit newMessageReceived(channelId, QJsonDocument(message).toJson());
}

void SlackShipDaemon::updateUnreadCounts()
{
    int total = calculateTotalUnread();
    emit unreadCountChanged(total);
}

void SlackShipDaemon::saveMessageToCache(const QJsonObject &message)
{
    QString channelId = message["channel"].toString();
    m_cacheManager->cacheMessage(channelId, message);
}

int SlackShipDaemon::calculateTotalUnread()
{
    int total = 0;
    for (int count : m_unreadCounts.values()) {
        total += count;
    }
    return total;
}

int SlackShipDaemon::getTotalUnreadCount() const
{
    int total = 0;
    for (int count : m_unreadCounts.values()) {
        total += count;
    }
    return total;
}
