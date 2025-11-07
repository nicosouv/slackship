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
