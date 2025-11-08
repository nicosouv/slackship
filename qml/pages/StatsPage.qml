import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

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
            spacing: Theme.paddingMedium

            PageHeader {
                title: qsTr("Workspace Insights")
            }

            // Overview Section
            SectionHeader {
                text: qsTr("📊 Overview")
            }

            Grid {
                width: parent.width - 2 * Theme.horizontalPageMargin
                x: Theme.horizontalPageMargin
                columns: 2
                spacing: Theme.paddingMedium

                StatCard {
                    width: (parent.width - parent.spacing) / 2
                    icon: "💬"
                    value: statsManager.totalMessages
                    label: qsTr("Total Messages")
                }

                StatCard {
                    width: (parent.width - parent.spacing) / 2
                    icon: "📅"
                    value: statsManager.messagesThisWeek
                    label: qsTr("This Week")
                }

                StatCard {
                    width: (parent.width - parent.spacing) / 2
                    icon: "📆"
                    value: statsManager.messagesThisMonth
                    label: qsTr("This Month")
                }

                StatCard {
                    width: (parent.width - parent.spacing) / 2
                    icon: "🔥"
                    value: statsManager.currentStreak
                    label: qsTr("Day Streak")
                }
            }

            // Top Channels
            SectionHeader {
                text: qsTr("🏆 Most Active")
            }

            BackgroundItem {
                width: parent.width
                height: Theme.itemSizeSmall

                Row {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.horizontalPageMargin
                    anchors.rightMargin: Theme.horizontalPageMargin
                    spacing: Theme.paddingMedium

                    Label {
                        text: "#"
                        font.pixelSize: Theme.fontSizeLarge
                        color: Theme.highlightColor
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width - Theme.fontSizeLarge - parent.spacing * 2

                        Label {
                            text: qsTr("Channel: %1").arg(getChannelName(statsManager.mostActiveChannel))
                            color: Theme.primaryColor
                            truncationMode: TruncationMode.Fade
                            width: parent.width
                        }

                        Label {
                            text: qsTr("Your most active channel")
                            font.pixelSize: Theme.fontSizeExtraSmall
                            color: Theme.secondaryColor
                        }
                    }
                }
            }

            BackgroundItem {
                width: parent.width
                height: Theme.itemSizeSmall

                Row {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.horizontalPageMargin
                    anchors.rightMargin: Theme.horizontalPageMargin
                    spacing: Theme.paddingMedium

                    Label {
                        text: statsManager.mostUsedEmoji ? ":" + statsManager.mostUsedEmoji + ":" : "😊"
                        font.pixelSize: Theme.fontSizeLarge
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width - Theme.fontSizeLarge - parent.spacing * 2

                        Label {
                            text: qsTr("Emoji: :%1:").arg(statsManager.mostUsedEmoji || "none")
                            color: Theme.primaryColor
                            truncationMode: TruncationMode.Fade
                            width: parent.width
                        }

                        Label {
                            text: qsTr("Your favorite emoji")
                            font.pixelSize: Theme.fontSizeExtraSmall
                            color: Theme.secondaryColor
                        }
                    }
                }
            }

            // Activity Breakdown
            SectionHeader {
                text: qsTr("📈 Activity")
            }

            Item {
                width: parent.width
                height: weeklyChart.height

                Row {
                    id: weeklyChart
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    x: Theme.horizontalPageMargin
                    spacing: Theme.paddingSmall

                    Repeater {
                        id: weeklyRepeater
                        model: ListModel { id: weeklyModel }

                        delegate: Column {
                            width: (weeklyChart.width - weeklyChart.spacing * 6) / 7
                            spacing: Theme.paddingSmall

                            Rectangle {
                                width: parent.width
                                height: Math.max(4, model.count * 2) // Scale height based on count
                                anchors.horizontalCenter: parent.horizontalCenter
                                color: model.count > 0 ? Theme.highlightColor : Theme.rgba(Theme.highlightColor, 0.2)
                                radius: Theme.paddingSmall / 2
                            }

                            Label {
                                width: parent.width
                                text: model.day
                                font.pixelSize: Theme.fontSizeExtraSmall
                                color: Theme.secondaryColor
                                horizontalAlignment: Text.AlignHCenter
                            }

                            Label {
                                width: parent.width
                                text: model.count
                                font.pixelSize: Theme.fontSizeExtraSmall
                                font.bold: true
                                color: Theme.primaryColor
                                horizontalAlignment: Text.AlignHCenter
                                visible: model.count > 0
                            }
                        }
                    }
                }
            }

            // Fun Facts
            SectionHeader {
                text: qsTr("✨ Fun Facts")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: qsTr("You've started %n thread(s) 🧵", "", statsManager.threadsStarted || 0)
                color: Theme.primaryColor
                wrapMode: Text.WordWrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: qsTr("You've replied to %n thread(s) 💬", "", statsManager.threadReplies || 0)
                color: Theme.primaryColor
                wrapMode: Text.WordWrap
            }

            Item {
                width: parent.width
                height: Theme.paddingLarge * 3
            }
        }

        VerticalScrollDecorator { }
    }

    Component.onCompleted: {
        console.log("StatsPage loaded")

        // Load weekly data
        try {
            var weeklyData = statsManager.getWeeklyActivity()
            console.log("Weekly data received:", JSON.stringify(weeklyData))

            if (weeklyData && weeklyData.days && weeklyData.days.length > 0) {
                var days = ["Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"]
                for (var i = 0; i < weeklyData.days.length; i++) {
                    var dayData = weeklyData.days[i]
                    if (dayData && dayData.date !== undefined && dayData.count !== undefined) {
                        var date = new Date(dayData.date)
                        weeklyModel.append({
                            "day": days[date.getDay()],
                            "count": dayData.count
                        })
                    }
                }
                console.log("Loaded", weeklyModel.count, "days of activity")
            } else {
                console.log("No weekly data available yet")
                // Add empty days for the week
                var emptyDays = ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"]
                for (var j = 0; j < emptyDays.length; j++) {
                    weeklyModel.append({
                        "day": emptyDays[j],
                        "count": 0
                    })
                }
            }
        } catch (e) {
            console.error("Error loading weekly data:", e)
        }
    }

    function getChannelName(channelId) {
        if (!channelId) return qsTr("None")

        // Try to get channel name from conversationModel
        for (var i = 0; i < conversationModel.rowCount(); i++) {
            var idx = conversationModel.index(i, 0)
            if (conversationModel.data(idx, 256) === channelId) { // IdRole = 256
                return conversationModel.data(idx, 257) // NameRole = 257
            }
        }

        return channelId.substring(0, 8) + "..."
    }

    RemorsePopup {
        id: remorse
    }
}

// Custom component for stat cards
Component {
    id: statCardComponent

    Rectangle {
        property string icon: ""
        property int value: 0
        property string label: ""

        height: Theme.itemSizeMedium
        color: Theme.rgba(Theme.highlightBackgroundColor, 0.1)
        radius: Theme.paddingSmall

        Column {
            anchors.centerIn: parent
            spacing: Theme.paddingSmall

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: icon
                font.pixelSize: Theme.fontSizeHuge
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: value
                font.pixelSize: Theme.fontSizeLarge
                font.bold: true
                color: Theme.highlightColor
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: label
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
            }
        }
    }
}
