import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtLocation
import QtSensors
import QtPositioning
import CyberTracker as C

C.ContentPage {
    id: page
    title: "Maps"

    readonly property string mapActiveLayers: "mapActiveLayers"
    readonly property var overlayHeader: pageHeader

    Component.onCompleted: {
        App.offlineMapManager.checkForUpdates()
    }

    C.EsriMapView {
        id: mapView
        anchors {
            left: parent.left
            top: parent.top
            right: parent.right
            bottom: parent.bottom
        }

        onInspectClicked: function (inspectList) {
            pushPageImmediate(page, "qrc:/MapInspectorPage.qml", { inspectList: inspectList })
        }

        onLongPress: (x, y) => {
            if (App.offlineMapManager.hasLookupLayers()) {
                pushPageImmediate(page, "qrc:/PlacePage.qml", { locationX: x, locationY: y })
            }
        }
    }

    // Translucent header.
    Rectangle {
        anchors.fill: pageHeader
        opacity: 0.8
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
        implicitHeight: Math.max(toolbarInternal.implicitHeight, titleRow.implicitHeight)

        ToolBar {
            id: toolbarInternal
            visible: false
        }

        Row {
            id: titleRow
            width: parent.width
            spacing: 0

            ToolButton {
                id: backButton
                icon.source: C.Style.backIconSource
                icon.width: C.Style.toolButtonSize
                icon.height: C.Style.toolButtonSize
                icon.color: pageHeader.textColor
                onClicked: {
                    popPageDefault(page)
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
                icon.source: App.settings.mapMoreDataVisible ? "qrc:/icons/chevron_up.svg" : "qrc:/icons/chevron_down.svg"
                onClicked: {
                    App.settings.mapMoreDataVisible = !App.settings.mapMoreDataVisible
                }
            }
        }
    }

    // BUGBUG: Should not be needed
    Connections {
        target: App.settings

        function onMapPanModeChanged() {
            let mapPanMode = App.settings.mapPanMode
            buttonPan.checked = mapPanMode === C.MapController.AutoPanModeOff
            buttonFollow.checked = mapPanMode === C.MapController.AutoPanModeRecenter
            buttonFollowNav.checked = mapPanMode === C.MapController.AutoPanModeNavigation
        }
    }

    // Footer.
    footer: RowLayout {
        spacing: 0

        ButtonGroup { id: buttonGroup }

        C.FooterButton {
            icon.source: "qrc:/icons/layers_outline.svg"
            text: qsTr("Layers")
            separatorRight: true
            onClicked: {
                pushPageImmediate(page, "qrc:/MapLayersPage.qml")
            }
        }

        C.FooterButton {
            id: buttonPan
            ButtonGroup.group: buttonGroup
            checkable: true
            checked: App.settings.mapPanMode === C.MapController.AutoPanModeOff
            icon.source: "qrc:/icons/pan.svg"
            text: qsTr("Pan")
            onClicked: {
                mapView.autoPanMode = C.MapController.AutoPanModeOff
            }
        }

        C.FooterButton {
            id: buttonFollow
            ButtonGroup.group: buttonGroup
            checkable: true
            checked: App.settings.mapPanMode === C.MapController.AutoPanModeRecenter
            icon.source: checked ? "qrc:/icons/gps_fixed.svg" : "qrc:/icons/gps_not_fixed.svg"
            text: qsTr("Follow")
            onClicked: {
                mapView.autoPanMode = C.MapController.AutoPanModeRecenter
            }
        }

        C.FooterButton {
            id: buttonFollowNav
            ButtonGroup.group: buttonGroup
            checkable: true
            checked: App.settings.mapPanMode === C.MapController.AutoPanModeNavigation
            icon.source: checked ? "qrc:/icons/navigation2.svg" : "qrc:/icons/navigation2_empty.svg"
            text: qsTr("Goto")
            onClicked: {
                mapView.autoPanMode = C.MapController.AutoPanModeNavigation
                setMoreDataPageIndex(2)
            }
        }

        C.FooterButton {
            id: buttonMore
            icon.source: "qrc:/icons/dots_horizontal.svg"
            text: qsTr("More")
            separatorLeft: true
            onClicked: {
                popupData.open()
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
                    { text: qsTr("Navigation"), icon: "qrc:/icons/navigation2_empty.svg" },
                    { text: qsTr("Place lookup"), icon: "qrc:/icons/map_search_outline.svg", visible: App.offlineMapManager.hasLookupLayers() }
                ]

                onClicked: function (index) {
                    if (index === 3) {
                        if (!App.lastLocationRecent ) {
                            App.showToast(qsTr("No recent location"))
                        } else {
                            pushPageImmediate(page, "qrc:/PlacePage.qml", { locationX: App.lastLocation.x, locationY: App.lastLocation.y })
                        }
                        return
                    }

                    setMoreDataPageIndex(index)
                    App.settings.mapMoreDataVisible = true
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
        visible: App.settings.mapMoreDataVisible

        Rectangle {
            anchors.fill: parent
            color: Material.theme !== Material.Dark ? "#FFFFFF" : "#000000"
            opacity: 0.8
        }

        PageIndicator {
            id: moreDataPageIndicator
            count: moreDataSwipeView.count
            currentIndex: moreDataSwipeView.currentIndex
            y: 0
            anchors.horizontalCenter: parent.horizontalCenter
            opacity: 0.8
        }

        SwipeView {
            id: moreDataSwipeView
            y: moreDataPageIndicator.height
            width: parent.width
            height: popupLocationData.implicitHeight + 16

            Component.onCompleted: {
                setMoreDataPageIndex(App.settings.mapMoreDataViewIndex)
            }

            onCurrentIndexChanged: {
                setMoreDataPageIndex(currentIndex)
            }

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
                    width: Math.min(moreDataSwipeView.height, page.width) - 16
                    height: Math.min(moreDataSwipeView.height, page.width) - 16
                    active: moreDataSwipeView.currentIndex === 1 // index of "skyplot" tab.
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
                            setMoreDataPageIndex(2) // index of "goto" tab.
                        }
                    }
                }
            }
        }
    }

    function setMoreDataPageIndex(index) {
        if (App.settings.mapMoreDataViewIndex !== index) {
            App.settings.mapMoreDataViewIndex = index
        }

        if (moreDataSwipeView.currentIndex !== index) {
            moreDataSwipeView.contentItem.highlightMoveDuration = 0
            moreDataSwipeView.contentItem.highlightMoveVelocity = -1
            moreDataSwipeView.currentIndex = index
        }
    }
}
