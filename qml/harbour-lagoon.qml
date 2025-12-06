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
        console.log("[App] Startup, workspaces:", workspaceManager.workspaceCount())

        if (workspaceManager.workspaceCount() > 0) {
            var token = workspaceManager.currentWorkspaceToken
            if (token && token.length > 0) {
                console.log("[App] Auto-login with saved token")
                slackAPI.authenticate(token)
                fileManager.setToken(token)
            } else {
                console.log("[App] No valid token, showing login")
                pageStack.replace(Qt.resolvedUrl("pages/LoginPage.qml"))
            }
        } else {
            console.log("[App] No workspaces, showing login")
            pageStack.replace(Qt.resolvedUrl("pages/LoginPage.qml"))
        }
    }

    Connections {
        target: workspaceManager

        onWorkspaceSwitched: {
            console.log("[App] Workspace switched to index:", index, "token length:", token.length)
            console.log("[App] Clearing models and re-authenticating...")
            conversationModel.clear()
            messageModel.clear()
            userModel.clear()
            slackAPI.disconnectWebSocket()
            console.log("[App] Calling slackAPI.authenticate with new token")
            slackAPI.authenticate(token)
            fileManager.setToken(token)
            console.log("[App] Workspace switch handler complete")
        }
    }

    // Helper function to navigate to a channel
    function openChannel(channelId) {
        // Find channel info from conversationModel
        var channelName = ""
        for (var i = 0; i < conversationModel.rowCount(); i++) {
            var idx = conversationModel.index(i, 0)
            if (conversationModel.data(idx, conversationModel.IdRole) === channelId) {
                channelName = conversationModel.data(idx, conversationModel.NameRole)
                break
            }
        }

        if (channelName) {
            console.log("[App] Opening channel:", channelName)
            conversationModel.updateUnreadCount(channelId, 0)
            notificationManager.clearChannelNotifications(channelId)
            messageModel.currentChannelId = channelId
            slackAPI.fetchConversationHistory(channelId)
            pageStack.clear()
            pageStack.push(Qt.resolvedUrl("pages/FirstPage.qml"))
            pageStack.push(Qt.resolvedUrl("pages/ConversationPage.qml"), {
                "channelId": channelId,
                "channelName": channelName
            })
        } else {
            console.warn("[App] Channel not found:", channelId)
        }
    }

    Connections {
        target: notificationManager
        onNotificationClicked: openChannel(channelId)
    }

    Connections {
        target: conversationModel
        onConversationsUpdated: {
            // After conversations are loaded, fetch unread counts for each one
            if (conversationIds.length > 0) {
                console.log("[App] Fetching unread counts for", conversationIds.length, "conversations")
                slackAPI.fetchConversationUnreads(conversationIds)
            }
        }
    }

    Connections {
        target: dbusAdaptor
        onPleaseOpenChannel: {
            appWindow.activate()
            openChannel(channelId)
        }
    }

    Connections {
        target: slackAPI

        onAuthenticationChanged: {
            if (slackAPI.isAuthenticated) {
                console.log("[App] Auth success:", slackAPI.workspaceName)
                slackAPI.fetchConversations()

                // Try to load users from cache first
                var teamId = slackAPI.teamId
                if (userModel.hasFreshCache(teamId) && userModel.loadUsersFromCache(teamId)) {
                    console.log("[App] Users loaded from cache, skipping API call")
                } else {
                    console.log("[App] Fetching users from API")
                    slackAPI.fetchUsers()
                }

                slackAPI.connectWebSocket()
            } else {
                pageStack.replace(Qt.resolvedUrl("pages/LoginPage.qml"))
            }
        }

        onTeamIdChanged: {
            if (slackAPI.teamId && slackAPI.teamId.length > 0) {
                console.log("[App] Saving workspace:", slackAPI.workspaceName)
                workspaceManager.addWorkspace(
                    slackAPI.workspaceName,
                    slackAPI.token,
                    slackAPI.teamId,
                    slackAPI.currentUserId,
                    slackAPI.workspaceName + ".slack.com"
                )
            }
        }

        onConversationUnreadReceived: {
            // Update conversation model with unread info
            conversationModel.updateUnreadInfo(channelId, unreadCount, lastMessageTime)
        }

        onConversationTimestampUpdated: {
            // Update conversation model with timestamp from history fetch
            conversationModel.updateTimestamp(channelId, lastMessageTime)
        }

        onAllUnreadsFetched: {
            // After all unreads are fetched, get timestamps for channels that still need them
            var channelsWithoutTs = conversationModel.getChannelsWithoutTimestamp()
            if (channelsWithoutTs.length > 0) {
                console.log("[App] Fetching timestamps for", channelsWithoutTs.length, "channels")
                slackAPI.fetchChannelTimestamps(channelsWithoutTs)
            }
        }

        onAuthenticationError: {
            console.error("[App] Auth error:", error)
        }

        onNetworkError: {
            console.error("[App] Network error:", error)
        }

        onMessageReceived: {
            if (message.type === "message" && message.channel) {
                if (messageModel.currentChannelId === message.channel) {
                    messageModel.addMessage(message)
                }
            }
        }
    }
}
