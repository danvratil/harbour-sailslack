#include "notificationlistener.h"
#include "sailslack_adaptor.h"

#include <QDebug>
#include <QtQuick/QQuickItem>

NotificationListener::NotificationListener(QQuickView *view, QObject *parent)
    : QObject(parent)
    , view(view) {

    new SailslackAdaptor(this);
    QDBusConnection connection = QDBusConnection::sessionBus();
    if (!connection.registerObject("/", this)) {
        qWarning() << "Failed to register on DBus:" << connection.lastError().message();
        return;
    }
    if (!connection.registerService("harbour.sailslack")) {
        qWarning() << "Failed to register DBus service:" << connection.lastError().message();
        return;
    }
    qDebug() << "NotificationListener set up";
}

void NotificationListener::activate(const QString &teamId, const QString &channelId) {
    qDebug() << "Activate notification received" << teamId << channelId;

    QMetaObject::invokeMethod(view->rootObject(), "activateChannel",
                              Q_ARG(QVariant, QVariant(teamId)),
                              Q_ARG(QVariant, QVariant(channelId)));
}
