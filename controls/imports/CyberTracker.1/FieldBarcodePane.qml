import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.0
import QtQuick.Controls.Material 2.12
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

    padding: 0
    contentItem: Item {
        width: root.width
        height: root.height

        Label {
            anchors.fill: parent
            font.pixelSize: App.settings.font16
            font.bold: true
            wrapMode: Label.Wrap
            text: fieldBinding.displayValue
            visible: !fieldBinding.isEmpty
            horizontalAlignment: Label.AlignHCenter
            verticalAlignment: Label.AlignVCenter
        }

        ToolButton {
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
            source: "qrc:/icons/qrcode_scan.svg"
            size: Style.iconSize64
            opacity: 0.5
        }
    }

    onClicked: {
        form.pushPage(Qt.resolvedUrl("FieldBarcodePage.qml"), { recordUid: fieldBinding.recordUid, fieldUid: fieldBinding.fieldUid } )
    }

    ConfirmPopup {
        id: popupConfirmReset
        text: qsTr("Clear code?")
        confirmText: qsTr("Yes, clear it")
        onConfirmed: {
            fieldBinding.resetValue()
        }
    }
}
