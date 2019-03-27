#ifndef NOTIFICATIONLISTENER_H
#define NOTIFICATIONLISTENER_H

#include <QObject>
#include <QtQuick/QQuickView>

class NotificationListener : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "harbour.sailslack")
public:
     explicit NotificationListener(QQuickView *view, QObject *parent = 0);

public slots:
     void activate(const QString &teamId, const QString &channelId);

private:
     QQuickView *view;
};

#endif // NOTIFICATIONLISTENER_H
