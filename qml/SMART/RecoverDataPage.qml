import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import QtGraphicalEffects 1.0
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    header: C.PageHeader {
        text: qsTr("Recover Connect data")
    }

    property int fontSize: App.settings.font16

    Pane {
        anchors.fill: parent
        padding: 12
        contentItem: ColumnLayout {
            spacing: 12

            Label {
                Layout.fillWidth: true
                text: qsTr("Use this option to recover data which cannot be uploaded. Once the data is recovered, it must be retrieved by importing from SMART desktop.")
                font.pixelSize: App.settings.font16
                wrapMode: Label.WordWrap
                opacity: 0.75
            }

            C.HorizontalDivider {
                Layout.fillWidth: true
                height: 2
            }

            Item {
                height: 16
            }

            Button {
                Layout.alignment: Qt.AlignHCenter
                font.pixelSize: App.settings.font14
                font.capitalization: Font.MixedCase
                text: qsTr("Recover data")
                onClicked: confirmRecover.open()
            }

            Item {
                Layout.fillHeight: true
            }
        }
    }

    C.ConfirmPopup {
        id: confirmRecover
        text: qsTr("Recover data?")
        confirmText: qsTr("Yes, recover now")
        onConfirmed: {
            busyCover.doWork = recoverAndClearData
            busyCover.start()
        }
    }

    C.MessagePopup {
        id: messagePopup
        onClosed: form.popPage()
    }

    function recoverAndClearData() {
        let success = form.provider.recoverAndClearData()
        if (success) {
            messagePopup.title = qsTr("Success")
            messagePopup.message = qsTr("Data is ready for import from the desktop")
        } else {
            messagePopup.title = qsTr("Error")
            messagePopup.message = qsTr("An error occurred during data export")
        }

        messagePopup.open()
        Globals.patrolChangeCount++
    }
}
