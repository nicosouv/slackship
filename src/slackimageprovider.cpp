#include "slackimageprovider.h"
#include <QNetworkRequest>
#include <QEventLoop>
#include <QTimer>
#include <QDebug>
#include <QUrl>

SlackImageProvider::SlackImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
    , m_cache(100)  // Cache up to 100 images
{
    qDebug() << "SlackImageProvider created";
}

SlackImageProvider::~SlackImageProvider()
{
}

void SlackImageProvider::setToken(const QString &token)
{
    QMutexLocker locker(&m_tokenMutex);
    m_token = token;
    qDebug() << "SlackImageProvider: Token set (length:" << token.length() << ")";
}

QImage SlackImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    // The 'id' contains the URL we want to load
    // It comes from QML as: image://slack/https://files.slack.com/...
    QString imageUrl = id;

    qDebug() << "SlackImageProvider: Requesting image:" << imageUrl;

    // Check cache first
    if (m_cache.contains(imageUrl)) {
        qDebug() << "SlackImageProvider: Image found in cache";
        QImage *cachedImage = m_cache.object(imageUrl);
        if (cachedImage) {
            if (size) {
                *size = cachedImage->size();
            }
            return *cachedImage;
        }
    }

    // Get token safely
    QString token;
    {
        QMutexLocker locker(&m_tokenMutex);
        token = m_token;
    }

    if (token.isEmpty()) {
        qWarning() << "SlackImageProvider: No token set, cannot load image";
        return QImage();
    }

    // Create request with authorization header
    QUrl url(imageUrl);
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", QString("Bearer %1").arg(token).toUtf8());
    request.setHeader(QNetworkRequest::UserAgentHeader, "Lagoon-SailfishOS");

    // Create network manager in current thread to avoid thread warnings
    QNetworkAccessManager networkManager;

    // Synchronous download (required by QQuickImageProvider::requestImage)
    QNetworkReply *reply = networkManager.get(request);

    // Wait for reply with event loop
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    timeout.setInterval(10000);  // 10 second timeout

    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);

    timeout.start();
    loop.exec();

    if (!reply->isFinished() || timeout.isActive() == false) {
        qWarning() << "SlackImageProvider: Request timed out";
        reply->abort();
        reply->deleteLater();
        return QImage();
    }

    timeout.stop();

    // Check for errors
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "SlackImageProvider: Network error:" << reply->errorString();
        qWarning() << "SlackImageProvider: URL:" << imageUrl;
        qWarning() << "SlackImageProvider: Status code:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        reply->deleteLater();
        return QImage();
    }

    // Load image from reply data
    QByteArray imageData = reply->readAll();
    reply->deleteLater();

    if (imageData.isEmpty()) {
        qWarning() << "SlackImageProvider: Empty image data";
        return QImage();
    }

    QImage image;
    if (!image.loadFromData(imageData)) {
        qWarning() << "SlackImageProvider: Failed to load image from data";
        return QImage();
    }

    qDebug() << "SlackImageProvider: Image loaded successfully:" << image.size();

    // Cache the image
    m_cache.insert(imageUrl, new QImage(image));

    // Set size if requested
    if (size) {
        *size = image.size();
    }

    // Scale if requested
    if (requestedSize.isValid() && requestedSize.width() > 0 && requestedSize.height() > 0) {
        image = image.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    return image;
}
