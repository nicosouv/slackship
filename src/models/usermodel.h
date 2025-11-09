#ifndef USERMODEL_H
#define USERMODEL_H

#include <QAbstractListModel>
#include <QJsonArray>
#include <QJsonObject>

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

public slots:
    void updateUsers(const QJsonArray &users);
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
    int findUserIndex(const QString &userId) const;
    User parseUser(const QJsonObject &json) const;
};

#endif // USERMODEL_H
