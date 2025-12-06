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
    property bool usersLoaded: false
    property bool conversationsLoaded: false

    // Collapsible section states (persisted in settings)
    property bool channelsCollapsed: appSettings.channelsCollapsed
    property bool dmCollapsed: appSettings.dmCollapsed
    property bool groupMessagesCollapsed: appSettings.groupMessagesCollapsed

    // Check if a section is collapsed (starred and unread are never collapsed)
    function isSectionCollapsed(sectionName) {
        if (sectionName === "starred" || sectionName === "unread") return false
        if (sectionName === "channel") return channelsCollapsed
        if (sectionName === "im") return dmCollapsed
        if (sectionName === "mpim") return groupMessagesCollapsed
        return false
    }

    // Toggle section collapse state
    function toggleSection(sectionName) {
        if (sectionName === "channel") {
            channelsCollapsed = !channelsCollapsed
            appSettings.channelsCollapsed = channelsCollapsed
        } else if (sectionName === "im") {
            dmCollapsed = !dmCollapsed
            appSettings.dmCollapsed = dmCollapsed
        } else if (sectionName === "mpim") {
            groupMessagesCollapsed = !groupMessagesCollapsed
            appSettings.groupMessagesCollapsed = groupMessagesCollapsed
        }
    }

    Component.onCompleted: {
        console.log("FirstPage loaded")
        console.log("Authenticated:", slackAPI.isAuthenticated)
        console.log("Workspace:", slackAPI.workspaceName)

        // Data loading is handled by harbour-lagoon.qml via onAuthenticationChanged
        // Just set loading state and wait for data
        isLoading = true
        updateVisibleCount()
    }

    // Attach StatsPage after page transition completes
    onStatusChanged: {
        if (status === PageStatus.Active && !pageStack.nextPage(firstPage)) {
            pageStack.pushAttached(Qt.resolvedUrl("StatsPage.qml"))
        }
    }

    // Listen for workspace switches
    Connections {
        target: workspaceManager
        onWorkspaceSwitched: {
            console.log("Workspace switched - resetting loading state")
            // Reset loading flags (data loading is triggered by authentication in harbour-lagoon.qml)
            usersLoaded = false
            conversationsLoaded = false
            isLoading = true
        }

        onAllWorkspacesRemoved: {
            console.log("All workspaces removed - logging out")
            // Clear all data
            conversationModel.clear()
            userModel.clear()
            messageModel.clear()
            statsManager.resetStats()

            // Logout from Slack API
            slackAPI.logout()

            // Navigate to login page
            pageStack.replace(Qt.resolvedUrl("LoginPage.qml"))
        }
    }

    // Listen for data loaded
    Connections {
        target: slackAPI
        onConversationsReceived: {
            console.log("[FirstPage] Conversations received")
            conversationsLoaded = true
            checkLoadingComplete()
        }
        onUsersReceived: {
            console.log("[FirstPage] Users received")
            usersLoaded = true
            checkLoadingComplete()
        }
        onNetworkError: {
            // If users.list fails, still mark as loaded to stop spinner
            console.log("[FirstPage] Network error - marking users as loaded")
            usersLoaded = true
            checkLoadingComplete()
        }
    }

    // Refresh list when users are loaded (to update DM names)
    Connections {
        target: userModel
        onUsersUpdated: {
            // Force the list to refresh so DM names are resolved
            conversationListView.model = null
            conversationListView.model = conversationModel
        }
    }

    function checkLoadingComplete() {
        console.log("[FirstPage] checkLoadingComplete: users=" + usersLoaded + " conversations=" + conversationsLoaded)
        // Only stop loading when both users and conversations are loaded
        if (usersLoaded && conversationsLoaded) {
            console.log("[FirstPage] Loading complete - hiding spinner")
            isLoading = false
            updateVisibleCount()
        }
    }

    function loadWorkspaceData() {
        isLoading = true
        usersLoaded = false
        conversationsLoaded = false
        slackAPI.fetchUsers()
        slackAPI.fetchConversations()
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
            }

            // User profile section
            BackgroundItem {
                id: profileSection
                width: parent.width
                height: profileRow.height + Theme.paddingMedium * 2
                visible: slackAPI.isAuthenticated

                // Re-evaluate avatar when users are loaded
                property string currentUserAvatar: ""

                Connections {
                    target: userModel
                    onUsersUpdated: {
                        profileSection.currentUserAvatar = userModel.getUserAvatar(slackAPI.currentUserId) || ""
                    }
                }

                Component.onCompleted: {
                    currentUserAvatar = userModel.getUserAvatar(slackAPI.currentUserId) || ""
                }

                onClicked: {
                    var userDetails = userModel.getUserDetails(slackAPI.currentUserId)
                    if (userDetails && userDetails.id) {
                        pageStack.push(Qt.resolvedUrl("UserProfilePage.qml"), {
                            "userId": slackAPI.currentUserId
                        })
                    }
                }

                Row {
                    id: profileRow
                    anchors {
                        left: parent.left
                        right: parent.right
                        verticalCenter: parent.verticalCenter
                        leftMargin: Theme.horizontalPageMargin
                        rightMargin: Theme.horizontalPageMargin
                    }
                    spacing: Theme.paddingMedium

                    // User avatar
                    Image {
                        id: userAvatar
                        width: Theme.iconSizeMedium
                        height: Theme.iconSizeMedium
                        anchors.verticalCenter: parent.verticalCenter
                        source: profileSection.currentUserAvatar
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true  // Load asynchronously for performance
                        visible: status === Image.Ready

                        layer.enabled: true
                        layer.effect: ShaderEffect {
                            property real radius: 0.5
                            fragmentShader: "
                                uniform sampler2D source;
                                uniform lowp float qt_Opacity;
                                varying highp vec2 qt_TexCoord0;
                                void main() {
                                    highp vec2 center = vec2(0.5, 0.5);
                                    highp float dist = distance(qt_TexCoord0, center);
                                    if (dist > 0.5) {
                                        gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
                                    } else {
                                        gl_FragColor = texture2D(source, qt_TexCoord0) * qt_Opacity;
                                    }
                                }
                            "
                        }

                        Rectangle {
                            anchors.fill: parent
                            color: "transparent"
                            border.color: Theme.rgba(Theme.highlightColor, 0.3)
                            border.width: 1
                            radius: width / 2
                        }
                    }

                    // Fallback avatar placeholder
                    Rectangle {
                        width: Theme.iconSizeMedium
                        height: Theme.iconSizeMedium
                        anchors.verticalCenter: parent.verticalCenter
                        radius: width / 2
                        color: Theme.rgba(Theme.highlightBackgroundColor, 0.3)
                        visible: userAvatar.status !== Image.Ready

                        Label {
                            anchors.centerIn: parent
                            text: {
                                var name = userModel.getUserName(slackAPI.currentUserId)
                                return name ? name.charAt(0).toUpperCase() : "?"
                            }
                            font.pixelSize: Theme.fontSizeMedium
                            font.bold: true
                            color: Theme.highlightColor
                        }
                    }

                    // User info
                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: Theme.paddingSmall / 2

                        Label {
                            text: userModel.getUserName(slackAPI.currentUserId) || slackAPI.currentUserId
                            font.pixelSize: Theme.fontSizeSmall
                            color: Theme.highlightColor
                        }

                        Label {
                            text: qsTr("Tap to view profile")
                            font.pixelSize: Theme.fontSizeExtraSmall
                            color: Theme.secondaryColor
                        }
                    }
                }
            }

            // Separator
            Separator {
                width: parent.width
                color: Theme.rgba(Theme.highlightColor, 0.2)
                visible: slackAPI.isAuthenticated
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
            BackgroundItem {
                id: sectionHeader
                width: parent.width
                height: sectionLabel.height + Theme.paddingLarge

                // Only collapsible sections are clickable
                enabled: section === "channel" || section === "im" || section === "mpim"

                // Determine if this section is collapsed
                // Use direct property bindings to ensure updates
                property bool isCollapsed: {
                    if (section === "channel") return channelsCollapsed
                    if (section === "im") return dmCollapsed
                    if (section === "mpim") return groupMessagesCollapsed
                    return false
                }
                property bool isCollapsible: section === "channel" || section === "im" || section === "mpim"

                Row {
                    anchors.left: parent.left
                    anchors.leftMargin: Theme.horizontalPageMargin
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: Theme.paddingSmall

                    // Collapse/expand icon (only for collapsible sections)
                    Icon {
                        id: collapseIcon
                        source: sectionHeader.isCollapsed ? "image://theme/icon-m-right" : "image://theme/icon-m-down"
                        width: Theme.iconSizeExtraSmall
                        height: Theme.iconSizeExtraSmall
                        color: Theme.highlightColor
                        visible: sectionHeader.isCollapsible
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Label {
                        id: sectionLabel
                        text: {
                            if (section === "starred") return qsTr("Starred")
                            if (section === "unread") return qsTr("Unread")
                            if (section === "channel") return qsTr("Channels")
                            if (section === "im") return qsTr("Direct Messages")
                            if (section === "mpim") return qsTr("Group Messages")
                            return ""
                        }
                        font.pixelSize: Theme.fontSizeSmall
                        font.bold: true
                        color: sectionHeader.highlighted ? Theme.highlightColor : Theme.highlightColor
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                onClicked: {
                    toggleSection(section)
                }
            }
        }

        delegate: ChannelDelegate {
            // Hide if doesn't match search (use visible)
            // Hide if section is collapsed (use height: 0 to keep section header visible)
            visible: matchesSearch(name)
            height: visible && !isSectionCollapsed(model.section) ? implicitHeight : 0
            clip: true
            opacity: height > 0 ? 1 : 0

            onChannelSelected: {
                pageStack.push(Qt.resolvedUrl("ConversationPage.qml"), {
                    "channelId": channelId,
                    "channelName": channelName,
                    "channelType": channelType,
                    "userId": userId
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
