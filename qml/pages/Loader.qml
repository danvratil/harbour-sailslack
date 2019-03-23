import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.slackfish 1.0

Page {
    id: page

    property Client slackClient

    property bool loading: false
    property string errorMessage: ""
    property string loadMessage: ""

    SilicaFlickable {
        anchors.fill: parent

        PageHeader { title: "Slackfish" }

        PullDownMenu {
            enabled: !page.loading

            MenuItem {
                text: qsTr("Login")
                onClicked: startLogin()
            }
        }

        Label {
            visible: loader.visible
            anchors.bottom: loader.top
            anchors.horizontalCenter: loader.horizontalCenter
            anchors.bottomMargin: Theme.paddingLarge
            color: Theme.highlightColor
            text: page.loadMessage
            font.pixelSize: Theme.fontSizeLarge
        }

        BusyIndicator {
            id: loader
            visible: loading && !errorMessageLabel.visible
            running: visible
            size: BusyIndicatorSize.Large
            anchors.centerIn: parent
        }

        Label {
            id: errorMessageLabel
            anchors.centerIn: parent
            color: Theme.highlightColor
            visible: text.length > 0
            text: page.errorMessage
        }

        Button {
            id: initButton
            visible: false
            anchors.top: errorMessageLabel.bottom
            anchors.horizontalCenter: errorMessageLabel.horizontalCenter
            anchors.topMargin: Theme.paddingLarge
            text: qsTr("Retry")
            onClicked: {
                initButton.visible = false
                errorMessage = ""
                initLoading()
            }
        }
    }

    onStatusChanged: {
        if (status === PageStatus.Active) {
            errorMessage = ""
            if (!loading) {
                if (slackClient.config.userId !== "") {
                    initLoading()
                } else {
                    errorMessage = qsTr("Not logged in")
                }
            }
        }
    }

    Component.onCompleted: {
        slackClient.onTestLoginSuccess.connect(handleLoginTestSuccess)
        slackClient.onTestLoginFail.connect(handleLoginTestFail)
        slackClient.onInitSuccess.connect(handleInitSuccess)
        slackClient.onInitFail.connect(handleInitFail)
        slackClient.onTestConnectionFail.connect(handleConnectionFail)
    }

    function initLoading() {
        loading = true
        slackClient.testLogin()
    }

    function handleLoginTestSuccess() {
        slackClient.init()
    }

    function handleLoginTestFail() {
        loading = false;
        if (slackClient.config.userId !== "") {
            startLogin()
        }
    }

    function startLogin() {
        loading = true
        var loginPage = pageStack.push(Qt.resolvedUrl("LoginPage.qml"))
        loginPage.onLoginSuccess.connect(handleLoginSuccess)
        loginPage.onLoginFail.connect(handleLoginFail)
    }

    function handleLoginSuccess(userId, teamId, teamName, accessToken) {
        errorMessage = ""
        loadMessage = qsTr("Loading")
        slackClient.config.userId = userId
        slackClient.config.teamId = teamId
        slackClient.config.teamName = teamName
        slackClient.config.accessToken = accessToken
        slackClient.init()
    }

    function handleLoginFail() {
        loading = false;
        errorMessage = qsTr("Login failed")
    }

    function handleInitSuccess() {
        pageStack.replace(Qt.resolvedUrl("ChannelList.qml"), { "slackClient": page.slackClient })
    }

    function handleInitFail() {
        loading = false
        errorMessage = qsTr("Error loading team information")
        initButton.visible = true
    }

    function handleConnectionFail() {
        loading = false
        errorMessage = qsTr("No network connection")
        initButton.visible = true
    }
}
