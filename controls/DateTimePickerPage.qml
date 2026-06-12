import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import CyberTracker as C

C.ContentPage {
    id: page

    property var initialDateTime: new Date()
    property alias selectedDateTime: datetimepicker.selDate
    property var callback

    header: C.PageHeader {
        text: page.title
        menuIcon: "qrc:/icons/check.svg"
        menuVisible: true
        onMenuClicked: popPageImmediate(page)
    }

    Pane {
        id: pane

        anchors.fill: parent

        padding: 0
        Material.background: Style.colorContent

        contentItem: GridLayout {
            id: control

            property bool islandscape: control.width > control.height

            flow: (control.islandscape) ? GridLayout.LeftToRight : GridLayout.TopToBottom

            Rectangle {
                id: showdatetime
                color: "transparent"

                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.minimumWidth: App.scaleByFontSize(50)
                Layout.minimumHeight: App.scaleByFontSize(50)

                GridLayout {
                    anchors.fill: parent
                    flow: !control.islandscape ? GridLayout.LeftToRight : GridLayout.TopToBottom

                    Label {
                        id: showdate
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        Layout.minimumWidth: App.scaleByFontSize(30)
                        Layout.minimumHeight: App.scaleByFontSize(30)
                        visible: datetimepicker.mode !== DateTimePicker.Mode.TimeOnly
                        text: (isNaN(control.selDate)) ? "---- \n -- -- " : control.selDate.toLocaleDateString(Qt.locale(), "yyyy\n MMM dd ")
                        verticalAlignment: (datetimepicker.mode === DateTimePicker.Mode.DateOnly) ? Text.AlignVCenter : Text.AlignBottom
                        horizontalAlignment: (datetimepicker.mode === DateTimePicker.Mode.DateOnly) ? Text.AlignHCenter : Text.AlignRight
                        fontSizeMode: Text.Fit
                        font.pixelSize: App.scaleByFontSize(80)
                        color: Style.frontColor1
                    }

                    Label {
                        id: showtime
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        Layout.minimumWidth: App.scaleByFontSize(40)
                        Layout.minimumHeight: App.scaleByFontSize(30)
                        visible: datetimepicker.mode !== DateTimePicker.Mode.DateOnly
                        text: (isNaN(datetimepicker.selDate)) ? "--:--_" : datetimepicker.selDate.toLocaleTimeString(Qt.locale(), "h:mm a")
                        verticalAlignment: control.islandscape ? ((datetimepicker.mode === DateTimePicker.Mode.TimeOnly) ? Text.AlignVCenter : Text.AlignTop) : Text.AlignBottom
                        horizontalAlignment: control.islandscape ? Text.AlignRight : ((datetimepicker.mode === DateTimePicker.Mode.TimeOnly) ? Text.AlignHCenter : Text.AlignLeft)
                        fontSizeMode: Text.Fit
                        font.pixelSize: App.scaleByFontSize(80)
                        color: Style.frontColor1
                    }
                }
            }

            DateTimePicker {
                id: datetimepicker
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.minimumWidth: 100
                Layout.minimumHeight: 150
                initDate: page.initialDateTime
            }
        }
    }
}
