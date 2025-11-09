import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: channelInfoPage

    property string channelId: ""
    property string channelName: ""
    property var channelInfo: null

    Component.onCompleted: {
        slackAPI.fetchConversationInfo(channelId)
    }

    Connections {
        target: slackAPI
        onConversationInfoReceived: function(info) {
            channelInfo = info
        }

        onConversationLeft: function(leftChannelId) {
            // Only navigate if this is the channel we just left
            if (leftChannelId === channelId) {
                // Navigate back to conversations list (FirstPage)
                pageStack.pop(pageStack.find(function(page) {
                    return page.objectName === "firstPage"
                }))
            }
        }
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: qsTr("Channel Info")
            }

            // Channel name
            DetailItem {
                label: qsTr("Channel")
                value: channelName
            }

            // Description/Purpose
            DetailItem {
                label: qsTr("Description")
                value: channelInfo && channelInfo.purpose ? (channelInfo.purpose.value || qsTr("No description")) : qsTr("Loading...")
                visible: channelInfo !== null
            }

            // Topic
            DetailItem {
                label: qsTr("Topic")
                value: channelInfo && channelInfo.topic ? (channelInfo.topic.value || qsTr("No topic")) : ""
                visible: channelInfo && channelInfo.topic && channelInfo.topic.value
            }

            // Member count
            DetailItem {
                label: qsTr("Members")
                value: channelInfo ? (channelInfo.num_members || 0) : 0
                visible: channelInfo !== null
            }

            // Created date
            DetailItem {
                label: qsTr("Created")
                value: {
                    if (channelInfo && channelInfo.created) {
                        var date = new Date(channelInfo.created * 1000)
                        return Qt.formatDate(date, "MMM dd yyyy")
                    }
                    return ""
                }
                visible: channelInfo !== null && channelInfo.created
            }

            // Creator
            DetailItem {
                label: qsTr("Created by")
                value: channelInfo && channelInfo.creator ? userModel.getUserName(channelInfo.creator) : ""
                visible: channelInfo !== null && channelInfo.creator
            }

            // Is archived
            DetailItem {
                label: qsTr("Status")
                value: channelInfo && channelInfo.is_archived ? qsTr("Archived") : qsTr("Active")
                visible: channelInfo !== null
            }

            // Separator
            Separator {
                width: parent.width
                color: Theme.secondaryHighlightColor
                horizontalAlignment: Qt.AlignHCenter
            }

            // Leave channel button
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Leave Channel")
                preferredWidth: Theme.buttonWidthLarge

                onClicked: {
                    remorse.execute(qsTr("Leaving channel"), function() {
                        slackAPI.leaveConversation(channelId)
                        // Navigation will be handled by onConversationLeft signal
                    })
                }
            }
        }

        VerticalScrollDecorator { }
    }

    RemorsePopup {
        id: remorse
    }
}
