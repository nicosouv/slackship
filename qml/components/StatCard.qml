import QtQuick 2.0
import Sailfish.Silica 1.0

Rectangle {
    property string icon: ""
    property int value: 0
    property string label: ""

    height: Theme.itemSizeMedium
    color: Theme.rgba(Theme.highlightBackgroundColor, 0.1)
    radius: Theme.paddingSmall

    Column {
        anchors.centerIn: parent
        spacing: Theme.paddingSmall

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: icon
            font.pixelSize: Theme.fontSizeHuge
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: value
            font.pixelSize: Theme.fontSizeLarge
            font.bold: true
            color: Theme.highlightColor
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: label
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryColor
        }
    }
}
