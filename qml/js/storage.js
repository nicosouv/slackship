// Local storage utilities for Lagoon

.pragma library

// Format timestamp to human readable format
function formatTimestamp(timestamp) {
    var date = new Date(parseFloat(timestamp) * 1000);
    var now = new Date();
    var diff = now - date;

    // Less than 1 minute
    if (diff < 60000) {
        return qsTr("just now");
    }

    // Less than 1 hour
    if (diff < 3600000) {
        var minutes = Math.floor(diff / 60000);
        return qsTr("%1 minutes ago").arg(minutes);
    }

    // Less than 24 hours
    if (diff < 86400000) {
        var hours = Math.floor(diff / 3600000);
        return qsTr("%1 hours ago").arg(hours);
    }

    // Format as date
    return Qt.formatDate(date, "MMM d");
}

// Get initials from name
function getInitials(name) {
    if (!name || name.length === 0) return "?";

    var parts = name.split(" ");
    if (parts.length === 1) {
        return name.substring(0, 2).toUpperCase();
    }

    return (parts[0].charAt(0) + parts[1].charAt(0)).toUpperCase();
}

// Parse Slack markdown to basic formatting
function parseMarkdown(text) {
    if (!text) return "";

    // Bold: *text* -> <b>text</b>
    text = text.replace(/\*([^\*]+)\*/g, "<b>$1</b>");

    // Italic: _text_ -> <i>text</i>
    text = text.replace(/_([^_]+)_/g, "<i>$1</i>");

    // Strikethrough: ~text~ -> <s>text</s>
    text = text.replace(/~([^~]+)~/g, "<s>$1</s>");

    // Code: `text` -> <code>text</code>
    text = text.replace(/`([^`]+)`/g, "<code>$1</code>");

    return text;
}

// Validate Slack token format
function isValidToken(token) {
    if (!token || token.length === 0) return false;

    // Slack tokens start with xoxp-, xoxb-, or xoxa-
    return token.match(/^xox[pba]-[0-9]+-[0-9]+-[0-9]+-[a-z0-9]+$/i) !== null;
}
