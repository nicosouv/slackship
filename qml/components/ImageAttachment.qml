import QtQuick 2.0
import Sailfish.Silica 1.0

Item {
    id: imageAttachment

    property string imageUrl: ""
    property string thumbUrl: ""
    property int imageWidth: 0
    property int imageHeight: 0
    property string title: ""

    width: parent.width
    height: imageLoader.height + (titleLabel.visible ? titleLabel.height : 0)

    Column {
        width: parent.width
        spacing: Theme.paddingSmall

        Label {
            id: titleLabel
            width: parent.width
            text: title
            visible: title.length > 0
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryColor
            wrapMode: Text.Wrap
        }

        Item {
            width: parent.width
            height: Math.min(imageLoader.sourceSize.height, Screen.height / 3)

            Image {
                id: imageLoader
                anchors.fill: parent
                // Use slack:// image provider for authenticated image loading
                source: {
                    var url = thumbUrl || imageUrl
                    if (url && url.length > 0) {
                        return "image://slack/" + url
                    }
                    return ""
                }
                fillMode: Image.PreserveAspectFit
                asynchronous: true
                smooth: true

                BusyIndicator {
                    anchors.centerIn: parent
                    running: imageLoader.status === Image.Loading
                    size: BusyIndicatorSize.Medium
                }

                Label {
                    anchors.centerIn: parent
                    text: qsTr("Failed to load image")
                    visible: imageLoader.status === Image.Error
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeSmall
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        pageStack.push(Qt.resolvedUrl("../pages/ImageViewerPage.qml"), {
                            "imageUrl": imageUrl,
                            "title": title
                        })
                    }
                }

                // Image size indicator
                Rectangle {
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    anchors.margins: Theme.paddingSmall
                    width: sizeLabel.width + Theme.paddingMedium * 2
                    height: sizeLabel.height + Theme.paddingSmall * 2
                    color: Theme.rgba(Theme.highlightDimmerColor, 0.7)
                    radius: Theme.paddingSmall
                    visible: imageWidth > 0 && imageHeight > 0

                    Label {
                        id: sizeLabel
                        anchors.centerIn: parent
                        text: imageWidth + " × " + imageHeight
                        font.pixelSize: Theme.fontSizeExtraSmall
                        color: Theme.primaryColor
                    }
                }
            }
        }
    }
}
