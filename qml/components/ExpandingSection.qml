import QtQuick 2.0
import Sailfish.Silica 1.0

Column {
    id: expandingSection

    property string title: ""
    property bool expanded: true
    property alias content: contentLoader

    width: parent.width
    spacing: 0

    // Header that toggles expand/collapse
    BackgroundItem {
        id: header
        width: parent.width
        height: Theme.itemSizeSmall

        onClicked: {
            expanded = !expanded
        }

        Row {
            anchors.fill: parent
            anchors.leftMargin: Theme.horizontalPageMargin
            anchors.rightMargin: Theme.horizontalPageMargin
            spacing: Theme.paddingMedium

            // Expand/collapse icon
            Icon {
                id: expandIcon
                source: "image://theme/icon-m-right"
                width: Theme.iconSizeSmall
                height: Theme.iconSizeSmall
                anchors.verticalCenter: parent.verticalCenter

                rotation: expanded ? 90 : 0

                Behavior on rotation {
                    NumberAnimation {
                        duration: 200
                        easing.type: Easing.InOutQuad
                    }
                }
            }

            // Section title
            Label {
                text: title
                font.pixelSize: Theme.fontSizeMedium
                font.bold: true
                color: header.highlighted ? Theme.highlightColor : Theme.primaryColor
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - expandIcon.width - parent.spacing
            }
        }
    }

    // Separator line
    Separator {
        width: parent.width
        color: Theme.secondaryHighlightColor
    }

    // Content area
    Loader {
        id: contentLoader
        width: parent.width
        visible: expanded
        height: expanded ? (item ? item.height : 0) : 0

        Behavior on height {
            NumberAnimation {
                duration: 200
                easing.type: Easing.InOutQuad
            }
        }
    }
}
