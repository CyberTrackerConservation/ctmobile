import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.0
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

Pane {
    id: pane

    property alias recordUid: fieldBinding.recordUid
    property alias fieldUid: fieldBinding.fieldUid

    clip: true
    contentWidth: parent.width
    contentHeight: paneContent.implicitHeight

    Binding { target: background; property: "color"; value: Style.colorContent }

    C.FieldBinding {
        id: fieldBinding
    }

    C.ConfirmPopup {
        id: popupConfirmReset
        text: qsTr("Clear all points?")
        confirmText: qsTr("Yes, clear them")
        onConfirmed: fieldBinding.resetValue()
    }

    contentItem: ColumnLayout {
        id: paneContent
        anchors.fill: parent

        Item {
            height: 8
        }

        Label {
            Layout.fillWidth: true
            horizontalAlignment: Label.AlignHCenter
            font.pixelSize: App.settings.font14
            text: fieldBinding.isEmpty ? qsTr("Empty") : fieldBinding.displayValue
            opacity: fieldBinding.isEmpty ? 0.5 : 1.0
        }

        Item {
            height: 8
            visible: !fieldBinding.isEmpty
        }

        Image {
            id: imageValue
            Layout.fillWidth: true
            source: fieldBinding.value ? "data:image/svg+xml;utf8," + Utils.renderPointsToSvg(fieldBinding.value, "black", 5, fieldBinding.fieldType === "AreaField") : ""
            sourceSize.height: 128
            visible: !fieldBinding.isEmpty
            fillMode: Image.PreserveAspectFit
            layer {
                enabled: true
                effect: ColorOverlay {
                    color: Material.foreground
                }
            }
        }

        Item {
            Layout.fillWidth: true
            height: 8
            C.HorizontalDivider {}
        }

        RowLayout {
            id: buttonRow
            Layout.preferredWidth: parent.width * 0.75
            Layout.alignment: Qt.AlignHCenter
            spacing: 0

            property int buttonWidth: buttonRow.width / 2
            property var buttonColor: Material.foreground

            ToolButton {
                Layout.preferredWidth: buttonRow.buttonWidth
                text: {
                    if (fieldBinding.fieldType === "LineField") {
                        return fieldBinding.isEmpty ? qsTr("Start line") : qsTr("Edit line")
                    } else {
                        return fieldBinding.isEmpty ? qsTr("Start area") : qsTr("Edit area")
                    }
                }

                font.capitalization: Font.AllUppercase
                font.pixelSize: App.settings.font12
                icon.source: "qrc:/icons/edit_location_outline.svg"
                opacity: enabled ? 1.0 : 0.5
                icon.color: buttonRow.buttonColor
                onClicked: form.pushPage(Qt.resolvedUrl("FieldLinePage.qml"), { recordUid: fieldBinding.recordUid, fieldUid: fieldBinding.fieldUid } )
            }

            ToolButton {
                Layout.preferredWidth: buttonRow.buttonWidth
                text: qsTr("Reset")
                icon.source: "qrc:/icons/delete_outline.svg"
                icon.color: buttonRow.buttonColor
                enabled: !fieldBinding.isEmpty
                opacity: enabled ? 1.0 : 0.5
                font.capitalization: Font.AllUppercase
                font.pixelSize: App.settings.font12
                onClicked: popupConfirmReset.open()
            }
        }
    }
}
