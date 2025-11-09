#ifndef DBUSADAPTOR_H
#define DBUSADAPTOR_H

#include <QDBusAbstractAdaptor>

class DBusAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.lagoon.slackship")

public:
    explicit DBusAdaptor(QObject *parent = nullptr);

signals:
    void pleaseOpenChannel(const QString &channelId);

public slots:
    void openChannel(const QString &channelId);
};

#endif // DBUSADAPTOR_H
