#ifndef OAUTHMANAGER_H
#define OAUTHMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTcpServer>
#include <QUrl>

class OAuthManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isAuthenticating READ isAuthenticating NOTIFY authenticatingChanged)

public:
    explicit OAuthManager(QObject *parent = nullptr);
    ~OAuthManager();

    bool isAuthenticating() const { return m_isAuthenticating; }

    // Slack OAuth configuration
    static const QString CLIENT_ID;
    static const QString CLIENT_SECRET;
    static const QString REDIRECT_URI;
    static const QString AUTHORIZATION_URL;
    static const QString TOKEN_URL;
    static const QString SCOPES;

public slots:
    // Start OAuth flow
    void startAuthentication();

    // Cancel ongoing authentication
    void cancelAuthentication();

    // Exchange authorization code for token
    void exchangeCodeForToken(const QString &code);

signals:
    // Authentication completed successfully
    void authenticationSucceeded(const QString &accessToken,
                                 const QString &teamId,
                                 const QString &teamName,
                                 const QString &userId,
                                 const QString &userName);

    // Authentication failed
    void authenticationFailed(const QString &error);

    // Status changed
    void authenticatingChanged();

private slots:
    void handleIncomingConnection();
    void handleTokenResponse();

private:
    void openAuthorizationUrl();
    bool startLocalServer();
    void stopLocalServer();
    QString generateRandomState();
    void sendResponseToClient(QTcpSocket *socket, const QString &message);

    QNetworkAccessManager *m_networkManager;
    QTcpServer *m_localServer;

    bool m_isAuthenticating;
    QString m_state; // CSRF protection
    quint16 m_localPort;
};

#endif // OAUTHMANAGER_H
