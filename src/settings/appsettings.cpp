#include "appsettings.h"

AppSettings::AppSettings(QObject *parent)
    : QObject(parent)
    , m_settings("harbour-lagoon", "lagoon")
{
}

bool AppSettings::notificationsEnabled() const
{
    return m_settings.value("notifications/enabled", true).toBool();
}

void AppSettings::setNotificationsEnabled(bool enabled)
{
    if (notificationsEnabled() != enabled) {
        m_settings.setValue("notifications/enabled", enabled);
        emit notificationsEnabledChanged();
    }
}

bool AppSettings::soundEnabled() const
{
    return m_settings.value("notifications/sound", true).toBool();
}

void AppSettings::setSoundEnabled(bool enabled)
{
    if (soundEnabled() != enabled) {
        m_settings.setValue("notifications/sound", enabled);
        emit soundEnabledChanged();
    }
}

QString AppSettings::theme() const
{
    return m_settings.value("appearance/theme", "system").toString();
}

void AppSettings::setTheme(const QString &theme)
{
    if (this->theme() != theme) {
        m_settings.setValue("appearance/theme", theme);
        emit themeChanged();
    }
}

QString AppSettings::language() const
{
    // Default to system language, falling back to English
    return m_settings.value("language", "").toString();
}

void AppSettings::setLanguage(const QString &language)
{
    if (this->language() != language) {
        m_settings.setValue("language", language);
        emit languageChanged();
    }
}

qint64 AppSettings::totalBandwidthBytes() const
{
    return m_settings.value("bandwidth/total", 0).toLongLong();
}

void AppSettings::addBandwidthBytes(qint64 bytes)
{
    qint64 newTotal = totalBandwidthBytes() + bytes;
    m_settings.setValue("bandwidth/total", newTotal);
    emit totalBandwidthBytesChanged();
}

void AppSettings::resetBandwidthStats()
{
    m_settings.setValue("bandwidth/total", 0);
    emit totalBandwidthBytesChanged();
}

int AppSettings::pollingInterval() const
{
    // Default to 30 seconds
    return m_settings.value("api/pollingInterval", 30).toInt();
}

void AppSettings::setPollingInterval(int seconds)
{
    if (pollingInterval() != seconds && seconds > 0) {
        m_settings.setValue("api/pollingInterval", seconds);
        emit pollingIntervalChanged();
    }
}

bool AppSettings::channelsSectionExpanded() const
{
    // Default to expanded (true)
    return m_settings.value("ui/channelsSectionExpanded", true).toBool();
}

void AppSettings::setChannelsSectionExpanded(bool expanded)
{
    if (channelsSectionExpanded() != expanded) {
        m_settings.setValue("ui/channelsSectionExpanded", expanded);
        emit channelsSectionExpandedChanged();
    }
}

bool AppSettings::dmsSectionExpanded() const
{
    // Default to expanded (true)
    return m_settings.value("ui/dmsSectionExpanded", true).toBool();
}

void AppSettings::setDmsSectionExpanded(bool expanded)
{
    if (dmsSectionExpanded() != expanded) {
        m_settings.setValue("ui/dmsSectionExpanded", expanded);
        emit dmsSectionExpandedChanged();
    }
}

bool AppSettings::groupsSectionExpanded() const
{
    // Default to expanded (true)
    return m_settings.value("ui/groupsSectionExpanded", true).toBool();
}

void AppSettings::setGroupsSectionExpanded(bool expanded)
{
    if (groupsSectionExpanded() != expanded) {
        m_settings.setValue("ui/groupsSectionExpanded", expanded);
        emit groupsSectionExpandedChanged();
    }
}

bool AppSettings::dndEnabled() const
{
    // Default to disabled (false)
    return m_settings.value("notifications/dndEnabled", false).toBool();
}

void AppSettings::setDndEnabled(bool enabled)
{
    if (dndEnabled() != enabled) {
        m_settings.setValue("notifications/dndEnabled", enabled);
        emit dndEnabledChanged();
    }
}

int AppSettings::dndStartHour() const
{
    // Default to 22:00 (10 PM)
    return m_settings.value("notifications/dndStartHour", 22).toInt();
}

void AppSettings::setDndStartHour(int hour)
{
    if (dndStartHour() != hour && hour >= 0 && hour < 24) {
        m_settings.setValue("notifications/dndStartHour", hour);
        emit dndStartHourChanged();
    }
}

int AppSettings::dndStartMinute() const
{
    // Default to :00
    return m_settings.value("notifications/dndStartMinute", 0).toInt();
}

void AppSettings::setDndStartMinute(int minute)
{
    if (dndStartMinute() != minute && minute >= 0 && minute < 60) {
        m_settings.setValue("notifications/dndStartMinute", minute);
        emit dndStartMinuteChanged();
    }
}

int AppSettings::dndEndHour() const
{
    // Default to 08:00 (8 AM)
    return m_settings.value("notifications/dndEndHour", 8).toInt();
}

void AppSettings::setDndEndHour(int hour)
{
    if (dndEndHour() != hour && hour >= 0 && hour < 24) {
        m_settings.setValue("notifications/dndEndHour", hour);
        emit dndEndHourChanged();
    }
}

int AppSettings::dndEndMinute() const
{
    // Default to :00
    return m_settings.value("notifications/dndEndMinute", 0).toInt();
}

void AppSettings::setDndEndMinute(int minute)
{
    if (dndEndMinute() != minute && minute >= 0 && minute < 60) {
        m_settings.setValue("notifications/dndEndMinute", minute);
        emit dndEndMinuteChanged();
    }
}
