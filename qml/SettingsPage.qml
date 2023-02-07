import QtQml 2.12
import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12
import QtGraphicalEffects 1.0
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

//    Component.onCompleted: {
//        for (var i = 0; i < languages.length; i++) {
//            console.log(languages[i].name)
//        }
//    }

    // Generated using "translator -l languages.json"
    property var languages: App.languages

    header: C.PageHeader {
        text: qsTr("Settings")
        backVisible: appPageStack.depth > 1
    }

    Flickable {
        anchors.fill: parent
        ScrollBar.vertical: ScrollBar { interactive: false }
        contentWidth: parent.width
        contentHeight: column.height

        ColumnLayout {
            id: column
            width: parent.width
            spacing: 0

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: C.Style.minRowHeight
                contentItem: RowLayout {
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("About")
                        font.pixelSize: App.settings.font14
                        font.capitalization: Font.MixedCase
                        font.bold: false
                        wrapMode: Label.WordWrap
                    }

                    C.SquareIcon {
                        source: "qrc:/icons/info.svg"
                        recolor: true
                        opacity: 0.5
                    }

                    C.ChevronRight {}
                }

                onClicked: {
                    if (typeof(formPageStack) !== "undefined") {
                        form.pushPage("qrc:/AboutPage.qml")
                    } else {
                        appPageStack.push("qrc:/AboutPage.qml")
                    }
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: C.Style.minRowHeight
                contentItem: RowLayout {
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Offline maps")
                        font.pixelSize: App.settings.font14
                        font.capitalization: Font.MixedCase
                        font.bold: false
                        wrapMode: Label.WordWrap
                    }

                    C.SquareIcon {
                        source: "qrc:/icons/map_plus.svg"
                        recolor: true
                        opacity: 0.5
                    }

                    C.ChevronRight {}
                }

                onClicked: {
                    if (typeof(formPageStack) !== "undefined") {
                        form.pushPage("qrc:/OfflineMapPage.qml")
                    } else {
                        appPageStack.push("qrc:/OfflineMapPage.qml")
                    }
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: C.Style.minRowHeight
                visible: App.config.allowCollectAs
                contentItem: RowLayout {
                    ColumnLayout {
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Collect as")
                            font.pixelSize: App.settings.font14
                            font.capitalization: Font.MixedCase
                            font.bold: false
                            wrapMode: Label.WordWrap
                        }

                        Label {
                            Layout.fillWidth: true
                            text: App.settings.username === "" ? "-" : App.settings.username
                            font.pixelSize: App.settings.font12
                            font.capitalization: Font.MixedCase
                            wrapMode: Label.WordWrap
                            opacity: 0.5
                        }
                    }

                    C.SquareIcon {
                        source: "qrc:/icons/account.svg"
                        recolor: true
                        opacity: 0.5
                    }

                    C.ChevronRight {}
                }

                onClicked: {
                    if (typeof(formPageStack) !== "undefined") {
                        form.pushPage("qrc:/imports/CyberTracker.1/UsernamePage.qml", { required: form.project.username === "" })
                    } else {
                        appPageStack.push("qrc:/imports/CyberTracker.1/UsernamePage.qml")
                    }
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: C.Style.minRowHeight
                contentItem: RowLayout {
                    Label {
                        Layout.preferredWidth: page.width / 3
                        text: qsTr("Language")
                        font.pixelSize: App.settings.font14
                        wrapMode: Label.WordWrap
                    }

                    ComboBox {
                        id: comboLanguage
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        font.pixelSize: App.settings.font14

                        onActivated: {
                            if (index != -1) {
                                App.settings.languageCode = languages[index].code
                            }
                        }

                        Component.onCompleted: {
                            var modelArray = []
                            var modelIndex = 0
                            let languageCode = App.settings.languageCode
                            for (var i = 0; i < languages.length; i++) {
                                var n = languages[i].name
                                var nn = languages[i].nativeName
                                modelArray.push(n === nn ? n : n + " (" + nn + ")")

                                if (languages[i].code === languageCode) {
                                    modelIndex = i;
                                }
                            }

                            model = modelArray
                            currentIndex = modelIndex
                        }

                        delegate: ItemDelegate {
                            width: comboLanguage.width
                            font.pixelSize: App.settings.font14
                            text: modelData
                        }
                    }
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: C.Style.minRowHeight
                contentItem: RowLayout {
                    spacing: 0
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Dark theme")
                        font.pixelSize: App.settings.font14
                        wrapMode: Label.WordWrap
                    }

                    Switch {
                        id: checkTheme
                        Layout.alignment: Qt.AlignRight
                        checked: App.settings.darkTheme
                        onCheckedChanged: {
                            App.settings.darkTheme = checkTheme.checked
                        }
                    }
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: C.Style.minRowHeight
                contentItem: RowLayout {
                    spacing: 0
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Toolbar captions")
                        font.pixelSize: App.settings.font14
                        wrapMode: Label.WordWrap
                    }

                    Switch {
                        id: checkFooterText
                        Layout.alignment: Qt.AlignRight
                        checked: App.settings.footerText
                        onCheckedChanged: {
                            App.settings.footerText = checkFooterText.checked
                        }
                    }
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: C.Style.minRowHeight
                contentItem: RowLayout {
                    spacing: 0

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Metric units")
                        font.pixelSize: App.settings.font14
                        wrapMode: Label.WordWrap
                    }

                    Switch {
                        id: checkMetricUnits
                        Layout.alignment: Qt.AlignRight
                        checked: App.settings.metricUnits
                        onCheckedChanged: {
                            App.settings.metricUnits = checkMetricUnits.checked
                        }
                    }
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: C.Style.minRowHeight
                visible: Qt.platform.os === "android"
                contentItem: RowLayout {
                    spacing: 0

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Full screen")
                        font.pixelSize: App.settings.font14
                        wrapMode: Label.WordWrap
                    }

                    Switch {
                        id: checkFullScreen
                        Layout.alignment: Qt.AlignRight
                        checked: App.settings.fullScreen
                        onCheckedChanged: {
                            appWindow.visibility = checkFullScreen.checked ? ApplicationWindow.FullScreen : ApplicationWindow.AutomaticVisibility
                            App.settings.fullScreen = checkFullScreen.checked
                        }
                    }
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: C.Style.minRowHeight
                contentItem: RowLayout {
                    id: fontSizeRow
                    property var modelStrings: ["100%", "125%", "150%", "175%", "200%"]
                    property var modelNumbers: [100, 125, 150, 175, 200]
                    Label {
                        Layout.preferredWidth: page.width / 3
                        text: qsTr("Font size")
                        font.pixelSize: App.settings.font14
                        wrapMode: Label.WordWrap
                    }

                    ComboBox {
                        id: comboFontSize
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        font.pixelSize: App.settings.font14
                        model: fontSizeRow.modelStrings
                        currentIndex: fontSizeRow.modelNumbers.indexOf(App.settings.fontSize)
                        onActivated: if (index != -1) {
                            App.settings.fontSize = fontSizeRow.modelNumbers[index]
                        }

                        delegate: ItemDelegate {
                            width: comboFontSize.width
                            font.pixelSize: App.settings.font14
                            text: modelData
                        }
                    }
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: C.Style.minRowHeight
                contentItem: RowLayout {
                    Label {
                        Layout.preferredWidth: page.width / 3
                        text: qsTr("Coordinates")
                        font.pixelSize: App.settings.font14
                        wrapMode: Label.WordWrap
                    }

                    ComboBox {
                        id: comboCoordinates
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        font.pixelSize: App.settings.font14
                        model: [ qsTr("Decimal degrees"), qsTr("Degrees minutes seconds"), qsTr("Degrees decimal minutes"), "UTM"]
                        currentIndex: App.settings.coordinateFormat
                        onActivated: {
                            if (index != -1) {
                                App.settings.coordinateFormat = index
                            }
                        }

                        delegate: ItemDelegate {
                            width: comboCoordinates.width
                            font.pixelSize: App.settings.font14
                            text: modelData
                        }
                    }
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: C.Style.minRowHeight
                contentItem: RowLayout {
                    spacing: 4

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("GPS accuracy") + " (" + qsTr("meters") + ")"
                        font.pixelSize: App.settings.font14
                        wrapMode: Label.WordWrap
                    }

                    ToolButton {
                        icon.source: "qrc:/icons/plus.svg"
                        icon.width: C.Style.toolButtonSize
                        icon.height: C.Style.toolButtonSize
                        onClicked: App.settings.locationAccuracyMeters = App.settings.locationAccuracyMeters + 1
                        enabled: App.settings.locationAccuracyMeters < 100
                        opacity: 0.5
                        autoRepeat: true
                    }

                    Label {
                        id: gpsAccuracyLabel
                        Layout.preferredWidth: Math.max(gpsAccuracyLabel.implicitWidth, 32)
                        text: App.settings.locationAccuracyMeters
                        font.pixelSize: App.settings.font16
                        font.bold: true
                        horizontalAlignment: Qt.AlignHCenter
                    }

                    ToolButton {
                        icon.source: "qrc:/icons/minus.svg"
                        icon.width: C.Style.toolButtonSize
                        icon.height: C.Style.toolButtonSize
                        onClicked: App.settings.locationAccuracyMeters = App.settings.locationAccuracyMeters - 1
                        enabled: App.settings.locationAccuracyMeters > 1
                        opacity: 0.5
                        autoRepeat: true
                    }
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: C.Style.minRowHeight
                visible: App.config.allowSimulateGPS || App.desktopOS
                contentItem: RowLayout {
                    spacing: 0
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Simulate location")
                        font.pixelSize: App.settings.font14
                        wrapMode: Label.WordWrap
                    }

                    Switch {
                        id: checkSimulateGPS
                        Layout.alignment: Qt.AlignRight
                        checked: App.settings.simulateGPS
                        onCheckedChanged: {
                            App.settings.simulateGPS = checkSimulateGPS.checked
                        }
                    }
                }
            }

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: C.Style.minRowHeight
                visible: App.config.allowSimulateGPS || App.desktopOS
                topPadding: 0
                contentItem: RowLayout {
                    spacing: 0
                    Label {
                        Layout.preferredWidth: page.width / 3
                        text: qsTr("Source file")
                        font.pixelSize: App.settings.font14
                        wrapMode: Label.WordWrap
                    }

                    ToolButton {
                        icon.source: "qrc:/icons/info.svg"
                        icon.color: Material.accent
                        icon.width: C.Style.toolButtonSize
                        icon.height: C.Style.toolButtonSize
                        onClicked: {
                            if (App.requestPermissionReadExternalStorage()) {
                                simulationSourcePopup.open()
                            }
                        }
                    }

                    ComboBox {
                        id: comboSimulateGPSFilename
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        font.pixelSize: App.settings.font14
                        model: App.settings.simulateGPSFiles
                        currentIndex: {
                            if (App.settings.simulateGPSFilename === "") {
                                return 0
                            } else {
                                return model.indexOf(App.settings.simulateGPSFilename)
                            }
                        }
                        onActivated: {
                            if (index != -1) {
                                App.settings.simulateGPSFilename = index === 0 ? "" : model[index]
                            }
                        }

                        delegate: ItemDelegate {
                            width: comboSimulateGPSFilename.width
                            font.pixelSize: App.settings.font14
                            text: modelData
                        }
                    }
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: C.Style.minRowHeight
                visible: App.desktopOS || Qt.platform.os === "android"
                contentItem: RowLayout {
                    spacing: 0

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Upload requires WiFi")
                        font.pixelSize: App.settings.font14
                        wrapMode: Label.WordWrap
                    }

                    Switch {
                        id: checkUploadRequiresWiFi
                        Layout.alignment: Qt.AlignRight
                        checked: App.settings.uploadRequiresWiFi
                        onCheckedChanged: {
                            App.settings.uploadRequiresWiFi = checkUploadRequiresWiFi.checked
                        }
                    }
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                contentItem: RowLayout {
                    ColumnLayout {
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Share bug report")
                            font.pixelSize: App.settings.font14
                            font.capitalization: Font.MixedCase
                            wrapMode: Label.WordWrap
                        }

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Share over email, text, etc.")
                            font.pixelSize: App.settings.font12
                            font.capitalization: Font.MixedCase
                            wrapMode: Label.WordWrap
                        }
                    }

                    C.SquareIcon {
                        source: "qrc:/icons/share_variant.svg"
                        recolor: true
                        opacity: 0.5
                    }
                }

                onClicked: {
                    pushBugReportPage(true)
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                visible: Qt.platform.os === "android"

                contentItem: RowLayout {
                    ColumnLayout {
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Save bug report")
                            font.pixelSize: App.settings.font14
                            font.capitalization: Font.MixedCase
                            wrapMode: Label.WordWrap
                        }

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Save to device for manual copy")
                            font.pixelSize: App.settings.font12
                            font.capitalization: Font.MixedCase
                            wrapMode: Label.WordWrap
                        }
                    }

                    C.SquareIcon {
                        source: "qrc:/icons/usb.svg"
                        recolor: true
                        opacity: 0.5
                    }
                }

                onClicked: {
                    pushBugReportPage(false)
                }

                C.HorizontalDivider {}
            }
        }
    }

    C.PopupBase {
        id: simulationSourcePopup

        contentItem: ColumnLayout {
            Label {
                Layout.fillWidth: true
                text: qsTr("Simulation source")
                font.pixelSize: App.settings.font16
                wrapMode: Label.WordWrap
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
            }

            Label {
                Layout.preferredWidth: page.width * 0.7
                text: qsTr("Generate track file using") + " https://nmeagen.org"
                font.pixelSize: App.settings.font12
                wrapMode: Label.WordWrap
            }

            Label {
                Layout.preferredWidth: page.width * 0.7
                text: qsTr("Copy to the Download folder")
                font.pixelSize: App.settings.font12
                wrapMode: Label.WordWrap
            }

            Label {
                Layout.preferredWidth: page.width * 0.7
                text: qsTr("Restart")
                font.pixelSize: App.settings.font12
                wrapMode: Label.WordWrap
            }
        }
    }

    function pushBugReportPage(useSharingOnMobile) {
        if (!App.requestPermissionReadExternalStorage()) {
            return
        }

        if (typeof(formPageStack) !== "undefined") {
            form.pushPage("qrc:/BugReportPage.qml", { useSharingOnMobile: useSharingOnMobile })
        } else {
            appPageStack.push("qrc:/BugReportPage.qml", { useSharingOnMobile: useSharingOnMobile })
        }
    }
}
