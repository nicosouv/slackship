import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

Page {
    id: conversationPage

    property string channelId
    property string channelName
    property bool isSendingMessage: false

    Component.onCompleted: {
        console.log("ConversationPage loaded")
        console.log("Channel:", channelName, channelId)
        console.log("Message count:", messageModel.rowCount())
    }

    function refreshMessages() {
        slackAPI.fetchConversationHistory(channelId)
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
                    // TODO: Show channel info
                }
            }
            MenuItem {
                text: qsTr("Search")
                onClicked: {
                    // TODO: Implement search in conversation
                }
            }
        }
    } // End SilicaListView

    DockedPanel {
        id: inputPanel
        dock: Dock.Bottom
        width: parent.width
        height: messageInput.height + Theme.paddingLarge * 2
        open: true

        Row {
            anchors.fill: parent
            anchors.margins: Theme.paddingMedium
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
    }
}
