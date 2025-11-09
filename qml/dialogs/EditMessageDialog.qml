import QtQuick 2.0
import Sailfish.Silica 1.0

Dialog {
    id: editMessageDialog

    property string originalText: ""
    property string editedText: ""

    canAccept: messageField.text.trim().length > 0

    onAccepted: {
        editedText = messageField.text.trim()
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingLarge

            DialogHeader {
                title: qsTr("Edit message")
                acceptText: qsTr("Save")
                cancelText: qsTr("Cancel")
            }

            TextArea {
                id: messageField
                width: parent.width
                placeholderText: qsTr("Message text")
                label: qsTr("Edit your message")
                text: originalText
                focus: true

                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: {
                    if (text.trim().length > 0) {
                        accept()
                    }
                }
            }
        }
    }
}
