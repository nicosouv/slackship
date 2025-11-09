#ifndef SLACKIMAGEPROVIDER_H
#define SLACKIMAGEPROVIDER_H

#include <QObject>
#include <QQuickImageProvider>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QImage>
#include <QCache>
#include <QWaitCondition>
#include <QMutex>

class SlackImageProvider : public QQuickImageProvider
{
public:
    explicit SlackImageProvider();
    ~SlackImageProvider();

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

    // Called from main thread to set token
    void setToken(const QString &token);

private:
    QString m_token;
    QNetworkAccessManager *m_networkManager;
    QCache<QString, QImage> m_cache;  // Cache images by URL
    QMutex m_tokenMutex;
};

#endif // SLACKIMAGEPROVIDER_H
