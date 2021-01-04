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
        case PageStatus.Active:
            server.listen(3000);
            Qt.openUrlExternally(page.startUrl + "&state=" + page.processId);
            break;
        case PageStatus.Deactivating:
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
            width: page.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: "Sign in with Slack"
            }

            Label {
                id: listening;
                font.italic: true
                wrapMode: Text.Wrap
            }

            Label {
                text: qsTr("You can login with your browser:")
            }

            Button {
                text: qsTr("Use browser")
                onClicked: {
                    Qt.openUrlExternally(page.startUrl + "&state=" + page.processId);
                }
            }

            Label {
                text: qsTr("Or use the Sailfish WebView:")
            }

            Button {
                text: qsTr("Use webview")
                onClicked: {
                    // Closing the 3000 port as even though the webview would successfully connect to it, the timing it navigates away from this page makes
                    // the webview crash (at onResultUrlAvailable signal).
                    server.close();
                    column.visible = false
                    browserLoadable.setSource ("LoginWebView.qml", { url: page.startUrl + "&state=" + page.processId })
                }
            }

            Label {
                text: qsTr("Or copy the link to use it on your desktop:")
            }

            Button {
                text: qsTr("Copy link")
                onClicked: {
                    Clipboard.text = page.startUrl + "&state=" + page.processId;
                }
            }

            Row {
                TextField {
                    id: returnUrlField
                    width: page.width - useReturnUrlButton.width
                    placeholderText: qsTr("Paste redirect url here")
                }

                Button {
                    id: useReturnUrlButton
                    width: Theme.buttonWidthExtraSmall
                    text: qsTr("Go")
                    enabled: returnUrlField.text.length > 0
                    onClicked: {
                        authenticator.fetchAccessToken(returnUrlField.text);
                    }
                }
            }
        }
    }

    Loader {
        id: browserLoadable
        anchors.fill: parent
    }

}
