TARGET = harbour-lagoon

CONFIG += sailfishapp
QT += network websockets sql dbus
CONFIG += link_pkgconfig
PKGCONFIG += sailfishapp nemonotifications-qt5

# OAuth credentials - loaded from .qmake.conf
CLIENT_ID = $$lagoon_client_id
CLIENT_SECRET = $$lagoon_client_secret

isEmpty(CLIENT_ID) {
    error("No LAGOON_CLIENT_ID defined - set environment variable before building")
}
isEmpty(CLIENT_SECRET) {
    error("No LAGOON_CLIENT_SECRET defined - set environment variable before building")
}

DEFINES += LAGOON_CLIENT_ID=\\\"$$CLIENT_ID\\\"
DEFINES += LAGOON_CLIENT_SECRET=\\\"$$CLIENT_SECRET\\\"

SOURCES += \
    src/main.cpp \
    src/slackapi.cpp \
    src/slackimageprovider.cpp \
    src/websocketclient.cpp \
    src/notificationmanager.cpp \
    src/workspacemanager.cpp \
    src/filemanager.cpp \
    src/dbusclient.cpp \
    src/dbusadaptor.cpp \
    src/dbusinterface.cpp \
    src/oauthmanager.cpp \
    src/statsmanager.cpp \
    src/updatechecker.cpp \
    src/models/conversationmodel.cpp \
    src/models/messagemodel.cpp \
    src/models/usermodel.cpp \
    src/cache/cachemanager.cpp \
    src/settings/appsettings.cpp

HEADERS += \
    src/slackapi.h \
    src/slackimageprovider.h \
    src/websocketclient.h \
    src/notificationmanager.h \
    src/workspacemanager.h \
    src/filemanager.h \
    src/dbusclient.h \
    src/dbusadaptor.h \
    src/dbusinterface.h \
    src/oauthmanager.h \
    src/statsmanager.h \
    src/updatechecker.h \
    src/models/conversationmodel.h \
    src/models/messagemodel.h \
    src/models/usermodel.h \
    src/cache/cachemanager.h \
    src/settings/appsettings.h

DISTFILES += \
    qml/harbour-lagoon.qml \
    qml/cover/CoverPage.qml \
    qml/pages/FirstPage.qml \
    qml/pages/ConversationPage.qml \
    qml/pages/SettingsPage.qml \
    qml/pages/LoginPage.qml \
    qml/pages/StatsPage.qml \
    qml/components/MessageDelegate.qml \
    qml/components/ChannelDelegate.qml \
    qml/components/StatCard.qml \
    qml/components/ReactionBubble.qml \
    qml/components/ImageAttachment.qml \
    qml/components/FileAttachment.qml \
    qml/dialogs/EmojiPicker.qml \
    qml/pages/ImageViewerPage.qml \
    qml/pages/WorkspaceSwitcher.qml \
    qml/js/storage.js \
    rpm/harbour-lagoon.spec \
    rpm/harbour-lagoon.yaml \
    rpm/harbour-lagoon.changes \
    translations/*.ts \
    harbour-lagoon.desktop

SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172

# Translations
CONFIG += sailfishapp_i18n
TRANSLATIONS += \
    translations/harbour-lagoon-en.ts \
    translations/harbour-lagoon-fr.ts \
    translations/harbour-lagoon-fi.ts \
    translations/harbour-lagoon-it.ts \
    translations/harbour-lagoon-es.ts
