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

    QtObject {
        id: internal
        property bool isArea: fieldBinding.fieldType === "AreaField"
    }

    padding: 0

    contentItem: Item {
        width: root.width
        height: root.height

        ColumnLayout {
            spacing: 0
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

            Image {
                Layout.fillWidth: true
                Layout.fillHeight: true
                source: fieldBinding.value ? "data:image/svg+xml;utf8," + Utils.renderPointsToSvg(fieldBinding.value, "black", App.scaleByFontSize(4), internal.isArea) : ""
                sourceSize.width: root.width
                fillMode: Image.PreserveAspectFit
                layer {
                    enabled: true
                    effect: ColorOverlay {
                        color: Material.foreground
                    }
                }
            }
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
            source: internal.isArea ? "qrc:/icons/shape_polygon_plus.svg" : "qrc:/icons/vector_polyline_plus.svg"
            size: Style.iconSize64
            opacity: 0.5
        }
    }

    onClicked: form.pushPage(Qt.resolvedUrl("FieldLinePage.qml"), { recordUid: fieldBinding.recordUid, fieldUid: fieldBinding.fieldUid } )

    ConfirmPopup {
        id: popupConfirmReset
        text: internal.isArea ? qsTr("Clear area?") : qsTr("Clear line?")
        confirmText: qsTr("Yes, clear all points")
        onConfirmed: {
            fieldBinding.resetValue()
        }
    }
}
