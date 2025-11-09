import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: browseChannelsPage

    property var joinedChannelIds: []  // List of channel IDs user has already joined

    Component.onCompleted: {
        slackAPI.fetchAllPublicChannels()
    }

    // Listen for public channels response
    Connections {
        target: slackAPI
        onPublicChannelsReceived: {
            channelListModel.clear()

            for (var i = 0; i < channels.length; i++) {
                var channel = channels[i]
                // Only add public channels
                if (!channel.is_private) {
                    channelListModel.append({
                        "channelId": channel.id || "",
                        "channelName": channel.name || "",
                        "purpose": channel.purpose ? (channel.purpose.value || "") : "",
                        "memberCount": channel.num_members || 0,
                        "isJoined": channel.is_member || false
                    })
                }
            }
        }
    }

    ListModel {
        id: channelListModel
    }

    SilicaFlickable {
        anchors.fill: parent

        PullDownMenu {
            MenuItem {
                text: qsTr("Refresh")
                onClicked: slackAPI.fetchAllPublicChannels()
            }
        }

        contentHeight: column.height

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: qsTr("Browse Channels")
            }

            SearchField {
                id: searchField
                width: parent.width
                placeholderText: qsTr("Search channels")
                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: focus = false
            }

            SilicaListView {
                id: channelList
                width: parent.width
                height: browseChannelsPage.height - searchField.height - Theme.paddingLarge * 2
                model: channelListModel
                clip: true

                delegate: ListItem {
                    id: channelItem
                    width: parent.width
                    contentHeight: Theme.itemSizeMedium

                    // Filter based on search
                    visible: searchField.text.length === 0 ||
                             model.channelName.toLowerCase().indexOf(searchField.text.toLowerCase()) >= 0

                    Column {
                        anchors {
                            left: parent.left
                            leftMargin: Theme.horizontalPageMargin
                            right: joinButton.left
                            rightMargin: Theme.paddingMedium
                            verticalCenter: parent.verticalCenter
                        }
                        spacing: Theme.paddingSmall

                        Row {
                            spacing: Theme.paddingSmall

                            Label {
                                text: "#" + model.channelName
                                font.bold: true
                                color: model.isJoined ? Theme.secondaryColor : Theme.highlightColor
                                font.pixelSize: Theme.fontSizeSmall
                            }

                            Label {
                                text: model.isJoined ? qsTr("(joined)") : ""
                                color: Theme.secondaryHighlightColor
                                font.pixelSize: Theme.fontSizeExtraSmall
                                visible: model.isJoined
                            }
                        }

                        Label {
                            width: parent.width
                            text: model.purpose || qsTr("No description")
                            color: Theme.secondaryColor
                            font.pixelSize: Theme.fontSizeExtraSmall
                            wrapMode: Text.WordWrap
                            maximumLineCount: 2
                            elide: Text.ElideRight
                        }

                        Label {
                            text: qsTr("%n members", "", model.memberCount)
                            color: Theme.secondaryHighlightColor
                            font.pixelSize: Theme.fontSizeExtraSmall
                        }
                    }

                    Button {
                        id: joinButton
                        anchors {
                            right: parent.right
                            rightMargin: Theme.horizontalPageMargin
                            verticalCenter: parent.verticalCenter
                        }
                        text: model.isJoined ? qsTr("View") : qsTr("Join")
                        preferredWidth: Theme.buttonWidthSmall

                        onClicked: {
                            if (model.isJoined) {
                                // Navigate to channel
                                pageStack.replace(Qt.resolvedUrl("ConversationPage.qml"), {
                                    channelId: model.channelId,
                                    channelName: "#" + model.channelName
                                })
                            } else {
                                // Join the channel
                                slackAPI.joinConversation(model.channelId)
                                // Update UI
                                channelListModel.setProperty(index, "isJoined", true)
                                // Refresh conversations list
                                slackAPI.fetchConversations()
                                // Show notification
                                var banner = Notices.show(qsTr("Joined #%1").arg(model.channelName), Notice.Short)
                            }
                        }
                    }

                    onClicked: {
                        if (model.isJoined) {
                            pageStack.replace(Qt.resolvedUrl("ConversationPage.qml"), {
                                channelId: model.channelId,
                                channelName: "#" + model.channelName
                            })
                        }
                    }
                }

                VerticalScrollDecorator { }

                ViewPlaceholder {
                    enabled: channelList.count === 0
                    text: qsTr("No channels found")
                    hintText: qsTr("Pull down to refresh")
                }
            }
        }
    }
}
