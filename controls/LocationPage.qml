import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import CyberTracker as C

ContentPage {
    id: page

    property var flagLocation: ({})
    property bool setGotoTarget: false
    property string replacePageOnConfirmUrl: ""
    property var replacePageOnConfirmParams: ({})
    property alias hideDataLayers: mapView.hideDataLayers
    property var contextData: ({})

    Component.onCompleted: {
        if (page.flagLocation !== undefined) {
            sync(() => {
                mapView.flagLocation = page.flagLocation
                mapView.centerOnFlagLocation()
                flagTextUpdate()
            })
        }
    }

    QtObject {
        id: internal
        property bool blockUpdate: false
    }

    header: PageHeader {
        id: topLine
        menuIcon: "qrc:/icons/check.svg"
        menuVisible: true
        onMenuClicked: {
            let emitLocationAcquired = false
            sync(() => {
                if (stackLayout.currentIndex === 0) {
                    mapView.flagLocation = mapView.centerPoint
                    mapView.flagLocation.ts = App.timeManager.currentDateTimeISO()
                }

                if (mapView.flagLocation.x !== undefined && mapView.flagLocation.y !== undefined) {
                    page.flagLocation = mapView.flagLocation
                    page.flagLocation.ts = App.timeManager.currentDateTimeISO()

                    if (page.setGotoTarget) {
                        App.gotoManager.setTarget(qsTr("Location"), [[mapView.flagLocation.x, mapView.flagLocation.y]], 0)
                    }

                    emitLocationAcquired = true
                }
            })

            Qt.inputMethod.hide()
            if (replacePageOnConfirmUrl !== "") {
                form.replaceLastPage(replacePageOnConfirmUrl, replacePageOnConfirmParams, StackView.Immediate)
            } else {
                popPageImmediate(page)
            }

            if (emitLocationAcquired) {
                App.locationAcquired(page.flagLocation.x, page.flagLocation.y, 0, 0, App.timeManager.currentDateTimeISO(), page.contextData)
            }
        }
        text: qsTr("Location")
    }

    footer: RowLayout {
        id: buttonRow
        spacing: 0

        ButtonGroup { id: buttonGroup }

        FooterButton {
            ButtonGroup.group: buttonGroup
            text: qsTr("Point mode")
            checkable: true
            checked: stackLayout.currentIndex === 0
            icon.source: checked ? "qrc:/icons/touch_app.svg" : "qrc:/icons/touch_outline.svg"
            onClicked: stackLayout.currentIndex = 0
        }

        FooterButton {
            ButtonGroup.group: buttonGroup
            text: qsTr("Manual mode")
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

            EsriMapView {
                id: mapView
                anchors.fill: parent
                mode: C.MapController.ModeFlag
                hideDataLayers: true

                onFlagLocationChanged: sync(flagTextUpdate)

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
                columnSpacing: App.scaleByFontSize(8)
                rowSpacing: App.scaleByFontSize(8)

                Label {
                    Layout.columnSpan: 2
                    Layout.fillWidth: true
                    horizontalAlignment: Qt.AlignHCenter
                    text: App.translateText(App.settings.projection.d)
                    font.pixelSize: App.settings.font20
                    wrapMode: Label.Wrap
                }

                Label {
                    Layout.preferredWidth: parent.width / 2
                    text: {
                        let result = App.settings.projection.a
                        return result === undefined ? qsTr("Latitude") : result
                    }
                    font.pixelSize: App.settings.font16
                }

                TextField {
                    id: coordA
                    Layout.fillWidth: true
                    font.pixelSize: App.settings.font16
                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                    onTextChanged: sync(flagTextChanged)
                }

                Label {
                    Layout.preferredWidth: parent.width / 2
                    text: {
                        let result = App.settings.projection.b
                        return result === undefined ? qsTr("Longitude") : result
                    }
                    font.pixelSize: App.settings.font16
                }

                TextField {
                    id: coordB
                    Layout.fillWidth: true
                    font.pixelSize: App.settings.font16
                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                    onTextChanged: sync(flagTextChanged)
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

    function sync(f) {
        if (internal.blockUpdate) {
            return
        }

        try {
            internal.blockUpdate = true
            f()
        } finally {
            internal.blockUpdate = false
        }
    }

    function flagTextChanged() {
        if (coordA.text.trim() === "" || coordB.text.trim() === "") {
            return
        }

        let a = parseFloat(coordA.text)
        let b = parseFloat(coordB.text)

        let location = App.unprojectLocation(a, b)
        if (location.error) {
            showError(qsTr("Coordinates not valid"))
            return
        }

        mapView.flagLocation = { x: location.x, y: location.y, spatialReference: { wkid: 4326 }, ts: App.timeManager.currentDateTimeISO() }
        mapView.centerOnFlagLocation()
    }

    function flagTextUpdate() {
        if (mapView.flagLocation === undefined || mapView.flagLocation.x === undefined || mapView.flagLocation.y === undefined) {
            coordA.text = coordB.text = ""
            return
        }

        let location = App.projectLocation(mapView.flagLocation.x, mapView.flagLocation.y)
        coordA.text = location.aText
        coordB.text = location.bText
    }
}
