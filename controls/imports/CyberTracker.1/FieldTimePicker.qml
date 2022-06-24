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
    property string value: App.timeManager.currentDateTimeISO()
    signal valueUpdated(var value)

    background: Rectangle { color: Style.colorContent }
    contentWidth: row.implicitWidth
    contentHeight: row.implicitHeight
    padding: 10

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
        padding: 10
        spacing: 10

        Tumbler {
            id: hoursTumbler
            width: root.fontPixelSize * 3
            model: 12
            delegate: delegateComponent
            onCurrentIndexChanged: snapTime()
        }

        Tumbler {
            id: minutesTumbler
            width: root.fontPixelSize * 3
            model: 60
            delegate: delegateComponent
            onCurrentIndexChanged: snapTime()
        }

        Tumbler {
            id: amPmTumbler
            width: root.fontPixelSize * 3
            model: [App.locale.amText, App.locale.pmText]
            delegate: delegateComponent
            onCurrentIndexChanged: snapTime()
        }
    }

    QtObject {
        id: internal
        property bool mutex: true
    }

    Component.onCompleted: {
        value = fieldBinding.getValue(value)
        fieldBinding.setValue(value)
        setupForTime(value)
    }

    function formatText(count, modelData) {
        let data = modelData
        if (count === 12) {
            data = modelData === 0 ? 12 : modelData
        }

        return data.toString().length < 2 ? "0" + data : data;
    }

    function setupForTime(value) {
        internal.mutex = true
        try {
            let time = Utils.decodeTimestamp(value)

            let hoursIndex = time.h % 12
            hoursTumbler.positionViewAtIndex(hoursIndex, Tumbler.Center)
            hoursTumbler.currentIndex = hoursIndex

            let minutesIndex = time.m
            minutesTumbler.positionViewAtIndex(minutesIndex, Tumbler.Center)
            minutesTumbler.currentIndex = minutesIndex

            let amPmIndex = time.h < 12 ? 0 : 1
            amPmTumbler.positionViewAtIndex(amPmIndex, Tumbler.Beginning)
            amPmTumbler.currentIndex = amPmIndex

        } finally {
            internal.mutex = false
        }
    }

    function snapTime() {
        if (internal.mutex) {
            return
        }

        value = fieldBinding.value

        let hours = hoursTumbler.currentIndex + (amPmTumbler.currentIndex === 1 ? 12 : 0)
        let minutes = minutesTumbler.currentIndex
        value = Utils.changeTime(value, hours, minutes, 0)
        valueUpdated(value)

        fieldBinding.setValue(value)
    }
}
