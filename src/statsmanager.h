#ifndef STATSMANAGER_H
#define STATSMANAGER_H

#include <QObject>
#include <QHash>
#include <QDate>
#include <QJsonObject>

// Simple daily stats per workspace
struct DailyStats {
    QDate date;
    int totalMessages = 0;      // All messages in workspace today
    int userMessages = 0;       // Current user's messages today
    QString currentUserId;      // To track user's own messages
};

class StatsManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int todayTotal READ todayTotal NOTIFY statsChanged)
    Q_PROPERTY(int todayUser READ todayUser NOTIFY statsChanged)

public:
    explicit StatsManager(QObject *parent = nullptr);
    ~StatsManager();

    // Property getters
    int todayTotal() const;
    int todayUser() const;

public slots:
    // Track new messages
    void trackMessage(const QJsonObject &message);

    // Set current user ID (called when workspace changes)
    void setCurrentUserId(const QString &userId);

    // Set current workspace (for per-workspace stats)
    void setCurrentWorkspace(const QString &workspaceId);

    // Persistence
    void loadStats();
    void saveStats();

    // Reset (clears all stats)
    Q_INVOKABLE void resetStats();

signals:
    void statsChanged();

private:
    // Stats per workspace
    QHash<QString, DailyStats> m_workspaceStats;  // workspaceId -> stats
    QString m_currentWorkspaceId;

    void checkAndResetDaily();
    DailyStats& getCurrentStats();
};

#endif // STATSMANAGER_H
