import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

Page {
    id: firstPage

    // Allow swiping right to access Stats page
    allowedOrientations: Orientation.All

    property bool isLoading: false

    Component.onCompleted: {
        console.log("FirstPage loaded - fetching conversations")
        console.log("Authenticated:", slackAPI.isAuthenticated)
        console.log("Workspace:", slackAPI.workspaceName)

        // Load conversations automatically
        loadWorkspaceData()

        // Attach StatsPage to the right
        pageStack.pushAttached(Qt.resolvedUrl("StatsPage.qml"))
    }

    // Listen for workspace switches
    Connections {
        target: workspaceManager
        onWorkspaceSwitched: {
            console.log("Workspace switched, reloading data...")
            // Clear old data
            conversationModel.clear()
            userModel.clear()
            messageModel.clear()

            // Show loading indicator and reload
            isLoading = true
            loadWorkspaceData()
        }
    }

    // Listen for conversations loaded
    Connections {
        target: slackAPI
        onConversationsReceived: {
            isLoading = false
        }
    }

    function loadWorkspaceData() {
        isLoading = true
        slackAPI.fetchConversations()
        slackAPI.fetchUsers()
    }

    SilicaListView {
        id: conversationListView
        anchors.fill: parent
        model: conversationModel

        // Group conversations by type (channels vs DMs)
        section.property: "type"
        section.criteria: ViewSection.FullString
        section.delegate: Component {
            SectionHeader {
                text: {
                    if (section === "im") return qsTr("Direct Messages")
                    else if (section === "mpim") return qsTr("Group Messages")
                    else if (section === "channel" || section === "group") return qsTr("Channels")
                    else return qsTr("Other")
                }
            }
        }

        PullDownMenu {
            MenuItem {
                text: qsTr("Settings")
                onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
            }
            MenuItem {
                text: qsTr("Workspace Insights")
                onClicked: pageStack.push(Qt.resolvedUrl("StatsPage.qml"))
            }
            MenuItem {
                text: qsTr("Switch Workspace")
                visible: workspaceManager.workspaceCount() > 1
                onClicked: pageStack.push(Qt.resolvedUrl("WorkspaceSwitcher.qml"))
            }
            MenuItem {
                text: qsTr("Browse Channels")
                onClicked: pageStack.push(Qt.resolvedUrl("BrowseChannelsPage.qml"))
            }
            MenuItem {
                text: qsTr("Search")
                onClicked: pageStack.push(Qt.resolvedUrl("SearchPage.qml"))
            }
            MenuItem {
                text: qsTr("Refresh")
                onClicked: slackAPI.fetchConversations()
            }
        }

        header: PageHeader {
            title: slackAPI.workspaceName || "Lagoon"
            description: slackAPI.isAuthenticated ? qsTr("Connected") : qsTr("Disconnected")
        }

        delegate: ChannelDelegate {
            onClicked: {
                console.log("Channel clicked:", model.name, model.id)

                // Mark channel as read (clear unread count)
                conversationModel.updateUnreadCount(model.id, 0)

                // Clear notifications for this channel
                notificationManager.clearChannelNotifications(model.id)

                // Set current channel ID (property assignment, not function call)
                messageModel.currentChannelId = model.id

                // Fetch messages for this channel
                slackAPI.fetchConversationHistory(model.id)

                // Navigate to conversation page
                pageStack.push(Qt.resolvedUrl("ConversationPage.qml"), {
                    "channelId": model.id,
                    "channelName": model.name
                })
            }
        }

        ViewPlaceholder {
            enabled: conversationListView.count === 0 && !isLoading
            text: qsTr("No conversations")
            hintText: qsTr("Pull down to refresh")
        }

        VerticalScrollDecorator { }
    }

    BusyIndicator {
        anchors.centerIn: parent
        size: BusyIndicatorSize.Large
        running: isLoading
        visible: running
    }
}
