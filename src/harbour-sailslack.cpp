#include <QtQuick>
#include <sailfishapp.h>

#include "authserver.h"
#include "slackconfig.h"
#include "slackclient.h"
#include "slackclientconfig.h"
#include "slackauthenticator.h"
#include "networkaccessmanagerfactory.h"
#include "notificationlistener.h"
#include "sailslack_adaptor.h"
#include "storage.h"
#include "filemodel.h"
#include "teamsmodel.h"
#include "models/messagemodel.h"
#include "models/channellistmodel.h"
#include "models/channelmessagesmodel.h"
#include "models/channelsortmodel.h"
#include "models/threadmessagesmodel.h"

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

    qmlRegisterSingletonType<SlackConfig>("harbour.sailslack", 1, 0, "Config", [](QQmlEngine *, QJSEngine *) -> QObject* {
        return new SlackConfig();
    });

    qmlRegisterType<AuthServer>("harbour.sailslack", 1, 0, "AuthServer");
    qmlRegisterType<SlackAuthenticator>("harbour.sailslack", 1, 0, "Authenticator");
    qmlRegisterType<SlackClient>("harbour.sailslack", 1, 0, "Client");
    qmlRegisterType<TeamsModel>("harbour.sailslack", 1, 0, "TeamsModel");
    qmlRegisterType<ChannelListModel>("harbour.sailslack", 1, 0, "ChannelListModel");
    qmlRegisterType<ChannelMessagesModel>("harbour.sailslack", 1, 0, "ChannelMessagesModel");
    qmlRegisterType<ChannelSortModel>("harbour.sailslack", 1, 0, "ChannelSortModel");
    qmlRegisterType<ThreadMessagesModel>("harbour.sailslack", 1, 0, "ThreadMessagesModel");
    qmlRegisterUncreatableType<SlackClientConfig>("harbour.sailslack", 1, 0, "ClientConfig", "Use Client.config to access current config");
    qmlRegisterUncreatableType<MessageModel>("harbour.sailslack", 1, 0, "MessageModel", "Obtain current MessageModel from SlackClient");

    view->rootContext()->setContextProperty("applicationVersion", APP_VERSION);
    view->rootContext()->setContextProperty("slackClientId", SLACK_CLIENT_ID);
    view->rootContext()->setContextProperty("fileModel", new FileModel());

    view->setSource(SailfishApp::pathTo("qml/harbour-sailslack.qml"));
    view->engine()->setNetworkAccessManagerFactory(new NetworkAccessManagerFactory());
    view->showFullScreen();

    QScopedPointer<NotificationListener> listener;
    QTimer::singleShot(0, [&listener, &view]() mutable {
        listener.reset(new NotificationListener(view.data()));
    });
    int result = app->exec();

    qDebug() << "Application terminating";

    return result;
}
