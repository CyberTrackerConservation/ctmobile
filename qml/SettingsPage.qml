import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import CyberTracker as C

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
                        opacity: 0.5
                    }

                    C.ChevronRight {}
                }

                onClicked: {
                    pushPageImmediate(page, "qrc:/AboutPage.qml")
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: C.Style.minRowHeight
                visible: App.settings.archiveSupport
                contentItem: RowLayout {
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Upload queue")
                        font.pixelSize: App.settings.font14
                        font.capitalization: Font.MixedCase
                        font.bold: false
                        wrapMode: Label.WordWrap
                    }

                    C.SquareIcon {
                        source: "qrc:/icons/tray_full.svg"
                        opacity: 0.5
                    }

                    C.ChevronRight {}
                }

                onClicked: {
                    pushPageImmediate(page, "qrc:/UploadQueuePage.qml")
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: C.Style.minRowHeight
                visible: App.settings.archiveSupport
                contentItem: RowLayout {
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Data archive")
                        font.pixelSize: App.settings.font14
                        font.capitalization: Font.MixedCase
                        font.bold: false
                        wrapMode: Label.WordWrap
                    }

                    C.SquareIcon {
                        source: "qrc:/icons/archive_outline.svg"
                        opacity: 0.5
                    }

                    C.ChevronRight {}
                }

                onClicked: {
                    pushPageImmediate(page, "qrc:/ArchiveConfigPage.qml")
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: C.Style.minRowHeight
                visible: typeof(formPageStack) === "undefined"
                contentItem: RowLayout {
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Edit %1").arg(App.alias_projects)
                        font.pixelSize: App.settings.font14
                        font.capitalization: Font.MixedCase
                        font.bold: false
                        wrapMode: Label.WordWrap
                    }

                    C.SquareIcon {
                        source: "qrc:/icons/clipboard_multiple_outline.svg"
                        opacity: 0.5
                    }

                    C.ChevronRight {}
                }

                onClicked: {
                    pushPageImmediate(page, "qrc:/ProjectsEditPage.qml")
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
                        opacity: 0.5
                    }

                    C.ChevronRight {}
                }

                onClicked: {
                    pushPageImmediate(page, "qrc:/OfflineMapPage.qml")
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: C.Style.minRowHeight
                contentItem: RowLayout {
                    ColumnLayout {
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Map projection")
                            font.pixelSize: App.settings.font14
                            font.capitalization: Font.MixedCase
                            font.bold: false
                            wrapMode: Label.WordWrap
                        }

                        Label {
                            Layout.fillWidth: true
                            text: App.translateText(App.settings.projection.d)
                            font.pixelSize: App.settings.font12
                            font.capitalization: Font.MixedCase
                            wrapMode: Label.WordWrap
                            opacity: 0.5
                        }
                    }

                    C.SquareIcon {
                        source: "qrc:/icons/earth_box.svg"
                        opacity: 0.5
                    }

                    C.ChevronRight {}
                }

                onClicked: {
                    pushPageImmediate(page, "qrc:/ProjectionPage.qml")
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
                        opacity: 0.5
                    }

                    C.ChevronRight {}
                }

                onClicked: {
                    pushPageImmediate(page, "qrc:/imports/CyberTracker/UsernamePage.qml", { required: (typeof(formPageStack) !== "undefined") && form.project.username === "" })
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

                        onActivated: function (index) {
                            if (index !== -1) {
                                App.settings.languageCode = languages[index].code
                            }
                        }

                        Component.onCompleted: {
                            var modelArray = []
                            var modelIndex = 0
                            let languageCode = App.settings.languageCode
                            for (let i = 0; i < languages.length; i++) {
                                let n = languages[i].name
                                let nn = languages[i].nativeName
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
                    Label {
                        Layout.preferredWidth: page.width / 3
                        text: qsTr("Auto launch")
                        font.pixelSize: App.settings.font14
                        wrapMode: Label.WordWrap
                    }

                    ComboBox {
                        id: comboAutoLaunch
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        font.pixelSize: App.settings.font14

                        onActivated: function (index) {
                            if (index !== -1) {
                                let project = model[index]
                                App.settings.autoLaunchProjectUid = project.uid
                                displayText = project.title
                            }
                        }

                        C.ProjectListModel {
                            id: projectListModel

                            onChanged: {
                                let modelArr = [{uid: "", title: "" }]
                                let index = -1

                                for (let i = 0; i < count; i++) {
                                    let project = get(i)
                                    modelArr.push({ uid: project.uid, title: project.title })
                                    if (project.uid === App.settings.autoLaunchProjectUid) {
                                        index = i
                                    }
                                }

                                comboAutoLaunch.model = modelArr
                                comboAutoLaunch.currentIndex = index
                                comboAutoLaunch.displayText = index === -1 ? "" : get(index).title
                            }
                        }

                        delegate: ItemDelegate {
                            width: comboAutoLaunch.width
                            font.pixelSize: App.settings.font14
                            text: modelData.title
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
                        text: qsTr("Dark mode")
                        font.pixelSize: App.settings.font14
                        wrapMode: Label.WordWrap
                    }

                    Switch {
                        id: checkTheme
                        Layout.alignment: Qt.AlignRight
                        scale: C.Style.switchScale
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
                    Label {
                        Layout.preferredWidth: page.width / 3
                        text: qsTr("Theme")
                        font.pixelSize: App.settings.font14
                        wrapMode: Label.WordWrap
                    }

                    ComboBox {
                        id: comboTheme
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        font.pixelSize: App.settings.font14
                        model: C.Style.themes
                        currentIndex: {
                            var current = App.settings.theme || App.config.defaultTheme || ""
                            for (var i = 0; i < C.Style.themes.length; ++i) {
                                if (C.Style.themes[i].name === current) return i
                            }
                            return -1
                        }
                        displayText: currentIndex === -1 ? "" : C.Style.themeTitle(C.Style.themes[currentIndex].name)
                        onActivated: function (index) {
                            if (index !== -1) {
                                App.settings.theme = C.Style.themes[index].name
                            }
                        }

                        delegate: ItemDelegate {
                            width: comboTheme.width
                            font.pixelSize: App.settings.font14
                            text: C.Style.themeTitle(modelData.name)
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
                        scale: C.Style.switchScale
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
                visible: App.config.allowUnits
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
                        scale: C.Style.switchScale
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
                contentItem: RowLayout {
                    spacing: 0

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("AI form filler")
                        font.pixelSize: App.settings.font14
                        wrapMode: Label.WordWrap
                    }

                    Switch {
                        id: checkFormFiller
                        Layout.alignment: Qt.AlignRight
                        scale: C.Style.switchScale
                        checked: App.settings.formFiller
                        onCheckedChanged: {
                            App.settings.formFiller = checkFormFiller.checked
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
                        text: qsTr("Require Bluetooth for audio recording")
                        font.pixelSize: App.settings.font14
                        wrapMode: Label.WordWrap
                    }

                    Switch {
                        id: bluetoothAudio
                        Layout.alignment: Qt.AlignRight
                        scale: C.Style.switchScale
                        checked: App.settings.bluetoothAudio
                        onCheckedChanged: {
                            App.settings.bluetoothAudio = bluetoothAudio.checked
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
                        scale: C.Style.switchScale
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
                    property var modelStrings: ["100%", "115%", "125%", "150%", "175%", "200%"]
                    property var modelNumbers: [100, 115, 125, 150, 175, 200]
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
                        onActivated: function (index) {
                            if (index !== -1) {
                                App.settings.fontSize = fontSizeRow.modelNumbers[index]
                            }
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
                    spacing: App.scaleByFontSize(4)

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
                        scale: C.Style.switchScale
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
                            simulationSourcePopup.open()
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
                        onActivated: function (index) {
                            if (index !== -1) {
                                App.settings.simulateGPSFilename = index === 0 ? "" : model[index]
                            }
                        }

                        delegate: ItemDelegate {
                            width: comboSimulateGPSFilename.width
                            contentItem: Label {
                                font.pixelSize: App.settings.font14
                                fontSizeMode: Label.Fit
                                text: modelData
                            }
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
                        scale: C.Style.switchScale
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
                Layout.minimumHeight: C.Style.minRowHeight
                contentItem: RowLayout {
                    spacing: 0

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Copy photos to gallery")
                        font.pixelSize: App.settings.font14
                        wrapMode: Label.WordWrap
                    }

                    Switch {
                        id: checkReplicateMedia
                        Layout.alignment: Qt.AlignRight
                        scale: C.Style.switchScale
                        checked: App.settings.replicateMedia
                        onCheckedChanged: {
                            App.settings.replicateMedia = checkReplicateMedia.checked
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
                        opacity: 0.5
                    }
                }

                onClicked: {
                    pushBugReportPage(false)
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: C.Style.minRowHeight
                visible: App.settings.archiveSupport
                contentItem: RowLayout {
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Activity log")
                        font.pixelSize: App.settings.font14
                        font.capitalization: Font.MixedCase
                        font.bold: false
                        wrapMode: Label.WordWrap
                    }

                    C.SquareIcon {
                        source: "qrc:/icons/clipboard_text_outline.svg"
                        opacity: 0.5
                    }

                    C.ChevronRight {}
                }

                onClicked: {
                    pushPageImmediate(page, "qrc:/AuditPage.qml")
                }

                C.HorizontalDivider {}
            }

            C.FooterSpacer {}
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
        pushPageImmediate(this, "qrc:/BugReportPage.qml", { useSharingOnMobile: useSharingOnMobile })
    }
}
