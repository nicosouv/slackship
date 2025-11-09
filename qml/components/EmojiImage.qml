import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components/EmojiHelper.js" as EmojiHelper

Item {
    id: emojiImage

    property string emoji: ""
    property string reactionName: ""
    property int size: Theme.iconSizeSmall
    property bool useTwemoji: true  // Use Twemoji images by default

    width: size
    height: size

    // Try to display image first (Twemoji)
    Image {
        id: twemojiImage
        anchors.fill: parent
        visible: useTwemoji && source != ""
        source: {
            if (!useTwemoji) return ""

            // Try to get URL from reaction name first
            if (reactionName) {
                return EmojiHelper.reactionNameToTwemojiUrl(reactionName, size)
            }

            // Otherwise try from emoji Unicode
            if (emoji) {
                return EmojiHelper.emojiToTwemojiUrl(emoji, size)
            }

            return ""
        }
        sourceSize.width: size
        sourceSize.height: size
        fillMode: Image.PreserveAspectFit
        asynchronous: true
        cache: true

        onStatusChanged: {
            if (status === Image.Error) {
                console.warn("Failed to load Twemoji for:", reactionName || emoji)
            }
        }
    }

    // Fallback to native Unicode emoji if image fails or is disabled
    Label {
        id: unicodeLabel
        anchors.centerIn: parent
        visible: !useTwemoji || twemojiImage.status === Image.Error || twemojiImage.source === ""
        text: {
            if (reactionName) {
                return EmojiHelper.reactionToEmoji(reactionName)
            }
            return emoji || ""
        }
        font.pixelSize: size * 0.8
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }

    // Loading indicator
    BusyIndicator {
        anchors.centerIn: parent
        size: BusyIndicatorSize.ExtraSmall
        running: useTwemoji && twemojiImage.status === Image.Loading
        visible: running
    }
}
