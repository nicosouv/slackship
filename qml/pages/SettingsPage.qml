import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: settingsPage

    property string previousLanguage: appSettings.language || "en"

    // Helper function to format bytes
    function formatBytes(bytes) {
        if (bytes < 1024) return bytes + " B"
        if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(2) + " KB"
        if (bytes < 1024 * 1024 * 1024) return (bytes / (1024 * 1024)).toFixed(2) + " MB"
        return (bytes / (1024 * 1024 * 1024)).toFixed(2) + " GB"
    }

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

            TextSwitch {
                id: dndSwitch
                text: qsTr("Do Not Disturb")
                description: qsTr("Silence notifications during quiet hours")
                checked: appSettings.dndEnabled !== undefined ? appSettings.dndEnabled : false
                enabled: appSettings.notificationsEnabled
                onCheckedChanged: appSettings.dndEnabled = checked
            }

            ValueButton {
                label: qsTr("Quiet hours start")
                value: {
                    var hour = appSettings.dndStartHour !== undefined ? appSettings.dndStartHour : 22
                    var minute = appSettings.dndStartMinute !== undefined ? appSettings.dndStartMinute : 0
                    return ("0" + hour).slice(-2) + ":" + ("0" + minute).slice(-2)
                }
                enabled: appSettings.notificationsEnabled && dndSwitch.checked
                onClicked: {
                    var dialog = pageStack.push("Sailfish.Silica.TimePickerDialog", {
                        hourMode: DateTime.TwentyFourHours,
                        hour: appSettings.dndStartHour !== undefined ? appSettings.dndStartHour : 22,
                        minute: appSettings.dndStartMinute !== undefined ? appSettings.dndStartMinute : 0
                    })
                    dialog.accepted.connect(function() {
                        appSettings.dndStartHour = dialog.hour
                        appSettings.dndStartMinute = dialog.minute
                    })
                }
            }

            ValueButton {
                label: qsTr("Quiet hours end")
                value: {
                    var hour = appSettings.dndEndHour !== undefined ? appSettings.dndEndHour : 8
                    var minute = appSettings.dndEndMinute !== undefined ? appSettings.dndEndMinute : 0
                    return ("0" + hour).slice(-2) + ":" + ("0" + minute).slice(-2)
                }
                enabled: appSettings.notificationsEnabled && dndSwitch.checked
                onClicked: {
                    var dialog = pageStack.push("Sailfish.Silica.TimePickerDialog", {
                        hourMode: DateTime.TwentyFourHours,
                        hour: appSettings.dndEndHour !== undefined ? appSettings.dndEndHour : 8,
                        minute: appSettings.dndEndMinute !== undefined ? appSettings.dndEndMinute : 0
                    })
                    dialog.accepted.connect(function() {
                        appSettings.dndEndHour = dialog.hour
                        appSettings.dndEndMinute = dialog.minute
                    })
                }
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
                text: qsTr("Logout from %1").arg(slackAPI.workspaceName || qsTr("workspace"))
                onClicked: {
                    var currentIndex = workspaceManager.currentWorkspaceIndex
                    var remainingCount = workspaceManager.workspaceCount() - 1

                    remorse.execute(qsTr("Logging out"), function() {
                        // Cleanup API state
                        slackAPI.logout()

                        // Remove only the current workspace
                        workspaceManager.removeWorkspace(currentIndex)

                        // If no workspaces left, go to login page
                        // Otherwise, the removeWorkspace automatically switches to another workspace
                        if (remainingCount === 0) {
                            pageStack.replace(Qt.resolvedUrl("LoginPage.qml"))
                        } else {
                            // Pop settings page to go back to FirstPage with new workspace
                            pageStack.pop()
                        }
                    })
                }
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Logout from all workspaces")
                visible: workspaceManager.workspaceCount() > 1
                onClicked: {
                    remorse.execute(qsTr("Logging out from all"), function() {
                        slackAPI.logout()
                        workspaceManager.clearAllWorkspaces()
                        pageStack.replace(Qt.resolvedUrl("LoginPage.qml"))
                    })
                }
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Clean Duplicate Workspaces")
                visible: workspaceManager.workspaceCount() > 1
                onClicked: {
                    var beforeCount = workspaceManager.workspaceCount()
                    console.log("Before cleanup: " + beforeCount + " workspaces")

                    remorse.execute(qsTr("Cleaning duplicates"), function() {
                        workspaceManager.removeDuplicates()

                        var afterCount = workspaceManager.workspaceCount()
                        console.log("After cleanup: " + afterCount + " workspaces")

                        var removed = beforeCount - afterCount
                        if (removed > 0) {
                            console.log("Removed " + removed + " duplicate(s)")
                        } else {
                            console.log("No duplicates found")
                        }
                    })
                }
            }

            SectionHeader {
                text: qsTr("Data Usage")
            }

            ComboBox {
                id: pollingIntervalComboBox
                label: qsTr("Polling interval")
                description: qsTr("How often to check for new messages")
                width: parent.width

                currentIndex: {
                    var interval = appSettings.pollingInterval
                    if (interval === 15) return 0
                    if (interval === 30) return 1
                    if (interval === 60) return 2
                    if (interval === 120) return 3
                    if (interval === 300) return 4
                    return 1  // Default to 30 seconds
                }

                menu: ContextMenu {
                    MenuItem { text: qsTr("15 seconds (more data)") }
                    MenuItem { text: qsTr("30 seconds (default)") }
                    MenuItem { text: qsTr("1 minute") }
                    MenuItem { text: qsTr("2 minutes") }
                    MenuItem { text: qsTr("5 minutes (less data)") }
                }

                onCurrentIndexChanged: {
                    var intervals = [15, 30, 60, 120, 300]
                    appSettings.pollingInterval = intervals[currentIndex]
                }
            }

            Column {
                width: parent.width
                spacing: Theme.paddingMedium

                DetailItem {
                    label: qsTr("Current session")
                    value: formatBytes(slackAPI.sessionBandwidthBytes)
                }

                DetailItem {
                    label: qsTr("Total since install")
                    value: formatBytes(appSettings.totalBandwidthBytes)
                }

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    text: {
                        var elapsedMs = new Date().getTime() - slackAPI.sessionStartTime
                        var elapsedMin = Math.max(1, Math.floor(elapsedMs / 60000))
                        var bytesPerMin = slackAPI.sessionBandwidthBytes / elapsedMin
                        return qsTr("API polling: ~%1/min").arg(formatBytes(bytesPerMin))
                    }
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryColor
                    wrapMode: Text.WordWrap
                }
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Reset Data Statistics")
                onClicked: {
                    remorse.execute(qsTr("Resetting data statistics"), function() {
                        appSettings.resetBandwidthStats()
                    })
                }
            }

            SectionHeader {
                text: qsTr("Daily Stats")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: qsTr("Stats are tracked per workspace and reset daily at midnight")
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.WordWrap
            }

            // Stats preview - simplified to 2 counters
            BackgroundItem {
                width: parent.width
                height: statsPreview.height + Theme.paddingLarge * 2
                onClicked: pageStack.push(Qt.resolvedUrl("StatsPage.qml"))

                Column {
                    id: statsPreview
                    anchors.centerIn: parent
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    spacing: Theme.paddingLarge

                    Row {
                        width: parent.width
                        spacing: Theme.paddingLarge

                        Column {
                            width: (parent.width - parent.spacing) / 2
                            Label {
                                width: parent.width
                                text: statsManager.todayTotal
                                font.pixelSize: Theme.fontSizeHuge
                                font.bold: true
                                color: Theme.highlightColor
                                horizontalAlignment: Text.AlignHCenter
                            }
                            Label {
                                width: parent.width
                                text: qsTr("Workspace Today")
                                font.pixelSize: Theme.fontSizeExtraSmall
                                color: Theme.secondaryColor
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }

                        Column {
                            width: (parent.width - parent.spacing) / 2
                            Label {
                                width: parent.width
                                text: statsManager.todayUser
                                font.pixelSize: Theme.fontSizeHuge
                                font.bold: true
                                color: Theme.highlightColor
                                horizontalAlignment: Text.AlignHCenter
                            }
                            Label {
                                width: parent.width
                                text: qsTr("Your Messages")
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

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Reset Statistics")
                onClicked: {
                    remorse.execute(qsTr("Resetting all statistics"), function() {
                        statsManager.resetStats()
                    })
                }
            }

            SectionHeader {
                text: qsTr("About")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Lagoon v0.37.11"
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

            // Update notification
            BackgroundItem {
                width: parent.width
                height: updateColumn.height + Theme.paddingLarge * 2
                visible: updateChecker.updateAvailable

                onClicked: {
                    Qt.openUrlExternally(updateChecker.releaseUrl)
                }

                Column {
                    id: updateColumn
                    anchors.centerIn: parent
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    spacing: Theme.paddingSmall

                    Label {
                        width: parent.width
                        text: qsTr("Update available: v%1").arg(updateChecker.latestVersion)
                        color: Theme.highlightColor
                        font.pixelSize: Theme.fontSizeSmall
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Label {
                        width: parent.width
                        text: qsTr("Tap to view release on GitHub")
                        color: Theme.secondaryHighlightColor
                        font.pixelSize: Theme.fontSizeExtraSmall
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }

            // Manual check button
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: updateChecker.checking ? qsTr("Checking...") : qsTr("Check for updates")
                enabled: !updateChecker.checking
                onClicked: updateChecker.checkForUpdates()
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
