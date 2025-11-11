import QtQuick 2.0
import Sailfish.Silica 1.0

BackgroundItem {
    id: fileAttachment

    property string fileId: ""
    property string fileName: ""
    property string fileType: ""
    property int fileSize: 0
    property string downloadUrl: ""

    width: parent.width
    height: Theme.itemSizeSmall

    Row {
        anchors.fill: parent
        anchors.margins: Theme.paddingSmall
        spacing: Theme.paddingMedium

        Icon {
            id: fileIcon
            anchors.verticalCenter: parent.verticalCenter
            source: getFileIcon(fileType)
            width: Theme.iconSizeMedium
            height: Theme.iconSizeMedium
        }

        Column {
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width - fileIcon.width - downloadIcon.width - Theme.paddingMedium * 3
            spacing: Theme.paddingSmall

            Label {
                width: parent.width
                text: fileName
                truncationMode: TruncationMode.Fade
                font.pixelSize: Theme.fontSizeSmall
                color: fileAttachment.highlighted ? Theme.highlightColor : Theme.primaryColor
            }

            Label {
                text: formatFileSize(fileSize)
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
            }
        }

        Icon {
            id: downloadIcon
            anchors.verticalCenter: parent.verticalCenter
            source: "image://theme/icon-m-cloud-download"
            width: Theme.iconSizeSmall
            height: Theme.iconSizeSmall
        }
    }

    onClicked: {
        if (downloadUrl) {
            fileManager.downloadFile(fileId, downloadUrl, "")
        }
    }

    function getFileIcon(type) {
        // Convert to string and handle empty/undefined values
        var typeStr = type ? type.toString() : ""

        if (typeStr.indexOf("image/") === 0) {
            return "image://theme/icon-m-file-image"
        } else if (typeStr.indexOf("video/") === 0) {
            return "image://theme/icon-m-file-video"
        } else if (typeStr.indexOf("audio/") === 0) {
            return "image://theme/icon-m-file-audio"
        } else if (typeStr === "application/pdf") {
            return "image://theme/icon-m-file-pdf"
        } else if (typeStr.indexOf("document") !== -1 || typeStr.indexOf("text") !== -1) {
            return "image://theme/icon-m-file-document"
        } else {
            return "image://theme/icon-m-file-other"
        }
    }

    function formatFileSize(bytes) {
        if (bytes < 1024) {
            return bytes + " B"
        } else if (bytes < 1024 * 1024) {
            return (bytes / 1024).toFixed(1) + " KB"
        } else if (bytes < 1024 * 1024 * 1024) {
            return (bytes / (1024 * 1024)).toFixed(1) + " MB"
        } else {
            return (bytes / (1024 * 1024 * 1024)).toFixed(1) + " GB"
        }
    }
}
