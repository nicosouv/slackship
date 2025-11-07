#ifndef CONVERSATIONMODEL_H
#define CONVERSATIONMODEL_H

#include <QAbstractListModel>
#include <QJsonArray>
#include <QJsonObject>

class ConversationModel : public QAbstractListModel
{
    Q_OBJECT

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
        UserIdRole  // For DMs - the user ID of the other person
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
    };

    QList<Conversation> m_conversations;
    int findConversationIndex(const QString &conversationId) const;
    Conversation parseConversation(const QJsonObject &json) const;
};

#endif // CONVERSATIONMODEL_H
