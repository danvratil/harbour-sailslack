import QtQuick 2.0
import Sailfish.Silica 1.0
import QtWebKit 3.0
import harbour.sailslack 1.0 as Slack

Page {
    id: pageLink

    property string processId: Math.random().toString(36).substring(7)
    property string startUrl: "https://slack.com/oauth/authorize?scope=client&client_id=" + slackClientId + "&redirect_uri=http%3A%2F%2Flocalhost%3A3000%2Foauth%2Fcallback"

    signal loginSuccess(string userId, string teamId, string teamName, string accessToken)

    function handleLoginSuccess(userId, teamId, teamName, accessToken) {
        loginSuccess(userId, teamId, teamName, accessToken)
    }

    Slack.Authenticator {
        id: authenticator

        onAccessTokenSuccess: function(userId, teamId, teamName, accessToken) {
            loginSuccess(userId, teamId, teamName, accessToken)
        }

        onAccessTokenFail: {
            console.log('access token failed')
        }
    }

    Slack.AuthServer {
        id: server

        onResultUrlAvailable: {
            authenticator.fetchAccessToken(url);
        }
    }

    Connections {
        target: server
        onListening: {
            listening.text = (server.isListening() ? qsTr("Listening on port %1") : qsTr("Not listening on port %1")).arg(3000)
        }
    }

    onStatusChanged: {
        switch (status) {
        case PageStatus.Activating:
            server.listen(3000);
            break;
        case PageStatus.Deactivating:
            console.log("Deactivating")
            server.close();
            break;
        }
    }

    SilicaFlickable {
        anchors.fill: parent
        contentWidth: column.width
        contentHeight: column.height

        Column {
            id: column
            width: pageLink.width
            anchors.verticalCenter: parent.verticalCenter
            spacing: Theme.paddingMedium

            PageHeader {
                title: qsTr("Sign in with Slack")
            }

            Label {
                id: listening;
                wrapMode: Text.Wrap
            }

            Label {
                text: qsTr("Click below to login with your browser")
            }

            Button {
                text: qsTr("Use browser")
                onClicked: {
                    Qt.openUrlExternally(pageLink.startUrl + "&state=" + pageLink.processId);
                }
            }

            Label {
                text: qsTr("Or use the sailfish WebView\n(Warning: works only from terminal!)")
            }

            Button {
                text: qsTr("Use webview")
                onClicked: {
                    var nextPage = pageStack.replace(Qt.resolvedUrl("LoginPage.qml"));
                    nextPage.onLoginSuccess.connect(handleLoginSuccess)
                }
            }
        }
    }

}
