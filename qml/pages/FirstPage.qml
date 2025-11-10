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
    property int visibleConversationCount: 0

    Component.onCompleted: {
        console.log("FirstPage loaded - fetching conversations")
        console.log("Authenticated:", slackAPI.isAuthenticated)
        console.log("Workspace:", slackAPI.workspaceName)

        // Load conversations automatically
        loadWorkspaceData()

        // Attach StatsPage to the right
        pageStack.pushAttached(Qt.resolvedUrl("StatsPage.qml"))

        // Update visible count initially
        updateVisibleCount()
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
            updateVisibleCount()
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

    // Update visible conversation count
    function updateVisibleCount() {
        var count = 0
        for (var i = 0; i < conversationModel.rowCount(); i++) {
            var conv = conversationModel.get(i)
            if (matchesSearch(conv.name)) {
                count++
            }
        }
        visibleConversationCount = count
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
                onTextChanged: {
                    searchText = text
                    updateVisibleCount()
                }

                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: focus = false
            }
        }

        // Group conversations by section (starred, then by type)
        section.property: "section"
        section.delegate: Component {
            Item {
                width: parent.width
                height: sectionLabel.height + Theme.paddingLarge

                Label {
                    id: sectionLabel
                    anchors.left: parent.left
                    anchors.leftMargin: Theme.horizontalPageMargin
                    anchors.verticalCenter: parent.verticalCenter
                    text: {
                        if (section === "starred") return qsTr("Starred")
                        if (section === "channel") return qsTr("Channels")
                        if (section === "im") return qsTr("Direct Messages")
                        if (section === "mpim") return qsTr("Group Messages")
                        return ""
                    }
                    font.pixelSize: Theme.fontSizeSmall
                    font.bold: true
                    color: Theme.highlightColor
                }
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
            enabled: visibleConversationCount === 0 && !isLoading
            text: searchText.length > 0 ? qsTr("No matching conversations") : qsTr("No conversations")
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
