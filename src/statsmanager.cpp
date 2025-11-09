#include "statsmanager.h"
#include <QSettings>
#include <QDebug>

StatsManager::StatsManager(QObject *parent)
    : QObject(parent)
{
    loadStats();
}

StatsManager::~StatsManager()
{
    saveStats();
}

int StatsManager::todayTotal() const
{
    if (m_currentWorkspaceId.isEmpty()) {
        return 0;
    }

    if (!m_workspaceStats.contains(m_currentWorkspaceId)) {
        return 0;
    }

    return m_workspaceStats[m_currentWorkspaceId].totalMessages;
}

int StatsManager::todayUser() const
{
    if (m_currentWorkspaceId.isEmpty()) {
        return 0;
    }

    if (!m_workspaceStats.contains(m_currentWorkspaceId)) {
        return 0;
    }

    return m_workspaceStats[m_currentWorkspaceId].userMessages;
}

void StatsManager::trackMessage(const QJsonObject &message)
{
    // Skip if no workspace set
    if (m_currentWorkspaceId.isEmpty()) {
        qDebug() << "StatsManager: No workspace set, ignoring message";
        return;
    }

    // Check and reset daily stats if needed
    checkAndResetDaily();

    // Get current workspace stats
    DailyStats &stats = getCurrentStats();

    // Get message info
    QString userId = message["user"].toString();

    // Skip bot messages
    if (message["bot_id"].toString().length() > 0) {
        return;
    }

    // Increment total messages for workspace
    stats.totalMessages++;

    // Increment user messages if it's from current user
    if (!userId.isEmpty() && userId == stats.currentUserId) {
        stats.userMessages++;
    }

    emit statsChanged();

    // Auto-save every 10 messages
    if (stats.totalMessages % 10 == 0) {
        saveStats();
    }

    qDebug() << "StatsManager: Tracked message -"
             << "Total today:" << stats.totalMessages
             << "User today:" << stats.userMessages;
}

void StatsManager::setCurrentUserId(const QString &userId)
{
    if (m_currentWorkspaceId.isEmpty()) {
        return;
    }

    DailyStats &stats = getCurrentStats();
    stats.currentUserId = userId;

    qDebug() << "StatsManager: Set current user ID to" << userId
             << "for workspace" << m_currentWorkspaceId;
}

void StatsManager::setCurrentWorkspace(const QString &workspaceId)
{
    if (workspaceId == m_currentWorkspaceId) {
        return;
    }

    m_currentWorkspaceId = workspaceId;

    // Initialize workspace stats if needed
    if (!m_workspaceStats.contains(workspaceId)) {
        DailyStats stats;
        stats.date = QDate::currentDate();
        m_workspaceStats[workspaceId] = stats;
    }

    qDebug() << "StatsManager: Switched to workspace" << workspaceId;
    emit statsChanged();
}

void StatsManager::checkAndResetDaily()
{
    if (m_currentWorkspaceId.isEmpty()) {
        return;
    }

    DailyStats &stats = getCurrentStats();
    QDate today = QDate::currentDate();

    // If the date has changed, reset stats for this workspace
    if (stats.date != today) {
        qDebug() << "StatsManager: New day detected! Resetting daily stats for workspace"
                 << m_currentWorkspaceId;
        qDebug() << "  Previous date:" << stats.date.toString()
                 << "| Today:" << today.toString();

        QString savedUserId = stats.currentUserId;  // Preserve user ID
        stats = DailyStats();
        stats.date = today;
        stats.currentUserId = savedUserId;

        emit statsChanged();
        saveStats();
    }
}

DailyStats& StatsManager::getCurrentStats()
{
    // Ensure workspace exists
    if (!m_workspaceStats.contains(m_currentWorkspaceId)) {
        DailyStats stats;
        stats.date = QDate::currentDate();
        m_workspaceStats[m_currentWorkspaceId] = stats;
    }

    return m_workspaceStats[m_currentWorkspaceId];
}

void StatsManager::loadStats()
{
    QSettings settings("harbour-lagoon", "stats");

    int workspaceCount = settings.beginReadArray("workspaces");
    for (int i = 0; i < workspaceCount; ++i) {
        settings.setArrayIndex(i);

        QString workspaceId = settings.value("workspaceId").toString();
        DailyStats stats;
        stats.date = settings.value("date").toDate();
        stats.totalMessages = settings.value("totalMessages", 0).toInt();
        stats.userMessages = settings.value("userMessages", 0).toInt();
        stats.currentUserId = settings.value("currentUserId").toString();

        m_workspaceStats[workspaceId] = stats;
    }
    settings.endArray();

    m_currentWorkspaceId = settings.value("currentWorkspaceId").toString();

    qDebug() << "StatsManager: Loaded stats for" << workspaceCount << "workspaces";
    qDebug() << "  Current workspace:" << m_currentWorkspaceId;
}

void StatsManager::saveStats()
{
    QSettings settings("harbour-lagoon", "stats");

    settings.beginWriteArray("workspaces");
    int idx = 0;
    for (auto it = m_workspaceStats.begin(); it != m_workspaceStats.end(); ++it) {
        settings.setArrayIndex(idx++);
        settings.setValue("workspaceId", it.key());
        settings.setValue("date", it.value().date);
        settings.setValue("totalMessages", it.value().totalMessages);
        settings.setValue("userMessages", it.value().userMessages);
        settings.setValue("currentUserId", it.value().currentUserId);
    }
    settings.endArray();

    settings.setValue("currentWorkspaceId", m_currentWorkspaceId);
    settings.sync();

    qDebug() << "StatsManager: Saved stats for" << m_workspaceStats.size() << "workspaces";
}

void StatsManager::resetStats()
{
    m_workspaceStats.clear();
    m_currentWorkspaceId.clear();

    saveStats();
    emit statsChanged();

    qDebug() << "StatsManager: All stats reset";
}
