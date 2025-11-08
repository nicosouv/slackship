#include "slackshipdaemon.h"
#include "dbusadaptor.h"
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    app.setOrganizationName("harbour-lagoon");
    app.setApplicationName("harbour-lagoon-daemon");
    app.setApplicationVersion("0.6.0");

    qDebug() << "Starting Lagoon Daemon...";

    // Create daemon instance
    SlackShipDaemon daemon;

    // Initialize daemon
    if (!daemon.initialize()) {
        qCritical() << "Failed to initialize daemon";
        return 1;
    }

    // Register D-Bus service
    QDBusConnection connection = QDBusConnection::sessionBus();

    if (!connection.registerService("org.harbour.lagoon")) {
        qCritical() << "Failed to register D-Bus service:" << connection.lastError().message();
        return 2;
    }

    // Create and register D-Bus adaptor
    new DBusAdaptor(&daemon);

    if (!connection.registerObject("/org/harbour/lagoon", &daemon)) {
        qCritical() << "Failed to register D-Bus object:" << connection.lastError().message();
        return 3;
    }

    qDebug() << "D-Bus service registered: org.harbour.lagoon";
    qDebug() << "D-Bus object path: /org/harbour/lagoon";

    // Start daemon
    daemon.start();

    qDebug() << "Lagoon Daemon started successfully";

    return app.exec();
}
