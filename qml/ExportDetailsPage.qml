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

    footer: RowLayout {
        spacing: 0

        C.FooterButton {
            text: qsTr("Delete")
            icon.source: "qrc:/icons/delete_outline.svg"
            onClicked: popupConfirmDelete.open()
        }

        C.FooterButton {
            text: qsTr("Share")
            icon.source: "qrc:/icons/share_variant_outline.svg"
            onClicked: App.sendFile(page.fileInfo.path, page.fileInfo.name)
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
