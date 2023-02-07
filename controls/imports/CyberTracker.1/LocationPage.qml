import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12
import QtQuick.Layouts 1.12
import CyberTracker 1.0 as C

ContentPage {
    id: page

    property var flagLocation
    property bool setGotoTarget: false
    property string replacePageOnConfirmUrl: ""
    property var replacePageOnConfirmParams: ({})
    property alias hideDataLayers: mapView.hideDataLayers

    Component.onCompleted: {
        mapView.flagLocation = page.flagLocation

        internal.blockUpdate = true

        if (zoneText.text === "") {
            let utmZone = App.settings.utmZone
            if (utmZone !== 0) {
                zoneText.text = utmZone
            }
        }

        if (zoneHemi.text === "") {
            zoneHemi.text = App.settings.utmHemi
        }

        internal.blockUpdate = false
    }

    QtObject {
        id: internal
        property bool blockUpdate: false
        property bool useUTM: App.settings.coordinateFormat === 3
    }

    header: PageHeader {
        id: topLine
        menuIcon: "qrc:/icons/check.svg"
        menuVisible: true
        onMenuClicked: {
            mapView.flagLocation = mapView.centerPoint
            mapView.flagLocation.ts = App.timeManager.currentDateTimeISO()
            page.flagLocation = mapView.flagLocation

            Qt.inputMethod.hide()
            if (replacePageOnConfirmUrl !== "") {
                form.replaceLastPage(replacePageOnConfirmUrl, replacePageOnConfirmParams, StackView.Immediate)
            } else if (typeof(formPageStack) !== "undefined") {
                form.popPage(StackView.Immediate)
            } else {
                appPageStack.pop()
            }
        }
        text: qsTr("Location")
    }

    footer: RowLayout {
        id: buttonRow
        spacing: 0

        ButtonGroup { id: buttonGroup }

        C.FooterButton {
            ButtonGroup.group: buttonGroup
            text: qsTr("Point")
            checkable: true
            checked: stackLayout.currentIndex === 0
            icon.source: checked ? "qrc:/icons/touch_app.svg" : "qrc:/icons/touch_outline.svg"
            onClicked: stackLayout.currentIndex = 0
        }

        C.FooterButton {
            ButtonGroup.group: buttonGroup
            text: qsTr("Manual")
            checkable: true
            checked: stackLayout.currentIndex === 1
            icon.source: checked ? "qrc:/icons/keyboard.svg" : "qrc:/icons/keyboard_outline.svg"
            onClicked: stackLayout.currentIndex = 1
        }
    }

    StackLayout {
        id: stackLayout
        anchors.fill: parent
        property bool loadComplete: false
        Component.onCompleted: {
            stackLayout.currentIndex = App.settings.locationPageTabIndex
            loadComplete = true
        }

        clip: true

        // Map positioning.
        Item {
            width: parent.width
            height: parent.height

            C.EsriMapView {
                id: mapView
                anchors.fill: parent
                flagMode: true

                onFlagLocationChanged: {
                    if (page.setGotoTarget && mapView.flagLocation !== undefined) {
                        App.gotoManager.setTarget(qsTr("Location"), [[mapView.flagLocation.x, mapView.flagLocation.y]], 0)
                    }

                    flagDropChanged()
                }

                Image {
                    anchors.centerIn: parent
                    source: App.settings.darkTheme ? "qrc:/app/targetDark.svg" : "qrc:/app/target.svg"
                    sourceSize.width: 48
                    sourceSize.height: 48
                    opacity: 0.75
                }
            }
        }

        // Manual entry.
        Item {
            height: parent.height
            width: parent.width

            GridLayout {
                x: 16
                y: 16
                width: parent.width - 32
                columns: 2
                columnSpacing: 8
                rowSpacing: 8

                Label {
                    Layout.columnSpan: 2
                    Layout.fillWidth: true
                    horizontalAlignment: Qt.AlignHCenter
                    text: internal.useUTM ? "UTM" : qsTr("Decimal degrees")
                    font.pixelSize: App.settings.font20
                }

                Label {
                    Layout.preferredWidth: parent.width / 2
                    text: internal.useUTM ? qsTr("Easting") : qsTr("Latitude")
                    font.pixelSize: App.settings.font16
                }

                TextField {
                    id: coordA
                    Layout.fillWidth: true
                    font.pixelSize: App.settings.font16
                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                    onTextChanged: flagTextChanged()
                }

                Label {
                    Layout.preferredWidth: parent.width / 2
                    text: internal.useUTM ? qsTr("Northing") : qsTr("Longitude")
                    font.pixelSize: App.settings.font16
                }

                TextField {
                    id: coordB
                    Layout.fillWidth: true
                    font.pixelSize: App.settings.font16
                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                    onTextChanged: flagTextChanged()
                }

                Label {
                    Layout.preferredWidth: parent.width / 2
                    text: qsTr("Zone")
                    font.pixelSize: App.settings.font16
                    visible: internal.useUTM
                }

                RowLayout {
                    Layout.fillWidth: true
                    visible: internal.useUTM

                    TextField {
                        id: zoneText
                        Layout.fillWidth: true
                        font.pixelSize: App.settings.font16
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        onTextChanged: flagTextChanged()
                    }

                    ToolButton {
                        id: zoneHemi
                        font.pixelSize: App.settings.font16
                        text: "N"
                        onClicked: {
                            text = text === "N" ? "S" : "N"
                            flagTextChanged()
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: stackLayout

        function onCurrentIndexChanged() {
            buttonRow.children[stackLayout.currentIndex].checked = true
            if (stackLayout.loadComplete) {
                App.settings.locationPageTabIndex = stackLayout.currentIndex
            }

            if (stackLayout.currentIndex === 0) {
                Qt.inputMethod.hide()
            }
        }
    }

    function flagTextChanged() {
        if (internal.blockUpdate) {
            return
        }

        if (coordA.text.trim() === "" || coordB.text.trim() === "") {
            return
        }

        if (internal.useUTM && zoneText.text.trim() === "") {
            return
        }

        internal.blockUpdate = true

        let a = parseFloat(coordA.text)
        let b = parseFloat(coordB.text)
        let zone = parseInt(zoneText.text)
        let hemi = zoneHemi.text

        let x = 0.0
        let y = 0.0
        let ts = App.timeManager.currentDateTimeISO()

        switch (App.settings.coordinateFormat) {
        case 0:
        case 1:
        case 2:
            x = b
            if (x < -180 || x > 180) {
                x = 0
            }

            y = a
            if (y < -90 || y > 90) {
                y = 0
            }

            mapView.flagLocation = { x: x, y: y, spatialReference: { wkid: 4326 }, ts: ts }

            break

        case 3: // UTM
            if (zone < 1 || zone > 60) {
                zone = 1
            }

            let loc = Utils.locationFromUTM(a, b, zone, hemi)
            mapView.flagLocation = { x: loc.x, y: loc.y, spatialReference: { wkid: 4326 }, ts: ts }

            break
        }

        mapView.centerOnFlagLocation()

        internal.blockUpdate = false
    }

    function flagDropChanged() {
        if (internal.blockUpdate) {
            return
        }

        if (mapView.flagLocation === undefined) {
            return
        }

        internal.blockUpdate = true

        switch (App.settings.coordinateFormat) {
        case 0:
        case 1:
        case 2:
            coordA.text = mapView.flagLocation !== undefined && mapView.flagLocation.y !== undefined ? Math.round(mapView.flagLocation.y * 100000) / 100000 : ""
            coordB.text = mapView.flagLocation !== undefined && mapView.flagLocation.x !== undefined ? Math.round(mapView.flagLocation.x * 100000) / 100000 : ""
            break

        case 3: // UTM
            let utmLocation = Utils.locationToUTM(mapView.flagLocation.y, mapView.flagLocation.x)
            coordA.text = utmLocation.ux
            coordB.text = utmLocation.uy
            zoneText.text = utmLocation.zone
            zoneHemi.text = utmLocation.hemi
            break
        }

        internal.blockUpdate = false
    }
}
