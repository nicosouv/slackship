#include "oauthmanager.h"
#include <QDesktopServices>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpSocket>
#include <QDateTime>
#include <QDebug>
#include <cstdlib>

// Slack OAuth configuration
// These are loaded from environment variables for security
// Set SLACKSHIP_CLIENT_ID and SLACKSHIP_CLIENT_SECRET before building
// For development/testing, you need to create your own Slack app at api.slack.com/apps
const QString OAuthManager::CLIENT_ID = QString::fromUtf8(qgetenv("SLACKSHIP_CLIENT_ID").isEmpty() ?
    "YOUR_CLIENT_ID_HERE" : qgetenv("SLACKSHIP_CLIENT_ID"));
const QString OAuthManager::CLIENT_SECRET = QString::fromUtf8(qgetenv("SLACKSHIP_CLIENT_SECRET").isEmpty() ?
    "YOUR_CLIENT_SECRET_HERE" : qgetenv("SLACKSHIP_CLIENT_SECRET"));
const QString OAuthManager::REDIRECT_URI = "http://localhost:8080/callback";
const QString OAuthManager::AUTHORIZATION_URL = "https://slack.com/oauth/v2/authorize";
const QString OAuthManager::TOKEN_URL = "https://slack.com/api/oauth.v2.access";
const QString OAuthManager::SCOPES = "channels:history,channels:read,channels:write,"
                                     "chat:write,groups:history,groups:read,"
                                     "im:history,im:read,im:write,"
                                     "mpim:history,mpim:read,"
                                     "users:read,reactions:read,reactions:write,"
                                     "files:read,files:write,search:read";

OAuthManager::OAuthManager(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_localServer(new QTcpServer(this))
    , m_isAuthenticating(false)
    , m_localPort(8080)
{
    connect(m_localServer, &QTcpServer::newConnection,
            this, &OAuthManager::handleIncomingConnection);
}

OAuthManager::~OAuthManager()
{
    stopLocalServer();
}

void OAuthManager::startAuthentication()
{
    if (m_isAuthenticating) {
        qDebug() << "Authentication already in progress";
        return;
    }

    qDebug() << "Starting OAuth authentication flow...";

    m_isAuthenticating = true;
    emit authenticatingChanged();

    // Generate random state for CSRF protection
    m_state = generateRandomState();

    // Start local HTTP server to receive callback
    if (!startLocalServer()) {
        emit authenticationFailed("Failed to start local callback server");
        m_isAuthenticating = false;
        emit authenticatingChanged();
        return;
    }

    // Open browser with authorization URL
    openAuthorizationUrl();
}

void OAuthManager::cancelAuthentication()
{
    qDebug() << "Canceling authentication";

    stopLocalServer();
    m_isAuthenticating = false;
    emit authenticatingChanged();
}

void OAuthManager::exchangeCodeForToken(const QString &code)
{
    qDebug() << "Exchanging authorization code for access token...";

    // Prepare token request
    QUrlQuery postData;
    postData.addQueryItem("client_id", CLIENT_ID);
    postData.addQueryItem("client_secret", CLIENT_SECRET);
    postData.addQueryItem("code", code);
    postData.addQueryItem("redirect_uri", REDIRECT_URI);

    QNetworkRequest request(TOKEN_URL);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QNetworkReply *reply = m_networkManager->post(request, postData.toString(QUrl::FullyEncoded).toUtf8());

    connect(reply, &QNetworkReply::finished,
            this, &OAuthManager::handleTokenResponse);
}

void OAuthManager::handleIncomingConnection()
{
    qDebug() << "Received OAuth callback connection";

    QTcpSocket *socket = m_localServer->nextPendingConnection();

    if (!socket) {
        return;
    }

    connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
        QString request = socket->readAll();
        qDebug() << "OAuth callback request:" << request;

        // Parse HTTP request
        QStringList lines = request.split("\r\n");
        if (lines.isEmpty()) {
            socket->deleteLater();
            return;
        }

        // Extract URL from first line (GET /callback?code=xxx&state=yyy HTTP/1.1)
        QStringList requestLine = lines[0].split(" ");
        if (requestLine.size() < 2) {
            sendResponseToClient(socket, "Invalid request");
            socket->deleteLater();
            return;
        }

        QString path = requestLine[1];
        QUrl url("http://localhost" + path);
        QUrlQuery query(url);

        // Check for error
        if (query.hasQueryItem("error")) {
            QString error = query.queryItemValue("error");
            qWarning() << "OAuth error:" << error;

            sendResponseToClient(socket, "Authentication failed: " + error);

            emit authenticationFailed(error);
            m_isAuthenticating = false;
            emit authenticatingChanged();

            socket->deleteLater();
            stopLocalServer();
            return;
        }

        // Verify state (CSRF protection)
        QString state = query.queryItemValue("state");
        if (state != m_state) {
            qWarning() << "State mismatch! Possible CSRF attack.";
            sendResponseToClient(socket, "Security error: state mismatch");

            emit authenticationFailed("State mismatch");
            m_isAuthenticating = false;
            emit authenticatingChanged();

            socket->deleteLater();
            stopLocalServer();
            return;
        }

        // Get authorization code
        QString code = query.queryItemValue("code");
        if (code.isEmpty()) {
            sendResponseToClient(socket, "No authorization code received");

            emit authenticationFailed("No authorization code");
            m_isAuthenticating = false;
            emit authenticatingChanged();

            socket->deleteLater();
            stopLocalServer();
            return;
        }

        // Send success response to browser
        sendResponseToClient(socket, "Authentication successful! You can close this window.");

        // Exchange code for token
        exchangeCodeForToken(code);

        socket->deleteLater();
        stopLocalServer();
    });
}

void OAuthManager::handleTokenResponse()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }

    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        QString error = reply->errorString();
        qWarning() << "Token request failed:" << error;

        emit authenticationFailed(error);
        m_isAuthenticating = false;
        emit authenticatingChanged();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (!doc.isObject()) {
        emit authenticationFailed("Invalid response from Slack");
        m_isAuthenticating = false;
        emit authenticatingChanged();
        return;
    }

    QJsonObject response = doc.object();

    if (!response["ok"].toBool()) {
        QString error = response["error"].toString();
        qWarning() << "Token exchange failed:" << error;

        emit authenticationFailed(error);
        m_isAuthenticating = false;
        emit authenticatingChanged();
        return;
    }

    // Extract token and team info
    QString accessToken = response["authed_user"].toObject()["access_token"].toString();
    QJsonObject team = response["team"].toObject();
    QString teamId = team["id"].toString();
    QString teamName = team["name"].toString();

    QJsonObject authedUser = response["authed_user"].toObject();
    QString userId = authedUser["id"].toString();

    qDebug() << "OAuth authentication succeeded!";
    qDebug() << "Team:" << teamName << "(" << teamId << ")";
    qDebug() << "User:" << userId;

    m_isAuthenticating = false;
    emit authenticatingChanged();

    // Emit success with all information
    emit authenticationSucceeded(accessToken, teamId, teamName, userId, QString());
}

void OAuthManager::openAuthorizationUrl()
{
    QUrl url(AUTHORIZATION_URL);
    QUrlQuery query;

    query.addQueryItem("client_id", CLIENT_ID);
    query.addQueryItem("scope", SCOPES);
    query.addQueryItem("redirect_uri", REDIRECT_URI);
    query.addQueryItem("state", m_state);
    query.addQueryItem("user_scope", SCOPES); // For user token

    url.setQuery(query);

    qDebug() << "Opening browser with URL:" << url.toString();

    if (!QDesktopServices::openUrl(url)) {
        qWarning() << "Failed to open browser";
        emit authenticationFailed("Failed to open browser");
        m_isAuthenticating = false;
        emit authenticatingChanged();
    }
}

bool OAuthManager::startLocalServer()
{
    // Try to bind to port 8080, if busy try 8081-8090
    for (quint16 port = 8080; port < 8090; ++port) {
        if (m_localServer->listen(QHostAddress::LocalHost, port)) {
            m_localPort = port;
            qDebug() << "Local server started on port:" << port;
            return true;
        }
    }

    qWarning() << "Failed to start local server on any port";
    return false;
}

void OAuthManager::stopLocalServer()
{
    if (m_localServer->isListening()) {
        m_localServer->close();
        qDebug() << "Local server stopped";
    }
}

QString OAuthManager::generateRandomState()
{
    // Generate a random 32-character state string
    const QString chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    QString state;

    // Seed random generator with current time if not already seeded
    static bool seeded = false;
    if (!seeded) {
        qsrand(static_cast<uint>(QDateTime::currentMSecsSinceEpoch()));
        seeded = true;
    }

    for (int i = 0; i < 32; ++i) {
        int index = qrand() % chars.length();
        state.append(chars.at(index));
    }

    return state;
}

void OAuthManager::sendResponseToClient(QTcpSocket *socket, const QString &message)
{
    QString html = QString(
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<title>SlackShip Authentication</title>"
        "<meta charset='utf-8'>"
        "<style>"
        "body { font-family: sans-serif; text-align: center; padding: 50px; }"
        "h1 { color: #4A154B; }"
        "</style>"
        "</head>"
        "<body>"
        "<h1>SlackShip</h1>"
        "<p>%1</p>"
        "<p><small>You can close this window now.</small></p>"
        "</body>"
        "</html>"
    ).arg(message);

    QString response = QString(
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Content-Length: %1\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%2"
    ).arg(html.toUtf8().length()).arg(html);

    socket->write(response.toUtf8());
    socket->flush();
}

QString OAuthManager::getAuthorizationUrl()
{
    // Generate state if not already set
    if (m_state.isEmpty()) {
        m_state = generateRandomState();
    }

    QUrl url(AUTHORIZATION_URL);
    QUrlQuery query;

    query.addQueryItem("client_id", CLIENT_ID);
    query.addQueryItem("scope", SCOPES);
    query.addQueryItem("redirect_uri", REDIRECT_URI);
    query.addQueryItem("state", m_state);
    query.addQueryItem("user_scope", SCOPES); // For user token

    url.setQuery(query);

    qDebug() << "Generated OAuth URL:" << url.toString();

    return url.toString();
}

void OAuthManager::handleWebViewCallback(const QString &code, const QString &state)
{
    qDebug() << "WebView callback received with code and state";

    // Verify state (CSRF protection)
    if (state != m_state) {
        qWarning() << "State mismatch! Possible CSRF attack.";
        emit authenticationFailed("State mismatch");
        m_isAuthenticating = false;
        emit authenticatingChanged();
        return;
    }

    if (code.isEmpty()) {
        emit authenticationFailed("No authorization code");
        m_isAuthenticating = false;
        emit authenticatingChanged();
        return;
    }

    // Exchange code for token
    exchangeCodeForToken(code);
}
