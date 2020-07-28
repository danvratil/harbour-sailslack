import QtQuick 2.0
import Sailfish.Silica 1.0
import QtWebKit 3.0
import harbour.sailslack 1.0 as Slack

Page {
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
            webView.visible = true
        }
    }

    SilicaWebView {
        id: webView
        anchors.fill: parent
        url: page.startUrl + "&state=" + page.processId

        header: PageHeader {
            title: "Sign in with Slack"
        }

        onNavigationRequested: {
            if (isReturnUrl(request.url)) {
                visible = false
                request.action = WebView.IgnoreRequest

                if (isSuccessUrl(request.url)) {
                    authenticator.fetchAccessToken(request.url)
                }
                else {
                    pageStack.pop(undefined,PageStackAction.Animated)
                }
            }
            else {
                request.action = WebView.AcceptRequest
            }
        }
    }

    function isReturnUrl(url) {
        return url.toString().indexOf('http://localhost:3000/oauth/callback') !== -1
    }

    function isSuccessUrl(url) {
        return url.toString().indexOf('error=') === -1 && url.toString().indexOf('state=' + page.processId) !== -1
    }
}
