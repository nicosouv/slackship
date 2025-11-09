import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: statsPage

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        PullDownMenu {
            MenuItem {
                text: qsTr("Reset Stats")
                onClicked: {
                    remorse.execute(qsTr("Resetting statistics"), function() {
                        statsManager.resetStats()
                    })
                }
            }
        }

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: qsTr("Daily Statistics")
                description: slackAPI.workspaceName || qsTr("No workspace")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: qsTr("Stats are tracked per workspace and reset daily at midnight")
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryHighlightColor
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }

            Item {
                width: parent.width
                height: Theme.paddingLarge * 2
            }

            // Today's workspace total
            Column {
                width: parent.width
                spacing: Theme.paddingSmall

                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: statsManager.todayTotal
                    font.pixelSize: Theme.fontSizeHuge * 2
                    font.bold: true
                    color: Theme.highlightColor
                }

                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("Messages in workspace today")
                    font.pixelSize: Theme.fontSizeMedium
                    color: Theme.secondaryColor
                }
            }

            Item {
                width: parent.width
                height: Theme.paddingLarge * 2
            }

            // Today's user messages
            Column {
                width: parent.width
                spacing: Theme.paddingSmall

                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: statsManager.todayUser
                    font.pixelSize: Theme.fontSizeHuge * 2
                    font.bold: true
                    color: Theme.primaryColor
                }

                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("Your messages today")
                    font.pixelSize: Theme.fontSizeMedium
                    color: Theme.secondaryColor
                }
            }

            Item {
                width: parent.width
                height: Theme.paddingLarge * 3
            }

            // Info text
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: qsTr("These counters track all messages sent in your current workspace today. They reset automatically at midnight.")
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }
        }

        VerticalScrollDecorator { }
    }

    RemorsePopup {
        id: remorse
    }
}
