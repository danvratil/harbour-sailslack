import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.sailslack 1.0

Page {
    id: page
    property Client slackClient
    property string userId

    SilicaFlickable {
        anchors.fill: parent

        PullDownMenu {
            MenuItem {
                text: qsTr("Direct Messages")
            }
        }

        Image {
            id: photo
            width: parent.width
            height: status == Image.Ready ? (sourceSize.height / sourceSize.width) * width : width
            asynchronous: true
            fillMode: Image.PreserveAspectFit

            BusyIndicator {
                anchors.centerIn: parent
                size: BusyIndicatorSize.Medium
                running: photo.status != Image.Ready
            }
        }
        Column {
            anchors {
                top: photo.bottom
                left: parent.left
                right: parent.right
                bottom: parent.bottom
                leftMargin: Theme.horizontalPageMargin
                rightMargin: Theme.horizontalPageMargin
            }

            spacing: Theme.paddingSmall



            Label {
                id: name
                font.pixelSize: Theme.fontSizeLarge
                width: parent.width
            }

            Label {
                id: title
                width: parent.width
                visible: text !== ""
                font.pixelSize: Theme.fontSizeSmall
            }

            DetailItem {
                id: displayName
                label: qsTr("Display Name")
                visible: value !== ""
                width: parent.width
            }

            DetailItem {
                id: email
                label: qsTr("Email")
                visible: value !== ""
                width: parent.width
            }

            DetailItem {
                id: timezone
                label: qsTr("Timezone")
                visible: value !== ""
                width: parent.width
            }

            DetailItem {
                id: phone
                label: qsTr("Phone")
                visible: value !== ""
                width: parent.width
            }

            DetailItem {
                id: skype
                label: qsTr("Skype")
                visible: value !== ""
                width: parent.width
            }
        }
    }

    Component.onCompleted: function() {
        slackClient.loadUserInfoSuccess.connect(handleLoadUserInfoSuccess);
        slackClient.loadUserInfoFail.connect(handleLoadUserInfoFail);

        slackClient.loadUserInfo(userId);
    }

    function handleLoadUserInfoSuccess(userId, user) {
        if (userId !== page.userId) {
            return
        }

        photo.source = user.profile.image_512
        name.text = user.profile.real_name
        title.text = user.profile.title
        displayName.value = user.profile.display_name
        email.value= user.profile.email
        timezone.value = user.tz_label
        phone.value = user.profile.phone
        skype.value = user.profile.skype
    }

    function handleLoadUserInfoFail(userId) {
        // TODO
    }
}
