#include "conversationmodel.h"

ConversationModel::ConversationModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ConversationModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_conversations.count();
}

QVariant ConversationModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_conversations.count())
        return QVariant();

    const Conversation &conversation = m_conversations.at(index.row());

    switch (role) {
    case IdRole:
        return conversation.id;
    case NameRole:
        return conversation.name;
    case TypeRole:
        return conversation.type;
    case IsPrivateRole:
        return conversation.isPrivate;
    case IsMemberRole:
        return conversation.isMember;
    case UnreadCountRole:
        return conversation.unreadCount;
    case LastMessageRole:
        return conversation.lastMessage;
    case LastMessageTimeRole:
        return conversation.lastMessageTime;
    case TopicRole:
        return conversation.topic;
    case PurposeRole:
        return conversation.purpose;
    case UserIdRole:
        return conversation.userId;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> ConversationModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[NameRole] = "name";
    roles[TypeRole] = "type";
    roles[IsPrivateRole] = "isPrivate";
    roles[IsMemberRole] = "isMember";
    roles[UnreadCountRole] = "unreadCount";
    roles[LastMessageRole] = "lastMessage";
    roles[LastMessageTimeRole] = "lastMessageTime";
    roles[TopicRole] = "topic";
    roles[PurposeRole] = "purpose";
    roles[UserIdRole] = "userId";
    return roles;
}

void ConversationModel::updateConversations(const QJsonArray &conversations)
{
    beginResetModel();
    m_conversations.clear();

    for (const QJsonValue &value : conversations) {
        if (value.isObject()) {
            Conversation conv = parseConversation(value.toObject());
            m_conversations.append(conv);
        }
    }

    endResetModel();
}

void ConversationModel::addConversation(const QJsonObject &conversation)
{
    Conversation conv = parseConversation(conversation);

    beginInsertRows(QModelIndex(), m_conversations.count(), m_conversations.count());
    m_conversations.append(conv);
    endInsertRows();
}

void ConversationModel::removeConversation(const QString &conversationId)
{
    int index = findConversationIndex(conversationId);
    if (index >= 0) {
        beginRemoveRows(QModelIndex(), index, index);
        m_conversations.removeAt(index);
        endRemoveRows();
    }
}

void ConversationModel::updateUnreadCount(const QString &conversationId, int count)
{
    int index = findConversationIndex(conversationId);
    if (index >= 0) {
        m_conversations[index].unreadCount = count;
        QModelIndex modelIndex = createIndex(index, 0);
        emit dataChanged(modelIndex, modelIndex, {UnreadCountRole});
    }
}

int ConversationModel::findConversationIndex(const QString &conversationId) const
{
    for (int i = 0; i < m_conversations.count(); ++i) {
        if (m_conversations.at(i).id == conversationId)
            return i;
    }
    return -1;
}

ConversationModel::Conversation ConversationModel::parseConversation(const QJsonObject &json) const
{
    Conversation conv;
    conv.id = json["id"].toString();
    conv.name = json["name"].toString();
    conv.type = json["is_channel"].toBool() ? "channel" :
                json["is_group"].toBool() ? "group" :
                json["is_im"].toBool() ? "im" :
                json["is_mpim"].toBool() ? "mpim" : "unknown";
    conv.isPrivate = json["is_private"].toBool();
    conv.isMember = json["is_member"].toBool();
    conv.unreadCount = json["unread_count"].toInt();
    conv.lastMessage = "";
    conv.lastMessageTime = 0;
    conv.userId = json["user"].toString();  // For DMs

    QJsonObject topic = json["topic"].toObject();
    conv.topic = topic["value"].toString();

    QJsonObject purpose = json["purpose"].toObject();
    conv.purpose = purpose["value"].toString();

    return conv;
}
