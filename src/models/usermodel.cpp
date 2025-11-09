#include "usermodel.h"

UserModel::UserModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int UserModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_users.count();
}

QVariant UserModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_users.count())
        return QVariant();

    const User &user = m_users.at(index.row());

    switch (role) {
    case IdRole:
        return user.id;
    case NameRole:
        return user.name;
    case RealNameRole:
        return user.realName;
    case DisplayNameRole:
        return user.displayName;
    case AvatarRole:
        return user.avatar;
    case StatusTextRole:
        return user.statusText;
    case StatusEmojiRole:
        return user.statusEmoji;
    case IsOnlineRole:
        return user.isOnline;
    case IsBotRole:
        return user.isBot;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> UserModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[NameRole] = "name";
    roles[RealNameRole] = "realName";
    roles[DisplayNameRole] = "displayName";
    roles[AvatarRole] = "avatar";
    roles[StatusTextRole] = "statusText";
    roles[StatusEmojiRole] = "statusEmoji";
    roles[IsOnlineRole] = "isOnline";
    roles[IsBotRole] = "isBot";
    return roles;
}

QString UserModel::getUserName(const QString &userId) const
{
    int index = findUserIndex(userId);
    if (index >= 0) {
        const User &user = m_users.at(index);
        // Priority: displayName > realName > name (username)
        if (!user.displayName.isEmpty()) {
            return user.displayName;
        } else if (!user.realName.isEmpty()) {
            return user.realName;
        } else {
            return user.name;
        }
    }
    return userId;
}

QString UserModel::getUserAvatar(const QString &userId) const
{
    int index = findUserIndex(userId);
    if (index >= 0) {
        return m_users.at(index).avatar;
    }
    return QString();
}

QVariantMap UserModel::getUserDetails(const QString &userId) const
{
    QVariantMap details;
    int index = findUserIndex(userId);

    if (index >= 0) {
        const User &user = m_users.at(index);
        details["id"] = user.id;
        details["name"] = user.name;
        details["realName"] = user.realName;
        details["displayName"] = user.displayName;
        details["avatar"] = user.avatar;
        details["statusText"] = user.statusText;
        details["statusEmoji"] = user.statusEmoji;
        details["isOnline"] = user.isOnline;
        details["isBot"] = user.isBot;
    }

    return details;
}

void UserModel::updateUsers(const QJsonArray &users)
{
    beginResetModel();
    m_users.clear();

    for (const QJsonValue &value : users) {
        if (value.isObject()) {
            User user = parseUser(value.toObject());
            m_users.append(user);
        }
    }

    endResetModel();
}

void UserModel::addUser(const QJsonObject &user)
{
    User u = parseUser(user);

    beginInsertRows(QModelIndex(), m_users.count(), m_users.count());
    m_users.append(u);
    endInsertRows();
}

void UserModel::updateUserStatus(const QString &userId, const QString &status, const QString &emoji)
{
    int index = findUserIndex(userId);
    if (index >= 0) {
        m_users[index].statusText = status;
        m_users[index].statusEmoji = emoji;
        QModelIndex modelIndex = createIndex(index, 0);
        emit dataChanged(modelIndex, modelIndex, {StatusTextRole, StatusEmojiRole});
    }
}

void UserModel::updateUserPresence(const QString &userId, bool isOnline)
{
    int index = findUserIndex(userId);
    if (index >= 0) {
        m_users[index].isOnline = isOnline;
        QModelIndex modelIndex = createIndex(index, 0);
        emit dataChanged(modelIndex, modelIndex, {IsOnlineRole});
    }
}

int UserModel::findUserIndex(const QString &userId) const
{
    for (int i = 0; i < m_users.count(); ++i) {
        if (m_users.at(i).id == userId)
            return i;
    }
    return -1;
}

UserModel::User UserModel::parseUser(const QJsonObject &json) const
{
    User user;
    user.id = json["id"].toString();
    user.name = json["name"].toString();

    QJsonObject profile = json["profile"].toObject();
    user.realName = profile["real_name"].toString();
    user.displayName = profile["display_name"].toString();
    user.avatar = profile["image_72"].toString();
    user.statusText = profile["status_text"].toString();
    user.statusEmoji = profile["status_emoji"].toString();

    user.isOnline = false;
    user.isBot = json["is_bot"].toBool();

    return user;
}

int UserModel::userCount(bool excludeBots) const
{
    if (!excludeBots) {
        return m_users.count();
    }

    int count = 0;
    for (const User &user : m_users) {
        if (!user.isBot) {
            count++;
        }
    }
    return count;
}

void UserModel::clear()
{
    if (m_users.isEmpty()) {
        return;
    }

    beginRemoveRows(QModelIndex(), 0, m_users.count() - 1);
    m_users.clear();
    endRemoveRows();
}
