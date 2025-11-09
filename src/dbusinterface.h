#ifndef DBUSINTERFACE_H
#define DBUSINTERFACE_H

#include <QObject>
#include "dbusadaptor.h"

const QString DBUS_INTERFACE_NAME = "org.lagoon.slackship";
const QString DBUS_PATH_NAME = "/org/lagoon/slackship";

class DBusInterface : public QObject
{
    Q_OBJECT

public:
    explicit DBusInterface(QObject *parent = nullptr);
    DBusAdaptor *getDBusAdaptor();

private:
    DBusAdaptor *m_dbusAdaptor;
};

#endif // DBUSINTERFACE_H
