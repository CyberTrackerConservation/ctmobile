import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import QtGraphicalEffects 1.0
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    property var fileInfo

    header: C.PageHeader {
        text: fileInfo.name
    }

    footer: ColumnLayout {
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: Material.theme === Material.Dark ? "#FFFFFF" : "#000000"
            opacity: Material.theme === Material.Dark ? 0.12 : 0.12
        }

        RowLayout {
            id: buttonRow
            spacing: 0
            Layout.fillWidth: true
            property int buttonCount: 2
            property int buttonWidth: page.width / buttonCount
            property var buttonColor: Material.theme === Material.Dark ? Material.foreground : Material.primary

            ToolButton {
                Layout.preferredWidth: buttonRow.buttonWidth
                Layout.fillHeight: true
                text: qsTr("Delete")
                icon.source: "qrc:/icons/delete_outline.svg"
                font.pixelSize: App.settings.font10
                font.capitalization: Font.MixedCase
                display: Button.TextUnderIcon
                Material.foreground: buttonRow.buttonColor
                onClicked: popupConfirmDelete.open()
            }

            ToolButton {
                Layout.preferredWidth: buttonRow.buttonWidth
                Layout.fillHeight: true
                text: qsTr("Share")
                icon.source: "qrc:/icons/share_variant_outline.svg"
                font.pixelSize: App.settings.font10
                font.capitalization: Font.MixedCase
                display: Button.TextUnderIcon
                Material.foreground: buttonRow.buttonColor
                onClicked: App.sendFile(page.fileInfo.path, page.fileInfo.name)
            }
        }
    }

    C.DetailsPane {
        anchors.fill: parent
        model: fileInfo.model
    }

    C.PopupLoader {
        id: popupConfirmDelete

        popupComponent: Component {
            C.ConfirmPopup {
                text: qsTr("Delete data file?")
                subText: qsTr("This file will be permanently removed.")
                confirmText: qsTr("Yes, delete it")
                confirmDelay: true
                onConfirmed: {
                    App.removeExportFile(page.fileInfo.path)

                    if (typeof(formPageStack) !== "undefined") {
                        form.popPage()
                    } else {
                        appPageStack.pop()
                    }
                }
            }
        }
    }
}
