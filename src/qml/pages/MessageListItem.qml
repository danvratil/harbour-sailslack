import QtQuick 2.1
import Sailfish.Silica 1.0
import harbour.sailslack 1.0

ListItem {
    id: item
    contentHeight: column.height + Theme.paddingMedium
    signal openThread(string threadId)
    signal markUnread(string timestamp)

    property Client slackClient
    property bool isUnread

    menu: ContextMenu {
        MenuItem {
            visible: !page.threadId
            text: qsTr("Reply in thread")
            onClicked: {
                openThread(message.timestamp)
            }
        }
        MenuItem {
            text: qsTr("Mark unread")
            onClicked: {
                markUnread(message.timestamp)
            }
        }

        MenuItem {
            text: qsTr("User Details")
            onClicked: showUserDetails(message.user.id)
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
                text: message.user.name
                anchors.left: parent.left
                font.pixelSize: Theme.fontSizeTiny
                color: infoColor
            }

            Label {
                anchors.right: parent.right
                text: message.time.toLocaleString(Qt.locale(), "H:mm")
                font.pixelSize: Theme.fontSizeTiny
                color: infoColor
            }
        }

        RichTextLabel {
            id: contentLabel
            width: parent.width
            font.pixelSize: Theme.fontSizeSmall
            font.weight: isUnread ? Font.Bold : Font.Normal
            color: textColor
            visible: text.length > 0
            value: message.content
            onLinkActivated: handleLink(link)
        }

        Item {
            width: parent.width
            height: childrenRect.height
            visible: message.reply_count > 0

            Label {
                text: qsTr("Replies: %1").arg(message.reply_count)
                anchors.left: parent.left
                font.italic: true
                font.pixelSize: Theme.fontSizeTiny
                color: infoColor
            }
        }

        Spacer {
            height: Theme.paddingMedium
            visible: contentLabel.visible && (imageRepeater.count > 0 || attachmentRepeater.count > 0)
        }

        Repeater {
            id: imageRepeater
            model: message.images
            Column {
                Component.onCompleted: console.log(model);
                spacing: Theme.paddingSmall
                Text {
                    font.pixelSize: Theme.fontSizeSmall
                    color: textColor
                    visible: text.length > 0
                    text: model.title
                }
                AnimatedImage {
                    width: parent.width
                    height: model.thumbSize.height
                    fillMode: Image.PreserveAspectFit
                    source: model.thumbUrl

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
            model: message.attachments

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
