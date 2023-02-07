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

    property date minDate: new Date(1920, 0, 1)
    property date maxDate: new Date(2100, 11, 31)
    property int fontPixelSize: App.settings.font16
    signal valueUpdated(var value)

    Material.background: colorContent || Style.colorContent
    contentWidth: row.implicitWidth
    contentHeight: row.implicitHeight
    padding: 10

    C.FieldBinding {
        id: fieldBinding
    }

    Rectangle {
        id: rowSelection
        height: parent.height * (1 / yearTumbler.visibleItemCount)
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
            id: yearTumbler
            width: root.fontPixelSize * 3
            model: root.maxDate.getFullYear() - root.minDate.getFullYear() + 1
            delegate: Label {
                color: index === yearTumbler.currentIndex ? Material.accent : Material.foreground
                opacity: 1.0 - Math.abs(Tumbler.displacement) / (Tumbler.tumbler.visibleItemCount / 2)
                text: currentYear(modelData)
                font.pixelSize: root.fontPixelSize
                font.bold: index === yearTumbler.currentIndex
                horizontalAlignment: Label.AlignHCenter
                verticalAlignment: Label.AlignVCenter

                function displacementItemOpacity(displacement, visibleItemCount) {
                    return 1.0 - Math.abs(displacement) / (visibleItemCount / 2)
                }
            }

            wrap: false
            onCurrentIndexChanged: snapDate()
        }

        Tumbler {
            id: monthTumbler
            width: root.fontPixelSize * 3
            model: {
                var months = 12
                if (isMaxYear()) {
                    months = root.maxDate.getMonth() + 1
                }
                if (isMinYear()) {
                    months -= root.minDate.getMonth()
                }
                return months
            }

            delegate: Label {
                color: index === parent.currentIndex ? Material.accent : Material.foreground
                opacity: 1.0 - Math.abs(Tumbler.displacement) / (Tumbler.tumbler.visibleItemCount / 2)
                text: App.locale.standaloneMonthName(currentMonth(modelData), Locale.ShortFormat)
                font.capitalization: Font.Capitalize
                font.pixelSize: root.fontPixelSize
                font.bold: index === parent.currentIndex
                horizontalAlignment: Label.AlignHCenter
                verticalAlignment: Text.AlignVCenter

                function displacementItemOpacity(displacement, visibleItemCount) {
                    return 1.0 - Math.abs(displacement) / (visibleItemCount / 2)
                }
            }

            onCurrentIndexChanged: snapDate()
        }

        Tumbler {
            id: dayTumbler
            width: root.fontPixelSize * 3
            model: {
                var days = daysInMonth(currentYear(yearTumbler.currentIndex), currentMonth(monthTumbler.currentIndex))
                if(isMaxYear() && isMaxMonth()) {
                    days = root.maxDate.getDate()
                }
                if(isMinYear() && isMinMonth()) {
                    days -= (root.minDate.getDate() - 1)
                }
                return days
            }

            delegate: Label {
                color: index === parent.currentIndex ? Material.accent : Material.foreground
                opacity: 1.0 - Math.abs(Tumbler.displacement) / (Tumbler.tumbler.visibleItemCount / 2)
                text: currentDay(modelData)
                font.pixelSize: root.fontPixelSize
                font.bold: index === parent.currentIndex
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter

                function displacementItemOpacity(displacement, visibleItemCount) {
                    return 1.0 - Math.abs(displacement) / (visibleItemCount / 2)
                }
            }

            onCurrentIndexChanged: snapDate()
        }
    }

    QtObject {
        id: internal
        property bool mutex: true
        property string initialValue: App.timeManager.currentDateTimeISO()
    }

    Component.onCompleted: {
        if (fieldBinding.fieldType !== "DateField" && fieldBinding.fieldType !== "DateTimeField") {
            return
        }

        let value = fieldBinding.getValue(internal.initialValue)
        fieldBinding.setValue(value)
        if (fieldBinding.field.minDate !== "") {
            minDate = fieldBinding.field.minDate
        }
        if (fieldBinding.field.maxDate !== "") {
            maxDate = fieldBinding.field.maxDate
        }

        setupForDate(value)
    }

    function isMinYear() {
        return yearTumbler.currentIndex === 0
    }

    function isMaxYear() {
        return yearTumbler.currentIndex === (yearTumbler.model - 1)
    }

    function isMinMonth() {
        return monthTumbler.currentIndex === 0
    }

    function isMaxMonth() {
        return monthTumbler.currentIndex === (monthTumbler.model - 1)
    }

    function currentYear(yearIndex) {
        return yearIndex + root.minDate.getFullYear()
    }

    function currentMonth(monthIndex) {
        if(yearTumbler.currentIndex === 0) {
            return monthIndex + root.minDate.getMonth()
        } else {
            return monthIndex
        }
    }

    function currentDay(dayIndex) {
        if(yearTumbler.currentIndex === 0 && monthTumbler.currentIndex === 0) {
            return dayIndex + root.minDate.getDate()
        } else {
            return dayIndex + 1
        }
    }

    function daysInMonth(year, month) {
        return new Date(year, month + 1, 0).getDate();
    }

    function setupForDate(value) {
        internal.mutex = true
        try {
            let dateComponents = Utils.decodeTimestamp(value)

            let yearIndex = dateComponents.year - minDate.getFullYear()
            yearTumbler.positionViewAtIndex(yearIndex, Tumbler.Center)
            yearTumbler.currentIndex = yearIndex

            if (isMinYear()) {
                let monthIndex = dateComponents.month - minDate.getMonth() - 1
                monthTumbler.positionViewAtIndex(monthIndex, Tumbler.Center)
                monthTumbler.currentIndex = monthIndex
            } else {
                let monthIndex = dateComponents.month - 1
                monthTumbler.positionViewAtIndex(monthIndex, Tumbler.Center)
                monthTumbler.currentIndex = monthIndex
            }

            if (isMinYear() && isMinMonth()) {
                let dayIndex = dateComponents.day - minDate.getDate() - 1
                dayTumbler.positionViewAtIndex(dayIndex, Tumbler.Center)
                dayTumbler.currentIndex = dayIndex
            } else {
                let dayIndex = dateComponents.day - 1
                dayTumbler.positionViewAtIndex(dayIndex, Tumbler.Center)
                dayTumbler.currentIndex = dayIndex
            }

        } finally {
            internal.mutex = false
        }
    }

    function snapDate() {
        if (internal.mutex) {
            return
        }

        let value = fieldBinding.getValue(internal.initialValue)

        let year = yearTumbler.currentIndex + minDate.getFullYear();
        let month = monthTumbler.currentIndex + 1
        let day = dayTumbler.currentIndex + 1
        value = Utils.changeDate(value, year, month, day)
        root.valueUpdated(value)

        fieldBinding.setValue(value)
    }
}
