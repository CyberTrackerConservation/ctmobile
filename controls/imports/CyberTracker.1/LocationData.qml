import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12

GridLayout {
    id: grid
    columns: 3
    columnSpacing: 0
    rowSpacing: 8

    property var location: App.lastLocation
    property int buttonWidth: (grid.width - 16) / 2

    property bool hasTimestamp: grid.location !== undefined && grid.location.t !== undefined

    // Date.
    Label {
        Layout.preferredWidth: grid.buttonWidth
        text: qsTr("Date")
        font.pixelSize: App.settings.font14
        horizontalAlignment: Label.AlignRight
    }

    Item {
        width: 16
    }

    Label {
        Layout.preferredWidth: grid.buttonWidth
        text: grid.hasTimestamp ? App.timeManager.getDateText(grid.location.ts) : "-"
        font.pixelSize: App.settings.font14
        font.bold: true
    }

    // Time.
    Label {
        Layout.preferredWidth: grid.buttonWidth
        text: qsTr("Time")
        font.pixelSize: App.settings.font14
        horizontalAlignment: Label.AlignRight
    }

    Item {
        width: 16
    }

    Label {
        Layout.preferredWidth: grid.buttonWidth
        text: grid.hasTimestamp ? App.timeManager.getTimeText(grid.location.ts) : "-"
        font.pixelSize: App.settings.font14
        font.bold: true
    }

    // TimeZone.
    Label {
        Layout.preferredWidth: grid.buttonWidth
        text: qsTr("Time zone")
        font.pixelSize: App.settings.font14
        horizontalAlignment: Label.AlignRight
    }

    Item {
        width: 16
    }

    Label {
        Layout.preferredWidth: grid.buttonWidth
        text: {
            grid.location
            return App.timeManager.timeZoneText
        }
        font.pixelSize: App.settings.font14
        font.bold: true
    }

    // Altitude.
    Label {
        Layout.preferredWidth: grid.buttonWidth
        text: qsTr("Altitude")
        font.pixelSize: App.settings.font14
        horizontalAlignment: Label.AlignRight
    }

    Item { width: 16 }

    Label {
        Layout.preferredWidth: grid.buttonWidth
        text: grid.location ? App.getDistanceText(grid.location.z) : "-"
        font.pixelSize: App.settings.font14
        font.bold: true
    }

    // Direction.
    Label {
        Layout.preferredWidth: grid.buttonWidth
        text: qsTr("Direction")
        font.pixelSize: App.settings.font14
        horizontalAlignment: Label.AlignRight
    }

    Item { width: 16 }

    Label {
        Layout.preferredWidth: grid.buttonWidth
        text: grid.location ? ((grid.location.d | 0) + " " + qsTr("degrees")) : "-"
        font.pixelSize: App.settings.font14
        font.bold: true
    }

    // Accuracy.
    Label {
        Layout.preferredWidth: grid.buttonWidth
        text: qsTr("Accuracy")
        font.pixelSize: App.settings.font14
        horizontalAlignment: Label.AlignRight
    }

    Item { width: 16 }

    Label {
        Layout.preferredWidth: grid.buttonWidth
        text: grid.location ? App.getDistanceText(grid.location.a) : "-"
        font.pixelSize: App.settings.font14
        font.bold: true
    }

    // Speed.
    Label {
        Layout.preferredWidth: grid.buttonWidth
        text: qsTr("Speed")
        font.pixelSize: App.settings.font14
        horizontalAlignment: Label.AlignRight
    }

    Item { width: 16 }

    Label {
        Layout.preferredWidth: grid.buttonWidth
        text: grid.location ? App.getSpeedText(grid.location.s) : "-"
        font.pixelSize: App.settings.font14
        font.bold: true
    }

    // Track timer.
    Label {
        Layout.preferredWidth: grid.buttonWidth
        font.pixelSize: App.settings.font14
        horizontalAlignment: Label.AlignRight
        text: qsTr("Track recorder")
    }

    Item { width: 16 }

    Label {
        Layout.preferredWidth: grid.buttonWidth
        font.pixelSize: App.settings.font14
        font.bold: true
        text: {
            if (typeof(form) === "undefined") {
                return qsTr("Off")
            } else {
                return form.trackStreamer.rateText()
            }
        }
    }

    // Spacer.
    Label {
        Layout.preferredWidth: grid.buttonWidth
        font.pixelSize: App.settings.font14
        horizontalAlignment: Label.AlignRight
    }

    Item { width: 16 }

    Label {
        Layout.preferredWidth: grid.buttonWidth
        font.pixelSize: App.settings.font14
        font.bold: true
    }
}
