import QtQuick 2.0 as QtQuick
import Sailfish.Silica 1.0

Page {
    id: page

    property variant model

    ProgressBar {
        width: parent.width
        anchors.centerIn: parent
        minimumValue: 0
        maximumValue: 1
        valueText: parseInt(value * 100) + "%"
        value: image.progress
        visible: image.status === QtQuick.Image.Loading
    }

    SilicaFlickable {
        anchors.fill: parent

        QtQuick.AnimatedImage {
            anchors.fill: parent
            id: image
            visible: status === QtQuick.Image.Ready
            source: model.url
            fillMode: QtQuick.Image.PreserveAspectFit
        }
    }
}
