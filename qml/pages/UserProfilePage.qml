import QtQuick 2.0
import Sailfish.Silica 1.0
import QtGraphicalEffects 1.0
import "../components/EmojiHelper.js" as EmojiHelper

Page {
    id: userProfilePage

    property string userId: ""
    property var userDetails: ({})

    Component.onCompleted: {
        userDetails = userModel.getUserDetails(userId)
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge * 2

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: qsTr("User Profile")
            }

            // User avatar (large)
            Item {
                width: parent.width
                height: Theme.itemSizeHuge + Theme.paddingLarge * 2

                Image {
                    id: avatarImage
                    anchors.centerIn: parent
                    width: Theme.itemSizeHuge
                    height: Theme.itemSizeHuge
                    source: userDetails.avatar || ""
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
                    anchors.centerIn: parent
                    width: Theme.itemSizeHuge
                    height: Theme.itemSizeHuge
                    radius: width / 2
                    color: Theme.rgba(Theme.highlightBackgroundColor, 0.2)
                    visible: avatarImage.status !== Image.Ready

                    Label {
                        anchors.centerIn: parent
                        text: (userDetails.displayName || userDetails.realName || userDetails.name || "?").charAt(0).toUpperCase()
                        font.pixelSize: Theme.fontSizeHuge * 2
                        color: Theme.primaryColor
                    }
                }

                // Online indicator
                Rectangle {
                    anchors.right: avatarImage.right
                    anchors.bottom: avatarImage.bottom
                    anchors.margins: Theme.paddingSmall
                    width: Theme.iconSizeSmall
                    height: Theme.iconSizeSmall
                    radius: width / 2
                    color: userDetails.isOnline ? Theme.rgba("#2AB27B", 1.0) : Theme.rgba(Theme.secondaryColor, 0.5)
                    border.width: 2
                    border.color: Theme.colorScheme === Theme.LightOnDark ? "black" : "white"
                }
            }

            // Display name
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: userDetails.displayName || userDetails.realName || userDetails.name || userId
                font.pixelSize: Theme.fontSizeLarge
                font.bold: true
                color: Theme.highlightColor
            }

            // Username (@name)
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "@" + (userDetails.name || userId)
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryColor
                visible: userDetails.name && userDetails.name.length > 0
            }

            // Status
            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: Theme.paddingSmall
                visible: userDetails.statusText && userDetails.statusText.length > 0

                Label {
                    text: EmojiHelper.convertEmoji(userDetails.statusEmoji || "")
                    font.pixelSize: Theme.fontSizeSmall
                    visible: userDetails.statusEmoji && userDetails.statusEmoji.length > 0
                }

                Label {
                    text: userDetails.statusText || ""
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.secondaryHighlightColor
                }
            }

            SectionHeader {
                text: qsTr("Details")
            }

            DetailItem {
                label: qsTr("Status")
                value: userDetails.isOnline ? qsTr("Active") : qsTr("Away")
                visible: userDetails.isOnline !== undefined
            }

            DetailItem {
                label: qsTr("Full name")
                value: userDetails.realName || qsTr("Not set")
                visible: userDetails.realName && userDetails.realName.length > 0
            }

            DetailItem {
                label: qsTr("Display name")
                value: userDetails.displayName || qsTr("Not set")
                visible: userDetails.displayName && userDetails.displayName.length > 0
            }

            DetailItem {
                label: qsTr("Account type")
                value: userDetails.isBot ? qsTr("Bot") : qsTr("User")
                visible: userDetails.isBot !== undefined
            }

            DetailItem {
                label: qsTr("User ID")
                value: userDetails.id || userId
            }

            // Action buttons
            Column {
                width: parent.width
                spacing: Theme.paddingMedium

                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("Send Message")
                    onClicked: {
                        // TODO: Open DM with this user
                        console.log("Send message to:", userId)
                    }
                }
            }
        }

        VerticalScrollDecorator { }
    }
}
