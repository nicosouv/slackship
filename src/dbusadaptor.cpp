#include "dbusadaptor.h"
#include <QDebug>

DBusAdaptor::DBusAdaptor(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
    qDebug() << "DBusAdaptor created";
}

void DBusAdaptor::openChannel(const QString &channelId)
{
    qDebug() << "=== DBUS openChannel CALLED ===" << channelId;
    emit pleaseOpenChannel(channelId);
}
