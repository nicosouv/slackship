#include "statsmanager.h"
#include <QSettings>
#include <QRegularExpression>
#include <QDebug>
#include <algorithm>

StatsManager::StatsManager(QObject *parent)
    : QObject(parent)
{
    loadStats();
}

StatsManager::~StatsManager()
{
    saveStats();
}

void StatsManager::trackMessage(const QJsonObject &message)
{
    QString userId = message["user"].toString();
    QString channelId = message["channel"].toString();
    QString text = message["text"].toString();
    QString threadTs = message["thread_ts"].toString();
    bool isThreadParent = !threadTs.isEmpty() && message["ts"].toString() == threadTs;
    bool isThreadReply = !threadTs.isEmpty() && message["ts"].toString() != threadTs;

    // Get timestamp (Qt 5.6 compatible - use fromMSecsSinceEpoch instead of fromSecsSinceEpoch)
    double tsDouble = message["ts"].toString().toDouble();
    QDateTime messageTime = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(tsDouble * 1000));

    // Update total messages
    m_stats.totalMessages++;

    // Update weekly/monthly counts
    QDateTime now = QDateTime::currentDateTime();
    qint64 daysAgo = messageTime.daysTo(now);

    if (daysAgo <= 7) {
        m_stats.messagesThisWeek++;
    }
    if (daysAgo <= 30) {
        m_stats.messagesThisMonth++;
    }

    // Track first/last message today
    QDate today = QDate::currentDate();
    if (messageTime.date() == today) {
        if (!m_stats.firstMessageToday.isValid() || messageTime < m_stats.firstMessageToday) {
            m_stats.firstMessageToday = messageTime;
        }
        if (!m_stats.lastMessageToday.isValid() || messageTime > m_stats.lastMessageToday) {
            m_stats.lastMessageToday = messageTime;
        }
    }

    // Track threads
    if (isThreadParent) {
        m_stats.threadsStarted++;
    } else if (isThreadReply) {
        m_stats.threadReplies++;
    }

    // Track channel activity
    if (!channelId.isEmpty()) {
        m_stats.channelActivity[channelId]++;
    }

    // Track user activity (DMs)
    if (!userId.isEmpty()) {
        m_stats.userActivity[userId]++;
    }

    // Track hourly activity
    int hour = messageTime.time().hour();
    m_hourlyActivity[hour]++;

    // Track daily activity
    QDate date = messageTime.date();
    m_dailyActivity[date]++;

    // Extract and count emojis
    extractEmojis(text);

    // Update streak
    updateStreak(messageTime);

    emit statsChanged();

    // Auto-save periodically (every 10 messages)
    if (m_stats.totalMessages % 10 == 0) {
        saveStats();
    }
}

void StatsManager::trackReaction(const QString &emoji)
{
    if (!emoji.isEmpty()) {
        m_stats.reactionReceived[emoji]++;
        emit statsChanged();
    }
}

void StatsManager::updateStreak(const QDateTime &messageTime)
{
    QDate messageDate = messageTime.date();
    QDate today = QDate::currentDate();

    // Only update streak for messages from today or yesterday (ignore old history)
    if (messageDate < today.addDays(-1)) {
        return;
    }

    if (!m_stats.lastMessageDate.isValid()) {
        // First message ever
        m_stats.currentStreak = 1;
        m_stats.lastMessageDate = messageTime;
        return;
    }

    QDate lastDate = m_stats.lastMessageDate.date();

    if (messageDate == today) {
        // Message today
        if (lastDate == today.addDays(-1)) {
            // Continue streak from yesterday
            m_stats.currentStreak++;
            m_stats.lastMessageDate = messageTime;
        } else if (lastDate == today) {
            // Already counted today, just update timestamp
            m_stats.lastMessageDate = messageTime;
        } else {
            // Broke streak, start new
            m_stats.currentStreak = 1;
            m_stats.lastMessageDate = messageTime;
        }
    } else if (messageDate == today.addDays(-1) && lastDate < messageDate) {
        // Message from yesterday
        if (lastDate == today.addDays(-2)) {
            // Continue streak
            m_stats.currentStreak++;
        } else {
            // Start new streak
            m_stats.currentStreak = 1;
        }
        m_stats.lastMessageDate = messageTime;
    }
}

void StatsManager::extractEmojis(const QString &text)
{
    // Extract :emoji_name: format
    QRegularExpression emojiRegex(":([a-z0-9_+-]+):");
    QRegularExpressionMatchIterator matches = emojiRegex.globalMatch(text);

    while (matches.hasNext()) {
        QRegularExpressionMatch match = matches.next();
        QString emoji = match.captured(1);
        m_stats.emojiUsage[emoji]++;
    }
}

QString StatsManager::getTopItem(const QHash<QString, int> &hash) const
{
    if (hash.isEmpty()) {
        return QString();
    }

    auto maxIt = std::max_element(hash.begin(), hash.end());
    return maxIt.key();
}

QString StatsManager::mostActiveChannel() const
{
    return getTopItem(m_stats.channelActivity);
}

QString StatsManager::mostUsedEmoji() const
{
    return getTopItem(m_stats.emojiUsage);
}

QString StatsManager::mostContactedUser() const
{
    return getTopItem(m_stats.userActivity);
}

QVariantMap StatsManager::getChannelStats()
{
    QVariantMap result;
    QVariantList channels;

    // Convert to list and sort by count
    QList<QPair<QString, int>> sortedChannels;
    for (auto it = m_stats.channelActivity.begin(); it != m_stats.channelActivity.end(); ++it) {
        sortedChannels.append(qMakePair(it.key(), it.value()));
    }

    std::sort(sortedChannels.begin(), sortedChannels.end(),
              [](const QPair<QString, int> &a, const QPair<QString, int> &b) {
        return a.second > b.second;
    });

    // Take top 10
    for (int i = 0; i < qMin(10, sortedChannels.size()); ++i) {
        QVariantMap channel;
        channel["id"] = sortedChannels[i].first;
        channel["count"] = sortedChannels[i].second;
        channels.append(channel);
    }

    result["channels"] = channels;
    return result;
}

QVariantMap StatsManager::getUserStats()
{
    QVariantMap result;
    QVariantList users;

    QList<QPair<QString, int>> sortedUsers;
    for (auto it = m_stats.userActivity.begin(); it != m_stats.userActivity.end(); ++it) {
        sortedUsers.append(qMakePair(it.key(), it.value()));
    }

    std::sort(sortedUsers.begin(), sortedUsers.end(),
              [](const QPair<QString, int> &a, const QPair<QString, int> &b) {
        return a.second > b.second;
    });

    for (int i = 0; i < qMin(10, sortedUsers.size()); ++i) {
        QVariantMap user;
        user["id"] = sortedUsers[i].first;
        user["count"] = sortedUsers[i].second;
        users.append(user);
    }

    result["users"] = users;
    return result;
}

QVariantMap StatsManager::getEmojiStats()
{
    QVariantMap result;
    QVariantList emojis;

    QList<QPair<QString, int>> sortedEmojis;
    for (auto it = m_stats.emojiUsage.begin(); it != m_stats.emojiUsage.end(); ++it) {
        sortedEmojis.append(qMakePair(it.key(), it.value()));
    }

    std::sort(sortedEmojis.begin(), sortedEmojis.end(),
              [](const QPair<QString, int> &a, const QPair<QString, int> &b) {
        return a.second > b.second;
    });

    for (int i = 0; i < qMin(10, sortedEmojis.size()); ++i) {
        QVariantMap emoji;
        emoji["name"] = sortedEmojis[i].first;
        emoji["count"] = sortedEmojis[i].second;
        emojis.append(emoji);
    }

    result["emojis"] = emojis;
    return result;
}

QVariantMap StatsManager::getActivityByHour()
{
    QVariantMap result;
    QVariantList hours;

    for (int h = 0; h < 24; ++h) {
        QVariantMap hour;
        hour["hour"] = h;
        hour["count"] = m_hourlyActivity.value(h, 0);
        hours.append(hour);
    }

    result["hours"] = hours;
    return result;
}

QVariantMap StatsManager::getWeeklyActivity()
{
    QVariantMap result;
    QVariantList days;

    QDate today = QDate::currentDate();
    for (int i = 6; i >= 0; --i) {
        QDate date = today.addDays(-i);
        QVariantMap day;
        day["date"] = date.toString("yyyy-MM-dd");
        day["count"] = m_dailyActivity.value(date, 0);
        days.append(day);
    }

    result["days"] = days;
    return result;
}

void StatsManager::loadStats()
{
    QSettings settings("harbour-lagoon", "stats");

    m_stats.totalMessages = settings.value("totalMessages", 0).toInt();
    m_stats.messagesThisWeek = settings.value("messagesThisWeek", 0).toInt();
    m_stats.messagesThisMonth = settings.value("messagesThisMonth", 0).toInt();
    m_stats.threadsStarted = settings.value("threadsStarted", 0).toInt();
    m_stats.threadReplies = settings.value("threadReplies", 0).toInt();
    m_stats.currentStreak = settings.value("currentStreak", 0).toInt();
    m_stats.lastMessageDate = settings.value("lastMessageDate").toDateTime();

    // Load channel activity
    int channelActivitySize = settings.beginReadArray("channelActivity");
    for (int i = 0; i < channelActivitySize; ++i) {
        settings.setArrayIndex(i);
        QString channelId = settings.value("channelId").toString();
        int count = settings.value("count").toInt();
        m_stats.channelActivity[channelId] = count;
    }
    settings.endArray();

    // Load user activity
    int userActivitySize = settings.beginReadArray("userActivity");
    for (int i = 0; i < userActivitySize; ++i) {
        settings.setArrayIndex(i);
        QString userId = settings.value("userId").toString();
        int count = settings.value("count").toInt();
        m_stats.userActivity[userId] = count;
    }
    settings.endArray();

    // Load emoji usage
    int emojiUsageSize = settings.beginReadArray("emojiUsage");
    for (int i = 0; i < emojiUsageSize; ++i) {
        settings.setArrayIndex(i);
        QString emoji = settings.value("emoji").toString();
        int count = settings.value("count").toInt();
        m_stats.emojiUsage[emoji] = count;
    }
    settings.endArray();

    // Load hourly activity
    int hourlyActivitySize = settings.beginReadArray("hourlyActivity");
    for (int i = 0; i < hourlyActivitySize; ++i) {
        settings.setArrayIndex(i);
        int hour = settings.value("hour").toInt();
        int count = settings.value("count").toInt();
        m_hourlyActivity[hour] = count;
    }
    settings.endArray();

    // Load daily activity
    int dailyActivitySize = settings.beginReadArray("dailyActivity");
    for (int i = 0; i < dailyActivitySize; ++i) {
        settings.setArrayIndex(i);
        QDate date = settings.value("date").toDate();
        int count = settings.value("count").toInt();
        m_dailyActivity[date] = count;
    }
    settings.endArray();

    qDebug() << "Loaded stats:" << m_stats.totalMessages << "total messages,"
             << m_dailyActivity.size() << "days of activity";
}

void StatsManager::saveStats()
{
    QSettings settings("harbour-lagoon", "stats");

    settings.setValue("totalMessages", m_stats.totalMessages);
    settings.setValue("messagesThisWeek", m_stats.messagesThisWeek);
    settings.setValue("messagesThisMonth", m_stats.messagesThisMonth);
    settings.setValue("threadsStarted", m_stats.threadsStarted);
    settings.setValue("threadReplies", m_stats.threadReplies);
    settings.setValue("currentStreak", m_stats.currentStreak);
    settings.setValue("lastMessageDate", m_stats.lastMessageDate);

    // Save channel activity
    settings.beginWriteArray("channelActivity");
    int idx = 0;
    for (auto it = m_stats.channelActivity.begin(); it != m_stats.channelActivity.end(); ++it) {
        settings.setArrayIndex(idx++);
        settings.setValue("channelId", it.key());
        settings.setValue("count", it.value());
    }
    settings.endArray();

    // Save user activity
    settings.beginWriteArray("userActivity");
    idx = 0;
    for (auto it = m_stats.userActivity.begin(); it != m_stats.userActivity.end(); ++it) {
        settings.setArrayIndex(idx++);
        settings.setValue("userId", it.key());
        settings.setValue("count", it.value());
    }
    settings.endArray();

    // Save emoji usage
    settings.beginWriteArray("emojiUsage");
    idx = 0;
    for (auto it = m_stats.emojiUsage.begin(); it != m_stats.emojiUsage.end(); ++it) {
        settings.setArrayIndex(idx++);
        settings.setValue("emoji", it.key());
        settings.setValue("count", it.value());
    }
    settings.endArray();

    // Save hourly activity
    settings.beginWriteArray("hourlyActivity");
    idx = 0;
    for (auto it = m_hourlyActivity.begin(); it != m_hourlyActivity.end(); ++it) {
        settings.setArrayIndex(idx++);
        settings.setValue("hour", it.key());
        settings.setValue("count", it.value());
    }
    settings.endArray();

    // Save daily activity
    settings.beginWriteArray("dailyActivity");
    idx = 0;
    for (auto it = m_dailyActivity.begin(); it != m_dailyActivity.end(); ++it) {
        settings.setArrayIndex(idx++);
        settings.setValue("date", it.key());
        settings.setValue("count", it.value());
    }
    settings.endArray();

    settings.sync();

    qDebug() << "Saved stats:" << m_stats.totalMessages << "total messages,"
             << m_dailyActivity.size() << "days of activity";
}

void StatsManager::resetStats()
{
    m_stats = MessageStats();
    m_hourlyActivity.clear();
    m_dailyActivity.clear();

    saveStats();
    emit statsChanged();
}
