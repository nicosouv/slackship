#include "conversationmodel.h"
#include <algorithm>

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
    case IsStarredRole:
        return conversation.isStarred;
    case SectionRole:
        // Starred items go in their own section
        if (conversation.isStarred) {
            return "starred";
        }
        // Otherwise group by type
        if (conversation.type == "channel" || conversation.type == "group") {
            return "channel";
        } else if (conversation.type == "im") {
            return "im";
        } else if (conversation.type == "mpim") {
            return "mpim";
        }
        return "other";
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
    roles[IsStarredRole] = "isStarred";
    roles[SectionRole] = "section";
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

    // Sort: unread first, then starred, then alphabetically
    sortConversations();

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

    // Calculate unread count using same strategy as SailSlack:
    // - For DMs/MPIMs: use unread_count_display if available
    // - For channels: compare last_read with latest message timestamp
    conv.unreadCount = 0;

    if (json.contains("unread_count_display")) {
        // DMs and group messages provide this
        conv.unreadCount = json["unread_count_display"].toInt();
    } else if (json.contains("last_read")) {
        // For channels, check if there are unread messages
        QString lastRead = json["last_read"].toString();
        QJsonObject latest = json["latest"].toObject();
        QString latestTs = latest["ts"].toString();

        // If latest message timestamp > last_read, there are unread messages
        if (!latestTs.isEmpty() && !lastRead.isEmpty()) {
            double lastReadDouble = lastRead.toDouble();
            double latestDouble = latestTs.toDouble();
            if (latestDouble > lastReadDouble) {
                conv.unreadCount = 1;  // We don't know exact count, but we know there are unreads
            }
        }
    } else {
        // Fallback to unread_count if present (though it's usually not for channels)
        conv.unreadCount = json["unread_count"].toInt();
    }

    conv.lastMessage = "";
    conv.lastMessageTime = 0;
    conv.userId = json["user"].toString();  // For DMs
    conv.isStarred = json["is_starred"].toBool();

    QJsonObject topic = json["topic"].toObject();
    conv.topic = topic["value"].toString();

    QJsonObject purpose = json["purpose"].toObject();
    conv.purpose = purpose["value"].toString();

    return conv;
}

int ConversationModel::publicChannelCount() const
{
    int count = 0;
    for (const Conversation &conv : m_conversations) {
        if (conv.type == "channel" && !conv.isPrivate && conv.isMember) {
            count++;
        }
    }
    return count;
}

int ConversationModel::privateChannelCount() const
{
    int count = 0;
    for (const Conversation &conv : m_conversations) {
        // Private channels can be either type "group" or type "channel" with isPrivate=true
        if ((conv.type == "group" || (conv.type == "channel" && conv.isPrivate)) && conv.isMember) {
            count++;
        }
    }
    return count;
}

void ConversationModel::sortConversations()
{
    std::sort(m_conversations.begin(), m_conversations.end(),
              [](const Conversation &a, const Conversation &b) {
        // Priority 0: Starred items first (all together at the top)
        if (a.isStarred && !b.isStarred) return true;
        if (!a.isStarred && b.isStarred) return false;

        // Priority 1: Type (channels first, then group messages, then DMs)
        // channel/group = 0, mpim = 1, im = 2
        auto getTypePriority = [](const QString &type) {
            if (type == "channel" || type == "group") return 0;
            if (type == "mpim") return 1;
            if (type == "im") return 2;
            return 3;
        };
        int aPriority = getTypePriority(a.type);
        int bPriority = getTypePriority(b.type);
        if (aPriority != bPriority) return aPriority < bPriority;

        // Priority 2: Unread messages (descending by count)
        if (a.unreadCount > 0 && b.unreadCount == 0) return true;
        if (a.unreadCount == 0 && b.unreadCount > 0) return false;
        if (a.unreadCount > 0 && b.unreadCount > 0) {
            if (a.unreadCount != b.unreadCount) {
                return a.unreadCount > b.unreadCount;
            }
        }

        // Priority 3: Alphabetical by name
        return a.name.toLower() < b.name.toLower();
    });
}

void ConversationModel::clear()
{
    if (m_conversations.isEmpty()) {
        return;
    }

    beginRemoveRows(QModelIndex(), 0, m_conversations.count() - 1);
    m_conversations.clear();
    endRemoveRows();
}

void ConversationModel::toggleStar(const QString &conversationId)
{
    int index = findConversationIndex(conversationId);
    if (index >= 0) {
        m_conversations[index].isStarred = !m_conversations[index].isStarred;

        // Resort the list
        beginResetModel();
        sortConversations();
        endResetModel();

        // Notify about data change
        QModelIndex modelIndex = createIndex(index, 0);
        emit dataChanged(modelIndex, modelIndex, {IsStarredRole});
    }
}

QVariantMap ConversationModel::get(int index) const
{
    QVariantMap result;

    if (index < 0 || index >= m_conversations.count()) {
        return result;
    }

    const Conversation &conv = m_conversations.at(index);

    result["id"] = conv.id;
    result["name"] = conv.name;
    result["type"] = conv.type;
    result["isPrivate"] = conv.isPrivate;
    result["isMember"] = conv.isMember;
    result["unreadCount"] = conv.unreadCount;
    result["lastMessage"] = conv.lastMessage;
    result["lastMessageTime"] = conv.lastMessageTime;
    result["topic"] = conv.topic;
    result["purpose"] = conv.purpose;
    result["userId"] = conv.userId;
    result["isStarred"] = conv.isStarred;

    // Add section property
    if (conv.isStarred) {
        result["section"] = "starred";
    } else if (conv.type == "channel" || conv.type == "group") {
        result["section"] = "channel";
    } else if (conv.type == "im") {
        result["section"] = "im";
    } else if (conv.type == "mpim") {
        result["section"] = "mpim";
    } else {
        result["section"] = "other";
    }

    return result;
}
