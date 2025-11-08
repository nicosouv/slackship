#ifdef QT_QML_DEBUG
#include <QtQuick>
#endif

#include <sailfishapp.h>
#include <QScopedPointer>
#include <QQuickView>
#include <QQmlContext>
#include <QGuiApplication>

#include "slackapi.h"
#include "notificationmanager.h"
#include "workspacemanager.h"
#include "filemanager.h"
#include "oauthmanager.h"
#include "statsmanager.h"
#include "models/conversationmodel.h"
#include "models/messagemodel.h"
#include "models/usermodel.h"
#include "settings/appsettings.h"

int main(int argc, char *argv[])
{
    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));

    app->setOrganizationName("harbour-lagoon");
    app->setApplicationName("harbour-lagoon");

    QScopedPointer<QQuickView> view(SailfishApp::createView());

    // Create managers
    WorkspaceManager *workspaceManager = new WorkspaceManager(app.data());
    NotificationManager *notificationManager = new NotificationManager(app.data());
    FileManager *fileManager = new FileManager(app.data());
    OAuthManager *oauthManager = new OAuthManager(app.data());
    StatsManager *statsManager = new StatsManager(app.data());

    // Create API instance
    SlackAPI *slackAPI = new SlackAPI(app.data());

    // Create models
    ConversationModel *conversationModel = new ConversationModel(app.data());
    MessageModel *messageModel = new MessageModel(app.data());
    UserModel *userModel = new UserModel(app.data());

    // Create settings
    AppSettings *settings = new AppSettings(app.data());

    // Connect workspace manager to API
    QObject::connect(workspaceManager, &WorkspaceManager::workspaceSwitched,
                     slackAPI, [slackAPI](int /*index*/, const QString &token) {
        slackAPI->authenticate(token);
    });

    // Connect API to models
    QObject::connect(slackAPI, &SlackAPI::conversationsReceived,
                     conversationModel, &ConversationModel::updateConversations);
    QObject::connect(slackAPI, &SlackAPI::messagesReceived,
                     messageModel, &MessageModel::updateMessages);
    QObject::connect(slackAPI, &SlackAPI::usersReceived,
                     userModel, &UserModel::updateUsers);

    // Connect API to notification manager and stats manager
    QObject::connect(slackAPI, &SlackAPI::messageReceived,
                     [notificationManager, statsManager, conversationModel, userModel](const QJsonObject &message) {
        QString channelId = message["channel"].toString();
        QString userId = message["user"].toString();
        QString text = message["text"].toString();

        // Track message in stats
        statsManager->trackMessage(message);

        // Find channel name
        QString channelName = channelId;
        for (int i = 0; i < conversationModel->rowCount(); ++i) {
            QModelIndex idx = conversationModel->index(i, 0);
            if (conversationModel->data(idx, ConversationModel::IdRole).toString() == channelId) {
                channelName = conversationModel->data(idx, ConversationModel::NameRole).toString();
                break;
            }
        }

        QString userName = userModel->getUserName(userId);

        // Check if it's a mention
        bool isMention = text.contains("@");

        if (isMention) {
            notificationManager->showMentionNotification(channelName, userName, text, channelId);
        } else {
            notificationManager->showMessageNotification(channelName, userName, text, channelId);
        }
    });

    // Track messages when history is received (only recent messages to avoid performance issues)
    QObject::connect(slackAPI, &SlackAPI::messagesReceived,
                     [statsManager](const QJsonArray &messages) {
        QDateTime thirtyDaysAgo = QDateTime::currentDateTime().addDays(-30);
        int trackedCount = 0;

        for (const QJsonValue &value : messages) {
            if (value.isObject()) {
                QJsonObject message = value.toObject();

                // Only track messages from the last 30 days to avoid performance issues
                double tsDouble = message["ts"].toString().toDouble();
                QDateTime messageTime = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(tsDouble * 1000));

                if (messageTime >= thirtyDaysAgo) {
                    statsManager->trackMessage(message);
                    trackedCount++;
                }
            }
        }

        qDebug() << "Tracked" << trackedCount << "recent messages out of" << messages.count() << "total";
    });

    // Connect notification manager to file manager
    QObject::connect(notificationManager, &NotificationManager::enabledChanged,
                     settings, [settings, notificationManager]() {
        settings->setNotificationsEnabled(notificationManager->enabled());
    });

    // Sync notification settings
    notificationManager->setEnabled(settings->notificationsEnabled());

    // Expose to QML
    QQmlContext *context = view->rootContext();
    context->setContextProperty("slackAPI", slackAPI);
    context->setContextProperty("workspaceManager", workspaceManager);
    context->setContextProperty("notificationManager", notificationManager);
    context->setContextProperty("fileManager", fileManager);
    context->setContextProperty("oauthManager", oauthManager);
    context->setContextProperty("statsManager", statsManager);
    context->setContextProperty("conversationModel", conversationModel);
    context->setContextProperty("messageModel", messageModel);
    context->setContextProperty("userModel", userModel);
    context->setContextProperty("appSettings", settings);

    view->setSource(SailfishApp::pathTo("qml/harbour-lagoon.qml"));
    view->show();

    return app->exec();
}
