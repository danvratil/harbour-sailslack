import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.WebView 1.0

WebViewFlickable {
    id: webViewF
    anchors.fill: parent

    property string url

    webView {
        active: true
        url: webViewF.url
        onUrlChanged: {
            // OLD INCORRECT COMMENT LEFT FOR THE LINKS:
            // This is currently not called for redirects (at least not in sailfish-components-webview 1.1.6.1).
            // so I just leave this here in case it starts working, otherwise AuthServer is actually listening on 3000.
            // See also https://git.sailfishos.org/mer-core/qtmozembed/blob/master/src/qmozview_defined_wrapper.h
            // which is wrapped in RawWebView https://github.com/sailfishos/sailfish-components-webview/blob/jb51257/1.1.6.1/import/webview/rawwebview.h
            if (isReturnUrl(url)) {
                visible = false

                if (isSuccessUrl(url)) {
                    authenticator.fetchAccessToken(url)
                }
                else {
                    pageStack.pop(undefined, PageStackAction.Animated)
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
