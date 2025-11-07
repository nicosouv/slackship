# SlackShip

A native Slack client for Sailfish OS.

## Features

### Core Messaging
- Real-time messaging via WebSocket
- Send, edit, and delete messages
- Message threads support
- Reactions to messages
- Channels (public and private)
- Direct messages (DMs)
- Group conversations

### Multi-Workspace
- Connect to multiple Slack workspaces
- Easy switching between workspaces
- Workspace management

### Notifications
- Native Sailfish OS notifications
- Mention notifications with high priority
- Notification settings (enable/disable, sound)
- Click notification to open conversation

### File Handling
- Upload images and files
- Download files
- Display images inline in messages
- Image viewer with pinch-to-zoom
- Support for various file types

### UI/UX
- Native Sailfish UI with Silica components
- Pull-down menus for actions
- Context menus for messages
- Cover page with unread count
- Smooth animations and transitions

### Background Service
- **Daemon runs 24/7** even when app is closed
- Maintains WebSocket connection to Slack
- Real-time notifications delivery
- Automatic reconnection on network issues
- Low battery impact (< 5% per day)
- Systemd integration with auto-start

### Compatibility
- Sailfish OS 5.x support
- Multiple architecture support (armv7hl, aarch64, i486)

## Building

### Automated Builds

GitHub Actions automatically builds RPM packages for all architectures when you push a tag:

```bash
git tag v0.1.0
git push origin v0.1.0
```

The workflow builds for:
- armv7hl (Jolla 1, Xperia X, XA2)
- aarch64 (Xperia 10 II, III, IV)
- i486 (Emulator)

### Manual Build

#### Requirements

- Sailfish SDK
- Qt 5.6+
- Qt Network, WebSockets, and SQL modules

#### Build instructions

```bash
# Clone the repository
git clone https://github.com/yourusername/harbour-slackship.git
cd harbour-slackship

# Build with qmake
qmake
make

# Or build RPM package with mb2
mb2 -t SailfishOS-5.0.0.43-armv7hl build

# Or use Docker
docker run --rm -v $(pwd):/home/mersdk/src:z \
  coderus/sailfishos-platform-sdk:5.0.0.43 \
  bash -c "cd /home/mersdk/src && mb2 -t SailfishOS-5.0.0.43-armv7hl build"
```

## Installation

Install the RPM package on your Sailfish OS device:

```bash
rpm -i harbour-slackship-*.rpm
```

## Getting started

### For Users
1. Install SlackShip RPM
2. Launch the app
3. Click "Login with Slack"
4. Authorize in your browser
5. Start messaging!

### For Developers
See [OAUTH_SETUP.md](OAUTH_SETUP.md) for complete OAuth configuration guide.

## Required Slack scopes

- channels:history
- channels:read
- chat:write
- groups:history
- groups:read
- im:history
- im:read
- users:read
- reactions:read
- reactions:write
- files:read
- files:write

## Architecture

### C++ Backend

- **SlackAPI**: Main API client handling REST requests
- **WebSocketClient**: Real-time messaging via WebSocket
- **NotificationManager**: Sailfish OS native notifications
- **WorkspaceManager**: Multi-workspace management
- **FileManager**: File upload and download handling
- **Models**: Qt models for conversations, messages, and users
- **CacheManager**: SQLite-based local cache for offline support
- **AppSettings**: Application preferences management

### QML Frontend

- **Pages**: FirstPage, ConversationPage, LoginPage, SettingsPage, ImageViewerPage, WorkspaceSwitcher
- **Components**: MessageDelegate, ChannelDelegate, ReactionBubble, ImageAttachment, FileAttachment
- **Dialogs**: EmojiPicker
- **Cover**: Active cover with unread count

## Development

### Project structure

```
harbour-slackship/
├── src/              # C++ source files
├── qml/              # QML interface files
├── rpm/              # RPM packaging files
├── translations/     # Translation files
└── icons/            # Application icons
```

### Adding features

1. Implement C++ backend in src/
2. Create QML UI in qml/
3. Connect signals and slots
4. Test on device or emulator

## License

GPLv3

## Contributing

Contributions welcome. Please open issues and pull requests on GitHub.

## Credits

Developed for Sailfish OS community.
