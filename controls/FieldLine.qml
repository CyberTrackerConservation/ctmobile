import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtPositioning
import CyberTracker as C

ContentPage {
    id: root

    property alias recordUid: fieldBinding.recordUid
    property alias fieldUid: fieldBinding.fieldUid

    property bool backPrompt: root.state === "auto"
    property string backPromptText: qsTr("Cancel capture?")
    property string backPromptConfirmText: qsTr("Yes, cancel capture")

    C.FieldBinding {
        id: fieldBinding
    }

    footer: RowLayout {
        spacing: 0

        FooterButton {
            icon.source: "qrc:/icons/layers_outline.svg"
            text: qsTr("Layers")
            onClicked: {
                form.pushPage("qrc:/MapLayersPage.qml", { hideDataLayers: true })
            }
        }

        FooterButton {
            checkable: true
            checked: mapView.autoPanMode === C.MapController.AutoPanModeRecenter
            icon.source: checked ? "qrc:/icons/gps_fixed.svg" : "qrc:/icons/gps_not_fixed.svg"
            text: qsTr("Follow")
            onClicked: {
                mapView.autoPanMode = (checked ? C.MapController.AutoPanModeRecenter : C.MapController.AutoPanModeOff)
            }
        }

        FooterButton {
            id: buttonReset
            enabled: !fieldBinding.isEmpty && root.state === "default"
            icon.source: "qrc:/icons/delete_outline.svg"
            text: qsTr("Reset")
            onClicked: popupConfirmReset.open()
        }

        FooterButton {
            id: buttonRemoveLast
            icon.source: "qrc:/icons/map_marker_remove_outline.svg"
            text: qsTr("Remove")
            enabled: !fieldBinding.isEmpty && (fieldBinding.value.length > 0)
            onClicked: removeLastPoint()
        }

        FooterButton {
            id: buttonAdd
            onClicked: {
                if (root.state === "default") {
                    popupMethod.open()
                } else {
                    root.state = "default"
                }
            }
        }
    }

    ConfirmPopup {
        id: popupConfirmReset
        text: qsTr("Clear current points?")
        confirmText: qsTr("Yes, clear them")
        onConfirmed: {
            fieldBinding.resetValue()
            root.state = "default"
        }
    }

    OptionsPopup {
        id: popupMethod

        icon: "qrc:/icons/map_marker_plus_outline.svg"

        model: [
            { text: qsTr("Tap"), icon: "qrc:/icons/touch_outline.svg", visible: fieldBinding.field.allowManual },
            { text: qsTr("GPS"), icon: "qrc:/icons/map_marker_account_outline.svg", visible: fieldBinding.field.allowManualGPS },
            { text: qsTr("Auto GPS"), icon: "qrc:/icons/map_marker_multiple_outline.svg", visible: fieldBinding.field.allowAutoGPS }
        ]

        onClicked: function (index) {
            switch (index) {
            case 0:
                root.state = "tap"
                break;

            case 1:
                root.state = "manual"
                break;

            case 2:
                root.state = "auto"
                break;
            }
        }
    }

    EsriMapView {
        id: mapView
        anchors.fill: parent
        mode: C.MapController.ModeField
        hideDataLayers: true
        recordUid: fieldBinding.recordUid
        fieldUid: fieldBinding.fieldUid

        onPointClicked: (x, y) => {
            if (root.state === "tap") {
                addPoint(x, y, 0, 0)
            }
        }


        Item {
            id: buttonAddPoint
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 24
            width: manualButton.implicitWidth + App.scaleByFontSize(16)
            height: manualButton.implicitHeight

            Rectangle {
                anchors.fill: manualButton
                color: colorContent || Style.colorContent
                opacity: 0.5
                border.color: Material.foreground
                border.width: 1
            }

            ToolButton {
                id: manualButton
                anchors.fill: parent
                font.pixelSize: App.settings.font13
                text: qsTr("Add point")
                icon.source: "qrc:/icons/map_marker_account_outline.svg"
                icon.width: Style.toolButtonSize
                icon.height: Style.toolButtonSize
                font.capitalization: Font.AllUppercase
                enabled: App.lastLocationRecent
                onClicked: {
                    if (!App.lastLocationAccurate) {
                        showToast(App.locationAccuracyText)
                        return
                    }

                    addPoint(App.lastLocation.x, App.lastLocation.y, App.lastLocation.z, App.lastLocation.accuracy)
                }
            }
        }
    }

    states: [
        State {
            name: "default"
            StateChangeScript {
                script: {
                    buttonAdd.icon.source = "qrc:/icons/map_marker_plus_outline.svg"
                    buttonAdd.text = qsTr("Start")
                    buttonAddPoint.visible = false
                }
            }
        },
        State {
            name: "tap"
            StateChangeScript {
                script: {
                    mapView.autoPanMode = C.MapController.AutoPanModeOff
                    buttonAdd.icon.source = "qrc:/icons/pause.svg"
                    buttonAdd.text = qsTr("Pause")
                    buttonAddPoint.visible = false
                }
            }
        },
        State {
            name: "manual"
            StateChangeScript {
                script: {
                    mapView.autoPanMode = C.MapController.AutoPanModeRecenter
                    buttonAdd.icon.source = "qrc:/icons/pause.svg"
                    buttonAdd.text = qsTr("Pause")
                    buttonAddPoint.visible = true
                }
            }
        },
        State {
            name: "auto"
            StateChangeScript {
                script: {
                    mapView.autoPanMode = C.MapController.AutoPanModeRecenter
                    buttonAdd.icon.source = "qrc:/icons/pause.svg"
                    buttonAdd.text = qsTr("Pause")
                    buttonAddPoint.visible = false
                    form.startFieldStreamer(root.recordUid, root.fieldUid, fieldBinding.field.autoGPSTimeInSeconds * 1000, 0)
                }
            }
        }
    ]

    onStateChanged: {
        popupMethod.close()
        form.setControlState(root.recordUid, root.fieldUid, "state", root.state)
        form.stopFieldStreamer()
    }

    function removeLastPoint() {
        let value = fieldBinding.getValue([])
        value.pop()
        fieldBinding.setValue(value)
        form.saveState()
    }

    function addPoint(x, y, z, a) {
        let points = fieldBinding.getValue([])
        let point = [ x, y, z, a ]
        points.push(point)
        fieldBinding.setValue(points)
        form.saveState()
    }

    Component.onCompleted: {
        if (fieldBinding.isEmpty) {
            mapView.initialPanMode = C.MapController.AutoPanModeRecenter

        } else {
            let extent = Utils.computeExtent(fieldBinding.getValue([]))
            if (extent.xmin !== extent.xmax) {
                mapView.initialExtent = extent
            }
        }

        root.state = form.getControlState(root.recordUid, root.fieldUid, "state", "default")
    }

    Component.onDestruction: {
        // Leaving the page scenario.
        // If the app is shutdown, then we want to preserve the state of the streamer and the page.
        if (!App.shutdown) {
            form.setControlState(root.recordUid, root.fieldUid, "state", "default")
            form.stopFieldStreamer()
            App.garbageCollectStreamers()
        }
    }
}
