import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtGraphicalEffects 1.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

Pane {
    id: root

    property alias recordUid: fieldBinding.recordUid
    property alias fieldUid: fieldBinding.fieldUid

    property int fontPixelSize: App.settings.font16
    signal valueUpdated(var value)

    Material.background: colorContent || Style.colorContent
    contentWidth: row.implicitWidth
    contentHeight: row.implicitHeight
    padding: App.scaleByFontSize(10)

    C.FieldBinding {
        id: fieldBinding
    }

    Component {
        id: delegateComponent

        Label {
            text: formatText(Tumbler.tumbler.count, modelData)
            opacity: 1.0 - Math.abs(Tumbler.displacement) / (Tumbler.tumbler.visibleItemCount / 2)
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: root.fontPixelSize
            font.bold: Tumbler.tumbler.currentIndex === model.index
            color: Tumbler.tumbler.currentIndex === model.index ? Material.accent : Material.foreground
        }
    }

    Rectangle {
        id: rowSelection
        height: parent.height * (1 / hoursTumbler.visibleItemCount)
        width: parent.width
        anchors.centerIn: parent
        color: Style.darkTheme ? "#353535" : "white"
    }

    DropShadow {
        anchors.fill: rowSelection
        horizontalOffset: 3
        verticalOffset: 3
        samples: 17
        color: "#60000000"
        source: rowSelection
    }

    Row {
        id: row
        padding: App.scaleByFontSize(10)
        spacing: App.scaleByFontSize(10)

        Tumbler {
            id: hoursTumbler
            width: root.fontPixelSize * 3
            model: 12
            delegate: delegateComponent
            currentIndex: Utils.decodeTimestamp(fieldBinding.getValue(internal.initialValue)).h % 12
            onCurrentIndexChanged: snapTime()
        }

        Tumbler {
            id: minutesTumbler
            width: root.fontPixelSize * 3
            model: 60
            delegate: delegateComponent
            currentIndex: Utils.decodeTimestamp(fieldBinding.getValue(internal.initialValue)).m
            onCurrentIndexChanged: snapTime()
        }

        Tumbler {
            id: amPmTumbler
            width: root.fontPixelSize * 3
            model: [App.locale.amText, App.locale.pmText]
            delegate: delegateComponent
            currentIndex: Utils.decodeTimestamp(fieldBinding.getValue(internal.initialValue)).h < 12 ? 0 : 1
            onCurrentIndexChanged: snapTime()
        }
    }

    QtObject {
        id: internal
        property bool mutex: true
        property string initialValue: App.timeManager.currentDateTimeISO()
    }

    Component.onCompleted: {
        let value = fieldBinding.getValue(internal.initialValue)
        fieldBinding.setValue(value)
        internal.mutex = false
    }

    function formatText(count, modelData) {
        let data = modelData
        if (count === 12) {
            data = modelData === 0 ? 12 : modelData
        }

        return data.toString().length < 2 ? "0" + data : data;
    }

    function snapTime() {
        if (internal.mutex) {
            return
        }

        let value = fieldBinding.value

        let hours = hoursTumbler.currentIndex + (amPmTumbler.currentIndex === 1 ? 12 : 0)
        let minutes = minutesTumbler.currentIndex
        value = Utils.changeTime(value, hours, minutes, 0)
        valueUpdated(value)

        fieldBinding.setValue(value)
    }
}
