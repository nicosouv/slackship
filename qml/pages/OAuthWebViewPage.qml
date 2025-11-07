import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.WebView 1.0

Page {
    id: oauthPage

    property string authUrl: ""
    property string redirectUri: "http://localhost:8080/callback"

    signal authCodeReceived(string code, string state)
    signal authError(string error)

    PageHeader {
        id: header
        title: qsTr("Login to Slack")
    }

    WebView {
        id: webView
        anchors {
            top: header.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }

        url: oauthPage.authUrl

        onUrlChanged: {
            console.log("WebView URL changed:", url)

            // Check if this is the callback URL
            var urlString = url.toString()
            if (urlString.indexOf(redirectUri) === 0) {
                console.log("Callback URL detected!")

                // Parse URL parameters
                var queryStart = urlString.indexOf('?')
                if (queryStart !== -1) {
                    var queryString = urlString.substring(queryStart + 1)
                    var params = {}
                    var pairs = queryString.split('&')

                    for (var i = 0; i < pairs.length; i++) {
                        var pair = pairs[i].split('=')
                        if (pair.length === 2) {
                            params[decodeURIComponent(pair[0])] = decodeURIComponent(pair[1])
                        }
                    }

                    console.log("Parsed params:", JSON.stringify(params))

                    // Check for error
                    if (params.error) {
                        console.error("OAuth error:", params.error)
                        oauthPage.authError(params.error)
                        pageStack.pop()
                        return
                    }

                    // Check for code
                    if (params.code && params.state) {
                        console.log("Authorization code received!")
                        oauthPage.authCodeReceived(params.code, params.state)
                        pageStack.pop()
                        return
                    }
                }
            }
        }

        onLoadingChanged: {
            if (loading) {
                busyIndicator.running = true
            } else {
                busyIndicator.running = false
            }
        }
    }

    BusyIndicator {
        id: busyIndicator
        anchors.centerIn: parent
        size: BusyIndicatorSize.Large
        running: false
    }

    // Pull down menu to cancel
    PullDownMenu {
        MenuItem {
            text: qsTr("Cancel")
            onClicked: {
                oauthPage.authError("cancelled")
                pageStack.pop()
            }
        }
    }
}
