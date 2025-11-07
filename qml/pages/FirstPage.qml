import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

Page {
    id: firstPage

    Component.onCompleted: {
        console.log("FirstPage loaded - fetching conversations")
        console.log("Authenticated:", slackAPI.isAuthenticated)
        console.log("Workspace:", slackAPI.workspaceName)

        // Load conversations automatically
        slackAPI.fetchConversations()
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
                text: qsTr("Switch Workspace")
                visible: workspaceManager.workspaceCount() > 1
                onClicked: pageStack.push(Qt.resolvedUrl("WorkspaceSwitcher.qml"))
            }
            MenuItem {
                text: qsTr("Search")
                onClicked: {
                    // TODO: Implement search
                }
            }
            MenuItem {
                text: qsTr("Refresh")
                onClicked: slackAPI.fetchConversations()
            }
        }

        header: PageHeader {
            title: slackAPI.workspaceName || "SlackShip"
            description: slackAPI.isAuthenticated ? qsTr("Connected") : qsTr("Disconnected")
        }

        delegate: ChannelDelegate {
            onClicked: {
                console.log("Channel clicked:", model.name, model.id)

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
            enabled: conversationListView.count === 0
            text: qsTr("No conversations")
            hintText: qsTr("Pull down to refresh")
        }

        VerticalScrollDecorator { }
    }
}
