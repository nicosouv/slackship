#include "messagemodel.h"
#include <QDebug>

MessageModel::MessageModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int MessageModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_messages.count();
}

QVariant MessageModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_messages.count())
        return QVariant();

    const Message &message = m_messages.at(index.row());

    switch (role) {
    case IdRole:
        return message.id;
    case TextRole:
        return message.text;
    case UserIdRole:
        return message.userId;
    case UserNameRole:
        return message.userName;
    case TimestampRole:
        return message.timestamp;
    case ThreadTsRole:
        return message.threadTs;
    case ThreadCountRole:
        return message.threadCount;
    case ReactionsRole:
        return message.reactions.toVariantList();
    case AttachmentsRole:
        return message.attachments.toVariantList();
    case FilesRole:
        return message.files.toVariantList();
    case IsEditedRole:
        return message.isEdited;
    case IsOwnMessageRole:
        return message.isOwnMessage;
    case ChannelIdRole:
        return message.channelId;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> MessageModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[TextRole] = "text";
    roles[UserIdRole] = "userId";
    roles[UserNameRole] = "userName";
    roles[TimestampRole] = "timestamp";
    roles[ThreadTsRole] = "threadTs";
    roles[ThreadCountRole] = "threadCount";
    roles[ReactionsRole] = "reactions";
    roles[AttachmentsRole] = "attachments";
    roles[FilesRole] = "files";
    roles[IsEditedRole] = "isEdited";
    roles[IsOwnMessageRole] = "isOwnMessage";
    roles[ChannelIdRole] = "channelId";
    return roles;
}

void MessageModel::setCurrentChannelId(const QString &channelId)
{
    qDebug() << "=== MessageModel::setCurrentChannelId CALLED ===";
    qDebug() << "Old channel ID:" << m_currentChannelId;
    qDebug() << "New channel ID:" << channelId;

    if (m_currentChannelId != channelId) {
        m_currentChannelId = channelId;
        qDebug() << "Channel changed, clearing messages";
        clear();
        emit currentChannelIdChanged();
    } else {
        qDebug() << "Channel ID unchanged, not clearing";
    }
}

void MessageModel::updateMessages(const QJsonArray &messages)
{
    qDebug() << "===  MessageModel::updateMessages CALLED ===";
    qDebug() << "Current channel ID:" << m_currentChannelId;
    qDebug() << "Received" << messages.count() << "messages";

    beginResetModel();
    m_messages.clear();

    for (const QJsonValue &value : messages) {
        if (value.isObject()) {
            Message msg = parseMessage(value.toObject());
            qDebug() << "  - Parsed message:" << msg.text << "from user:" << msg.userId << "ts:" << msg.timestamp;
            m_messages.append(msg);
        }
    }

    qDebug() << "Total messages in model:" << m_messages.count();
    endResetModel();
    qDebug() << "Model reset complete, rowCount():" << rowCount();
}

void MessageModel::addMessage(const QJsonObject &message)
{
    Message msg = parseMessage(message);

    beginInsertRows(QModelIndex(), 0, 0);
    m_messages.prepend(msg);
    endInsertRows();
}

void MessageModel::updateMessage(const QJsonObject &message)
{
    QString ts = message["ts"].toString();
    int index = findMessageIndex(ts);

    if (index >= 0) {
        m_messages[index] = parseMessage(message);
        QModelIndex modelIndex = createIndex(index, 0);
        emit dataChanged(modelIndex, modelIndex);
    }
}

void MessageModel::removeMessage(const QString &messageId)
{
    int index = findMessageIndex(messageId);
    if (index >= 0) {
        beginRemoveRows(QModelIndex(), index, index);
        m_messages.removeAt(index);
        endRemoveRows();
    }
}

void MessageModel::clear()
{
    qDebug() << "=== MessageModel::clear() CALLED ===";
    qDebug() << "Clearing" << m_messages.count() << "messages";
    beginResetModel();
    m_messages.clear();
    endResetModel();
    qDebug() << "Clear complete, rowCount():" << rowCount();
}

int MessageModel::findMessageIndex(const QString &timestamp) const
{
    for (int i = 0; i < m_messages.count(); ++i) {
        if (m_messages.at(i).timestamp == timestamp)
            return i;
    }
    return -1;
}

MessageModel::Message MessageModel::parseMessage(const QJsonObject &json) const
{
    Message msg;
    msg.id = json["client_msg_id"].toString();
    msg.text = json["text"].toString();
    msg.userId = json["user"].toString();
    msg.userName = ""; // Will be resolved from user model
    msg.timestamp = json["ts"].toString();
    msg.threadTs = json["thread_ts"].toString();
    msg.threadCount = json["reply_count"].toInt();
    msg.reactions = json["reactions"].toArray();
    msg.attachments = json["attachments"].toArray();
    msg.files = json["files"].toArray();
    msg.isEdited = json["edited"].isObject();
    msg.isOwnMessage = false; // Will be set based on current user
    msg.channelId = m_currentChannelId; // Set from the current channel context

    return msg;
}
