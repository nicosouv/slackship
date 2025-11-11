import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: workspaceSwitcherPage

    property var _pageStack: pageStack

    SilicaListView {
        id: workspaceList
        anchors.fill: parent
        model: workspaceManager

        header: PageHeader {
            title: qsTr("Switch Workspace")
        }

        delegate: ListItem {
            id: workspaceItem
            contentHeight: Theme.itemSizeMedium

            highlighted: model.isActive

            Row {
                anchors.fill: parent
                anchors.leftMargin: Theme.horizontalPageMargin
                anchors.rightMargin: Theme.horizontalPageMargin
                spacing: Theme.paddingMedium

                Rectangle {
                    width: Theme.iconSizeMedium
                    height: Theme.iconSizeMedium
                    anchors.verticalCenter: parent.verticalCenter
                    radius: Theme.paddingSmall
                    color: Theme.rgba(Theme.highlightBackgroundColor, 0.2)

                    Label {
                        anchors.centerIn: parent
                        text: model.name.charAt(0).toUpperCase()
                        font.pixelSize: Theme.fontSizeLarge
                        font.bold: true
                        color: Theme.highlightColor
                    }
                }

                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - Theme.iconSizeMedium - activeIndicator.width - Theme.paddingMedium * 3
                    spacing: Theme.paddingSmall

                    Label {
                        width: parent.width
                        text: model.name
                        truncationMode: TruncationMode.Fade
                        font.bold: model.isActive
                        color: workspaceItem.highlighted ? Theme.highlightColor : Theme.primaryColor
                    }

                    Label {
                        width: parent.width
                        text: model.domain
                        truncationMode: TruncationMode.Fade
                        font.pixelSize: Theme.fontSizeExtraSmall
                        color: Theme.secondaryColor
                    }
                }

                Icon {
                    id: activeIndicator
                    anchors.verticalCenter: parent.verticalCenter
                    source: "image://theme/icon-m-acknowledge"
                    visible: model.isActive
                    color: Theme.highlightColor
                }
            }

            onClicked: {
                console.log("Switching to workspace:", model.name, "index:", model.index)
                workspaceManager.switchWorkspace(model.index)
                workspaceSwitcherPage._pageStack.pop()
            }

            menu: ContextMenu {
                MenuItem {
                    text: qsTr("Remove")
                    onClicked: {
                        remorse.execute(workspaceItem, qsTr("Removing workspace"), function() {
                            workspaceManager.removeWorkspace(model.index)
                        })
                    }
                }
            }

            RemorseItem {
                id: remorse
            }
        }

        ViewPlaceholder {
            enabled: workspaceList.count === 0
            text: qsTr("No workspaces")
            hintText: qsTr("Add a workspace from login page")
        }

        VerticalScrollDecorator { }
    }
}
