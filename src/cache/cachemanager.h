#ifndef CACHEMANAGER_H
#define CACHEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QJsonObject>
#include <QJsonArray>

class CacheManager : public QObject
{
    Q_OBJECT

public:
    explicit CacheManager(QObject *parent = nullptr);
    ~CacheManager();

    bool initialize();

public slots:
    // Conversations
    void cacheConversation(const QJsonObject &conversation);
    QJsonArray getCachedConversations();
    QJsonObject getCachedConversation(const QString &channelId);
    void clearConversations();

    // Messages
    void cacheMessage(const QString &channelId, const QJsonObject &message);
    QJsonArray getCachedMessages(const QString &channelId, int limit = 50);
    void clearMessages(const QString &channelId);

    // Users
    void cacheUser(const QJsonObject &user);
    QJsonObject getCachedUser(const QString &userId);
    void clearUsers();

    // Workspace
    void setWorkspaceToken(const QString &token);
    QString getWorkspaceToken();
    void clearWorkspace();

private:
    bool createTables();

    QSqlDatabase m_database;
};

#endif // CACHEMANAGER_H
