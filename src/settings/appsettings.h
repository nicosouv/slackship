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

signals:
    void notificationsEnabledChanged();
    void soundEnabledChanged();
    void themeChanged();
    void languageChanged();

private:
    QSettings m_settings;
};

#endif // APPSETTINGS_H
