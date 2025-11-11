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
    if (m_currentChannelId != channelId) {
        m_currentChannelId = channelId;
        clear();
        emit currentChannelIdChanged();
    }
}

void MessageModel::updateMessages(const QJsonArray &messages)
{
    beginResetModel();
    m_messages.clear();

    for (const QJsonValue &value : messages) {
        if (value.isObject()) {
            Message msg = parseMessage(value.toObject());
            m_messages.append(msg);
        }
    }

    endResetModel();
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
    beginResetModel();
    m_messages.clear();
    endResetModel();
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

    // Debug image attachments and files
    if (!msg.attachments.isEmpty()) {
        qDebug() << "[IMAGE DEBUG] Message" << msg.timestamp << "has" << msg.attachments.count() << "attachments";
        for (const QJsonValue &att : msg.attachments) {
            QJsonObject attObj = att.toObject();
            qDebug() << "  - Attachment:" << attObj.keys();
            if (attObj.contains("image_url")) {
                qDebug() << "    image_url:" << attObj["image_url"].toString();
            }
            if (attObj.contains("thumb_url")) {
                qDebug() << "    thumb_url:" << attObj["thumb_url"].toString();
            }
        }
    }
    if (!msg.files.isEmpty()) {
        qDebug() << "[IMAGE DEBUG] Message" << msg.timestamp << "has" << msg.files.count() << "files";
        for (const QJsonValue &file : msg.files) {
            QJsonObject fileObj = file.toObject();
            qDebug() << "  - File:" << fileObj["name"].toString() << "mimetype:" << fileObj["mimetype"].toString();
            qDebug() << "    url_private:" << fileObj["url_private"].toString();
            qDebug() << "    thumb_360:" << fileObj["thumb_360"].toString();

            // Debug: show all available keys for empty files
            if (fileObj["name"].toString().isEmpty() && fileObj["mimetype"].toString().isEmpty()) {
                qDebug() << "    [EMPTY FILE] Available keys:" << fileObj.keys();
                qDebug() << "    [EMPTY FILE] Full JSON:" << QJsonDocument(fileObj).toJson(QJsonDocument::Compact);
            }
        }
    }

    return msg;
}
