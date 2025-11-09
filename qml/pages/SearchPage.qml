import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components/EmojiHelper.js" as EmojiHelper

Page {
    id: searchPage

    property string currentQuery: ""
    property int totalResults: 0

    // Listen for search results
    Connections {
        target: slackAPI
        onSearchResultsReceived: {
            searchResultsModel.clear()
            busyIndicator.running = false

            var messages = results.messages
            if (messages) {
                var matches = messages.matches
                totalResults = messages.total || 0

                if (matches && matches.length > 0) {
                    for (var i = 0; i < matches.length; i++) {
                        var match = matches[i]
                        searchResultsModel.append({
                            "messageText": match.text || "",
                            "userId": match.user || "",
                            "channelId": match.channel ? match.channel.id : "",
                            "channelName": match.channel ? match.channel.name : "",
                            "timestamp": match.ts || "",
                            "threadTs": match.thread_ts || "",
                            "permalink": match.permalink || ""
                        })
                    }
                }
            }
        }
    }

    ListModel {
        id: searchResultsModel
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: qsTr("Search Messages")
            }

            SearchField {
                id: searchField
                width: parent.width
                placeholderText: qsTr("Search for messages...")

                EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                EnterKey.onClicked: {
                    if (text.trim().length > 0) {
                        currentQuery = text.trim()
                        busyIndicator.running = true
                        searchResultsModel.clear()
                        slackAPI.searchMessages(currentQuery)
                        focus = false
                    }
                }
            }

            BusyIndicator {
                id: busyIndicator
                anchors.horizontalCenter: parent.horizontalCenter
                size: BusyIndicatorSize.Large
                running: false
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("%n result(s) found", "", totalResults)
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryHighlightColor
                visible: searchResultsModel.count > 0 && !busyIndicator.running
            }

            SilicaListView {
                id: searchResultsList
                width: parent.width
                height: searchPage.height - searchField.height - Theme.paddingLarge * 4
                clip: true
                model: searchResultsModel

                delegate: ListItem {
                    id: searchResultItem
                    width: parent.width
                    contentHeight: resultColumn.height + Theme.paddingLarge * 2

                    Column {
                        id: resultColumn
                        anchors {
                            left: parent.left
                            right: parent.right
                            leftMargin: Theme.horizontalPageMargin
                            rightMargin: Theme.horizontalPageMargin
                            verticalCenter: parent.verticalCenter
                        }
                        spacing: Theme.paddingSmall

                        Row {
                            width: parent.width
                            spacing: Theme.paddingMedium

                            Label {
                                text: "#" + (model.channelName || qsTr("unknown"))
                                font.bold: true
                                font.pixelSize: Theme.fontSizeSmall
                                color: Theme.highlightColor
                            }

                            Label {
                                text: userModel.getUserName(model.userId) || model.userId
                                font.pixelSize: Theme.fontSizeSmall
                                color: Theme.secondaryHighlightColor
                            }

                            Label {
                                text: {
                                    var msgDate = new Date(parseFloat(model.timestamp) * 1000)
                                    var today = new Date()
                                    if (msgDate.toDateString() === today.toDateString()) {
                                        return Qt.formatDateTime(msgDate, "hh:mm")
                                    } else if (msgDate.getFullYear() === today.getFullYear()) {
                                        return Qt.formatDateTime(msgDate, "MMM dd")
                                    } else {
                                        return Qt.formatDateTime(msgDate, "MMM dd yyyy")
                                    }
                                }
                                font.pixelSize: Theme.fontSizeExtraSmall
                                color: Theme.secondaryColor
                            }
                        }

                        Label {
                            width: parent.width
                            text: EmojiHelper.convertEmoji(model.messageText)
                            font.pixelSize: Theme.fontSizeSmall
                            color: Theme.primaryColor
                            wrapMode: Text.Wrap
                            maximumLineCount: 3
                            elide: Text.ElideRight
                        }
                    }

                    onClicked: {
                        // Navigate to the channel and message
                        if (model.channelId) {
                            if (model.threadTs && model.threadTs !== model.timestamp) {
                                // This is a thread reply - go to thread
                                pageStack.push(Qt.resolvedUrl("ThreadPage.qml"), {
                                    channelId: model.channelId,
                                    channelName: "#" + model.channelName,
                                    threadTs: model.threadTs
                                })
                            } else {
                                // Go to conversation
                                pageStack.push(Qt.resolvedUrl("ConversationPage.qml"), {
                                    channelId: model.channelId,
                                    channelName: "#" + model.channelName
                                })
                            }
                        }
                    }
                }

                VerticalScrollDecorator { }

                ViewPlaceholder {
                    enabled: searchResultsModel.count === 0 && !busyIndicator.running && currentQuery.length > 0
                    text: qsTr("No results found")
                    hintText: qsTr("Try a different search query")
                }

                ViewPlaceholder {
                    enabled: searchResultsModel.count === 0 && !busyIndicator.running && currentQuery.length === 0
                    text: qsTr("Search messages")
                    hintText: qsTr("Enter a search query and press enter")
                }
            }
        }
    }
}
