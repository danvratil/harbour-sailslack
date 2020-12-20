import QtQuick 2.0 as QtQuick
import QtQuick.Layouts 1.0
import Sailfish.Silica 1.0
import harbour.sailslack 1.0

QtQuick.Grid {
    property Client slackClient

    property alias model: repeater.model

    spacing: Theme.paddingMedium

    QtQuick.Repeater {
        id: repeater

        RowLayout {
            spacing: Theme.paddingSmall

            QtQuick.Image {
                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                Layout.preferredHeight: Theme.fontSizeSmall
                Layout.preferredWidth: Theme.fontSizeSmall

                height: Theme.fontSizeSmall
                width: Theme.fontSizeSmall
                source: slackClient.emojiProvider.urlForEmoji(name)

                fillMode: QtQuick.Image.PreserveAspectFit
            }

            Label {
                Layout.alignment: Qt.AlignVCenter

                text: count
                font.pixelSize: Theme.fontSizeExtraSmall
            }
        }
    }
}
