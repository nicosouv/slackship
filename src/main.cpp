#ifdef QT_QML_DEBUG
#include <QtQuick>
#endif

#include <sailfishapp.h>
#include <QScopedPointer>
#include <QQuickView>
#include <QQmlContext>
#include <QGuiApplication>
#include <QTranslator>
#include <QLocale>

#include "slackapi.h"
#include "slackimageprovider.h"
#include "notificationmanager.h"
#include "workspacemanager.h"
#include "filemanager.h"
#include "oauthmanager.h"
#include "statsmanager.h"
#include "updatechecker.h"
#include "dbusinterface.h"
#include "draftmanager.h"
#include "models/conversationmodel.h"
#include "models/messagemodel.h"
#include "models/usermodel.h"
#include "settings/appsettings.h"

int main(int argc, char *argv[])
{
    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));

    app->setOrganizationName("harbour-lagoon");
    app->setApplicationName("harbour-lagoon");

    // Load translation
    QScopedPointer<QTranslator> translator(new QTranslator(app.data()));
    AppSettings tempSettings; // Temporary settings object to read language preference
    QString language = tempSettings.language();

    // If no language is set, use system locale
    if (language.isEmpty()) {
        language = QLocale::system().name();
    }

    // Try to load the translation file
    QString translationFile = QString("harbour-lagoon-%1").arg(language);
    if (translator->load(translationFile, SailfishApp::pathTo("translations").toLocalFile())) {
        app->installTranslator(translator.data());
    } else {
        // Try loading just the language code (e.g., "fr" instead of "fr_FR")
        QString shortLang = language.left(2);
        translationFile = QString("harbour-lagoon-%1").arg(shortLang);
        if (translator->load(translationFile, SailfishApp::pathTo("translations").toLocalFile())) {
            app->installTranslator(translator.data());
        }
    }

    QScopedPointer<QQuickView> view(SailfishApp::createView());

    // Create and register image provider for authenticated Slack images
    SlackImageProvider *imageProvider = new SlackImageProvider();
    view->engine()->addImageProvider("slack", imageProvider);

    // Create managers
    WorkspaceManager *workspaceManager = new WorkspaceManager(app.data());
    NotificationManager *notificationManager = new NotificationManager(app.data());
    FileManager *fileManager = new FileManager(app.data());
    OAuthManager *oauthManager = new OAuthManager(app.data());
    StatsManager *statsManager = new StatsManager(app.data());
    UpdateChecker *updateChecker = new UpdateChecker(app.data());
    DraftManager *draftManager = new DraftManager(app.data());

    // Create DBus interface for notification clicks
    DBusInterface *dbusInterface = new DBusInterface(app.data());
    DBusAdaptor *dbusAdaptor = dbusInterface->getDBusAdaptor();

    // Create API instance
    SlackAPI *slackAPI = new SlackAPI(app.data());

    // Connect SlackAPI token to image provider
    QObject::connect(slackAPI, &SlackAPI::tokenChanged,
                     [imageProvider, slackAPI]() {
        imageProvider->setToken(slackAPI->token());
    });
    // Set initial token if already authenticated
    if (!slackAPI->token().isEmpty()) {
        imageProvider->setToken(slackAPI->token());
    }

    // Create models
    ConversationModel *conversationModel = new ConversationModel(app.data());
    MessageModel *messageModel = new MessageModel(app.data());
    UserModel *userModel = new UserModel(app.data());

    // Create settings
    AppSettings *settings = new AppSettings(app.data());

    // Connect workspace manager to API and stats
    QObject::connect(workspaceManager, &WorkspaceManager::workspaceSwitched,
                     slackAPI, [slackAPI, statsManager](int /*index*/, const QString &token) {
        slackAPI->authenticate(token);
        // Stats will be updated when teamIdChanged signal is emitted after authentication
    });

    // Connect stats manager to API for workspace and user tracking
    QObject::connect(slackAPI, &SlackAPI::teamIdChanged,
                     statsManager, [statsManager, slackAPI]() {
        statsManager->setCurrentWorkspace(slackAPI->teamId());
    });

    QObject::connect(slackAPI, &SlackAPI::currentUserChanged,
                     statsManager, [statsManager, slackAPI]() {
        statsManager->setCurrentUserId(slackAPI->currentUserId());
    });

    // Connect conversation model to API for starred channels persistence
    QObject::connect(slackAPI, &SlackAPI::teamIdChanged,
                     conversationModel, [conversationModel, slackAPI]() {
        conversationModel->setTeamId(slackAPI->teamId());
    });

    // Connect API to models
    QObject::connect(slackAPI, &SlackAPI::conversationsReceived,
                     conversationModel, &ConversationModel::updateConversations);
    QObject::connect(slackAPI, &SlackAPI::messagesReceived,
                     messageModel, &MessageModel::updateMessages);

    // Also track historical messages for stats
    QObject::connect(slackAPI, &SlackAPI::messagesReceived,
                     statsManager, [statsManager](const QJsonArray &messages) {
        for (const QJsonValue &value : messages) {
            if (value.isObject()) {
                statsManager->trackMessage(value.toObject());
            }
        }
    });

    QObject::connect(slackAPI, &SlackAPI::usersReceived,
                     userModel, &UserModel::updateUsers);

    // Connect API to notification manager and stats manager
    QObject::connect(slackAPI, &SlackAPI::messageReceived,
                     notificationManager, [notificationManager, statsManager, conversationModel, userModel](const QJsonObject &message) {
        QString channelId = message["channel"].toString();
        QString userId = message["user"].toString();
        QString text = message["text"].toString();

        // Track message in stats (simplified)
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

    // Connect newUnreadMessages signal to notifications (from polling)
    QObject::connect(slackAPI, &SlackAPI::newUnreadMessages,
                     notificationManager, [notificationManager, conversationModel, userModel](const QString &channelId, int newCount) {
        // Update unread count in conversation model (for CoverPage and bold channels)
        conversationModel->updateUnreadCount(channelId, newCount);

        // Find channel name and type
        QString channelName = channelId;
        QString channelType = "channel";
        for (int i = 0; i < conversationModel->rowCount(); ++i) {
            QModelIndex idx = conversationModel->index(i, 0);
            if (conversationModel->data(idx, ConversationModel::IdRole).toString() == channelId) {
                channelName = conversationModel->data(idx, ConversationModel::NameRole).toString();
                channelType = conversationModel->data(idx, ConversationModel::TypeRole).toString();
                break;
            }
        }

        // Show notification
        QString summary;
        if (channelType == "im") {
            summary = QString("New message from %1").arg(channelName);
        } else {
            summary = QString("%1 in #%2").arg(newCount > 1 ? QString::number(newCount) + " new messages" : "New message", channelName);
        }

        QString body = newCount > 1 ? QString("You have %1 unread messages").arg(newCount) : "Tap to view message";

        notificationManager->showMessageNotification(channelName, "", body, channelId);
    });

    // Note: We don't track message history anymore to keep stats simple and daily-only

    // Connect notification manager to file manager
    QObject::connect(notificationManager, &NotificationManager::enabledChanged,
                     settings, [settings, notificationManager]() {
        settings->setNotificationsEnabled(notificationManager->enabled());
    });

    // Sync notification settings
    notificationManager->setEnabled(settings->notificationsEnabled());
    notificationManager->setAppSettings(settings);

    // Connect bandwidth tracking
    QObject::connect(slackAPI, &SlackAPI::bandwidthBytesAdded,
                     settings, &AppSettings::addBandwidthBytes);

    // Connect polling interval settings
    slackAPI->setRefreshInterval(settings->pollingInterval());  // Initialize from settings
    QObject::connect(settings, &AppSettings::pollingIntervalChanged,
                     slackAPI, [slackAPI, settings]() {
        slackAPI->setRefreshInterval(settings->pollingInterval());
    });

    // Expose to QML
    QQmlContext *context = view->rootContext();
    context->setContextProperty("slackAPI", slackAPI);
    context->setContextProperty("workspaceManager", workspaceManager);
    context->setContextProperty("notificationManager", notificationManager);
    context->setContextProperty("fileManager", fileManager);
    context->setContextProperty("oauthManager", oauthManager);
    context->setContextProperty("statsManager", statsManager);
    context->setContextProperty("updateChecker", updateChecker);
    context->setContextProperty("draftManager", draftManager);
    context->setContextProperty("dbusAdaptor", dbusAdaptor);
    context->setContextProperty("conversationModel", conversationModel);
    context->setContextProperty("messageModel", messageModel);
    context->setContextProperty("userModel", userModel);
    context->setContextProperty("appSettings", settings);

    // Check for updates once on startup
    updateChecker->checkForUpdates();

    view->setSource(SailfishApp::pathTo("qml/harbour-lagoon.qml"));
    view->show();

    return app->exec();
}
