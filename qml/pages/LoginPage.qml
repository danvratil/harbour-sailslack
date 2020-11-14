import QtQuick 2.0
import Sailfish.Silica 1.0
import QtWebKit 3.0
import Sailfish.WebView 1.0
import harbour.sailslack 1.0 as Slack

WebViewPage {
    id: page

    property string processId: Math.random().toString(36).substring(7)
    property string startUrl: "https://slack.com/oauth/authorize?scope=client&client_id=" + slackClientId + "&redirect_uri=http%3A%2F%2Flocalhost%3A3000%2Foauth%2Fcallback"

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
            webViewF.visible = true
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
        case PageStatus.Activating:
            server.listen(3000);
            break;
        case PageStatus.Deactivating:
            server.close();
            break;
        }
    }

    WebViewFlickable {
        id: webViewF
        anchors.fill: parent

        webView {
            active: true
            url: page.startUrl + "&state=" + page.processId

            onUrlChanged: {
                // This is currently not called for redirects (at least not in sailfish-components-webview 1.1.6.1).
                // so I just leave this here in case it starts working, otherwise AuthServer is actually listening on 3000.
                // See also https://git.sailfishos.org/mer-core/qtmozembed/blob/master/src/qmozview_defined_wrapper.h
                // which is wrapped in RawWebView https://github.com/sailfishos/sailfish-components-webview/blob/jb51257/1.1.6.1/import/webview/rawwebview.h
                var request = {url: String(webView.url)};
                if (isReturnUrl(request.url)) {
                    visible = false
                    request.action = WebView.IgnoreRequest

                    if (isSuccessUrl(request.url)) {
                        Slack.Client.fetchAccessToken(request.url)
                    }
                    else {
                        pageStack.pop(undefined, PageStackAction.Animated)
                    }
                }
                else {
                    request.action = WebView.AcceptRequest
                }
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
