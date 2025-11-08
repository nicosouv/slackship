TARGET = harbour-lagoon-daemon

CONFIG += console
CONFIG -= app_bundle
QT += core network websockets sql dbus
QT -= gui

CONFIG += link_pkgconfig
PKGCONFIG += nemonotifications-qt5

SOURCES += \
    src/daemon/main.cpp \
    src/daemon/slackshipdaemon.cpp \
    src/daemon/dbusadaptor.cpp \
    src/slackapi.cpp \
    src/websocketclient.cpp \
    src/notificationmanager.cpp \
    src/workspacemanager.cpp \
    src/cache/cachemanager.cpp

HEADERS += \
    src/daemon/slackshipdaemon.h \
    src/daemon/dbusadaptor.h \
    src/slackapi.h \
    src/websocketclient.h \
    src/notificationmanager.h \
    src/workspacemanager.h \
    src/cache/cachemanager.h

# Installation
target.path = /usr/bin

# Systemd service file
systemd.path = /usr/lib/systemd/user
systemd.files = daemon/harbour-lagoon-daemon.service

# D-Bus service file
dbus.path = /usr/share/dbus-1/services
dbus.files = daemon/org.harbour.lagoon.service

INSTALLS += target systemd dbus
