import QtQuick 2.0
import Sailfish.Silica 1.0
import Qt.labs.settings 1.0
import "../components"

Page {
    id: conversationPage

    property string channelId
    property string channelName
    property bool isSendingMessage: false
    property var typingUsers: []
    property var typingTimers: ({})

    // Draft management with Qt.labs.settings
    Settings {
        id: draftSettings
        category: "MessageDrafts"
    }

    Component.onCompleted: {
        console.log("ConversationPage loaded")
        console.log("Channel:", channelName, channelId)
        console.log("Message count:", messageModel.rowCount())

        // Restore draft for this channel
        loadDraft()
    }

    Component.onDestruction: {
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
        if (draftText.length > 0) {
            // Save non-empty draft
            draftSettings.setValue("draft_" + channelId, draftText)
            console.log("Draft saved for channel:", channelId)
        } else {
            // Remove draft if empty
            draftSettings.remove("draft_" + channelId)
        }
    }

    function loadDraft() {
        var draftText = draftSettings.value("draft_" + channelId, "")
        if (draftText.length > 0) {
            messageInput.text = draftText
            console.log("Draft loaded for channel:", channelId)
        }
    }

    function clearDraft() {
        draftSettings.remove("draft_" + channelId)
        console.log("Draft cleared for channel:", channelId)
    }

    SilicaListView {
        id: messageListView
        anchors.fill: parent
        anchors.bottomMargin: inputPanel.height

        model: messageModel
        verticalLayoutDirection: ListView.BottomToTop

        header: PageHeader {
            title: channelName
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
                        channelName: conversationPage.channelName
                    })
                }
            }
            MenuItem {
                text: qsTr("Search")
                onClicked: {
                    pageStack.push(Qt.resolvedUrl("SearchPage.qml"), {
                        "searchInChannel": channelName
                    })
                }
            }
        }
    } // End SilicaListView

    DockedPanel {
        id: inputPanel
        dock: Dock.Bottom
        width: parent.width
        height: messageInput.height + Theme.paddingLarge * 2 + (typingIndicator.visible ? typingIndicator.height : 0)
        open: true

        Column {
            anchors.fill: parent
            anchors.margins: Theme.paddingMedium
            spacing: Theme.paddingSmall

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
                            var messageText = messageInput.text
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
