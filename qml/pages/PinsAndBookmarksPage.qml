import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

Page {
    id: pinsPage

    property string channelId: ""
    property string channelName: ""
    property var pins: []
    property var bookmarks: []

    Component.onCompleted: {
        slackAPI.fetchPins(channelId)
        slackAPI.fetchBookmarks(channelId)
    }

    Connections {
        target: slackAPI
        onPinsReceived: function(ch_id, items) {
            if (ch_id === channelId) {
                pins = items
            }
        }
        onBookmarksReceived: function(ch_id, items) {
            if (ch_id === channelId) {
                bookmarks = items
            }
        }
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        PullDownMenu {
            MenuItem {
                text: qsTr("Refresh")
                onClicked: {
                    slackAPI.fetchPins(channelId)
                    slackAPI.fetchBookmarks(channelId)
                }
            }
        }

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingMedium

            PageHeader {
                title: qsTr("Pins & Bookmarks")
                description: channelName
            }

            // Bookmarks section
            SectionHeader {
                text: qsTr("Bookmarks") + " (" + bookmarks.length + ")"
            }

            Repeater {
                model: bookmarks

                BackgroundItem {
                    width: parent.width
                    height: bookmarkColumn.height + Theme.paddingLarge

                    onClicked: {
                        var link = modelData.link || ""
                        if (link.length > 0) {
                            Qt.openUrlExternally(link)
                        }
                    }

                    Column {
                        id: bookmarkColumn
                        anchors.centerIn: parent
                        width: parent.width - 2 * Theme.horizontalPageMargin
                        spacing: Theme.paddingSmall

                        Row {
                            width: parent.width
                            spacing: Theme.paddingMedium

                            Icon {
                                source: {
                                    var type = modelData.type || "link"
                                    if (type === "link") return "image://theme/icon-m-link"
                                    return "image://theme/icon-m-file-other"
                                }
                                width: Theme.iconSizeSmall
                                height: Theme.iconSizeSmall
                                color: Theme.highlightColor
                            }

                            Label {
                                text: modelData.title || modelData.link || qsTr("Untitled")
                                width: parent.width - parent.spacing - Theme.iconSizeSmall
                                truncationMode: TruncationMode.Fade
                                color: highlighted ? Theme.highlightColor : Theme.primaryColor
                                font.pixelSize: Theme.fontSizeSmall
                            }
                        }

                        Label {
                            text: modelData.link || ""
                            visible: modelData.link && modelData.link.length > 0
                            width: parent.width
                            truncationMode: TruncationMode.Fade
                            font.pixelSize: Theme.fontSizeExtraSmall
                            color: Theme.secondaryColor
                        }
                    }
                }
            }

            Item {
                width: parent.width
                height: bookmarks.length === 0 ? Theme.itemSizeHuge : 0
                visible: bookmarks.length === 0

                ViewPlaceholder {
                    enabled: bookmarks.length === 0
                    text: qsTr("No bookmarks")
                    hintText: qsTr("Bookmarks appear in the channel header")
                }
            }

            // Pins section
            SectionHeader {
                text: qsTr("Pinned Messages") + " (" + pins.length + ")"
            }

            Repeater {
                model: pins

                BackgroundItem {
                    width: parent.width
                    height: Math.max(pinColumn.height + Theme.paddingLarge, Theme.itemSizeMedium)

                    onClicked: {
                        // TODO: Navigate to message in conversation
                        console.log("Pin clicked:", JSON.stringify(modelData))
                    }

                    Column {
                        id: pinColumn
                        anchors.centerIn: parent
                        width: parent.width - 2 * Theme.horizontalPageMargin
                        spacing: Theme.paddingSmall

                        Row {
                            width: parent.width
                            spacing: Theme.paddingSmall

                            Icon {
                                source: "image://theme/icon-s-secure"
                                width: Theme.iconSizeSmall
                                height: Theme.iconSizeSmall
                                color: Theme.highlightColor
                            }

                            Label {
                                text: {
                                    if (modelData.type === "message" && modelData.message) {
                                        var userName = userModel.getUserName(modelData.message.user) || "Unknown"
                                        return userName
                                    }
                                    return qsTr("File or comment")
                                }
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.highlightColor
                            }
                        }

                        Label {
                            text: {
                                if (modelData.type === "message" && modelData.message) {
                                    return modelData.message.text || qsTr("(no text)")
                                }
                                return qsTr("(unsupported item type)")
                            }
                            width: parent.width
                            wrapMode: Text.Wrap
                            maximumLineCount: 3
                            elide: Text.ElideRight
                            font.pixelSize: Theme.fontSizeExtraSmall
                            color: Theme.primaryColor
                        }

                        Label {
                            text: {
                                var created = modelData.created || 0
                                if (created > 0) {
                                    var date = new Date(created * 1000)
                                    return qsTr("Pinned on") + " " + date.toLocaleDateString()
                                }
                                return ""
                            }
                            visible: text.length > 0
                            font.pixelSize: Theme.fontSizeExtraSmall
                            color: Theme.secondaryColor
                        }
                    }
                }
            }

            Item {
                width: parent.width
                height: pins.length === 0 ? Theme.itemSizeHuge : 0
                visible: pins.length === 0

                ViewPlaceholder {
                    enabled: pins.length === 0
                    text: qsTr("No pins")
                    hintText: qsTr("Pin important messages to find them quickly")
                }
            }
        }

        VerticalScrollDecorator { }
    }
}
