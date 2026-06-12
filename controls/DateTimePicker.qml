import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

ColumnLayout {
    id: control

    property date initDate: new Date()
    property date selDate

    property int mode: DateTimePicker.Mode.DateTime

    enum Mode {
        DateOnly,
        TimeOnly,
        DateTime
    }

    RowLayout {
        id: selectorhead
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumHeight: 15
        property bool showcalendar: (control.mode === DateTimePicker.Mode.TimeOnly) ? false : true
        onShowcalendarChanged: {
            if (selectorhead.showcalendar) {
                calendarshader.iTime = 1.0;
                timeshader.iTime = 0.0;
            } else {
                calendarshader.iTime = 0.0;
                timeshader.iTime = 1.0;
            }
        }

        Rectangle {
            id: calendarbutton
            color: "transparent"
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.minimumWidth: 50
            visible: (control.mode !== DateTimePicker.Mode.TimeOnly)
            ToolButton {
                id: calendarshader
                icon.source: selectorhead.showcalendar ? "qrc:/icons/calendar_range.svg" : "qrc:/icons/calendar_range_outline.svg"
                icon.width: Style.toolButtonSize
                icon.height: Style.toolButtonSize

                height: Math.min(calendarbutton.width, calendarbutton.height) * 0.8
                width: height
                anchors.centerIn: calendarbutton
                property real iTime: 1.0
                Behavior on iTime {
                    SmoothedAnimation {
                        velocity: 1.5
                    }
                }
                onClicked: {
                    selectorhead.showcalendar = true;
                }
            }
            Rectangle {
                id: calendarline
                width: parent.width * calendarshader.iTime
                height: parent.height * 0.05
                anchors.verticalCenter: parent.bottom
                anchors.right: parent.right
                color: Style.frontColor3
            }
        }
        Rectangle {
            id: timebutton
            color: "transparent"
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.minimumWidth: 50
            visible: (control.mode !== DateTimePicker.Mode.DateOnly)
            ToolButton {
                id: timeshader
                icon.source: !selectorhead.showcalendar ? "qrc:/icons/clock_time_four.svg" : "qrc:/icons/clock_time_four_outline.svg"
                icon.width: Style.toolButtonSize
                icon.height: Style.toolButtonSize

                height: Math.min(timebutton.width, timebutton.height) * 0.8
                width: height
                anchors.centerIn: timebutton
                property real iTime: 0.0
                Behavior on iTime {
                    SmoothedAnimation {
                        velocity: 1.5
                    }
                }
            }

            MouseArea {
                id: timebuttonarea
                anchors.fill: parent
                onClicked: {
                    selectorhead.showcalendar = false;
                }
            }
            Rectangle {
                id: timeline
                width: parent.width * timeshader.iTime
                height: parent.height * 0.05
                anchors.verticalCenter: parent.bottom
                anchors.left: parent.left
                color: Style.frontColor3
            }
        }
    }
    Rectangle {
        id: division
        Layout.fillWidth: true
        Layout.maximumHeight: calendarline.height * 0.5
        Layout.minimumHeight: calendarline.height * 0.4
        Layout.alignment: Qt.AlignTop
        opacity: 0.5
        color: Style.midColor1
    }

    DatePicker {
        id: datePicker
        initDate: control.initDate
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumHeight: 125
        visible: selectorhead.showcalendar
        clip: true
        onSelDateChanged: {
            control.selDate.setFullYear(datePicker.selDate.getFullYear());
            control.selDate.setMonth(datePicker.selDate.getMonth());
            control.selDate.setDate(datePicker.selDate.getDate());
            if (control.mode !== DateTimePicker.Mode.DateOnly)
                selectorhead.showcalendar = false;
        }
    }
    TimePicker {
        id: timepicker
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumHeight: 125
        visible: !selectorhead.showcalendar
        hour: control.initDate.getHours()
        minute: control.initDate.getMinutes()
        onSelected: {
            if (isNaN(control.selDate))
                control.selDate = new Date();
            control.selDate.setHours(timepicker.hour);
            control.selDate.setMinutes(timepicker.minute);
        }
    }
}
