import QtQuick 2.0
import Sailfish.Silica 1.0
import QtGraphicalEffects 1.0
import "EmojiHelper.js" as EmojiHelper

ListItem {
    id: messageItem
    contentHeight: messageColumn.height + Theme.paddingMedium * 2

    // Highlight parent message in threads
    Rectangle {
        anchors.fill: parent
        color: Theme.rgba(Theme.highlightBackgroundColor, 0.1)
        visible: model.isParent !== undefined && model.isParent
        radius: Theme.paddingSmall
    }

    Component {
        id: imageAttachmentComponent
        ImageAttachment {
            imageUrl: attachmentData.image_url || ""
            thumbUrl: attachmentData.thumb_url || ""
            imageWidth: attachmentData.image_width || 0
            imageHeight: attachmentData.image_height || 0
            title: attachmentData.title || ""
        }
    }

    Component {
        id: fileAttachmentComponent
        FileAttachment {
            fileId: attachmentData.id || ""
            fileName: attachmentData.name || attachmentData.title || ""
            fileType: attachmentData.mimetype || ""
            fileSize: attachmentData.size || 0
            downloadUrl: attachmentData.url_private || ""
        }
    }

    // Thread tree visualization
    Item {
        anchors.fill: parent
        visible: model.isParent !== undefined && !model.isParent

        // Vertical line connecting to parent
        Rectangle {
            x: Theme.horizontalPageMargin + Theme.paddingLarge - 1
            y: 0
            width: 2
            height: parent.height / 2 + Theme.paddingMedium
            color: Theme.rgba(Theme.highlightColor, 0.3)
        }

        // Horizontal branch line
        Rectangle {
            x: Theme.horizontalPageMargin + Theme.paddingLarge - 1
            y: parent.height / 2 + Theme.paddingMedium
            width: Theme.paddingLarge
            height: 2
            color: Theme.rgba(Theme.highlightColor, 0.3)
        }
    }

    Row {
        anchors.fill: parent
        anchors.margins: Theme.paddingMedium
        // Add left margin for thread replies (not parent messages)
        anchors.leftMargin: (model.isParent !== undefined && !model.isParent) ? Theme.paddingLarge * 2 : Theme.paddingMedium
        spacing: Theme.paddingMedium

        // User avatar (clickable)
        BackgroundItem {
            id: avatarContainer
            width: Theme.iconSizeMedium
            height: Theme.iconSizeMedium

            onClicked: {
                pageStack.push(Qt.resolvedUrl("../pages/UserProfilePage.qml"), {
                    userId: model.userId
                })
            }

            Image {
                id: avatarImage
                anchors.fill: parent
                source: userModel.getUserAvatar(model.userId) || ""
                fillMode: Image.PreserveAspectCrop
                layer.enabled: true
                layer.effect: OpacityMask {
                    maskSource: Rectangle {
                        width: avatarImage.width
                        height: avatarImage.height
                        radius: width / 2
                    }
                }
                visible: status === Image.Ready
            }

            // Fallback placeholder if image fails to load
            Rectangle {
                id: avatarPlaceholder
                anchors.fill: parent
                radius: width / 2
                color: Theme.rgba(Theme.highlightBackgroundColor, 0.2)
                visible: avatarImage.status !== Image.Ready

                Label {
                    anchors.centerIn: parent
                    text: userModel.getUserName(model.userId).charAt(0).toUpperCase()
                    font.pixelSize: Theme.fontSizeMedium
                    color: Theme.primaryColor
                }
            }
        }

        Column {
            id: messageColumn
            width: parent.width - avatarContainer.width - parent.spacing * 2
            spacing: Theme.paddingSmall

            Row {
                width: parent.width
                spacing: Theme.paddingMedium

                BackgroundItem {
                    width: userNameLabel.width
                    height: userNameLabel.height

                    onClicked: {
                        pageStack.push(Qt.resolvedUrl("../pages/UserProfilePage.qml"), {
                            userId: model.userId
                        })
                    }

                    Label {
                        id: userNameLabel
                        text: userModel.getUserName(model.userId) || model.userId
                        font.bold: true
                        color: Theme.highlightColor
                        font.pixelSize: Theme.fontSizeSmall
                    }
                }

                // "Thread starter" badge for parent message
                Rectangle {
                    visible: model.isParent !== undefined && model.isParent
                    height: Theme.fontSizeExtraSmall + Theme.paddingSmall
                    width: threadStarterLabel.width + Theme.paddingMedium
                    radius: Theme.paddingSmall
                    color: Theme.rgba(Theme.highlightColor, 0.2)
                    border.color: Theme.rgba(Theme.highlightColor, 0.4)
                    border.width: 1

                    Label {
                        id: threadStarterLabel
                        anchors.centerIn: parent
                        text: "🧵 " + qsTr("Thread")
                        font.pixelSize: Theme.fontSizeExtraSmall
                        color: Theme.highlightColor
                    }
                }

                Label {
                    text: {
                        var msgDate = new Date(parseFloat(model.timestamp) * 1000)
                        var today = new Date()
                        var yesterday = new Date(today)
                        yesterday.setDate(yesterday.getDate() - 1)

                        // Check if message is from today
                        if (msgDate.toDateString() === today.toDateString()) {
                            return Qt.formatDateTime(msgDate, "hh:mm")
                        }
                        // Check if message is from yesterday
                        else if (msgDate.toDateString() === yesterday.toDateString()) {
                            return qsTr("Yesterday") + " " + Qt.formatDateTime(msgDate, "hh:mm")
                        }
                        // Check if message is from this year
                        else if (msgDate.getFullYear() === today.getFullYear()) {
                            return Qt.formatDateTime(msgDate, "MMM dd, hh:mm")
                        }
                        // Message is from a previous year
                        else {
                            return Qt.formatDateTime(msgDate, "MMM dd yyyy, hh:mm")
                        }
                    }
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryColor
                }

                Label {
                    text: qsTr("edited")
                    visible: model.isEdited
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryHighlightColor
                }
            }

            Label {
                width: parent.width
                text: EmojiHelper.convertEmoji(model.text)
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.primaryColor
                textFormat: Text.PlainText
            }

            // Reactions (emojis)
            Flow {
                width: parent.width
                spacing: Theme.paddingSmall
                visible: model.reactions && model.reactions.length > 0

                Repeater {
                    model: messageItem.model.reactions || []

                    delegate: BackgroundItem {
                        width: reactionRow.width + Theme.paddingMedium * 2
                        height: Theme.itemSizeExtraSmall

                        // Check if current user has reacted
                        property bool userHasReacted: {
                            var currentUserId = slackAPI.currentUserId()
                            var users = modelData.users || []
                            for (var i = 0; i < users.length; i++) {
                                if (users[i] === currentUserId) {
                                    return true
                                }
                            }
                            return false
                        }

                        Rectangle {
                            anchors.fill: parent
                            radius: Theme.paddingSmall
                            // Highlight if user has reacted
                            color: userHasReacted ? Theme.rgba(Theme.highlightBackgroundColor, 0.4) : Theme.rgba(Theme.highlightBackgroundColor, 0.2)
                            border.color: userHasReacted ? Theme.highlightColor : Theme.rgba(Theme.highlightColor, 0.3)
                            border.width: userHasReacted ? 2 : 1
                        }

                        Row {
                            id: reactionRow
                            anchors.centerIn: parent
                            spacing: Theme.paddingSmall

                            Label {
                                text: EmojiHelper.reactionToEmoji(modelData.name)
                                font.pixelSize: Theme.fontSizeSmall
                                color: Theme.primaryColor
                            }

                            Label {
                                text: modelData.count || 1
                                font.pixelSize: Theme.fontSizeExtraSmall
                                font.bold: true
                                color: Theme.highlightColor
                            }
                        }

                        onClicked: {
                            // Check if current user has reacted with this emoji
                            var currentUserId = slackAPI.currentUserId()
                            var users = modelData.users || []
                            var hasReacted = false

                            for (var i = 0; i < users.length; i++) {
                                if (users[i] === currentUserId) {
                                    hasReacted = true
                                    break
                                }
                            }

                            // Toggle: remove if already reacted, add if not
                            if (hasReacted) {
                                slackAPI.removeReaction(model.channelId, model.timestamp, modelData.name)
                            } else {
                                slackAPI.addReaction(model.channelId, model.timestamp, modelData.name)
                            }
                        }
                    }
                }
            }

            // Image attachments
            Repeater {
                model: messageItem.model.attachments || []

                delegate: Loader {
                    width: messageColumn.width
                    sourceComponent: {
                        var attachment = modelData
                        if (attachment.image_url || attachment.thumb_url) {
                            return imageAttachmentComponent
                        } else if (attachment.url) {
                            return fileAttachmentComponent
                        }
                        return null
                    }

                    property var attachmentData: modelData
                }
            }

            // Files (uploaded images, documents, etc.)
            Repeater {
                model: messageItem.model.files || []

                delegate: Loader {
                    width: messageColumn.width
                    sourceComponent: {
                        var file = modelData
                        // Check if it's an image file
                        if (file.mimetype && file.mimetype.startsWith("image/")) {
                            return imageAttachmentComponent
                        } else {
                            return fileAttachmentComponent
                        }
                    }

                    property var attachmentData: {
                        var file = modelData
                        // Map file object to attachment-like structure for components
                        return {
                            "image_url": file.url_private || file.permalink,
                            "thumb_url": file.thumb_360 || file.thumb_480 || file.thumb_160 || file.thumb_80,
                            "image_width": file.original_w || 0,
                            "image_height": file.original_h || 0,
                            "title": file.title || file.name || "",
                            "id": file.id || "",
                            "name": file.name || file.title || "",
                            "mimetype": file.mimetype || "",
                            "size": file.size || 0,
                            "url_private": file.url_private || ""
                        }
                    }
                }
            }

            // Thread indicator
            BackgroundItem {
                width: parent.width
                height: Theme.itemSizeExtraSmall
                visible: model.threadCount > 0

                Row {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: Theme.paddingSmall

                    Icon {
                        source: "image://theme/icon-s-chat"
                        width: Theme.iconSizeExtraSmall
                        height: Theme.iconSizeExtraSmall
                        color: Theme.highlightColor
                    }

                    Label {
                        text: qsTr("%n replies", "", model.threadCount)
                        color: Theme.highlightColor
                        font.pixelSize: Theme.fontSizeExtraSmall
                    }
                }

                onClicked: {
                    // Get the full message object to pass to ThreadPage
                    var messageObj = {
                        "client_msg_id": model.id,
                        "text": model.text,
                        "user": model.userId,
                        "ts": model.timestamp,
                        "thread_ts": model.threadTs || model.timestamp,
                        "reactions": model.reactions,
                        "attachments": model.attachments,
                        "edited": model.isEdited ? {} : undefined
                    }

                    pageStack.push(Qt.resolvedUrl("../pages/ThreadPage.qml"), {
                        "channelId": conversationPage.channelId,
                        "channelName": conversationPage.channelName,
                        "threadTs": model.threadTs || model.timestamp,
                        "parentMessage": messageObj
                    })
                }
            }
        }
    }

    menu: ContextMenu {
        MenuItem {
            text: qsTr("Reply in thread")
            onClicked: {
                // Get the full message object to pass to ThreadPage
                var messageObj = {
                    "client_msg_id": model.id,
                    "text": model.text,
                    "user": model.userId,
                    "ts": model.timestamp,
                    "thread_ts": model.threadTs || model.timestamp,
                    "reactions": model.reactions,
                    "attachments": model.attachments,
                    "edited": model.isEdited ? {} : undefined
                }

                pageStack.push(Qt.resolvedUrl("../pages/ThreadPage.qml"), {
                    "channelId": conversationPage.channelId,
                    "channelName": conversationPage.channelName,
                    "threadTs": model.threadTs || model.timestamp,
                    "parentMessage": messageObj
                })
            }
        }

        MenuItem {
            text: qsTr("Add reaction")
            onClicked: {
                var dialog = pageStack.push(Qt.resolvedUrl("../dialogs/EmojiPicker.qml"))
                dialog.accepted.connect(function() {
                    // Convert Unicode emoji to Slack reaction name
                    var reactionName = EmojiHelper.emojiToReactionName(dialog.selectedEmoji)
                    slackAPI.addReaction(model.channelId, model.timestamp, reactionName)
                })
            }
        }

        MenuItem {
            text: qsTr("Copy text")
            onClicked: {
                Clipboard.text = model.text
            }
        }

        MenuItem {
            text: qsTr("Edit")
            visible: model.isOwnMessage
            onClicked: {
                var dialog = pageStack.push(Qt.resolvedUrl("../dialogs/EditMessageDialog.qml"), {
                    originalText: model.text
                })
                dialog.accepted.connect(function() {
                    slackAPI.updateMessage(model.channelId, model.timestamp, dialog.editedText)
                })
            }
        }

        MenuItem {
            text: qsTr("Delete")
            visible: model.isOwnMessage
            onClicked: {
                remorseAction(qsTr("Deleting"), function() {
                    slackAPI.deleteMessage(model.channelId, model.timestamp)
                })
            }
        }
    }
}
