#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QObject>
#include <QSettings>

class AppSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool notificationsEnabled READ notificationsEnabled WRITE setNotificationsEnabled NOTIFY notificationsEnabledChanged)
    Q_PROPERTY(bool soundEnabled READ soundEnabled WRITE setSoundEnabled NOTIFY soundEnabledChanged)
    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(qint64 totalBandwidthBytes READ totalBandwidthBytes NOTIFY totalBandwidthBytesChanged)
    Q_PROPERTY(int pollingInterval READ pollingInterval WRITE setPollingInterval NOTIFY pollingIntervalChanged)
    Q_PROPERTY(bool channelsSectionExpanded READ channelsSectionExpanded WRITE setChannelsSectionExpanded NOTIFY channelsSectionExpandedChanged)
    Q_PROPERTY(bool dmsSectionExpanded READ dmsSectionExpanded WRITE setDmsSectionExpanded NOTIFY dmsSectionExpandedChanged)
    Q_PROPERTY(bool groupsSectionExpanded READ groupsSectionExpanded WRITE setGroupsSectionExpanded NOTIFY groupsSectionExpandedChanged)

public:
    explicit AppSettings(QObject *parent = nullptr);

    bool notificationsEnabled() const;
    void setNotificationsEnabled(bool enabled);

    bool soundEnabled() const;
    void setSoundEnabled(bool enabled);

    QString theme() const;
    void setTheme(const QString &theme);

    QString language() const;
    void setLanguage(const QString &language);

    // Bandwidth tracking
    qint64 totalBandwidthBytes() const;
    void addBandwidthBytes(qint64 bytes);
    void resetBandwidthStats();

    // Polling configuration
    int pollingInterval() const;
    void setPollingInterval(int seconds);

    // Section expanded states
    bool channelsSectionExpanded() const;
    void setChannelsSectionExpanded(bool expanded);

    bool dmsSectionExpanded() const;
    void setDmsSectionExpanded(bool expanded);

    bool groupsSectionExpanded() const;
    void setGroupsSectionExpanded(bool expanded);

signals:
    void notificationsEnabledChanged();
    void soundEnabledChanged();
    void themeChanged();
    void languageChanged();
    void totalBandwidthBytesChanged();
    void pollingIntervalChanged();
    void channelsSectionExpandedChanged();
    void dmsSectionExpandedChanged();
    void groupsSectionExpandedChanged();

private:
    QSettings m_settings;
};

#endif // APPSETTINGS_H
