import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: loginPage

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: qsTr("Login to Slack")
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width - Theme.horizontalPageMargin * 2
                text: qsTr("Connect your Slack account")
                wrapMode: Text.WordWrap
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeLarge
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
            }

            // OAuth Login Button (Primary method)
            Button {
                id: oauthButton
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Login with Slack")
                preferredWidth: Theme.buttonWidthLarge
                enabled: !oauthManager.isAuthenticating

                onClicked: {
                    // Start WebView authentication (starts HTTP server + returns OAuth URL)
                    var authUrl = oauthManager.startWebViewAuthentication()

                    if (!authUrl) {
                        console.error("Failed to start WebView authentication")
                        return
                    }

                    // Push WebView page
                    var webViewPage = pageStack.push(Qt.resolvedUrl("OAuthWebViewPage.qml"), {
                        authUrl: authUrl
                    })

                    // Connect signals
                    webViewPage.authCodeReceived.connect(function(code, state) {
                        console.log("Authorization code received from WebView")
                        oauthManager.handleWebViewCallback(code, state)
                    })

                    webViewPage.authError.connect(function(error) {
                        console.error("WebView error:", error)
                        oauthManager.cancelAuthentication()
                    })
                }
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Opens in-app browser for secure authentication")
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
            }

            // Separator
            Item {
                width: parent.width
                height: Theme.paddingLarge * 2

                Row {
                    anchors.centerIn: parent
                    spacing: Theme.paddingMedium
                    width: parent.width * 0.8

                    Rectangle {
                        width: (parent.width - orLabel.width - Theme.paddingMedium * 2) / 2
                        height: 1
                        color: Theme.secondaryColor
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Label {
                        id: orLabel
                        text: qsTr("or")
                        color: Theme.secondaryColor
                        font.pixelSize: Theme.fontSizeSmall
                    }

                    Rectangle {
                        width: (parent.width - orLabel.width - Theme.paddingMedium * 2) / 2
                        height: 1
                        color: Theme.secondaryColor
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }

            // Manual token entry (Advanced/Fallback)
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width - Theme.horizontalPageMargin * 2
                text: qsTr("Advanced: Enter token manually")
                wrapMode: Text.WordWrap
                color: Theme.secondaryHighlightColor
                font.pixelSize: Theme.fontSizeSmall
                horizontalAlignment: Text.AlignHCenter
            }

            TextField {
                id: tokenField
                width: parent.width
                placeholderText: qsTr("xoxp-...")
                label: qsTr("Workspace token")
                inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
                visible: advancedExpander.expanded

                EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                EnterKey.onClicked: loginButton.clicked()
            }

            Button {
                id: loginButton
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Login with Token")
                enabled: tokenField.text.length > 0
                visible: advancedExpander.expanded

                onClicked: {
                    if (tokenField.text.trim().length > 0) {
                        var token = tokenField.text.trim()
                        slackAPI.authenticate(token)
                        fileManager.setToken(token)

                        // Temporarily store token for workspace creation
                        workspaceManager.currentWorkspaceToken = token
                    }
                }
            }

            ExpandingSection {
                id: advancedExpander
                anchors.horizontalCenter: parent.horizontalCenter
                title: qsTr("Show advanced options")
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width - Theme.horizontalPageMargin * 2
                text: qsTr("How to get your token:")
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.highlightColor
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width - Theme.horizontalPageMargin * 2
                text: qsTr("1. Go to api.slack.com/apps\n" +
                          "2. Create a new app\n" +
                          "3. Get your OAuth token")
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
            }

            Item {
                width: parent.width
                height: Theme.paddingLarge
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Lagoon v0.2.12"
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
                opacity: 0.6
            }
        }
    }

    BusyIndicator {
        anchors.centerIn: parent
        size: BusyIndicatorSize.Large
        running: oauthManager.isAuthenticating
    }

    // OAuth connections
    Connections {
        target: oauthManager

        onAuthenticationSucceeded: {
            console.log("OAuth authentication succeeded!")
            console.log("Team:", teamName, "User:", userId)

            // Authenticate with Slack API using the access token
            slackAPI.authenticate(accessToken)
            fileManager.setToken(accessToken)

            // Add workspace
            workspaceManager.addWorkspace(teamName, accessToken, teamId, userId, teamName + ".slack.com")

            // Navigate back
            pageStack.pop()
        }

        onAuthenticationFailed: {
            console.error("OAuth authentication failed:", error)

            // Show error banner
            var banner = Notices.show(qsTr("Authentication failed: %1").arg(error), Notice.Long)
        }
    }
}
