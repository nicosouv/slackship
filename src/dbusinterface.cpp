#include "dbusinterface.h"
#include <QDBusConnection>
#include <QDBusError>
#include <QDebug>

DBusInterface::DBusInterface(QObject *parent)
    : QObject(parent)
    , m_dbusAdaptor(nullptr)
{
    qDebug() << "=== DBUS INTERFACE INITIALIZATION ===";

    // Create the DBus adaptor
    m_dbusAdaptor = new DBusAdaptor(this);

    // Get the session bus
    QDBusConnection sessionBus = QDBusConnection::sessionBus();

    if (!sessionBus.isConnected()) {
        qWarning() << "ERROR: Cannot connect to D-Bus session bus";
        return;
    }

    qDebug() << "Connected to D-Bus session bus";

    // Register the object at the specified path
    if (!sessionBus.registerObject(DBUS_PATH_NAME, this)) {
        qWarning() << "ERROR: Cannot register object to D-Bus:"
                   << sessionBus.lastError().message();
        return;
    }

    qDebug() << "Registered D-Bus object at path:" << DBUS_PATH_NAME;

    // Register the service (may fail on sandboxed apps - this is expected)
    if (!sessionBus.registerService(DBUS_INTERFACE_NAME)) {
        qDebug() << "D-Bus service registration skipped (sandboxed app)";
    } else {
        qDebug() << "Registered D-Bus service:" << DBUS_INTERFACE_NAME;
    }

    qDebug() << "=== DBUS INTERFACE READY ===";
}

DBusAdaptor *DBusInterface::getDBusAdaptor()
{
    return m_dbusAdaptor;
}
