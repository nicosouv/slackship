#include "usermodel.h"
#include <QDateTime>
#include <QDebug>

UserModel::UserModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_userCache("harbour-lagoon", "users")
    , m_fullUserCache("harbour-lagoon", "users-full")
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

    // Fallback to cached name if user not in memory
    QString cachedName = getUserNameFromCache(userId);
    if (!cachedName.isEmpty()) {
        return cachedName;
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

void UserModel::updateUsers(const QJsonArray &users, const QString &teamId)
{
    beginResetModel();
    m_users.clear();

    for (const QJsonValue &value : users) {
        if (value.isObject()) {
            User user = parseUser(value.toObject());
            m_users.append(user);
            saveUserToCache(user);
        }
    }

    endResetModel();

    // Save full cache if teamId provided
    if (!teamId.isEmpty()) {
        saveFullUserCache(teamId);
    }

    emit usersUpdated();
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

void UserModel::saveUserToCache(const User &user)
{
    // Determine the best name to cache
    QString name;
    if (!user.displayName.isEmpty()) {
        name = user.displayName;
    } else if (!user.realName.isEmpty()) {
        name = user.realName;
    } else {
        name = user.name;
    }

    if (!name.isEmpty()) {
        m_userCache.setValue(user.id, name);
    }
}

QString UserModel::getUserNameFromCache(const QString &userId) const
{
    return m_userCache.value(userId).toString();
}

bool UserModel::hasFreshCache(const QString &teamId) const
{
    if (teamId.isEmpty()) {
        return false;
    }

    QString timestampKey = QString("timestamp/%1").arg(teamId);
    qint64 cachedTime = m_fullUserCache.value(timestampKey, 0).toLongLong();

    if (cachedTime == 0) {
        return false;
    }

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    qint64 ageMs = now - cachedTime;
    qint64 maxAgeMs = CACHE_VALIDITY_HOURS * 60 * 60 * 1000;

    bool isFresh = ageMs < maxAgeMs;
    qDebug() << "[UserModel] Cache for" << teamId << "age:" << (ageMs / 1000 / 60) << "min, fresh:" << isFresh;

    return isFresh;
}

bool UserModel::loadUsersFromCache(const QString &teamId)
{
    if (teamId.isEmpty()) {
        return false;
    }

    QString countKey = QString("count/%1").arg(teamId);
    int count = m_fullUserCache.value(countKey, 0).toInt();

    if (count == 0) {
        qDebug() << "[UserModel] No cached users for" << teamId;
        return false;
    }

    qDebug() << "[UserModel] Loading" << count << "users from cache for" << teamId;

    beginResetModel();
    m_users.clear();

    for (int i = 0; i < count; ++i) {
        QString prefix = QString("users/%1/%2/").arg(teamId).arg(i);

        User user;
        user.id = m_fullUserCache.value(prefix + "id").toString();
        user.name = m_fullUserCache.value(prefix + "name").toString();
        user.realName = m_fullUserCache.value(prefix + "realName").toString();
        user.displayName = m_fullUserCache.value(prefix + "displayName").toString();
        user.avatar = m_fullUserCache.value(prefix + "avatar").toString();
        user.statusText = m_fullUserCache.value(prefix + "statusText").toString();
        user.statusEmoji = m_fullUserCache.value(prefix + "statusEmoji").toString();
        user.isOnline = m_fullUserCache.value(prefix + "isOnline", false).toBool();
        user.isBot = m_fullUserCache.value(prefix + "isBot", false).toBool();

        if (!user.id.isEmpty()) {
            m_users.append(user);
        }
    }

    endResetModel();

    qDebug() << "[UserModel] Loaded" << m_users.count() << "users from cache";
    emit usersUpdated();

    return m_users.count() > 0;
}

void UserModel::saveFullUserCache(const QString &teamId)
{
    if (teamId.isEmpty() || m_users.isEmpty()) {
        return;
    }

    qDebug() << "[UserModel] Saving" << m_users.count() << "users to cache for" << teamId;

    // Save timestamp
    QString timestampKey = QString("timestamp/%1").arg(teamId);
    m_fullUserCache.setValue(timestampKey, QDateTime::currentMSecsSinceEpoch());

    // Save count
    QString countKey = QString("count/%1").arg(teamId);
    m_fullUserCache.setValue(countKey, m_users.count());

    // Save each user
    for (int i = 0; i < m_users.count(); ++i) {
        const User &user = m_users.at(i);
        QString prefix = QString("users/%1/%2/").arg(teamId).arg(i);

        m_fullUserCache.setValue(prefix + "id", user.id);
        m_fullUserCache.setValue(prefix + "name", user.name);
        m_fullUserCache.setValue(prefix + "realName", user.realName);
        m_fullUserCache.setValue(prefix + "displayName", user.displayName);
        m_fullUserCache.setValue(prefix + "avatar", user.avatar);
        m_fullUserCache.setValue(prefix + "statusText", user.statusText);
        m_fullUserCache.setValue(prefix + "statusEmoji", user.statusEmoji);
        m_fullUserCache.setValue(prefix + "isOnline", user.isOnline);
        m_fullUserCache.setValue(prefix + "isBot", user.isBot);
    }

    m_fullUserCache.sync();
}
