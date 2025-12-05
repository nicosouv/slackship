#include "conversationmodel.h"
#include <algorithm>
#include <QDebug>

ConversationModel::ConversationModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_starredSettings("harbour-lagoon", "starred-channels")
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
        // Unread items get their own section (before other sections)
        if (conversation.unreadCount > 0) {
            return "unread";
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

    // Load starred channels from settings for current workspace
    loadStarredChannels();

    // Sort: starred first, then by type, unread, and alphabetically
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
        int oldCount = m_conversations[index].unreadCount;
        m_conversations[index].unreadCount = count;
        QModelIndex modelIndex = createIndex(index, 0);
        // Include SectionRole because section depends on unreadCount
        emit dataChanged(modelIndex, modelIndex, {UnreadCountRole, SectionRole});

        // If unread status changed, re-sort to move item to/from Unread section
        bool wasUnread = oldCount > 0;
        bool isUnread = count > 0;
        if (wasUnread != isUnread) {
            beginResetModel();
            sortConversations();
            endResetModel();
        }
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

    // Calculate unread count:
    // - For DMs/MPIMs: use unread_count or unread_count_display
    // - For channels: compare last_read with latest message timestamp
    conv.unreadCount = 0;

    // Try unread_count first (works for DMs)
    if (json.contains("unread_count")) {
        conv.unreadCount = json["unread_count"].toInt();
    }
    // Override with unread_count_display if available (more accurate for DMs)
    if (json.contains("unread_count_display")) {
        conv.unreadCount = json["unread_count_display"].toInt();
    }

    // For channels without unread_count, check last_read vs latest timestamp
    if (conv.unreadCount == 0 && json.contains("last_read")) {
        QString lastRead = json["last_read"].toString();
        QJsonObject latest = json["latest"].toObject();
        QString latestTs = latest["ts"].toString();

        if (!latestTs.isEmpty() && !lastRead.isEmpty()) {
            double lastReadDouble = lastRead.toDouble();
            double latestDouble = latestTs.toDouble();
            if (latestDouble > lastReadDouble) {
                // There are unread messages - we don't know exact count
                conv.unreadCount = 1;
            }
        }
    }

    // Extract last message timestamp
    conv.lastMessage = "";
    conv.lastMessageTime = 0;

    // Try "latest" object first (contains last message details)
    if (json.contains("latest")) {
        QJsonObject latest = json["latest"].toObject();
        QString latestTs = latest["ts"].toString();
        if (!latestTs.isEmpty()) {
            // Convert Slack timestamp (Unix timestamp with decimals) to milliseconds
            conv.lastMessageTime = static_cast<qint64>(latestTs.toDouble() * 1000);
        }
    }
    // Fallback to "updated" field (Unix timestamp of last activity)
    if (conv.lastMessageTime == 0 && json.contains("updated")) {
        qint64 updated = json["updated"].toVariant().toLongLong();
        if (updated > 0) {
            // "updated" is already in milliseconds or seconds depending on API version
            // If it's less than year 2000 in ms, it's probably in seconds
            if (updated < 946684800000) {
                conv.lastMessageTime = updated * 1000;
            } else {
                conv.lastMessageTime = updated;
            }
        }
    }

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

        // Priority 1: Unread items second (non-starred with unread messages)
        bool aHasUnread = !a.isStarred && a.unreadCount > 0;
        bool bHasUnread = !b.isStarred && b.unreadCount > 0;
        if (aHasUnread && !bHasUnread) return true;
        if (!aHasUnread && bHasUnread) return false;

        // Within unread section, sort by unread count descending
        if (aHasUnread && bHasUnread) {
            if (a.unreadCount != b.unreadCount) {
                return a.unreadCount > b.unreadCount;
            }
        }

        // Priority 2: Type (channels first, then group messages, then DMs)
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

        // Save starred channels to persistent storage
        saveStarredChannels();

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
    } else if (conv.unreadCount > 0) {
        result["section"] = "unread";
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

void ConversationModel::setTeamId(const QString &teamId)
{
    if (m_currentTeamId != teamId) {
        m_currentTeamId = teamId;
        qDebug() << "ConversationModel: Team ID set to" << m_currentTeamId;

        // Load starred channels for this workspace
        loadStarredChannels();
    }
}

void ConversationModel::loadStarredChannels()
{
    if (m_currentTeamId.isEmpty()) {
        qDebug() << "ConversationModel: Cannot load starred channels - no team ID set";
        return;
    }

    // Load starred channel IDs for this workspace
    QString key = QString("starred/%1").arg(m_currentTeamId);
    QStringList starredIds = m_starredSettings.value(key).toStringList();

    qDebug() << "ConversationModel: Loading" << starredIds.size() << "starred channels for workspace" << m_currentTeamId;

    // Update isStarred flag for matching conversations
    for (Conversation &conv : m_conversations) {
        conv.isStarred = starredIds.contains(conv.id);
    }
}

void ConversationModel::saveStarredChannels()
{
    if (m_currentTeamId.isEmpty()) {
        qDebug() << "ConversationModel: Cannot save starred channels - no team ID set";
        return;
    }

    // Collect all starred channel IDs
    QStringList starredIds;
    for (const Conversation &conv : m_conversations) {
        if (conv.isStarred) {
            starredIds.append(conv.id);
        }
    }

    // Save to settings with workspace-specific key
    QString key = QString("starred/%1").arg(m_currentTeamId);
    m_starredSettings.setValue(key, starredIds);
    m_starredSettings.sync();

    qDebug() << "ConversationModel: Saved" << starredIds.size() << "starred channels for workspace" << m_currentTeamId;
}
