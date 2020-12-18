import QtQuick 2.0 as QtQuick
import QtQuick.Layouts 1.0
import Sailfish.Silica 1.0
import harbour.sailslack 1.0

QtQuick.Grid {
    property alias model: repeater.model

    spacing: Theme.paddingSmall

    QtQuick.Repeater {
        id: repeater

        RowLayout {
            spacing: Theme.paddingSmall

            QtQuick.Image {
                Layout.alignment: Qt.AlignLeft | Qt.AlignHCenter
                Layout.preferredWidth: sourceSize.width
                Layout.preferredHeight: sourceSize.height

                width: sourceSize.width
                height: sourceSize.height
                source: EmojiProvider.urlForEmoji(name)
            }

            Label {
                text: count
                font.pixelSize: Theme.fontSizeExtraSmall
            }
        }
    }
}
