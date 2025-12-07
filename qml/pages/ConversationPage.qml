import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

Page {
    id: conversationPage

    property string channelId
    property string channelName
    property string channelType: ""
    property string userId: ""
    property bool isSendingMessage: false
    property var typingUsers: []
    property var typingTimers: ({})

    // @mention autocomplete
    property bool showMentionSuggestions: false
    property string mentionQuery: ""
    property int mentionStartPos: -1
    property var mentionSuggestions: []

    // Computed display name - resolve user name for DMs
    property string displayName: {
        if (channelType === "im" && userId) {
            return userModel.getUserName(userId)
        }
        return channelName
    }

    Component.onCompleted: {
        console.log("ConversationPage loaded")
        console.log("Channel:", displayName, channelId)

        // Set as active channel (for RTM unread tracking)
        slackAPI.activeChannelId = channelId

        // Fetch messages for this channel
        refreshMessages()

        // Restore draft for this channel
        loadDraft()

        // Mark conversation as read after a short delay
        // to allow messages to load first
        markAsReadTimer.start()
    }

    // Timer to mark conversation as read (with delay as recommended by Slack)
    Timer {
        id: markAsReadTimer
        interval: 5000  // 5 seconds as recommended by Slack docs
        repeat: false
        onTriggered: {
            markConversationAsRead()
        }
    }

    onStatusChanged: {
        if (status === PageStatus.Active) {
            // Restart timer when page becomes active
            markAsReadTimer.restart()
        }
    }

    function markConversationAsRead() {
        // Get the timestamp of the most recent message
        var latestTimestamp = messageModel.getLatestTimestamp()

        if (latestTimestamp && latestTimestamp.length > 0) {
            slackAPI.markConversationRead(channelId, latestTimestamp)
            // Convert Slack timestamp to milliseconds and save locally
            var timestampMs = Math.floor(parseFloat(latestTimestamp) * 1000)
            console.log("[ConversationPage] markAsRead timestamp:", latestTimestamp, "->", timestampMs)
            conversationModel.markAsRead(channelId, timestampMs)
        }
    }

    Component.onDestruction: {
        // Clear active channel (for RTM unread tracking)
        slackAPI.activeChannelId = ""

        // Save draft when leaving the page
        saveDraft()
    }

    function refreshMessages() {
        slackAPI.fetchConversationHistory(channelId)
    }

    function addTypingUser(userId) {
        // Add user to typing list if not already there
        var users = typingUsers.slice()
        if (users.indexOf(userId) === -1) {
            users.push(userId)
            typingUsers = users
        }

        // Clear existing timer for this user
        if (typingTimers[userId]) {
            typingTimers[userId].stop()
            typingTimers[userId].destroy()
        }

        // Create new timer to remove user after 3 seconds
        var timer = Qt.createQmlObject('import QtQuick 2.0; Timer { interval: 3000; repeat: false }', conversationPage)
        timer.triggered.connect(function() {
            removeTypingUser(userId)
        })
        typingTimers[userId] = timer
        timer.start()
    }

    function removeTypingUser(userId) {
        var users = typingUsers.slice()
        var index = users.indexOf(userId)
        if (index > -1) {
            users.splice(index, 1)
            typingUsers = users
        }

        if (typingTimers[userId]) {
            typingTimers[userId].stop()
            typingTimers[userId].destroy()
            delete typingTimers[userId]
        }
    }

    function saveDraft() {
        var draftText = messageInput.text.trim()
        draftManager.saveDraft(channelId, draftText)
        if (draftText.length > 0) {
            console.log("Draft saved for channel:", channelId)
        }
    }

    function loadDraft() {
        var draftText = draftManager.getDraft(channelId)
        if (draftText.length > 0) {
            messageInput.text = draftText
            console.log("Draft loaded for channel:", channelId)
        }
    }

    function clearDraft() {
        draftManager.clearDraft(channelId)
        console.log("Draft cleared for channel:", channelId)
    }

    // @mention functions
    function checkForMention(text, cursorPos) {
        // Find the last @ before cursor
        var lastAtPos = -1
        for (var i = cursorPos - 1; i >= 0; i--) {
            var ch = text.charAt(i)
            if (ch === '@') {
                lastAtPos = i
                break
            }
            // Stop if we hit a space or newline (not in a mention)
            if (ch === ' ' || ch === '\n' || ch === '\t') {
                break
            }
        }

        if (lastAtPos >= 0) {
            var query = text.substring(lastAtPos + 1, cursorPos)
            // Only show suggestions if query doesn't contain spaces
            if (query.indexOf(' ') === -1) {
                mentionStartPos = lastAtPos
                mentionQuery = query
                mentionSuggestions = userModel.searchUsers(query, 8)
                showMentionSuggestions = mentionSuggestions.length > 0
                return
            }
        }

        // No valid mention
        showMentionSuggestions = false
        mentionQuery = ""
        mentionStartPos = -1
        mentionSuggestions = []
    }

    function insertMention(userId, userName) {
        if (mentionStartPos < 0) return

        var text = messageInput.text
        var beforeMention = text.substring(0, mentionStartPos)
        var afterMention = text.substring(mentionStartPos + mentionQuery.length + 1)

        // Display readable @username (will be converted to Slack format on send)
        var mention = "@" + userName + " "
        messageInput.text = beforeMention + mention + afterMention
        messageInput.cursorPosition = beforeMention.length + mention.length

        showMentionSuggestions = false
        mentionQuery = ""
        mentionStartPos = -1
        mentionSuggestions = []
    }

    // Convert @username mentions to Slack format <@USER_ID> before sending
    function convertMentionsToSlackFormat(text) {
        // Find all @mentions and replace with Slack format
        var result = text
        var mentionRegex = /@(\w+)/g
        var match

        // We need to process matches from end to start to preserve positions
        var matches = []
        while ((match = mentionRegex.exec(text)) !== null) {
            matches.push({index: match.index, username: match[1], fullMatch: match[0]})
        }

        // Process from end to start
        for (var i = matches.length - 1; i >= 0; i--) {
            var m = matches[i]
            var userId = userModel.getUserIdByName(m.username)
            if (userId && userId.length > 0) {
                var before = result.substring(0, m.index)
                var after = result.substring(m.index + m.fullMatch.length)
                result = before + "<@" + userId + ">" + after
            }
        }

        return result
    }

    SilicaListView {
        id: messageListView
        anchors.fill: parent
        anchors.bottomMargin: inputPanel.height

        model: messageModel
        verticalLayoutDirection: ListView.BottomToTop

        // Performance optimizations
        cacheBuffer: 400  // Pre-render items above/below viewport
        clip: true

        header: PageHeader {
            title: displayName
            description: qsTr("%n messages", "", messageListView.count)
        }

        delegate: MessageDelegate { }

        ViewPlaceholder {
            enabled: messageListView.count === 0
            text: qsTr("No messages yet")
            hintText: qsTr("Send a message to start the conversation")
        }

        VerticalScrollDecorator { }

        PullDownMenu {
            MenuItem {
                text: qsTr("Channel info")
                onClicked: {
                    pageStack.push(Qt.resolvedUrl("ChannelInfoPage.qml"), {
                        channelId: conversationPage.channelId,
                        channelName: conversationPage.displayName
                    })
                }
            }
            MenuItem {
                text: qsTr("Pins & Bookmarks")
                onClicked: {
                    pageStack.push(Qt.resolvedUrl("PinsAndBookmarksPage.qml"), {
                        channelId: conversationPage.channelId,
                        channelName: conversationPage.displayName
                    })
                }
            }
            MenuItem {
                text: qsTr("Search")
                onClicked: {
                    pageStack.push(Qt.resolvedUrl("SearchPage.qml"), {
                        "searchInChannel": displayName
                    })
                }
            }
        }
    } // End SilicaListView

    DockedPanel {
        id: inputPanel
        dock: Dock.Bottom
        width: parent.width
        height: messageInput.height + Theme.paddingLarge * 2 +
                (typingIndicator.visible ? typingIndicator.height : 0) +
                (mentionList.visible ? mentionList.height : 0)
        open: true

        Column {
            anchors.fill: parent
            anchors.margins: Theme.paddingMedium
            spacing: Theme.paddingSmall

            // @mention suggestions list
            ListView {
                id: mentionList
                width: parent.width
                height: visible ? Math.min(contentHeight, Theme.itemSizeSmall * 4) : 0
                visible: showMentionSuggestions && mentionSuggestions.length > 0
                clip: true
                model: mentionSuggestions

                delegate: BackgroundItem {
                    width: parent.width
                    height: Theme.itemSizeSmall

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.paddingMedium
                        anchors.rightMargin: Theme.paddingMedium
                        spacing: Theme.paddingMedium

                        // Avatar
                        Image {
                            width: Theme.iconSizeSmall
                            height: Theme.iconSizeSmall
                            anchors.verticalCenter: parent.verticalCenter
                            source: modelData.avatar || ""
                            fillMode: Image.PreserveAspectCrop
                            visible: status === Image.Ready

                            Rectangle {
                                anchors.fill: parent
                                color: "transparent"
                                border.color: Theme.rgba(Theme.highlightColor, 0.3)
                                border.width: 1
                                radius: width / 2
                            }
                        }

                        // Fallback avatar
                        Rectangle {
                            width: Theme.iconSizeSmall
                            height: Theme.iconSizeSmall
                            anchors.verticalCenter: parent.verticalCenter
                            radius: width / 2
                            color: Theme.rgba(Theme.highlightBackgroundColor, 0.3)
                            visible: !modelData.avatar || modelData.avatar.length === 0

                            Label {
                                anchors.centerIn: parent
                                text: (modelData.displayName || modelData.name || "?").charAt(0).toUpperCase()
                                font.pixelSize: Theme.fontSizeExtraSmall
                                color: Theme.highlightColor
                            }
                        }

                        Column {
                            anchors.verticalCenter: parent.verticalCenter

                            Label {
                                text: modelData.displayName || modelData.name
                                font.pixelSize: Theme.fontSizeSmall
                                color: highlighted ? Theme.highlightColor : Theme.primaryColor
                            }

                            Label {
                                text: "@" + modelData.name
                                font.pixelSize: Theme.fontSizeExtraSmall
                                color: Theme.secondaryColor
                                visible: modelData.displayName && modelData.displayName !== modelData.name
                            }
                        }
                    }

                    onClicked: {
                        insertMention(modelData.id, modelData.name)
                    }
                }
            }

            // Typing indicator
            Label {
                id: typingIndicator
                width: parent.width
                visible: typingUsers.length > 0
                text: {
                    if (typingUsers.length === 0) return ""
                    if (typingUsers.length === 1) {
                        var userName = userModel.getUserName(typingUsers[0])
                        return userName + " " + qsTr("is typing...")
                    } else if (typingUsers.length === 2) {
                        var user1 = userModel.getUserName(typingUsers[0])
                        var user2 = userModel.getUserName(typingUsers[1])
                        return user1 + " " + qsTr("and") + " " + user2 + " " + qsTr("are typing...")
                    } else {
                        return qsTr("Several people are typing...")
                    }
                }
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryHighlightColor
                truncationMode: TruncationMode.Fade
            }

        Row {
            width: parent.width
            spacing: Theme.paddingSmall

            IconButton {
                id: attachButton
                anchors.verticalCenter: parent.verticalCenter
                icon.source: "image://theme/icon-m-attach"

                onClicked: {
                    pageStack.push(filePickerPage)
                }
            }

            TextArea {
                id: messageInput
                width: parent.width - sendButton.width - attachButton.width - parent.spacing * 2
                placeholderText: qsTr("Type a message...")
                label: qsTr("Message")

                EnterKey.iconSource: "image://theme/icon-m-enter-next"
                EnterKey.onClicked: sendButton.clicked()

                onTextChanged: {
                    // Check for @mention
                    checkForMention(text, cursorPosition)
                }

                onCursorPositionChanged: {
                    // Re-check when cursor moves
                    if (text.length > 0) {
                        checkForMention(text, cursorPosition)
                    }
                }
            }

            Item {
                id: sendButtonContainer
                width: Theme.iconSizeMedium
                height: Theme.iconSizeMedium
                anchors.verticalCenter: parent.verticalCenter

                IconButton {
                    id: sendButton
                    anchors.fill: parent
                    icon.source: "image://theme/icon-m-message"
                    enabled: messageInput.text.length > 0 && !isSendingMessage
                    visible: !isSendingMessage

                    onClicked: {
                        if (messageInput.text.trim().length > 0) {
                            isSendingMessage = true
                            var messageText = convertMentionsToSlackFormat(messageInput.text)
                            messageInput.text = ""
                            clearDraft()  // Clear saved draft after sending
                            slackAPI.sendMessage(channelId, messageText)

                            // Refresh messages after a short delay to show the new message
                            refreshTimer.start()
                        }
                    }
                }

                BusyIndicator {
                    anchors.centerIn: parent
                    size: BusyIndicatorSize.Small
                    running: isSendingMessage
                    visible: isSendingMessage
                }
            }
        }
        }
    }

    Component {
        id: filePickerPage
        Page {
            id: picker

            SilicaFlickable {
                anchors.fill: parent
                contentHeight: column.height

                Column {
                    id: column
                    width: parent.width

                    PageHeader {
                        title: qsTr("Upload File")
                    }

                    Button {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: qsTr("Choose Image")
                        onClicked: {
                            imagePickerDialog.open()
                        }
                    }

                    Button {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: qsTr("Choose File")
                        onClicked: {
                            filePickerDialog.open()
                        }
                    }
                }
            }

            // Image picker dialog
            Loader {
                id: imagePickerDialog
                function open() {
                    active = true
                    if (item) item.open()
                }

                active: false
                sourceComponent: Component {
                    Dialog {
                        property string selectedFile: ""

                        canAccept: selectedFile.length > 0

                        SilicaListView {
                            anchors.fill: parent
                            header: DialogHeader {
                                title: qsTr("Select Image")
                            }

                            model: Qt.application.arguments // Placeholder
                            delegate: ListItem {
                                Label {
                                    text: qsTr("Image picker not yet implemented")
                                }
                            }
                        }

                        onAccepted: {
                            if (selectedFile.length > 0) {
                                fileManager.uploadImage(channelId, selectedFile)
                                pageStack.pop()
                            }
                        }
                    }
                }
            }

            // File picker dialog (similar structure)
            Loader {
                id: filePickerDialog
                function open() {
                    active = true
                    if (item) item.open()
                }

                active: false
                sourceComponent: Component {
                    Dialog {
                        property string selectedFile: ""

                        canAccept: selectedFile.length > 0

                        SilicaListView {
                            anchors.fill: parent
                            header: DialogHeader {
                                title: qsTr("Select File")
                            }

                            model: Qt.application.arguments // Placeholder
                            delegate: ListItem {
                                Label {
                                    text: qsTr("File picker not yet implemented")
                                }
                            }
                        }

                        onAccepted: {
                            if (selectedFile.length > 0) {
                                fileManager.uploadFile(channelId, selectedFile)
                                pageStack.pop()
                            }
                        }
                    }
                }
            }
        }
    }

    // Timer to refresh messages after sending
    Timer {
        id: refreshTimer
        interval: 1000  // 1 second delay
        repeat: false
        onTriggered: {
            refreshMessages()
            isSendingMessage = false
        }
    }

    Connections {
        target: slackAPI

        onMessageReceived: {
            if (message.channel === channelId) {
                messageModel.addMessage(message)
            }
        }

        onMessageUpdated: {
            // Update message in the model (e.g., after reaction changes)
            messageModel.updateMessage(message)
        }

        // Handle API errors (like failed message send)
        onApiError: {
            if (isSendingMessage) {
                isSendingMessage = false
                refreshTimer.stop()
            }
        }

        // Handle typing indicators
        onUserTyping: {
            if (typingChannelId === channelId) {
                addTypingUser(typingUserId)
            }
        }
    }
}
