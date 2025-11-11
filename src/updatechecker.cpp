#include "updatechecker.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QUrl>

UpdateChecker::UpdateChecker(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_currentVersion("0.33.1")
    , m_updateAvailable(false)
    , m_checking(false)
{
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &UpdateChecker::handleNetworkReply);
}

UpdateChecker::~UpdateChecker()
{
}

void UpdateChecker::checkForUpdates()
{
    if (m_checking) {
        qDebug() << "Update check already in progress";
        return;
    }

    setChecking(true);

    QUrl url("https://api.github.com/repos/nicosouv/lagoon/releases/latest");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "Lagoon-SailfishOS");

    qDebug() << "Checking for updates at:" << url.toString();
    m_networkManager->get(request);
}

void UpdateChecker::handleNetworkReply(QNetworkReply *reply)
{
    setChecking(false);

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Update check failed:" << reply->errorString();
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Invalid JSON response from GitHub API";
        return;
    }

    QJsonObject obj = doc.object();
    QString tagName = obj["tag_name"].toString();
    QString htmlUrl = obj["html_url"].toString();

    if (tagName.isEmpty()) {
        qWarning() << "No tag_name found in GitHub response";
        return;
    }

    // Remove 'v' prefix if present (e.g., "v0.25.1" -> "0.25.1")
    QString version = tagName;
    if (version.startsWith("v", Qt::CaseInsensitive)) {
        version = version.mid(1);
    }

    qDebug() << "Latest version on GitHub:" << version;
    qDebug() << "Current version:" << m_currentVersion;
    qDebug() << "Release URL:" << htmlUrl;

    setLatestVersion(version);
    setReleaseUrl(htmlUrl);

    bool newer = isNewerVersion(version, m_currentVersion);
    setUpdateAvailable(newer);

    if (newer) {
        qDebug() << "Update available:" << version;
    } else {
        qDebug() << "App is up to date";
    }
}

bool UpdateChecker::isNewerVersion(const QString &latest, const QString &current)
{
    // Parse version strings like "0.25.1" into components
    QStringList latestParts = latest.split('.');
    QStringList currentParts = current.split('.');

    // Ensure both have at least 3 parts (major.minor.patch)
    while (latestParts.size() < 3) latestParts.append("0");
    while (currentParts.size() < 3) currentParts.append("0");

    // Compare each component
    for (int i = 0; i < 3; ++i) {
        int latestNum = latestParts[i].toInt();
        int currentNum = currentParts[i].toInt();

        if (latestNum > currentNum) {
            return true;
        } else if (latestNum < currentNum) {
            return false;
        }
        // If equal, continue to next component
    }

    // Versions are equal
    return false;
}

void UpdateChecker::setLatestVersion(const QString &version)
{
    if (m_latestVersion != version) {
        m_latestVersion = version;
        emit latestVersionChanged();
    }
}

void UpdateChecker::setUpdateAvailable(bool available)
{
    if (m_updateAvailable != available) {
        m_updateAvailable = available;
        emit updateAvailableChanged();
    }
}

void UpdateChecker::setChecking(bool checking)
{
    if (m_checking != checking) {
        m_checking = checking;
        emit checkingChanged();
    }
}

void UpdateChecker::setReleaseUrl(const QString &url)
{
    if (m_releaseUrl != url) {
        m_releaseUrl = url;
        emit releaseUrlChanged();
    }
}
