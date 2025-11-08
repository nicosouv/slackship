import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: settingsPage

    property string previousLanguage: appSettings.language || "en"

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
                text: qsTr("Language")
            }

            ComboBox {
                id: languageComboBox
                label: qsTr("Application language")
                width: parent.width

                currentIndex: {
                    var lang = appSettings.language || ""
                    if (lang === "" || lang.startsWith("en")) return 0  // English (default)
                    if (lang.startsWith("fr")) return 1  // French
                    if (lang.startsWith("fi")) return 2  // Finnish
                    if (lang.startsWith("it")) return 3  // Italian
                    if (lang.startsWith("es")) return 4  // Spanish
                    return 0  // Default to English
                }

                menu: ContextMenu {
                    MenuItem { text: "English" }
                    MenuItem { text: "Français" }
                    MenuItem { text: "Suomi" }
                    MenuItem { text: "Italiano" }
                    MenuItem { text: "Español" }
                }

                onCurrentIndexChanged: {
                    var langCodes = ["en", "fr", "fi", "it", "es"]
                    var newLang = langCodes[currentIndex]

                    // Only show dialog if language actually changed
                    if (newLang !== previousLanguage) {
                        appSettings.language = newLang
                        restartDialog.open()
                    }
                }
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

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: qsTr("Total workspaces: %1").arg(workspaceManager.workspaceCount())
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.WordWrap
                visible: workspaceManager.workspaceCount() > 1
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Switch Workspace")
                visible: workspaceManager.workspaceCount() > 1
                onClicked: pageStack.push(Qt.resolvedUrl("WorkspaceSwitcher.qml"))
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Add Workspace")
                onClicked: pageStack.push(Qt.resolvedUrl("LoginPage.qml"))
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
                text: qsTr("Insights")
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Reset Statistics")
                onClicked: {
                    remorse.execute(qsTr("Resetting all statistics"), function() {
                        statsManager.resetStats()
                    })
                }
            }

            // Stats preview
            BackgroundItem {
                width: parent.width
                height: statsPreview.height + Theme.paddingLarge * 2
                onClicked: pageStack.push(Qt.resolvedUrl("StatsPage.qml"))

                Column {
                    id: statsPreview
                    anchors.centerIn: parent
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    spacing: Theme.paddingMedium

                    Row {
                        width: parent.width
                        spacing: Theme.paddingLarge

                        Column {
                            width: (parent.width - parent.spacing * 2) / 3
                            Label {
                                width: parent.width
                                text: statsManager.totalMessages
                                font.pixelSize: Theme.fontSizeHuge
                                font.bold: true
                                color: Theme.highlightColor
                                horizontalAlignment: Text.AlignHCenter
                            }
                            Label {
                                width: parent.width
                                text: qsTr("Messages")
                                font.pixelSize: Theme.fontSizeExtraSmall
                                color: Theme.secondaryColor
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }

                        Column {
                            width: (parent.width - parent.spacing * 2) / 3
                            Label {
                                width: parent.width
                                text: statsManager.messagesThisWeek
                                font.pixelSize: Theme.fontSizeHuge
                                font.bold: true
                                color: Theme.highlightColor
                                horizontalAlignment: Text.AlignHCenter
                            }
                            Label {
                                width: parent.width
                                text: qsTr("This Week")
                                font.pixelSize: Theme.fontSizeExtraSmall
                                color: Theme.secondaryColor
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }

                        Column {
                            width: (parent.width - parent.spacing * 2) / 3
                            Label {
                                width: parent.width
                                text: "🔥 " + statsManager.currentStreak
                                font.pixelSize: Theme.fontSizeHuge
                                font.bold: true
                                color: Theme.highlightColor
                                horizontalAlignment: Text.AlignHCenter
                            }
                            Label {
                                width: parent.width
                                text: qsTr("Streak")
                                font.pixelSize: Theme.fontSizeExtraSmall
                                color: Theme.secondaryColor
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }
                    }

                    Label {
                        width: parent.width
                        text: qsTr("Tap to view detailed statistics →")
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.highlightColor
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }

            SectionHeader {
                text: qsTr("About")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Lagoon v0.15.0"
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

    Dialog {
        id: restartDialog

        DialogHeader {
            title: qsTr("Restart Required")
            acceptText: qsTr("Restart")
            cancelText: qsTr("Cancel")
        }

        Label {
            anchors.centerIn: parent
            width: parent.width - 2 * Theme.horizontalPageMargin
            text: qsTr("The application needs to restart to apply the new language. Restart now?")
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }

        onAccepted: {
            Qt.quit()  // Force app restart
        }

        onRejected: {
            // Revert to previous language
            appSettings.language = previousLanguage
            // Reset ComboBox index (need to recalculate based on previousLanguage)
            var langCodes = ["en", "fr", "fi", "it", "es"]
            for (var i = 0; i < langCodes.length; i++) {
                if (previousLanguage.startsWith(langCodes[i])) {
                    languageComboBox.currentIndex = i
                    break
                }
            }
        }
    }
}
