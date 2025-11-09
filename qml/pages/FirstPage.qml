import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

Page {
    id: firstPage
    objectName: "firstPage"

    // Allow swiping right to access Stats page
    allowedOrientations: Orientation.All

    property bool isLoading: false
    property string searchText: ""

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

    // Filter conversations by search text
    function matchesSearch(name) {
        if (searchText.length === 0) return true
        return name.toLowerCase().indexOf(searchText.toLowerCase()) >= 0
    }

    SilicaListView {
        id: conversationListView
        anchors.fill: parent
        model: conversationModel

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
                text: qsTr("Refresh")
                onClicked: slackAPI.fetchConversations()
            }
        }

        header: Column {
            width: parent.width

            PageHeader {
                title: slackAPI.workspaceName || "Lagoon"
                description: slackAPI.isAuthenticated ? qsTr("Connected") : qsTr("Disconnected")
            }

            SearchField {
                id: searchField
                width: parent.width
                placeholderText: qsTr("Filter conversations...")
                onTextChanged: searchText = text

                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: focus = false
            }
        }

        delegate: ChannelDelegate {
            visible: matchesSearch(name)
            height: visible ? implicitHeight : 0

            onClicked: {
                console.log("Conversation clicked:", name, id)

                // Mark as read
                conversationModel.updateUnreadCount(id, 0)
                notificationManager.clearChannelNotifications(id)

                // Set current channel and fetch messages
                messageModel.currentChannelId = id
                slackAPI.fetchConversationHistory(id)

                // Navigate to conversation
                pageStack.push(Qt.resolvedUrl("ConversationPage.qml"), {
                    "channelId": id,
                    "channelName": name
                })
            }
        }

        ViewPlaceholder {
            enabled: conversationModel.rowCount() === 0 && !isLoading
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
