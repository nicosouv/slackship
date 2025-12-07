import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.WebView 1.0
import Sailfish.WebEngine 1.0

Page {
    id: loginPage

    property string authUrl: ""

    // Helper function to extract code from redirect URL
    function extractCodeFromUrl(url) {
        var match = url.match(/[?&]code=([^&]+)/)
        return match ? match[1] : ""
    }

    // Helper function to extract state from redirect URL
    function extractStateFromUrl(url) {
        var match = url.match(/[?&]state=([^&]+)/)
        return match ? match[1] : ""
    }

    Component.onCompleted: {
        // Start OAuth flow when page loads
        authUrl = oauthManager.startWebViewAuthentication()
        console.log("[Login] Auth URL:", authUrl)
    }

    Connections {
        target: oauthManager

        onAuthenticationSucceeded: {
            console.log("[Login] Authentication succeeded!")
            console.log("[Login] Team:", teamName, "(" + teamId + ")")
            console.log("[Login] User:", userId)

            // Store token and authenticate
            fileManager.setToken(accessToken)
            slackAPI.authenticate(accessToken)

            // Navigate to main view
            pageStack.replace(Qt.resolvedUrl("FirstPage.qml"))
        }

        onAuthenticationFailed: {
            console.error("[Login] Authentication failed:", error)
            // Show error and stay on login page
            errorLabel.text = qsTr("Login failed: %1").arg(error)
            errorLabel.visible = true
        }
    }

    SilicaFlickable {
        id: mainFlickable
        anchors.fill: parent
        contentHeight: column.height
        visible: !webViewLoader.active

        PullDownMenu {
            MenuItem {
                text: qsTr("Use browser instead")
                onClicked: {
                    oauthManager.startAuthentication()
                }
            }
        }

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
                text: qsTr("Connect your Slack workspace")
                wrapMode: Text.WordWrap
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeLarge
                horizontalAlignment: Text.AlignHCenter
            }

            Label {
                id: errorLabel
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width - Theme.horizontalPageMargin * 2
                wrapMode: Text.WordWrap
                color: Theme.errorColor
                font.pixelSize: Theme.fontSizeSmall
                horizontalAlignment: Text.AlignHCenter
                visible: false
            }

            // Login with WebView button
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Login with Slack")
                preferredWidth: Theme.buttonWidthLarge

                onClicked: {
                    if (authUrl.length > 0) {
                        webViewLoader.active = true
                    } else {
                        // Restart OAuth flow
                        authUrl = oauthManager.startWebViewAuthentication()
                        if (authUrl.length > 0) {
                            webViewLoader.active = true
                        }
                    }
                }
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Opens Slack login in app")
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

            // Advanced options
            ExpandingSectionGroup {
                ExpandingSection {
                    id: advancedSection
                    title: qsTr("Advanced options")

                    content.sourceComponent: Column {
                        width: parent.width
                        spacing: Theme.paddingMedium

                        // Copy link for desktop login
                        SectionHeader {
                            text: qsTr("Login from desktop")
                        }

                        Label {
                            x: Theme.horizontalPageMargin
                            width: parent.width - 2 * Theme.horizontalPageMargin
                            text: qsTr("Copy the link, login on your computer, then paste the redirect URL here")
                            wrapMode: Text.WordWrap
                            font.pixelSize: Theme.fontSizeSmall
                            color: Theme.secondaryColor
                        }

                        Button {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: qsTr("Copy login link")
                            onClicked: {
                                Clipboard.text = authUrl
                            }
                        }

                        TextField {
                            id: redirectUrlField
                            width: parent.width
                            placeholderText: qsTr("Paste redirect URL here...")
                            label: qsTr("Redirect URL")
                            inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase

                            EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                            EnterKey.onClicked: {
                                if (redirectUrlField.text.trim().length > 0) {
                                    oauthManager.handleWebViewCallback(
                                        extractCodeFromUrl(redirectUrlField.text.trim()),
                                        extractStateFromUrl(redirectUrlField.text.trim())
                                    )
                                }
                            }
                        }

                        Button {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: qsTr("Use redirect URL")
                            enabled: redirectUrlField.text.length > 0
                            onClicked: {
                                if (redirectUrlField.text.trim().length > 0) {
                                    oauthManager.handleWebViewCallback(
                                        extractCodeFromUrl(redirectUrlField.text.trim()),
                                        extractStateFromUrl(redirectUrlField.text.trim())
                                    )
                                }
                            }
                        }

                        // Manual token entry
                        SectionHeader {
                            text: qsTr("Manual token")
                        }

                        TextField {
                            id: tokenField
                            width: parent.width
                            placeholderText: qsTr("xoxp-...")
                            label: qsTr("Workspace token")
                            inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase

                            EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                            EnterKey.onClicked: {
                                if (tokenField.text.trim().length > 0) {
                                    var token = tokenField.text.trim()
                                    slackAPI.authenticate(token)
                                    fileManager.setToken(token)
                                    pageStack.replace(Qt.resolvedUrl("FirstPage.qml"))
                                }
                            }
                        }

                        Button {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: qsTr("Login with Token")
                            enabled: tokenField.text.length > 0

                            onClicked: {
                                if (tokenField.text.trim().length > 0) {
                                    var token = tokenField.text.trim()
                                    slackAPI.authenticate(token)
                                    fileManager.setToken(token)
                                    pageStack.replace(Qt.resolvedUrl("FirstPage.qml"))
                                }
                            }
                        }

                        Label {
                            x: Theme.horizontalPageMargin
                            width: parent.width - 2 * Theme.horizontalPageMargin
                            text: qsTr("Get a token from api.slack.com/apps")
                            wrapMode: Text.WordWrap
                            font.pixelSize: Theme.fontSizeExtraSmall
                            color: Theme.secondaryColor
                        }
                    }
                }
            }

            Item {
                width: parent.width
                height: Theme.paddingLarge
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Lagoon v0.37.28"
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
                opacity: 0.6
            }
        }
    }

    // WebView for OAuth login
    Loader {
        id: webViewLoader
        anchors.fill: parent
        active: false

        sourceComponent: Item {
            anchors.fill: parent

            PageHeader {
                id: webViewHeader
                title: qsTr("Sign in with Slack")

                // Back button
                IconButton {
                    anchors.left: parent.left
                    anchors.leftMargin: Theme.horizontalPageMargin
                    anchors.verticalCenter: parent.verticalCenter
                    icon.source: "image://theme/icon-m-back"
                    onClicked: {
                        oauthManager.cancelAuthentication()
                        webViewLoader.active = false
                    }
                }
            }

            WebView {
                id: slackWebView
                anchors {
                    top: webViewHeader.bottom
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                }

                url: authUrl

                onUrlChanged: {
                    console.log("[Login] WebView URL:", url)

                    // Check if this is the callback URL
                    var urlStr = url.toString()
                    if (urlStr.indexOf("localhost:8080") !== -1 ||
                        urlStr.indexOf("127.0.0.1:8080") !== -1) {
                        console.log("[Login] Detected callback URL")
                        // The local server will handle this
                    }
                }
            }

            BusyIndicator {
                anchors.centerIn: slackWebView
                size: BusyIndicatorSize.Large
                running: slackWebView.loading
            }
        }
    }
}
