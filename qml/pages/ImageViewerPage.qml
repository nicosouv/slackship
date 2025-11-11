import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: imageViewerPage

    property string imageUrl: ""
    property string title: ""
    property var copyBanner: null
    property string finalImageUrl: ""

    allowedOrientations: Orientation.All

    Component.onCompleted: {
        // Check if URL requires Slack authentication
        if (imageUrl.indexOf("files.slack.com") !== -1) {
            finalImageUrl = "image://slack/" + imageUrl
            console.log("ImageViewerPage: Using auth provider: " + finalImageUrl)
        } else {
            finalImageUrl = imageUrl
            console.log("ImageViewerPage: Using direct URL: " + finalImageUrl)
        }
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        PullDownMenu {
            MenuItem {
                text: qsTr("Save to Gallery")
                onClicked: {
                    // Download image to Pictures directory
                    var picturesPath = StandardPaths.pictures
                    var fileName = imageUrl.split('/').pop()
                    fileManager.downloadImage(imageUrl, picturesPath + "/" + fileName)
                }
            }

            MenuItem {
                text: qsTr("Copy image URL")
                onClicked: {
                    Clipboard.text = imageUrl

                    // Destroy existing banner if present
                    if (copyBanner) {
                        copyBanner.destroy()
                    }

                    // Create and show banner notification
                    copyBanner = Qt.createQmlObject(
                        'import QtQuick 2.0; import Sailfish.Silica 1.0; ' +
                        'Label { ' +
                        '    anchors.centerIn: parent; ' +
                        '    text: qsTr("URL copied to clipboard"); ' +
                        '    color: Theme.highlightColor; ' +
                        '    font.pixelSize: Theme.fontSizeLarge; ' +
                        '    opacity: 1.0; ' +
                        '    Behavior on opacity { FadeAnimation {} } ' +
                        '}',
                        imageViewerPage
                    )

                    bannerTimer.restart()
                }
            }
        }

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: imageViewerPage.title || qsTr("Image")
            }

            Item {
                width: parent.width
                height: Screen.height - pageHeader.height

                PinchArea {
                    id: pinchArea
                    anchors.fill: parent

                    property real initialWidth
                    property real initialHeight

                    onPinchStarted: {
                        initialWidth = imageViewer.width
                        initialHeight = imageViewer.height
                    }

                    onPinchUpdated: {
                        var scale = pinch.scale
                        imageViewer.width = Math.max(parent.width, Math.min(parent.width * 3, initialWidth * scale))
                        imageViewer.height = Math.max(parent.height, Math.min(parent.height * 3, initialHeight * scale))
                    }

                    Flickable {
                        anchors.fill: parent
                        contentWidth: imageViewer.width
                        contentHeight: imageViewer.height
                        clip: true

                        Image {
                            id: imageViewer
                            width: parent.width
                            height: parent.height
                            source: finalImageUrl
                            fillMode: Image.PreserveAspectFit
                            asynchronous: true
                            smooth: true

                            BusyIndicator {
                                anchors.centerIn: parent
                                running: imageViewer.status === Image.Loading
                                size: BusyIndicatorSize.Large
                            }

                            Label {
                                anchors.centerIn: parent
                                text: qsTr("Failed to load image")
                                visible: imageViewer.status === Image.Error
                                color: Theme.secondaryColor
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            onDoubleClicked: {
                                // Reset zoom
                                imageViewer.width = Qt.binding(function() { return parent.width })
                                imageViewer.height = Qt.binding(function() { return parent.height })
                            }
                        }
                    }
                }
            }
        }
    }

    // Timer to hide the copy banner
    Timer {
        id: bannerTimer
        interval: 2000
        repeat: false
        onTriggered: {
            if (copyBanner) {
                copyBanner.opacity = 0.0
                // Destroy after fade animation completes
                Qt.callLater(function() {
                    if (copyBanner) {
                        copyBanner.destroy()
                        copyBanner = null
                    }
                })
            }
        }
    }
}
