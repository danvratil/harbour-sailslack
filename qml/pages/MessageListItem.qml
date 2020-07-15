import QtQuick 2.1
import Sailfish.Silica 1.0

ListItem {
    id: item
    contentHeight: column.height + Theme.paddingMedium

    menu: ContextMenu {
        MenuItem {
            text: qsTr("User Details")
            onClicked: showUserDetails(user.id)
        }
    }

    property color infoColor: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
    property color textColor: item.highlighted ? Theme.highlightColor : Theme.primaryColor

    Column {
        id: column
        width: parent.width - Theme.paddingLarge * (Screen.sizeCategory >= Screen.Large ? 4 : 2)
        anchors.verticalCenter: parent.verticalCenter
        x: Theme.paddingLarge * (Screen.sizeCategory >= Screen.Large ? 2 : 1)

        Item {
            width: parent.width
            height: childrenRect.height

            Label {
                text: user.name
                anchors.left: parent.left
                font.pixelSize: Theme.fontSizeTiny
                color: infoColor
            }

            Label {
                anchors.right: parent.right
                text: time.toLocaleString(Qt.locale(), "H:mm")
                font.pixelSize: Theme.fontSizeTiny
                color: infoColor
            }
        }

        RichTextLabel {
            id: contentLabel
            width: parent.width
            font.pixelSize: Theme.fontSizeSmall
            color: textColor
            visible: text.length > 0
            value: content
            onLinkActivated: handleLink(link)
        }

        Spacer {
            height: Theme.paddingMedium
            visible: contentLabel.visible && (imageRepeater.count > 0 || attachmentRepeater.count > 0)
        }

        Repeater {
            id: imageRepeater
            model: images
            Column {
                spacing: Theme.paddingSmall
                Text {
                    font.pixelSize: Theme.fontSizeSmall
                    color: textColor
                    visible: text.length > 0
                    text: model.title
                }
                Image {
                    width: parent.width
                    height: model.thumbSize.height
                    fillMode: Image.PreserveAspectFit
                    source: model.thumbUrl
                    sourceSize.width: model.thumbSize.width
                    sourceSize.height: model.thumbSize.height

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            pageStack.push(Qt.resolvedUrl("Image.qml"), {"model": model})
                        }
                    }
                }
            }
        }

        Repeater {
            id: attachmentRepeater
            model: attachments

            Attachment {
                width: column.width
                attachment: model
                onLinkClicked: handleLink(link)
            }
        }
    }

    function handleLink(link) {
        if (link.indexOf("sailslack://") === 0) {
            var parts = link.split('/')
            if (parts[2] === "user") {
                showUserDetails(parts[3])
            }
        } else {
            console.log("external link", link)
            Qt.openUrlExternally(link)
        }
    }

    function showUserDetails(userId) {
        pageStack.push(Qt.resolvedUrl("UserView.qml"), {"slackClient": slackClient, "userId": userId})
    }
}
