#ifndef USERMODEL_H
#define USERMODEL_H

#include <QAbstractListModel>
#include <QJsonArray>
#include <QJsonObject>
#include <QSettings>

class UserModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum UserRoles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        RealNameRole,
        DisplayNameRole,
        AvatarRole,
        StatusTextRole,
        StatusEmojiRole,
        IsOnlineRole,
        IsBotRole
    };

    explicit UserModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE QString getUserName(const QString &userId) const;
    Q_INVOKABLE QString getUserAvatar(const QString &userId) const;
    Q_INVOKABLE int userCount(bool excludeBots = true) const;
    Q_INVOKABLE QVariantMap getUserDetails(const QString &userId) const;
    Q_INVOKABLE bool loadUsersFromCache(const QString &teamId);
    Q_INVOKABLE bool hasFreshCache(const QString &teamId) const;
    Q_INVOKABLE QVariantList searchUsers(const QString &query, int maxResults = 10) const;
    Q_INVOKABLE QString getUserIdByName(const QString &username) const;

signals:
    void usersUpdated();  // Emitted when users are loaded/updated

public slots:
    void updateUsers(const QJsonArray &users, const QString &teamId = QString());
    void addUser(const QJsonObject &user);
    void updateUserStatus(const QString &userId, const QString &status, const QString &emoji);
    void updateUserPresence(const QString &userId, bool isOnline);
    void clear();  // Clear all users (for workspace switch)

private:
    struct User {
        QString id;
        QString name;
        QString realName;
        QString displayName;
        QString avatar;
        QString statusText;
        QString statusEmoji;
        bool isOnline;
        bool isBot;
    };

    QList<User> m_users;
    QSettings m_userCache;  // Persistent cache for user names
    QSettings m_fullUserCache;  // Full user data cache per workspace
    int findUserIndex(const QString &userId) const;
    User parseUser(const QJsonObject &json) const;
    void saveUserToCache(const User &user);
    QString getUserNameFromCache(const QString &userId) const;
    void saveFullUserCache(const QString &teamId);

    static const int CACHE_VALIDITY_HOURS = 6;  // Cache valid for 6 hours
};

#endif // USERMODEL_H
