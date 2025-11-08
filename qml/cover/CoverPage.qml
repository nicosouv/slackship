import QtQuick 2.0
import Sailfish.Silica 1.0

CoverBackground {
    id: cover

    property int unreadCount: 0
    property string lastMessageText: ""
    property string lastMessageChannel: ""
    property bool hasNewMentions: false

    // Calculate total unread messages
    Component.onCompleted: {
        updateUnreadCount()
    }

    Connections {
        target: conversationModel
        onDataChanged: updateUnreadCount()
        onRowsInserted: updateUnreadCount()
    }

    Connections {
        target: slackAPI
        onMessageReceived: {
            var channelId = message.channel
            var text = message.text
            var userId = message.user

            // Update last message info
            lastMessageText = text
            for (var i = 0; i < conversationModel.rowCount(); i++) {
                var idx = conversationModel.index(i, 0)
                if (conversationModel.data(idx, 256) === channelId) { // IdRole = 256
                    lastMessageChannel = conversationModel.data(idx, 257) // NameRole = 257
                    break
                }
            }

            // Check if it's a mention
            if (text.indexOf("@") !== -1) {
                hasNewMentions = true
            }

            updateUnreadCount()
        }
    }

    function updateUnreadCount() {
        var total = 0
        for (var i = 0; i < conversationModel.rowCount(); i++) {
            var idx = conversationModel.index(i, 0)
            var count = conversationModel.data(idx, 261) // UnreadCountRole = 261
            total += count || 0
        }
        unreadCount = total
    }

    // Background gradient for active state
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: Theme.rgba(Theme.highlightBackgroundColor, 0.1) }
            GradientStop { position: 1.0; color: "transparent" }
        }
        visible: slackAPI.isAuthenticated
    }

    Column {
        anchors.centerIn: parent
        width: parent.width - Theme.paddingLarge * 2
        spacing: Theme.paddingMedium

        // App logo image
        Image {
            anchors.horizontalCenter: parent.horizontalCenter
            width: Theme.iconSizeSmall
            height: Theme.iconSizeSmall
            source: "/usr/share/icons/hicolor/86x86/apps/harbour-lagoon.png"
            smooth: true
        }

        // Workspace name (larger, more prominent)
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: workspaceManager.currentWorkspaceName || "Lagoon"
            font.pixelSize: Theme.fontSizeLarge
            font.bold: true
            color: Theme.primaryColor
            truncationMode: TruncationMode.Fade
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
        }

        // Unread count badge
        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.max(unreadBadge.width + Theme.paddingLarge, Theme.iconSizeMedium)
            height: Theme.iconSizeMedium
            radius: height / 2
            color: hasNewMentions ? Theme.highlightColor : Theme.rgba(Theme.highlightBackgroundColor, 0.5)
            visible: unreadCount > 0

            Label {
                id: unreadBadge
                anchors.centerIn: parent
                text: unreadCount > 99 ? "99+" : unreadCount
                font.pixelSize: Theme.fontSizeLarge
                font.bold: true
                color: hasNewMentions ? Theme.primaryColor : Theme.highlightColor
            }
        }

        // Last message preview
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width
            text: lastMessageChannel ? "#" + lastMessageChannel : ""
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryHighlightColor
            horizontalAlignment: Text.AlignHCenter
            truncationMode: TruncationMode.Fade
            visible: lastMessageText.length > 0
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width
            text: lastMessageText
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryColor
            horizontalAlignment: Text.AlignHCenter
            maximumLineCount: 2
            wrapMode: Text.WordWrap
            truncationMode: TruncationMode.Fade
            visible: lastMessageText.length > 0
        }

        // Connection status or stats
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: {
                if (!slackAPI.isAuthenticated) {
                    return qsTr("Disconnected")
                } else if (lastMessageText.length > 0) {
                    return ""
                } else if (statsManager.totalMessages > 0) {
                    return qsTr("%1 messages • %2 day streak").arg(statsManager.totalMessages).arg(statsManager.currentStreak)
                } else {
                    return qsTr("Connected")
                }
            }
            font.pixelSize: Theme.fontSizeExtraSmall
            color: slackAPI.isAuthenticated ? Theme.highlightColor : Theme.secondaryColor
            visible: text.length > 0
            horizontalAlignment: Text.AlignHCenter
            width: parent.width
        }
    }

    // Cover actions
    CoverActionList {
        id: coverActionList

        CoverAction {
            iconSource: "image://theme/icon-cover-refresh"
            onTriggered: {
                if (slackAPI.isAuthenticated) {
                    slackAPI.fetchConversations()
                }
            }
        }

        CoverAction {
            iconSource: "image://theme/icon-cover-new"
            onTriggered: {
                appWindow.activate()
            }
        }
    }

    // Alternative actions when there are unread messages
    CoverActionList {
        id: coverActionListUnread
        enabled: unreadCount > 0

        CoverAction {
            iconSource: "image://theme/icon-cover-pause"
            onTriggered: {
                // Mark all as read (feature to implement)
                hasNewMentions = false
            }
        }

        CoverAction {
            iconSource: "image://theme/icon-cover-new"
            onTriggered: {
                appWindow.activate()
            }
        }
    }
}
