import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: settingsPage

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: qsTr("Settings")
            }

            SectionHeader {
                text: qsTr("Notifications")
            }

            TextSwitch {
                text: qsTr("Enable notifications")
                description: qsTr("Show notifications for new messages")
                checked: appSettings.notificationsEnabled
                onCheckedChanged: appSettings.notificationsEnabled = checked
            }

            TextSwitch {
                text: qsTr("Sound")
                description: qsTr("Play sound for notifications")
                checked: appSettings.soundEnabled
                enabled: appSettings.notificationsEnabled
                onCheckedChanged: appSettings.soundEnabled = checked
            }

            SectionHeader {
                text: qsTr("Account")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: qsTr("Workspace: %1").arg(slackAPI.workspaceName || qsTr("Not connected"))
                color: Theme.secondaryHighlightColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.WordWrap
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Logout")
                onClicked: {
                    remorse.execute(qsTr("Logging out"), function() {
                        slackAPI.logout()
                        pageStack.replace(Qt.resolvedUrl("LoginPage.qml"))
                    })
                }
            }

            SectionHeader {
                text: qsTr("About")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Lagoon v0.3.0"
                color: Theme.highlightColor
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: qsTr("A native Slack client for Sailfish OS")
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.WordWrap
            }
        }

        VerticalScrollDecorator { }
    }

    RemorsePopup {
        id: remorse
    }
}
