#include <QtQuick>
#include <sailfishapp.h>

#include "slackconfig.h"
#include "slackclient.h"
#include "slackclientconfig.h"
#include "slackauthenticator.h"
#include "networkaccessmanagerfactory.h"
#include "notificationlistener.h"
#include "dbusadaptor.h"
#include "storage.h"
#include "filemodel.h"

int main(int argc, char *argv[])
{
    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));
    QScopedPointer<QQuickView> view(SailfishApp::createView());

    QSettings settings;
    QString lastVersion = settings.value("app/lastVersion").toString();
    if (lastVersion.isEmpty()) {
        qDebug() << "No last version set, removing previous access token";
        settings.remove("user/accessToken");
    }

    qDebug() << "Setting last version" << APP_VERSION;
    settings.setValue("app/lastVersion", QVariant(APP_VERSION));




    SlackClientConfig::clearWebViewCache();

    qmlRegisterSingletonType<SlackConfig>("harbour.slackfish", 1, 0, "Config", [](QQmlEngine *, QJSEngine *) -> QObject* {
        return new SlackConfig();
    });
    qmlRegisterSingletonType<SlackClientFactory>("harbour.slackfish", 1, 0, "ClientFactory",
                                                 [](QQmlEngine *, QJSEngine *) -> QObject* {
        return new SlackClientFactory();
    });

    qmlRegisterType<SlackAuthenticator>("harbour.slackfish", 1, 0, "Authenticator");
    qmlRegisterType<SlackClient>("harbour.slackfish", 1, 0, "Client");
    qmlRegisterUncreatableType<SlackClientConfig>("harbour.slackfish", 1, 0, "ClientConfig", "Use Client.config to access current config");


    view->rootContext()->setContextProperty("applicationVersion", APP_VERSION);
    view->rootContext()->setContextProperty("slackClientId", SLACK_CLIENT_ID);
    view->rootContext()->setContextProperty("fileModel", new FileModel());

    view->setSource(SailfishApp::pathTo("qml/harbour-slackfish.qml"));
    view->engine()->setNetworkAccessManagerFactory(new NetworkAccessManagerFactory());
    view->showFullScreen();

    NotificationListener* listener = new NotificationListener(view.data());
    new DBusAdaptor(listener);
    QDBusConnection connection = QDBusConnection::sessionBus();
    connection.registerService("harbour.slackfish");
    connection.registerObject("/", listener);

    int result = app->exec();

    qDebug() << "Application terminating";

    delete listener;

    return result;
}
