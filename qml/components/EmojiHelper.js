// Emoji Helper for converting Slack emoji codes to Unicode
.pragma library

// Common Slack emoji mappings to Unicode
var emojiMap = {
    // Smileys & People
    "smile": "😄",
    "smiley": "😃",
    "grin": "😁",
    "laughing": "😆",
    "sweat_smile": "😅",
    "joy": "😂",
    "rofl": "🤣",
    "relaxed": "☺️",
    "blush": "😊",
    "innocent": "😇",
    "slightly_smiling_face": "🙂",
    "upside_down_face": "🙃",
    "wink": "😉",
    "relieved": "😌",
    "heart_eyes": "😍",
    "kissing_heart": "😘",
    "kissing": "😗",
    "kissing_smiling_eyes": "😙",
    "kissing_closed_eyes": "😚",
    "yum": "😋",
    "stuck_out_tongue": "😛",
    "stuck_out_tongue_winking_eye": "😜",
    "stuck_out_tongue_closed_eyes": "😝",
    "neutral_face": "😐",
    "expressionless": "😑",
    "no_mouth": "😶",
    "smirk": "😏",
    "unamused": "😒",
    "grimacing": "😬",
    "lying_face": "🤥",
    "pensive": "😔",
    "sleepy": "😪",
    "drooling_face": "🤤",
    "sleeping": "😴",
    "mask": "😷",
    "face_with_thermometer": "🤒",
    "face_with_head_bandage": "🤕",
    "nauseated_face": "🤢",
    "sneezing_face": "🤧",
    "dizzy_face": "😵",
    "zipper_mouth_face": "🤐",
    "woozy_face": "🥴",
    "exploding_head": "🤯",
    "cowboy_hat_face": "🤠",
    "partying_face": "🥳",
    "sunglasses": "😎",
    "nerd_face": "🤓",
    "face_with_monocle": "🧐",
    "confused": "😕",
    "worried": "😟",
    "slightly_frowning_face": "🙁",
    "frowning_face": "☹️",
    "open_mouth": "😮",
    "hushed": "😯",
    "astonished": "😲",
    "flushed": "😳",
    "pleading_face": "🥺",
    "frowning": "😦",
    "anguished": "😧",
    "fearful": "😨",
    "cold_sweat": "😰",
    "disappointed_relieved": "😥",
    "cry": "😢",
    "sob": "😭",
    "scream": "😱",
    "confounded": "😖",
    "persevere": "😣",
    "disappointed": "😞",
    "sweat": "😓",
    "weary": "😩",
    "tired_face": "😫",
    "yawning_face": "🥱",
    "triumph": "😤",
    "rage": "😡",
    "angry": "😠",
    "smiling_imp": "😈",
    "imp": "👿",
    "skull": "💀",
    "skull_and_crossbones": "☠️",

    // Gestures & Body Parts
    "wave": "👋",
    "raised_back_of_hand": "🤚",
    "raised_hand_with_fingers_splayed": "🖐️",
    "hand": "✋",
    "vulcan_salute": "🖖",
    "ok_hand": "👌",
    "pinching_hand": "🤏",
    "v": "✌️",
    "crossed_fingers": "🤞",
    "love_you_gesture": "🤟",
    "metal": "🤘",
    "call_me_hand": "🤙",
    "point_left": "👈",
    "point_right": "👉",
    "point_up_2": "👆",
    "point_down": "👇",
    "point_up": "☝️",
    "+1": "👍",
    "thumbsup": "👍",
    "-1": "👎",
    "thumbsdown": "👎",
    "fist": "✊",
    "facepunch": "👊",
    "left-facing_fist": "🤛",
    "right-facing_fist": "🤜",
    "clap": "👏",
    "raised_hands": "🙌",
    "open_hands": "👐",
    "palms_up_together": "🤲",
    "handshake": "🤝",
    "pray": "🙏",

    // Hearts & Symbols
    "heart": "❤️",
    "orange_heart": "🧡",
    "yellow_heart": "💛",
    "green_heart": "💚",
    "blue_heart": "💙",
    "purple_heart": "💜",
    "black_heart": "🖤",
    "brown_heart": "🤎",
    "white_heart": "🤍",
    "broken_heart": "💔",
    "two_hearts": "💕",
    "sparkling_heart": "💖",
    "heartpulse": "💗",
    "heartbeat": "💓",
    "revolving_hearts": "💞",
    "cupid": "💘",
    "gift_heart": "💝",
    "heart_decoration": "💟",
    "peace_symbol": "☮️",
    "star": "⭐",
    "sparkles": "✨",
    "fire": "🔥",
    "100": "💯",
    "zap": "⚡",
    "boom": "💥",
    "tada": "🎉",
    "confetti_ball": "🎊",

    // Objects
    "computer": "💻",
    "keyboard": "⌨️",
    "phone": "📱",
    "email": "📧",
    "memo": "📝",
    "calendar": "📅",
    "clock": "🕐",
    "hourglass": "⌛",
    "bulb": "💡",
    "book": "📖",
    "pencil2": "✏️",
    "mag": "🔍",
    "lock": "🔒",
    "unlock": "🔓",
    "key": "🔑",
    "bell": "🔔",
    "bookmark": "🔖",
    "link": "🔗",
    "paperclip": "📎",
    "rocket": "🚀",
    "airplane": "✈️",
    "house": "🏠",

    // Nature
    "sunny": "☀️",
    "cloud": "☁️",
    "umbrella": "☂️",
    "snowflake": "❄️",
    "rainbow": "🌈",
    "tree": "🌳",
    "herb": "🌿",
    "four_leaf_clover": "🍀",
    "seedling": "🌱",
    "rose": "🌹",
    "tulip": "🌷",
    "bug": "🐛",
    "bee": "🐝",
    "cat": "🐱",
    "dog": "🐶",
    "mouse": "🐭",
    "hamster": "🐹",
    "rabbit": "🐰",
    "fox_face": "🦊",
    "bear": "🐻",
    "panda_face": "🐼",

    // Food
    "apple": "🍎",
    "banana": "🍌",
    "grapes": "🍇",
    "strawberry": "🍓",
    "watermelon": "🍉",
    "pizza": "🍕",
    "hamburger": "🍔",
    "fries": "🍟",
    "hotdog": "🌭",
    "taco": "🌮",
    "burrito": "🌯",
    "cake": "🍰",
    "cookie": "🍪",
    "ice_cream": "🍦",
    "doughnut": "🍩",
    "coffee": "☕",
    "beer": "🍺",
    "wine_glass": "🍷",
    "cocktail": "🍸",

    // Flags (common ones)
    "flag-us": "🇺🇸",
    "flag-gb": "🇬🇧",
    "flag-fr": "🇫🇷",
    "flag-de": "🇩🇪",
    "flag-es": "🇪🇸",
    "flag-it": "🇮🇹",
    "flag-jp": "🇯🇵",
    "flag-cn": "🇨🇳",
    "flag-kr": "🇰🇷",
    "flag-ca": "🇨🇦",
    "flag-au": "🇦🇺",
    "flag-br": "🇧🇷",
    "flag-in": "🇮🇳",
    "flag-ru": "🇷🇺",

    // Other
    "arrow_right": "➡️",
    "arrow_left": "⬅️",
    "arrow_up": "⬆️",
    "arrow_down": "⬇️",
    "white_check_mark": "✅",
    "x": "❌",
    "warning": "⚠️",
    "bangbang": "‼️",
    "question": "❓",
    "information_source": "ℹ️",
    "recycle": "♻️",
    "copyright": "©️",
    "registered": "®️",
    "tm": "™️"
}

// Convert Slack emoji codes like :smile: to Unicode emoji
function convertEmoji(text) {
    if (!text) return text

    return text.replace(/:([a-z0-9_+-]+):/g, function(match, emojiCode) {
        return emojiMap[emojiCode] || match
    })
}

// Convert a reaction object to emoji
function reactionToEmoji(reactionName) {
    return emojiMap[reactionName] || ":" + reactionName + ":"
}

// Convert Unicode emoji back to Slack reaction name
function emojiToReactionName(emoji) {
    // Search through the map for the Unicode emoji
    for (var name in emojiMap) {
        if (emojiMap[name] === emoji) {
            return name
        }
    }
    // If not found, return the emoji as-is (might be a custom emoji)
    return emoji
}

// Convert Unicode emoji to Twemoji CDN URL
function emojiToTwemojiUrl(emoji, size) {
    if (!emoji) return ""

    // Default size is 72x72
    size = size || 72

    // Convert emoji string to Unicode codepoints
    var codePoints = []
    for (var i = 0; i < emoji.length; i++) {
        var code = emoji.charCodeAt(i)

        // Handle surrogate pairs for emojis beyond U+FFFF
        if (code >= 0xD800 && code <= 0xDBFF && i + 1 < emoji.length) {
            var high = code
            var low = emoji.charCodeAt(i + 1)
            if (low >= 0xDC00 && low <= 0xDFFF) {
                // Calculate actual codepoint from surrogate pair
                var codepoint = ((high - 0xD800) * 0x400) + (low - 0xDC00) + 0x10000
                codePoints.push(codepoint.toString(16))
                i++ // Skip the low surrogate
                continue
            }
        }

        // Skip variation selectors (U+FE00-U+FE0F) and zero-width joiners (U+200D)
        if (code === 0xFE0E || code === 0xFE0F || code === 0x200D) {
            if (code === 0x200D) {
                // Keep ZWJ for multi-part emojis (like family emojis)
                codePoints.push(code.toString(16))
            }
            continue
        }

        codePoints.push(code.toString(16))
    }

    // Join codepoints with hyphens
    var codepointStr = codePoints.join("-")

    // Build Twemoji CDN URL (using version 14.0.2)
    // Available sizes: 72x72, svg
    var twemojiSize = "72x72"  // Fixed size, since Twemoji only provides 72x72 PNGs
    return "https://cdn.jsdelivr.net/gh/twitter/twemoji@14.0.2/assets/" + twemojiSize + "/" + codepointStr + ".png"
}

// Get Twemoji URL from Slack reaction name
function reactionNameToTwemojiUrl(reactionName, size) {
    var emoji = emojiMap[reactionName]
    if (!emoji) {
        // If not in our map, it might be a custom emoji or unsupported
        return ""
    }
    return emojiToTwemojiUrl(emoji, size)
}

// Format Slack mrkdwn to HTML (for RichText)
// userLookup is an optional function(userId) that returns the username
function formatSlackText(text, userLookup) {
    if (!text) return text

    // First, escape HTML entities
    text = text.replace(/&/g, "&amp;")
             .replace(/</g, "&lt;")
             .replace(/>/g, "&gt;")

    // Convert Slack link format: <url|text> or <url>
    text = text.replace(/&lt;(https?:\/\/[^\|\>]+)\|([^\>]+)&gt;/g, '<a href="$1">$2</a>')
    text = text.replace(/&lt;(https?:\/\/[^\>]+)&gt;/g, '<a href="$1">$1</a>')

    // Convert special mentions
    text = text.replace(/&lt;!channel&gt;/g, '<b>@channel</b>')
    text = text.replace(/&lt;!here&gt;/g, '<b>@here</b>')
    text = text.replace(/&lt;!everyone&gt;/g, '<b>@everyone</b>')

    // Convert user mentions: <@USERID> -> @username
    if (userLookup) {
        text = text.replace(/&lt;@([A-Z0-9]+)&gt;/g, function(match, userId) {
            var userName = userLookup(userId)
            return '<b>@' + (userName || userId) + '</b>'
        })
    } else {
        text = text.replace(/&lt;@([A-Z0-9]+)&gt;/g, '<b>@$1</b>')
    }

    // Convert channel mentions: <#CHANNELID|channelname> or <#CHANNELID>
    text = text.replace(/&lt;#[A-Z0-9]+\|([^\>]+)&gt;/g, '<b>#$1</b>')
    text = text.replace(/&lt;#([A-Z0-9]+)&gt;/g, '<b>#channel</b>')

    // Convert Slack markdown to HTML with inline styles (for better Qt support)
    // Strikethrough: ~text~ -> styled span
    text = text.replace(/~([^~]+)~/g, '<span style="text-decoration: line-through">$1</span>')

    // Bold: *text* -> <b>text</b>
    text = text.replace(/\*([^\*]+)\*/g, '<b>$1</b>')

    // Italic: _text_ -> <i>text</i>
    text = text.replace(/(^|[\s\(])_([^_]+)_([\s\.\,\!\?\)]|$)/g, '$1<i>$2</i>$3')

    // Code: `text` -> styled span with monospace font
    text = text.replace(/`([^`]+)`/g, '<span style="font-family: monospace; background-color: rgba(128,128,128,0.2); padding: 2px 4px; border-radius: 3px">$1</span>')

    // Convert emojis
    text = convertEmoji(text)

    return text
}
