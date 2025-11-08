#include "cachemanager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QJsonDocument>
#include <QDateTime>

CacheManager::CacheManager(QObject *parent)
    : QObject(parent)
{
}

CacheManager::~CacheManager()
{
    if (m_database.isOpen()) {
        m_database.close();
    }
}

bool CacheManager::initialize()
{
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dataDir(dataPath);
    if (!dataDir.exists()) {
        dataDir.mkpath(".");
    }

    QString dbPath = dataPath + "/lagoon.db";

    m_database = QSqlDatabase::addDatabase("QSQLITE");
    m_database.setDatabaseName(dbPath);

    if (!m_database.open()) {
        qWarning() << "Failed to open database:" << m_database.lastError().text();
        return false;
    }

    return createTables();
}

bool CacheManager::createTables()
{
    QSqlQuery query(m_database);

    // Conversations table
    if (!query.exec("CREATE TABLE IF NOT EXISTS conversations ("
                    "id TEXT PRIMARY KEY, "
                    "name TEXT, "
                    "type TEXT, "
                    "data TEXT, "
                    "last_updated INTEGER)")) {
        qWarning() << "Failed to create conversations table:" << query.lastError().text();
        return false;
    }

    // Messages table
    if (!query.exec("CREATE TABLE IF NOT EXISTS messages ("
                    "id TEXT PRIMARY KEY, "
                    "channel_id TEXT, "
                    "timestamp TEXT, "
                    "data TEXT, "
                    "last_updated INTEGER)")) {
        qWarning() << "Failed to create messages table:" << query.lastError().text();
        return false;
    }

    // Users table
    if (!query.exec("CREATE TABLE IF NOT EXISTS users ("
                    "id TEXT PRIMARY KEY, "
                    "name TEXT, "
                    "data TEXT, "
                    "last_updated INTEGER)")) {
        qWarning() << "Failed to create users table:" << query.lastError().text();
        return false;
    }

    // Workspace settings table
    if (!query.exec("CREATE TABLE IF NOT EXISTS workspace ("
                    "key TEXT PRIMARY KEY, "
                    "value TEXT)")) {
        qWarning() << "Failed to create workspace table:" << query.lastError().text();
        return false;
    }

    return true;
}

void CacheManager::cacheConversation(const QJsonObject &conversation)
{
    QSqlQuery query(m_database);
    query.prepare("INSERT OR REPLACE INTO conversations (id, name, type, data, last_updated) "
                  "VALUES (:id, :name, :type, :data, :timestamp)");

    QString id = conversation["id"].toString();
    QString name = conversation["name"].toString();
    QString type = conversation["is_channel"].toBool() ? "channel" : "dm";

    QJsonDocument doc(conversation);
    QString data = doc.toJson(QJsonDocument::Compact);

    query.bindValue(":id", id);
    query.bindValue(":name", name);
    query.bindValue(":type", type);
    query.bindValue(":data", data);
    query.bindValue(":timestamp", QDateTime::currentMSecsSinceEpoch() / 1000);

    if (!query.exec()) {
        qWarning() << "Failed to cache conversation:" << query.lastError().text();
    }
}

QJsonArray CacheManager::getCachedConversations()
{
    QJsonArray result;

    QSqlQuery query(m_database);
    if (query.exec("SELECT data FROM conversations ORDER BY last_updated DESC")) {
        while (query.next()) {
            QString data = query.value(0).toString();
            QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
            if (doc.isObject()) {
                result.append(doc.object());
            }
        }
    }

    return result;
}

void CacheManager::clearConversations()
{
    QSqlQuery query(m_database);
    query.exec("DELETE FROM conversations");
}

void CacheManager::cacheMessage(const QString &channelId, const QJsonObject &message)
{
    QSqlQuery query(m_database);
    query.prepare("INSERT OR REPLACE INTO messages (id, channel_id, timestamp, data, last_updated) "
                  "VALUES (:id, :channel_id, :timestamp, :data, :last_updated)");

    QString id = message["client_msg_id"].toString();
    QString ts = message["ts"].toString();

    QJsonDocument doc(message);
    QString data = doc.toJson(QJsonDocument::Compact);

    query.bindValue(":id", id);
    query.bindValue(":channel_id", channelId);
    query.bindValue(":timestamp", ts);
    query.bindValue(":data", data);
    query.bindValue(":last_updated", QDateTime::currentMSecsSinceEpoch() / 1000);

    if (!query.exec()) {
        qWarning() << "Failed to cache message:" << query.lastError().text();
    }
}

QJsonArray CacheManager::getCachedMessages(const QString &channelId, int limit)
{
    QJsonArray result;

    QSqlQuery query(m_database);
    query.prepare("SELECT data FROM messages WHERE channel_id = :channel_id "
                  "ORDER BY timestamp DESC LIMIT :limit");
    query.bindValue(":channel_id", channelId);
    query.bindValue(":limit", limit);

    if (query.exec()) {
        while (query.next()) {
            QString data = query.value(0).toString();
            QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
            if (doc.isObject()) {
                result.append(doc.object());
            }
        }
    }

    return result;
}

void CacheManager::clearMessages(const QString &channelId)
{
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM messages WHERE channel_id = :channel_id");
    query.bindValue(":channel_id", channelId);
    query.exec();
}

void CacheManager::cacheUser(const QJsonObject &user)
{
    QSqlQuery query(m_database);
    query.prepare("INSERT OR REPLACE INTO users (id, name, data, last_updated) "
                  "VALUES (:id, :name, :data, :timestamp)");

    QString id = user["id"].toString();
    QString name = user["name"].toString();

    QJsonDocument doc(user);
    QString data = doc.toJson(QJsonDocument::Compact);

    query.bindValue(":id", id);
    query.bindValue(":name", name);
    query.bindValue(":data", data);
    query.bindValue(":timestamp", QDateTime::currentMSecsSinceEpoch() / 1000);

    if (!query.exec()) {
        qWarning() << "Failed to cache user:" << query.lastError().text();
    }
}

QJsonObject CacheManager::getCachedUser(const QString &userId)
{
    QSqlQuery query(m_database);
    query.prepare("SELECT data FROM users WHERE id = :id");
    query.bindValue(":id", userId);

    if (query.exec() && query.next()) {
        QString data = query.value(0).toString();
        QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
        return doc.object();
    }

    return QJsonObject();
}

void CacheManager::clearUsers()
{
    QSqlQuery query(m_database);
    query.exec("DELETE FROM users");
}

void CacheManager::setWorkspaceToken(const QString &token)
{
    QSqlQuery query(m_database);
    query.prepare("INSERT OR REPLACE INTO workspace (key, value) VALUES ('token', :token)");
    query.bindValue(":token", token);
    query.exec();
}

QString CacheManager::getWorkspaceToken()
{
    QSqlQuery query(m_database);
    query.prepare("SELECT value FROM workspace WHERE key = 'token'");

    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }

    return QString();
}

void CacheManager::clearWorkspace()
{
    QSqlQuery query(m_database);
    query.exec("DELETE FROM workspace");
}
