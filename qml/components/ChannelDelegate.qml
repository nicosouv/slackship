import QtQuick 2.0
import Sailfish.Silica 1.0

ListItem {
    id: channelItem
    contentHeight: Theme.itemSizeMedium

    // Support both model-based (ListView) and property-based (Repeater) usage
    // When used with modelData, these properties should be bound explicitly
    property var channelData: model || modelData || {}
    property string channelId: channelData.id || ""
    property string channelName: channelData.name || ""
    property string channelType: channelData.type || ""
    property int channelUnreadCount: channelData.unreadCount || 0
    property bool channelIsStarred: channelData.isStarred || false
    property bool channelIsPrivate: channelData.isPrivate || false
    property string channelUserId: channelData.userId || ""
    property string channelTopic: channelData.topic || ""
    property string channelLastMessage: channelData.lastMessage || ""
    property bool channelIsMember: channelData.isMember !== undefined ? channelData.isMember : true

    Row {
        anchors.fill: parent
        anchors.leftMargin: Theme.horizontalPageMargin
        anchors.rightMargin: Theme.horizontalPageMargin
        spacing: Theme.paddingMedium

        // Unread indicator
        Rectangle {
            width: Theme.paddingSmall
            height: parent.height
            color: channelUnreadCount > 0 ? Theme.highlightColor : "transparent"
            visible: channelUnreadCount > 0
        }

        Column {
            width: parent.width - unreadBadge.width - Theme.paddingLarge * 3
            anchors.verticalCenter: parent.verticalCenter
            spacing: Theme.paddingSmall

            Row {
                width: parent.width
                spacing: Theme.paddingSmall

                // Star icon if starred
                Label {
                    text: "⭐"
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.highlightColor
                    visible: channelIsStarred
                }

                // Icon based on type
                Label {
                    text: channelType === "im" ? "💬" :
                          channelType === "mpim" ? "👥" :
                          channelIsPrivate ? "🔒" : "#"
                    font.pixelSize: Theme.fontSizeSmall
                    color: channelItem.highlighted ? Theme.highlightColor : Theme.primaryColor
                }

                Label {
                    width: parent.width - parent.spacing * 2
                    text: {
                        if (channelType === "im" && channelUserId) {
                            // For DMs, show user's real name
                            var userName = userModel.getUserName(channelUserId)
                            return userName
                        } else if (channelType === "mpim") {
                            // For multi-person DMs, show name without #
                            return channelName
                        } else {
                            // For channels, show name (without # since we have icon)
                            return channelName
                        }
                    }
                    font.bold: channelUnreadCount > 0
                    truncationMode: TruncationMode.Fade
                    color: channelItem.highlighted ? Theme.highlightColor : Theme.primaryColor
                }
            }

            Label {
                width: parent.width
                text: channelTopic || channelLastMessage || qsTr("No messages")
                truncationMode: TruncationMode.Fade
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
            }
        }

        Label {
            id: unreadBadge
            anchors.verticalCenter: parent.verticalCenter
            text: channelUnreadCount > 0 ? channelUnreadCount : ""
            font.pixelSize: Theme.fontSizeExtraSmall
            font.bold: true
            color: Theme.highlightColor
            visible: text.length > 0
        }
    }

    menu: ContextMenu {
        MenuItem {
            text: channelIsStarred ? qsTr("Unstar") : qsTr("Star")
            onClicked: {
                conversationModel.toggleStar(channelId)
            }
        }
        MenuItem {
            text: qsTr("Mark as read")
            visible: channelUnreadCount > 0
            onClicked: {
                conversationModel.updateUnreadCount(channelId, 0)
                notificationManager.clearChannelNotifications(channelId)
            }
        }
        MenuItem {
            text: qsTr("Leave channel")
            visible: channelIsMember
            onClicked: {
                remorseAction(qsTr("Leaving channel"), function() {
                    slackAPI.leaveConversation(channelId)
                })
            }
        }
    }
}
