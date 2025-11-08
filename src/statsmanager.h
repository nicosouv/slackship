#ifndef STATSMANAGER_H
#define STATSMANAGER_H

#include <QObject>
#include <QHash>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>

struct MessageStats {
    int totalMessages = 0;
    int messagesThisWeek = 0;
    int messagesThisMonth = 0;
    int threadsStarted = 0;
    int threadReplies = 0;
    QDateTime firstMessageToday;
    QDateTime lastMessageToday;
    QHash<QString, int> channelActivity;  // channelId -> message count
    QHash<QString, int> userActivity;     // userId -> message count
    QHash<QString, int> emojiUsage;       // emoji -> count
    QHash<QString, int> reactionReceived; // emoji -> count
    QDateTime lastMessageDate;
    int currentStreak = 0;
};

class StatsManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int totalMessages READ totalMessages NOTIFY statsChanged)
    Q_PROPERTY(int messagesThisWeek READ messagesThisWeek NOTIFY statsChanged)
    Q_PROPERTY(int messagesThisMonth READ messagesThisMonth NOTIFY statsChanged)
    Q_PROPERTY(int currentStreak READ currentStreak NOTIFY statsChanged)
    Q_PROPERTY(QString mostActiveChannel READ mostActiveChannel NOTIFY statsChanged)
    Q_PROPERTY(QString mostUsedEmoji READ mostUsedEmoji NOTIFY statsChanged)
    Q_PROPERTY(QString mostContactedUser READ mostContactedUser NOTIFY statsChanged)

public:
    explicit StatsManager(QObject *parent = nullptr);
    ~StatsManager();

    // Property getters
    int totalMessages() const { return m_stats.totalMessages; }
    int messagesThisWeek() const { return m_stats.messagesThisWeek; }
    int messagesThisMonth() const { return m_stats.messagesThisMonth; }
    int currentStreak() const { return m_stats.currentStreak; }
    QString mostActiveChannel() const;
    QString mostUsedEmoji() const;
    QString mostContactedUser() const;

public slots:
    // Track new messages
    void trackMessage(const QJsonObject &message);
    void trackReaction(const QString &emoji);

    // Get detailed stats
    Q_INVOKABLE QVariantMap getChannelStats();
    Q_INVOKABLE QVariantMap getUserStats();
    Q_INVOKABLE QVariantMap getEmojiStats();
    Q_INVOKABLE QVariantMap getActivityByHour();
    Q_INVOKABLE QVariantMap getWeeklyActivity();

    // Persistence
    void loadStats();
    void saveStats();

    // Reset
    Q_INVOKABLE void resetStats();

signals:
    void statsChanged();

private:
    MessageStats m_stats;
    QHash<int, int> m_hourlyActivity;  // hour (0-23) -> message count
    QHash<QDate, int> m_dailyActivity; // date -> message count

    void updateStreak(const QDateTime &messageTime);
    void extractEmojis(const QString &text);
    QString getTopItem(const QHash<QString, int> &hash) const;
};

#endif // STATSMANAGER_H
