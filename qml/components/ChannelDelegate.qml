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
    property var channelLastMessageTime: channelData.lastMessageTime || 0
    property bool channelIsMember: channelData.isMember !== undefined ? channelData.isMember : true
    property bool isLoadingUnread: false

    // Listen for loading state changes
    Connections {
        target: slackAPI
        onChannelLoadingChanged: {
            if (channelId === channelItem.channelId) {
                channelItem.isLoadingUnread = isLoading
            }
        }
    }

    // Format timestamp for display
    function formatMessageTime(timestamp) {
        if (!timestamp || timestamp === 0) {
            return ""
        }

        var now = new Date()
        var messageDate = new Date(timestamp)
        var diffMs = now - messageDate
        var diffDays = Math.floor(diffMs / (1000 * 60 * 60 * 24))

        // Today: show time only
        if (diffDays === 0) {
            return messageDate.toLocaleTimeString(Qt.locale(), "HH:mm")
        }
        // Yesterday
        else if (diffDays === 1) {
            return qsTr("Yesterday")
        }
        // This week: show day name
        else if (diffDays < 7) {
            return messageDate.toLocaleDateString(Qt.locale(), "dddd")
        }
        // This year: show date without year
        else if (messageDate.getFullYear() === now.getFullYear()) {
            return messageDate.toLocaleDateString(Qt.locale(), "dd MMM")
        }
        // Older: show full date
        else {
            return messageDate.toLocaleDateString(Qt.locale(), "dd MMM yyyy")
        }
    }

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
                Icon {
                    source: "image://theme/icon-s-favorite"
                    width: Theme.iconSizeExtraSmall
                    height: Theme.iconSizeExtraSmall
                    color: Theme.highlightColor
                    visible: channelIsStarred
                    anchors.verticalCenter: parent.verticalCenter
                }

                // Icon based on type - using Icon for private channels
                Icon {
                    source: "image://theme/icon-s-secure"
                    width: Theme.iconSizeExtraSmall
                    height: Theme.iconSizeExtraSmall
                    color: channelItem.highlighted ? Theme.highlightColor : Theme.secondaryColor
                    visible: (channelType === "channel" || channelType === "group") && channelIsPrivate
                    anchors.verticalCenter: parent.verticalCenter
                }

                // Text indicator for type
                Label {
                    text: channelType === "im" ? "DM" :
                          channelType === "mpim" ? "GM" : "#"
                    font.pixelSize: Theme.fontSizeSmall
                    color: channelItem.highlighted ? Theme.highlightColor : Theme.secondaryColor
                    width: Theme.fontSizeSmall * 1.5
                    visible: !(channelType === "channel" || channelType === "group") || !channelIsPrivate
                    anchors.verticalCenter: parent.verticalCenter
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

            Row {
                width: parent.width
                spacing: Theme.paddingSmall

                // Mini spinner when loading unread info
                BusyIndicator {
                    size: BusyIndicatorSize.ExtraSmall
                    running: isLoadingUnread
                    visible: running
                    anchors.verticalCenter: parent.verticalCenter
                }

                Label {
                    text: formatMessageTime(channelLastMessageTime)
                    truncationMode: TruncationMode.Fade
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryColor
                    visible: text.length > 0 && !isLoadingUnread
                }
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
                // Use current timestamp to mark everything as read
                var now = Date.now() / 1000  // Convert to seconds
                var timestamp = now.toFixed(6)  // Slack timestamp format
                slackAPI.markConversationRead(channelId, timestamp)
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
