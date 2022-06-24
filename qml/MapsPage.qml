import QtQml 2.12
import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import Esri.ArcGISRuntime 100.14 as Esri
import QtQuick.Controls.Material 2.12
import QtLocation 5.12
import QtSensors 5.12
import QtPositioning 5.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page
    title: "Maps"

    readonly property string mapActiveLayers: "mapActiveLayers"
    readonly property var overlayHeader: pageHeader

    C.EsriMapView {
        id: mapView
        anchors {
            left: parent.left
            top: parent.top
            right: parent.right
            bottom: parent.bottom
        }

        onInspectClicked: {
            if (typeof(formPageStack) !== "undefined") {
                form.pushPage("qrc:/MapInspectorPage.qml", { inspectList: inspectList })
            } else {
                appPageStack.push("qrc:/MapInspectorPage.qml", { inspectList: inspectList })
            }
        }
    }

    // Translucent header.
    Rectangle {
        anchors.fill: pageHeader
        opacity: 0.5
        color: App.settings.darkTheme ? "black" : "white"
    }

    Rectangle {
        id: pageHeader

        property var textColor: Material.foreground
        property var outlineColor: Material.background

        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }

        color: "transparent"
        implicitHeight: toolbarInternal.implicitHeight

        ToolBar {
            id: toolbarInternal
            visible: false
        }

        Row {
            anchors.fill: parent
            spacing: 0

            ToolButton {
                id: backButton
                icon.source: C.Style.backIconSource
                icon.width: C.Style.toolButtonSize
                icon.height: C.Style.toolButtonSize
                icon.color: pageHeader.textColor
                onClicked: {
                    if (typeof(formPageStack) !== "undefined") {
                        form.popPage()
                    } else {
                        appPageStack.pop()
                    }
                }
            }

            Label {
                id: titleLabel
                text: App.lastLocationText
                font.pixelSize: App.settings.font20
                font.bold: true
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
                width: parent.width - backButton.width - toggleMoreButton.width
                height: parent.height
                fontSizeMode: Label.Fit
                clip: true
                x: backButton.width
                color: pageHeader.textColor
            }

            ToolButton {
                id: toggleMoreButton
                icon.width: C.Style.toolButtonSize
                icon.height: C.Style.toolButtonSize
                icon.color: pageHeader.textColor
                onClicked: page.state = page.state === "moreDataShow" ? "moreDataHide" : "moreDataShow"
            }
        }
    }

    // Footer.
    footer: ColumnLayout {
        spacing: 0
        width: parent.width

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: Material.theme === Material.Dark ? "#FFFFFF" : "#000000"
            opacity: Material.theme === Material.Dark ? 0.12 : 0.12
        }

        RowLayout {
            id: buttonRow
            spacing: 0
            Layout.fillWidth: true
            property int buttonCount: 5
            property int buttonWidth: page.width / buttonCount
            property var buttonColor: Material.theme === Material.Dark ? Material.foreground : Material.primary

            ToolButton {
                Layout.preferredWidth: buttonRow.buttonWidth
                Layout.fillWidth: true
                Layout.fillHeight: true
                icon.source: "qrc:/icons/layers_outline.svg"
                Material.foreground: buttonRow.buttonColor
                display: Button.TextUnderIcon
                text: qsTr("Layers")
                font.pixelSize: App.settings.font10
                font.capitalization: Font.MixedCase
                onClicked: {
                    if (typeof(formPageStack) !== "undefined") {
                        form.pushPage("qrc:/MapLayersPage.qml", {})
                    } else {
                        appPageStack.push("qrc:/MapLayersPage.qml", {}, StackView.Immediate)
                    }
                }
            }

            ButtonGroup {
                id: buttonGroupAutoPanMode
            }

            Rectangle {
                width: 2
                Layout.fillHeight: true
                color: Material.theme === Material.Dark ? "#FFFFFF" : "#000000"
                opacity: Material.theme === Material.Dark ? 0.12 : 0.12
            }

            ToolButton {
                id: buttonPan
                ButtonGroup.group: buttonGroupAutoPanMode
                checkable: true
                checked: mapView.locationDisplay.autoPanMode === Esri.Enums.LocationDisplayAutoPanModeOff
                Layout.preferredWidth: buttonRow.buttonWidth
                Layout.fillWidth: true
                Layout.fillHeight: true
                icon.source: "qrc:/icons/pan.svg"
                Material.foreground: buttonRow.buttonColor
                display: Button.TextUnderIcon
                text: qsTr("Pan")
                font.pixelSize: App.settings.font10
                font.capitalization: Font.MixedCase
                onClicked: mapView.locationDisplay.autoPanMode = Esri.Enums.LocationDisplayAutoPanModeOff
            }

            ToolButton {
                id: buttonFollow
                ButtonGroup.group: buttonGroupAutoPanMode
                checkable: true
                checked: mapView.locationDisplay.autoPanMode === Esri.Enums.LocationDisplayAutoPanModeRecenter
                Layout.preferredWidth: buttonRow.buttonWidth
                Layout.fillWidth: true
                Layout.fillHeight: true
                icon.source: checked ? "qrc:/icons/gps_fixed.svg" : "qrc:/icons/gps_not_fixed.svg"
                Material.foreground: buttonRow.buttonColor
                display: Button.TextUnderIcon
                text: qsTr("Follow")
                font.pixelSize: App.settings.font10
                font.capitalization: Font.MixedCase
                onClicked: mapView.locationDisplay.autoPanMode = Esri.Enums.LocationDisplayAutoPanModeRecenter
            }

            ToolButton {
                id: buttonFollowNav
                ButtonGroup.group: buttonGroupAutoPanMode
                checkable: true
                checked: mapView.locationDisplay.autoPanMode === Esri.Enums.LocationDisplayAutoPanModeNavigation
                Layout.preferredWidth: buttonRow.buttonWidth
                Layout.fillWidth: true
                Layout.fillHeight: true
                icon.source: checked ? "qrc:/icons/navigation2.svg" : "qrc:/icons/navigation2_empty.svg"
                Material.foreground: buttonRow.buttonColor
                display: Button.TextUnderIcon
                text: qsTr("Goto")
                font.pixelSize: App.settings.font10
                font.capitalization: Font.MixedCase
                onClicked: {
                    mapView.locationDisplay.autoPanMode = Esri.Enums.LocationDisplayAutoPanModeNavigation
                    moreDataSwipeView.currentIndex = 2
                    page.state = "moreDataShow"
                }
            }

            Rectangle {
                width: 2
                Layout.fillHeight: true
                color: Material.theme === Material.Dark ? "#FFFFFF" : "#000000"
                opacity: Material.theme === Material.Dark ? 0.12 : 0.12
            }

            ToolButton {
                id: buttonMore
                Layout.preferredWidth: buttonRow.buttonWidth
                Layout.fillWidth: true
                Layout.fillHeight: true
                icon.source: "qrc:/icons/dots_horizontal.svg"
                Material.foreground: buttonRow.buttonColor
                display: Button.TextUnderIcon
                text: qsTr("More")
                font.pixelSize: App.settings.font10
                font.capitalization: Font.MixedCase
                onClicked: {
                    if (page.state === "moreDataShow") {
                        page.state = "moreDataHide"
                        return
                    }

                    popupData.open()
                }
            }
        }
    }

    C.PopupLoader {
        id: popupData

        popupComponent: Component {
            C.OptionsPopup {
                icon: "qrc:/icons/dots_horizontal.svg"

                model: [
                    { text: qsTr("Data"), icon: "qrc:/icons/data_list.svg" },
                    { text: qsTr("Compass"), icon: "qrc:/icons/compass_outline.svg" },
                    { text: qsTr("Navigation"), icon: "qrc:/icons/navigation2_empty.svg" }
                ]

                onClicked: function (index) {
                    App.settings.mapMoreDataViewIndex = index
                    page.state = "moreDataShow"
                }
            }
        }
    }

    // More data view.
    Item {
        id: popupMoreData
        y: pageHeader.height
        width: page.width
        height: moreDataSwipeView.height + moreDataPageIndicator.height

        Rectangle {
            anchors.fill: parent
            color: Material.theme !== Material.Dark ? "#FFFFFF" : "#000000"
            opacity: 0.5
        }

        PageIndicator {
            id: moreDataPageIndicator
            count: moreDataSwipeView.count
            currentIndex: moreDataSwipeView.currentIndex
            y: 0
            anchors.horizontalCenter: parent.horizontalCenter
            opacity: 0.5
        }

        SwipeView {
            id: moreDataSwipeView
            y: moreDataPageIndicator.height
            width: parent.width
            height: popupLocationData.implicitHeight + 16

            onCurrentIndexChanged: App.settings.mapMoreDataViewIndex = moreDataSwipeView.currentIndex

            Item {
                width: parent.height
                height: parent.height

                C.LocationData {
                    id: popupLocationData
                    anchors.centerIn: parent
                    y: 8
                    width: page.width - 16
                }
            }

            Item {
                width: parent.height
                height: parent.height

                C.Skyplot {
                    anchors.centerIn: parent
                    width: moreDataSwipeView.height - 8
                    height: moreDataSwipeView.height - 8
                    active: moreDataSwipeView.currentIndex === 1
                    satNumbersVisible: false
                    legendVisible: false
                }
            }

            Item {
                width: parent.height
                height: parent.height
                C.GotoLocation {
                    anchors.centerIn: parent
                    width: page.width - 16
                    height: parent.height - 16

                    Connections {
                        target: App.gotoManager

                        function onGotoTargetChanged(title) {
                            moreDataSwipeView.currentIndex = 2 // index of "goto" tab
                            page.state = title !== "" ? "moreDataShow" : "moreDataHide"
                        }
                    }
                }
            }
        }
    }

    state: ""
    states: [
        State {
            name: "moreDataHide"
            StateChangeScript {
                script: {
                    buttonMore.checked = false
                    App.settings.mapMoreDataVisible = false
                    popupMoreData.visible = false
                    toggleMoreButton.icon.source = "qrc:/icons/chevron_down.svg"
                }
            }
        },
        State {
            name: "moreDataShow"
            StateChangeScript {
                script: {
                    buttonMore.checked = true
                    App.settings.mapMoreDataVisible = true

                    // Disable swipe animation.
                    moreDataSwipeView.contentItem.highlightMoveDuration = 0
                    moreDataSwipeView.contentItem.highlightMoveVelocity = -1
                    moreDataSwipeView.currentIndex = App.settings.mapMoreDataViewIndex

                    popupMoreData.visible = true
                    toggleMoreButton.icon.source = "qrc:/icons/chevron_up.svg"
                }
            }
        }
    ]

    Component.onCompleted: {
        page.state = App.settings.mapMoreDataVisible ? "moreDataShow" : "moreDataHide"
    }
}
