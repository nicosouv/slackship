import QtQuick 2.0
import Sailfish.Silica 1.0
import Amber.Web.Authorization 1.0

Page {
    id: loginPage

    // Amber Web Authorization OAuth2 component for Slack
    OAuth2Ac {
        id: slackOAuth

        // Read client credentials from compiled defines
        clientId: oauthManager.clientId
        clientSecret: oauthManager.clientSecret

        // Slack OAuth2 endpoints
        authorizationEndpoint: "https://slack.com/oauth/v2/authorize"
        tokenEndpoint: "https://slack.com/api/oauth.v2.access"

        // Scopes for user token
        scopes: ["channels:read", "channels:history", "chat:write", "users:read", "im:read", "im:history", "groups:read", "groups:history"]

        // Redirect listener port (Amber creates local server)
        redirectListener.port: 8080

        // Override redirect URI to use redirectmeto.com proxy
        // This allows HTTPS redirect URL while still redirecting to local HTTP server
        // Note: Amber's redirect listener accepts any path, so we can use /callback
        redirectUri: "https://redirectmeto.com/http://127.0.0.1:8080/callback"

        Component.onCompleted: {
            console.log("=== OAUTH2 DEBUG ===")
            console.log("Client ID:", clientId)
            console.log("Client Secret:", clientSecret ? "SET (length: " + clientSecret.length + ")" : "NOT SET")
            console.log("Auth Endpoint:", authorizationEndpoint)
            console.log("Token Endpoint:", tokenEndpoint)
            console.log("Scopes:", scopes)
            console.log("Redirect Port:", redirectListener.port)
            console.log("Redirect URI:", redirectUri)
            console.log("===================")
        }

        onErrorOccurred: {
            console.error("=== OAUTH ERROR ===")
            console.error("Error code:", code)
            console.error("Error message:", message)
            console.error("===================")
            var banner = Notices.show(qsTr("Authentication failed: %1").arg(message), Notice.Long)
        }

        onReceivedAuthorizationCode: {
            console.log("=== AUTHORIZATION CODE RECEIVED ===")
            console.log("Code length:", code ? code.length : 0)
            console.log("===================================")
        }

        onReceivedAccessToken: {
            console.log("=== ACCESS TOKEN RECEIVED ===")
            console.log("Token type:", token.token_type)
            console.log("Access token length:", token.access_token ? token.access_token.length : 0)
            console.log("Scopes:", token.scope)
            console.log("=============================")

            // Store token
            fileManager.setToken(token.access_token)
            slackAPI.authenticate(token.access_token)

            // For now we don't have full team info from OAuth2Ac response
            // The app will fetch this after authentication

            // Navigate to main view
            pageStack.replace(Qt.resolvedUrl("FirstPage.qml"))
        }
    }

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

                onClicked: {
                    console.log("=== STARTING OAUTH FLOW ===")
                    console.log("About to call authorizeInBrowser()")

                    // Use Amber Web Authorization to handle OAuth2 flow
                    // This will open the system browser and handle the redirect
                    slackOAuth.authorizeInBrowser()

                    console.log("authorizeInBrowser() called")
                    console.log("===========================")
                }
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Opens system browser for secure authentication")
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
                        console.log("=== MANUAL TOKEN LOGIN ===")
                        console.log("Token length:", token.length)
                        console.log("Token prefix:", token.substring(0, 10) + "...")

                        slackAPI.authenticate(token)
                        fileManager.setToken(token)

                        console.log("Navigating to FirstPage")
                        console.log("==========================")

                        // Navigate to main view
                        pageStack.replace(Qt.resolvedUrl("FirstPage.qml"))
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
                text: "Lagoon v0.32.6"
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
                opacity: 0.6
            }
        }
    }

}
