import QtQuick 2.0
import Sailfish.Silica 1.0
import "pages"

ApplicationWindow {
    id: appWindow

    initialPage: Component {
        FirstPage { }
    }

    cover: Qt.resolvedUrl("cover/CoverPage.qml")

    allowedOrientations: defaultAllowedOrientations

    Component.onCompleted: {
        console.log("=== APP STARTUP ===")
        console.log("Workspace count:", workspaceManager.workspaceCount())

        // Check if we have saved workspaces
        if (workspaceManager.workspaceCount() > 0) {
            var token = workspaceManager.currentWorkspaceToken
            console.log("Found saved workspace, auto-login with token:", token ? (token.substring(0, 10) + "...") : "null")

            if (token && token.length > 0) {
                slackAPI.authenticate(token)
                fileManager.setToken(token)
                console.log("Auto-login initiated")
            } else {
                console.log("No valid token, showing login page")
                pageStack.replace(Qt.resolvedUrl("pages/LoginPage.qml"))
            }
        } else {
            console.log("No saved workspaces, showing login page")
            pageStack.replace(Qt.resolvedUrl("pages/LoginPage.qml"))
        }
        console.log("===================")
    }

    Connections {
        target: workspaceManager

        onWorkspaceSwitched: {
            console.log("=== WORKSPACE SWITCHED ===")
            console.log("New token:", token ? (token.substring(0, 10) + "...") : "null")

            // Clear current data
            conversationModel.clear()
            messageModel.clear()
            userModel.clear()

            // Disconnect from current workspace WebSocket
            slackAPI.disconnectWebSocket()

            // Authenticate with new workspace token
            slackAPI.authenticate(token)
            fileManager.setToken(token)

            console.log("Switched to workspace at index:", index)
        }
    }

    // Helper function to navigate to a channel
    function openChannel(channelId) {
        console.log("=== OPENING CHANNEL ===")
        console.log("Channel ID:", channelId)

        // Find channel info from conversationModel
        var channelName = ""
        for (var i = 0; i < conversationModel.rowCount(); i++) {
            var idx = conversationModel.index(i, 0)
            if (conversationModel.data(idx, conversationModel.IdRole) === channelId) {
                channelName = conversationModel.data(idx, conversationModel.NameRole)
                console.log("Found channel name:", channelName)
                break
            }
        }

        // Navigate to the channel
        if (channelName) {
            // Mark channel as read (clear unread count)
            conversationModel.updateUnreadCount(channelId, 0)

            // Clear notifications for this channel
            notificationManager.clearChannelNotifications(channelId)

            // Set current channel and fetch history
            messageModel.currentChannelId = channelId
            slackAPI.fetchConversationHistory(channelId)

            // Clear the page stack and push ConversationPage
            pageStack.clear()
            pageStack.push(Qt.resolvedUrl("pages/FirstPage.qml"))
            pageStack.push(Qt.resolvedUrl("pages/ConversationPage.qml"), {
                "channelId": channelId,
                "channelName": channelName
            })
            console.log("Navigated to channel")
        } else {
            console.warn("Channel not found in conversation list:", channelId)
        }

        console.log("=== END OPENING CHANNEL ===")
    }

    Connections {
        target: notificationManager

        onNotificationClicked: {
            console.log("=== NOTIFICATION CLICKED IN QML ===")
            console.log("Channel ID:", channelId)
            openChannel(channelId)
        }
    }

    // DBus connection for notification clicks
    Connections {
        target: dbusAdaptor

        onPleaseOpenChannel: {
            console.log("=== DBUS PLEASE OPEN CHANNEL ===")
            console.log("Channel ID:", channelId)
            appWindow.activate()
            openChannel(channelId)
        }
    }

    Connections {
        target: slackAPI

        onAuthenticationChanged: {
            if (slackAPI.isAuthenticated) {
                console.log("=== AUTHENTICATION SUCCESS ===")
                console.log("Saving workspace with token length:", slackAPI.token.length)
                console.log("teamId:", slackAPI.teamId)
                console.log("workspaceName:", slackAPI.workspaceName)
                console.log("currentUserId:", slackAPI.currentUserId)

                slackAPI.fetchConversations()
                slackAPI.fetchUsers()
                slackAPI.connectWebSocket()

                // Save workspace info after successful authentication
                workspaceManager.addWorkspace(
                    slackAPI.workspaceName,
                    slackAPI.token,
                    slackAPI.teamId,  // Use real team ID from auth.test
                    slackAPI.currentUserId,
                    slackAPI.workspaceName + ".slack.com"
                )

                console.log("Workspace saved successfully")
            } else {
                pageStack.replace(Qt.resolvedUrl("pages/LoginPage.qml"))
            }
        }

        onAuthenticationError: {
            console.error("Authentication error:", error)
        }

        onNetworkError: {
            console.error("Network error:", error)
        }

        onMessageReceived: {
            // Handle real-time message
            console.log("=== REAL-TIME MESSAGE RECEIVED ===")
            console.log("Message type:", message.type)
            console.log("Channel:", message.channel)

            if (message.type === "message" && message.channel) {
                // If we're currently viewing this channel, add the message
                if (messageModel.currentChannelId === message.channel) {
                    console.log("Adding message to current channel")
                    messageModel.addMessage(message)
                }
            }
        }
    }
}
