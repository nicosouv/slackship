#ifndef UPDATECHECKER_H
#define UPDATECHECKER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>

class UpdateChecker : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentVersion READ currentVersion CONSTANT)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY latestVersionChanged)
    Q_PROPERTY(bool updateAvailable READ updateAvailable NOTIFY updateAvailableChanged)
    Q_PROPERTY(bool checking READ checking NOTIFY checkingChanged)
    Q_PROPERTY(QString releaseUrl READ releaseUrl NOTIFY releaseUrlChanged)

public:
    explicit UpdateChecker(QObject *parent = nullptr);
    ~UpdateChecker();

    QString currentVersion() const { return m_currentVersion; }
    QString latestVersion() const { return m_latestVersion; }
    bool updateAvailable() const { return m_updateAvailable; }
    bool checking() const { return m_checking; }
    QString releaseUrl() const { return m_releaseUrl; }

public slots:
    void checkForUpdates();

signals:
    void latestVersionChanged();
    void updateAvailableChanged();
    void checkingChanged();
    void releaseUrlChanged();

private slots:
    void handleNetworkReply(QNetworkReply *reply);

private:
    QNetworkAccessManager *m_networkManager;
    QString m_currentVersion;
    QString m_latestVersion;
    bool m_updateAvailable;
    bool m_checking;
    QString m_releaseUrl;

    bool isNewerVersion(const QString &latest, const QString &current);
    void setLatestVersion(const QString &version);
    void setUpdateAvailable(bool available);
    void setChecking(bool checking);
    void setReleaseUrl(const QString &url);
};

#endif // UPDATECHECKER_H
