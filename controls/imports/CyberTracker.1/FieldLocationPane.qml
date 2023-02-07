import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.0
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

Pane {
    id: pane

    property string recordUid
    property string fieldUid
    property alias active: fieldLocation.active

    clip: true
    padding: 0
    contentWidth: parent.width
    contentHeight: content.implicitHeight

    C.FieldBinding {
        id: fieldBinding
        recordUid: pane.recordUid
        fieldUid: pane.fieldUid
    }

    contentItem: ColumnLayout {
        id: content
        width: parent.width
        spacing: 8

        property var location: fieldBinding.value
        property bool hasTimestamp: content.location !== undefined && content.location.ts !== undefined && content.location.ts !== null
        property bool hasAltitude: content.location !== undefined && content.location.z !== undefined && content.location.z !== null && !isNaN(content.location.z)
        property bool hasDirection: content.location !== undefined && content.location.d !== undefined && content.location.d !== null && !isNaN(content.location.d)
        property bool hasAccuracy: content.location !== undefined && content.location.a !== undefined && content.location.a !== null && !isNaN(content.location.a)
        property bool hasSpeed: content.location !== undefined && content.location.s !== undefined && content.location.s !== null && !isNaN(content.location.s)
        property int detailWidth: (parent.width - 16) / 2
        property color color: fieldBinding.isCalculated ? Style.colorCalculated : Material.foreground

        Item { height: 1 }

        C.FieldLocation {
            id: fieldLocation
            Layout.fillWidth: true
            font.pixelSize: App.settings.font18
            fontSizeMode: Label.Fit
            horizontalAlignment: Label.AlignHCenter
            placeholderText: "??"
            recordUid: pane.recordUid
            fieldUid: pane.fieldUid
            onFixCounterChanged: skyplot.progressPercent = (fieldLocation.fixCounter * 100) / fieldBinding.field.fixCount
            visible: !fieldLocation.active
            opacity: fieldBinding.value ? 1.0 : 0.5
            color: content.color
        }

        Grid {
            id: gridData
            columns: 2
            Layout.fillWidth: true
            visible: !fieldLocation.active
            rowSpacing: 8
            columnSpacing: 16

            property int w: (content.width - rowSpacing) / 2

            // Date.
            Label {
                width: gridData.w
                text: qsTr("Date")
                font.pixelSize: App.settings.font14
                horizontalAlignment: Label.AlignRight
                opacity: content.hasTimestamp ? 1.0 : 0.5
                color: content.color
            }
            Label {
                width: gridData.w
                text: content.hasTimestamp ? App.timeManager.getDateText(content.location.ts) : "-"
                font.pixelSize: App.settings.font14
                font.bold: true
                opacity: content.hasTimestamp ? 1.0 : 0.5
                color: content.color
            }

            // Time.
            Label {
                width: gridData.w
                text: qsTr("Time")
                font.pixelSize: App.settings.font14
                horizontalAlignment: Label.AlignRight
                opacity: content.hasTimestamp ? 1.0 : 0.5
                color: content.color
            }
            Label {
                width: gridData.w
                text: content.hasTimestamp ? App.timeManager.getTimeText(content.location.ts) : "-"
                font.pixelSize: App.settings.font14
                font.bold: true
                opacity: content.hasTimestamp ? 1.0 : 0.5
                color: content.color
            }

            // TimeZone.
            Label {
                width: gridData.w
                text: qsTr("Time zone")
                font.pixelSize: App.settings.font14
                horizontalAlignment: Label.AlignRight
                opacity: content.hasTimestamp ? 1.0 : 0.5
                color: content.color
            }
            Label {
                width: gridData.w
                text: {
                    content.location
                    return App.timeManager.timeZoneText
                }
                font.pixelSize: App.settings.font14
                font.bold: true
                opacity: content.hasTimestamp ? 1.0 : 0.5
                color: content.color
            }

            // Altitude.
            Label {
                width: gridData.w
                text: qsTr("Altitude")
                font.pixelSize: App.settings.font14
                horizontalAlignment: Label.AlignRight
                opacity: content.hasAltitude ? 1.0 : 0.5
                color: content.color
            }
            Label {
                width: gridData.w
                text: content.hasAltitude ? App.getDistanceText(content.location.z) : "-"
                font.pixelSize: App.settings.font14
                font.bold: true
                opacity: content.hasAltitude ? 1.0 : 0.5
                color: content.color
            }

            // Direction.
            Label {
                width: gridData.w
                text: qsTr("Direction")
                font.pixelSize: App.settings.font14
                horizontalAlignment: Label.AlignRight
                opacity: content.hasDirection ? 1.0 : 0.5
                color: content.color
            }
            Label {
                width: gridData.w
                text: content.hasDirection ? ((content.location.d | 0) + " " + qsTr("degrees")) : "-"
                font.pixelSize: App.settings.font14
                font.bold: true
                opacity: content.hasDirection ? 1.0 : 0.5
                color: content.color
            }

            // Accuracy.
            Label {
                width: gridData.w
                text: qsTr("Accuracy")
                font.pixelSize: App.settings.font14
                horizontalAlignment: Label.AlignRight
                opacity: content.hasAccuracy ? 1.0 : 0.5
                color: content.color
            }
            Label {
                width: gridData.w
                text: content.hasAccuracy ? App.getDistanceText(content.location.a) : "-"
                font.pixelSize: App.settings.font14
                font.bold: true
                opacity: content.hasAccuracy ? 1.0 : 0.5
                color: content.color
            }

            // Speed.
            Label {
                width: gridData.w
                text: qsTr("Speed")
                font.pixelSize: App.settings.font14
                horizontalAlignment: Label.AlignRight
                opacity: content.hasSpeed ? 1.0 : 0.5
                color: content.color
            }
            Label {
                width: gridData.w
                text: content.hasSpeed ? App.getSpeedText(content.location.s) : "-"
                font.pixelSize: App.settings.font14
                font.bold: true
                opacity: content.hasSpeed ? 1.0 : 0.5
                color: content.color
            }
        }

        C.Skyplot {
            id: skyplot
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            height: fieldLocation.height + gridData.height + content.spacing
            active: fieldLocation.active
            satNumbersVisible: false
            legendVisible: false
            progressVisible: true
            visible: fieldLocation.active
        }

        RowLayout {
            id: buttonRow
            Layout.alignment: Qt.AlignHCenter
            spacing: parent.width / 8
            property var buttonColor: Utils.changeAlpha(Material.foreground, 128)

            RoundButton {
                display: RoundButton.IconOnly
                font.pixelSize: App.settings.font10
                font.capitalization: Font.MixedCase
                icon.source: fieldLocation.active ? "qrc:/icons/cancel.svg" : "qrc:/icons/refresh.svg"
                icon.color: buttonRow.buttonColor
                icon.width: Style.toolButtonSize
                icon.height: Style.toolButtonSize
                onClicked: {
                    fieldBinding.resetValue()
                    fieldLocation.active = !fieldLocation.active
                }
            }

            RoundButton {
                display: RoundButton.IconOnly
                font.pixelSize: App.settings.font10
                font.capitalization: Font.MixedCase
                icon.source: "qrc:/icons/delete_outline.svg"
                icon.color: buttonRow.buttonColor
                icon.width: Style.toolButtonSize
                icon.height: Style.toolButtonSize
                enabled: !fieldLocation.active && !fieldBinding.isEmpty
                opacity: enabled ? 1.0 : 0.5
                onClicked: fieldBinding.resetValue()
            }

            RoundButton {
                display: ToolButton.IconOnly
                font.pixelSize: App.settings.font10
                font.capitalization: Font.MixedCase
                icon.source: "qrc:/icons/edit_location_outline.svg"
                icon.color: buttonRow.buttonColor
                icon.width: Style.toolButtonSize
                icon.height: Style.toolButtonSize
                enabled: !fieldLocation.active
                opacity: enabled ? 1.0 : 0.5
                visible: fieldBinding.field.allowManual
                onClicked: form.pushPage(Qt.resolvedUrl("FieldLocationPage.qml"), { recordUid: pane.recordUid, fieldUid: pane.fieldUid } )
            }
        }
    }
}
