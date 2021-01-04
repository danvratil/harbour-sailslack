import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.sailslack 1.0 as Slack
import Sailfish.WebView 1.0

// Using WebViewPage allows the WebView to better react to virtual keyboard.
WebViewPage {
    id: page

    property string processId: Math.random().toString(36).substring(7)
    property string startUrl: "https://slack.com/oauth/authorize?scope=client&client_id=" + slackClientId + "&redirect_uri=https%3A%2F%2Flocalhost%3A3000%2Foauth%2Fcallback"

    signal loginSuccess(string userId, string teamId, string teamName, string accessToken)

    PageHeader {
        title: "Sign in with Slack"
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

    onStatusChanged: {
        switch (status) {
        case PageStatus.Active:
            server.listen(3000);
            break;
        case PageStatus.Deactivating:
            server.close();
            break;
        }
    }

    Column {
        id: column
        width: page.width
        anchors.verticalCenter: parent.verticalCenter
        spacing: Theme.paddingMedium

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
                Qt.openUrlExternally(page.startUrl + "&state=" + page.processId);
            }
        }

        Label {
            text: qsTr("Or use the sailfish WebView\n(Warning: works only from terminal!)")
        }

        Button {
            text: qsTr("Use webview")
            onClicked: {
                server.close();
                column.visible = false
                browserLoadable.setSource ("LoginWebView.qml", { url: page.startUrl + "&state=" + page.processId })
            }
        }
    }

    Loader {
        id: browserLoadable
        anchors.fill: parent
    }

}
