import QtQuick 2.2
import Sailfish.Silica 1.0
import harbour.slackfish 1.0

Page {
    id: page

    property var slackClient

    property bool appActive: Qt.application.state === Qt.ApplicationActive

    onAppActiveChanged: {
        slackClient.setAppActive(appActive)
    }

    SilicaFlickable {
        anchors.fill: parent

        PullDownMenu {
            id: topMenu

           MenuItem {
                text: qsTr("Open chat")
                onClicked: {
                    pageStack.push(Qt.resolvedUrl("ChatSelect.qml"), { "slackClient": page.slackClient })
                }
            }
            MenuItem {
                text: qsTr("Join channel")
                onClicked: {
                    pageStack.push(Qt.resolvedUrl("ChannelSelect.qml"), { "slackClient": page.slackClient })
                }
            }
        }

        ChannelListView {
            id: listView
            anchors.fill: parent
            slackClient: page.slackClient
        }
    }

    ConnectionPanel {
        slackClient: parent.slackClient
    }
}
