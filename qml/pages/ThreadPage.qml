import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

Page {
    id: threadPage

    property string channelId
    property string channelName
    property string threadTs  // Thread timestamp (parent message)
    property var parentMessage  // The parent message object
    property bool isSendingReply: false

    Component.onCompleted: {
        console.log("ThreadPage loaded")
        console.log("Channel:", channelName, channelId)
        console.log("Thread:", threadTs)

        // Fetch thread replies
        if (threadTs && channelId) {
            slackAPI.fetchThreadReplies(channelId, threadTs)
        }
    }

    function refreshThread() {
        if (threadTs && channelId) {
            slackAPI.fetchThreadReplies(channelId, threadTs)
        }
    }

    SilicaListView {
        id: threadListView
        anchors.fill: parent
        anchors.bottomMargin: replyPanel.height
        verticalLayoutDirection: ListView.BottomToTop

        header: PageHeader {
            title: qsTr("Thread in #%1").arg(channelName)
        }

        model: ListModel {
            id: threadModel
        }

        delegate: MessageDelegate { }

        ViewPlaceholder {
            enabled: threadListView.count === 0
            text: qsTr("No replies yet")
            hintText: qsTr("Be the first to reply")
        }

        VerticalScrollDecorator { }
    }

    DockedPanel {
        id: replyPanel
        dock: Dock.Bottom
        width: parent.width
        height: replyInput.height + Theme.paddingLarge * 2
        open: true

        Row {
            anchors.fill: parent
            anchors.margins: Theme.paddingMedium
            spacing: Theme.paddingSmall

            TextArea {
                id: replyInput
                width: parent.width - sendReplyButton.width - parent.spacing
                placeholderText: qsTr("Reply to thread...")
                label: qsTr("Reply")

                EnterKey.iconSource: "image://theme/icon-m-enter-next"
                EnterKey.onClicked: sendReplyButton.clicked()
            }

            Item {
                id: sendReplyButton
                width: Theme.iconSizeMedium
                height: Theme.iconSizeMedium
                anchors.verticalCenter: parent.verticalCenter

                IconButton {
                    anchors.fill: parent
                    icon.source: "image://theme/icon-m-message"
                    enabled: replyInput.text.length > 0 && !isSendingReply
                    visible: !isSendingReply

                    onClicked: {
                        if (replyInput.text.trim().length > 0) {
                            isSendingReply = true
                            var replyText = replyInput.text
                            replyInput.text = ""
                            slackAPI.sendThreadReply(channelId, threadTs, replyText)

                            // Refresh after a short delay
                            refreshTimer.start()
                        }
                    }
                }

                BusyIndicator {
                    anchors.centerIn: parent
                    size: BusyIndicatorSize.Small
                    running: isSendingReply
                    visible: isSendingReply
                }
            }
        }
    }

    Timer {
        id: refreshTimer
        interval: 1000
        repeat: false
        onTriggered: {
            refreshThread()
            isSendingReply = false
        }
    }

    Connections {
        target: slackAPI

        onThreadRepliesReceived: {
            console.log("Thread replies received:", replies.length)
            threadModel.clear()

            // Slack's conversations.replies includes the parent message as the first item
            // So we use the API response directly instead of the parentMessage prop
            for (var i = 0; i < replies.length; i++) {
                var msg = replies[i]
                var isParent = (i === 0)  // First message is the parent

                threadModel.append({
                    "id": msg.client_msg_id || "",
                    "text": msg.text || "",
                    "userId": msg.user || "",
                    "userName": "",
                    "timestamp": msg.ts || "",
                    "threadTs": msg.thread_ts || "",
                    "threadCount": 0,
                    "reactions": msg.reactions || [],
                    "attachments": msg.attachments || [],
                    "isEdited": msg.edited !== undefined,
                    "isOwnMessage": false,
                    "isParent": isParent  // Mark parent for visual differentiation
                })
            }
        }

        onApiError: {
            if (isSendingReply) {
                isSendingReply = false
                refreshTimer.stop()
            }
        }
    }
}
