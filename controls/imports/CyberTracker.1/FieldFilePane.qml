import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Dialogs 1.2
import QtGraphicalEffects 1.0
import QtQuick.Controls.Material 2.12
import QtWebView 1.1
import Qt.labs.settings 1.0 as Labs
import CyberTracker 1.0 as C

ItemDelegate {
    id: root

    property string recordUid
    property string fieldUid

    C.FieldBinding {
        id: fieldBinding
        recordUid: root.recordUid
        fieldUid: root.fieldUid
    }

    Labs.Settings {
        fileName: App.iniPath
        category: "FieldFileDialog"
        property alias fileDialogFolder: fileDialog.folder
    }

    FileDialog {
        id: fileDialog
        folder: shortcuts.pictures
        onAccepted: {
            let filename = App.copyToMedia(fileDialog.fileUrl)
            if (filename !== "") {
                fieldBinding.setValue(filename)
                return
            }

            showError(qsTr("Failed to copy"))
        }
    }

    Component {
        id: webViewComponent

        WebView {
            anchors.fill: parent
            url: App.getMediaFileUrl(fieldBinding.value)
            onLoadingChanged: {
                if (loadRequest.status === WebView.LoadStoppedStatus) {
                    let html = "<!DOCTYPE html><html><head><style>.C { "
                    html += "position: absolute; top:50%; left: 50%; font-size: " + App.settings.fontSize.toString() + "%; "
                    html += "transform: translateX(-50%) translateY(-50%) "
                    html += "}</style></head><body><span class=\"C\">"
                    html += qsTr("Preview not available")
                    html += "</span></body></html>"
                    loadHtml(html, "")
                }
            }
        }
    }

    padding: 0
    contentItem: Item {
        width: root.width
        height: root.height

        ColumnLayout {
            spacing: App.scaleByFontSize(4)
            anchors.fill: parent
            visible: !fieldBinding.isEmpty

            Label {
                Layout.fillWidth: true
                visible: !fieldBinding.isEmpty
                horizontalAlignment: Label.AlignHCenter
                font.pixelSize: App.settings.font14
                fontSizeMode: Label.Fit
                text: fieldBinding.displayValue
                opacity: 0.5
            }

            Loader {
                id: webViewLoader
                Layout.fillWidth: true
                Layout.fillHeight: true
                sourceComponent: fieldBinding.isEmpty ? null : webViewComponent
                visible: !fieldBinding.isEmpty
            }
        }

        RoundButton {
            anchors {
                bottom: parent.bottom
                horizontalCenter: parent.horizontalCenter
            }

            visible: !fieldBinding.isEmpty
            icon.source: "qrc:/icons/delete_outline.svg"
            icon.width: Style.toolButtonSize
            icon.height: Style.toolButtonSize
            icon.color: Utils.changeAlpha(Material.foreground, 128)
            onClicked: {
                popupConfirmReset.open()
            }
        }

        SquareIcon {
            anchors.centerIn: parent
            visible: fieldBinding.isEmpty
            source: "qrc:/icons/file_plus_outline.svg"
            size: Style.iconSize64
            opacity: 0.5
            recolor: true
        }
    }

    onClicked: {
        fileDialog.nameFilters = Utils.buildFileDialogFilters(fieldBinding.fieldName, fieldBinding.field.accept)
        fileDialog.open()
    }

    ConfirmPopup {
        id: popupConfirmReset
        text: qsTr("Clear file?")
        subText: qsTr("The original will not be deleted.")
        confirmText: qsTr("Yes, remove it")
        onConfirmed: {
            App.removeMedia(fieldBinding.value)
            fieldBinding.resetValue()
        }
    }
}
