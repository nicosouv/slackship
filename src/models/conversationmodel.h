#ifndef CONVERSATIONMODEL_H
#define CONVERSATIONMODEL_H

#include <QAbstractListModel>
#include <QJsonArray>
#include <QJsonObject>
#include <QSettings>

class ConversationModel : public QAbstractListModel
{
    Q_OBJECT

signals:
    void conversationsUpdated(const QStringList &conversationIds);

public:
    enum ConversationRoles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        TypeRole,
        IsPrivateRole,
        IsMemberRole,
        UnreadCountRole,
        LastMessageRole,
        LastMessageTimeRole,
        TopicRole,
        PurposeRole,
        UserIdRole,  // For DMs - the user ID of the other person
        IsStarredRole,  // Whether the channel is starred/favorited
        SectionRole  // Section name for grouping: "starred", "channel", "im", or "mpim"
    };

    explicit ConversationModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

public slots:
    void updateConversations(const QJsonArray &conversations);
    void addConversation(const QJsonObject &conversation);
    void removeConversation(const QString &conversationId);
    void updateUnreadCount(const QString &conversationId, int count);
    void updateUnreadInfo(const QString &conversationId, int unreadCount, qint64 lastMessageTime);
    void updateTimestamp(const QString &conversationId, qint64 lastMessageTime);
    void incrementUnread(const QString &conversationId, qint64 messageTimestamp);  // RTM: increment unread on new message
    void markAsRead(const QString &conversationId, qint64 timestamp = 0);  // Mark channel as read and save timestamp
    void toggleStar(const QString &conversationId);
    void clear();  // Clear all conversations (for workspace switch)
    void setTeamId(const QString &teamId);  // Set current workspace team ID

    // Stats helpers
    Q_INVOKABLE int publicChannelCount() const;
    Q_INVOKABLE int privateChannelCount() const;

    // QML helper to get conversation data by index
    Q_INVOKABLE QVariantMap get(int index) const;

    // Get all conversation IDs for unread fetching
    Q_INVOKABLE QStringList getConversationIds() const;

    // Get channel IDs without timestamps (for fetching timestamps)
    Q_INVOKABLE QStringList getChannelsWithoutTimestamp() const;

private:
    struct Conversation {
        QString id;
        QString name;
        QString type;
        bool isPrivate;
        bool isMember;
        int unreadCount;
        QString lastMessage;
        qint64 lastMessageTime;
        QString topic;
        QString purpose;
        QString userId;  // For DMs - the other user's ID
        bool isStarred;  // Whether the channel is starred/favorited
    };

    QList<Conversation> m_conversations;
    QString m_currentTeamId;
    QSettings m_starredSettings;
    QSettings m_lastReadSettings;  // Store last read timestamps per channel

    int findConversationIndex(const QString &conversationId) const;
    Conversation parseConversation(const QJsonObject &json) const;
    void sortConversations();
    void loadStarredChannels();
    void saveStarredChannels();
    void loadLastReadTimestamps();
    void saveLastReadTimestamp(const QString &channelId, qint64 timestamp);
    qint64 getLastReadTimestamp(const QString &channelId) const;
};

#endif // CONVERSATIONMODEL_H
