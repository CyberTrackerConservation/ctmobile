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
        backVisible: false
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
                onClicked: Qt.openUrlExternally(App.config.webPageUrl)
                contentItem: ColumnLayout {
                    Layout.fillWidth: true
                    Image {
                        Layout.alignment: Qt.AlignHCenter
                        source: App.config.logo
                        sourceSize.height: 80
                    }

                    Label {
                        Layout.fillWidth: true
                        text: App.config.title
                        font.pixelSize: App.settings.font14
                        font.preferShaping: !text.includes("Cy") // Hack to fix iOS bug
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Label {
                        Layout.fillWidth: true
                        text: "<html><style type=\"text/css\"></style><a href=\"" + App.config.webPageUrl + "\">" + App.config.webPageTitle + "</a></html>"
                        font.pixelSize: App.settings.font12
                        font.preferShaping: !text.includes("Cy") // Hack to fix iOS bug
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Label {
                        Layout.fillWidth: true
                        text: App.buildString
                        font.pixelSize: App.settings.font12
                        horizontalAlignment: Text.AlignHCenter
                    }

                    ColumnLayout {
                        width: parent.width

                        Item {
                            implicitHeight: 8
                        }

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Powered by") + " CyberTracker, ArcGIS & Qt"
                            font.pixelSize: App.settings.font12
                            font.bold: true
                            font.preferShaping: !text.includes("Cy") // Hack to fix iOS bug
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Label.WordWrap
                        }

                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            Image {
                                source: "qrc:/app/appicon.svg"
                                sourceSize.width: 32
                                sourceSize.height: 32
                            }
                            Image {
                                source: "qrc:/app/esrilogo.svg"
                                sourceSize.height: 32
                            }

                            Image {
                                source: "qrc:/app/qtlogo.svg"
                                sourceSize.height: 32
                            }
                        }
                    }
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                onClicked: Qt.openUrlExternally(App.config.privacyStatementUrl)
                contentItem: Label {
                    font.pixelSize: App.settings.font14
                    horizontalAlignment: Label.AlignHCenter
                    verticalAlignment: Label.AlignVCenter
                    text: "<html><style type=\"text/css\"></style><a href=\"" + App.config.privacyStatementUrl + "\">" + qsTr("Privacy Statement") + "</a></html>"
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                onClicked: Qt.openUrlExternally(App.config.licenseUrl)
                contentItem: Label {
                    font.pixelSize: App.settings.font14
                    horizontalAlignment: Label.AlignHCenter
                    verticalAlignment: Label.AlignVCenter
                    text: "<html><style type=\"text/css\"></style><a href=\"" + App.config.licenseUrlUrl + "\">" + qsTr("Software License") + "</a></html>"
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: row0.height
                contentItem: Button {
                    text: qsTr("Check for project updates")
                    font.pixelSize: App.settings.font14
                    font.capitalization: Font.MixedCase
                    icon.source: "qrc:/icons/autorenew.svg"

                    onClicked: {
                        busyCover.doWork = syncProjects
                        busyCover.start()
                    }
                }

                visible: App.config.updateConnectors

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: row0.height
                visible: App.config.allowCollectAs
                contentItem: RowLayout {
                    ColumnLayout {
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Collect as")
                            font.pixelSize: App.settings.font14
                            font.capitalization: Font.MixedCase
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

                    Image {
                        source: "qrc:/icons/account.svg"
                        layer {
                            enabled: true
                            effect: ColorOverlay {
                                color: Material.foreground
                            }
                        }
                    }

                    C.ChevronRight {}
                }

                onClicked: {
                    appPageStack.push("qrc:/imports/CyberTracker.1/UsernamePage.qml")
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                id: row0
                Layout.fillWidth: true
                contentItem: RowLayout {
                    Label {
                        Layout.preferredWidth: page.width / 3
                        text: qsTr("Language")
                        font.pixelSize: App.settings.font14
                        wrapMode: Label.WordWrap
                    }

                    ComboBox {
                        Layout.fillWidth: true
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
                    }
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: row0.height
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
                        font.pixelSize: App.settings.font14
                        checked: App.settings.darkTheme
                        onClicked: App.settings.darkTheme = checkTheme.checked
                    }
                }

                onClicked: {
                    if (App.config.fullRowCheck) {
                        checkTheme.checked = !checkTheme.checked
                        checkTheme.clicked()
                    }
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: row0.height
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
                        onClicked: App.settings.metricUnits = checkMetricUnits.checked
                    }
                }

                onClicked: {
                    if (App.config.fullRowCheck) {
                        checkMetricUnits.checked = !checkMetricUnits.checked
                        checkMetricUnits.clicked()
                    }
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: row0.height
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
                        onClicked: {
                            appWindow.visibility = checkFullScreen.checked ? ApplicationWindow.FullScreen : ApplicationWindow.AutomaticVisibility
                            App.settings.fullScreen = checkFullScreen.checked
                        }
                    }
                }

                onClicked: {
                    if (App.config.fullRowCheck) {
                        checkFullScreen.checked = !checkFullScreen.checked
                        checkFullScreen.clicked()
                    }
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: row0.height
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
                        Layout.fillWidth: true
                        font.pixelSize: App.settings.font14
                        model: fontSizeRow.modelStrings
                        currentIndex: fontSizeRow.modelNumbers.indexOf(App.settings.fontSize)
                        onActivated: if (index != -1) {
                            App.settings.fontSize = fontSizeRow.modelNumbers[index]
                        }
                    }
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: row0.height
                contentItem: RowLayout {
                    Label {
                        Layout.preferredWidth: page.width / 3
                        text: qsTr("Coordinates")
                        font.pixelSize: App.settings.font14
                        wrapMode: Label.WordWrap
                    }

                    ComboBox {
                        Layout.fillWidth: true
                        font.pixelSize: App.settings.font14
                        model: [ qsTr("Decimal degrees"), qsTr("Degrees minutes seconds"), qsTr("Degrees decimal minutes"), "UTM"]
                        currentIndex: App.settings.coordinateFormat
                        onActivated: {
                            if (index != -1) {
                                App.settings.coordinateFormat = index
                            }
                        }
                    }
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: row0.height
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
                        onClicked: App.settings.locationAccuracyMeters = App.settings.locationAccuracyMeters + 1
                        enabled: App.settings.locationAccuracyMeters < 100
                        autoRepeat: true
                    }

                    Label {
                        id: gpsAccuracyLabel
                        Layout.preferredWidth: Math.max(gpsAccuracyLabel.implicitWidth, 32)
                        text: App.settings.locationAccuracyMeters
                        font.pixelSize: App.settings.font16
                        font.bold: true
                        opacity: 0.75
                        horizontalAlignment: Qt.AlignHCenter
                    }

                    ToolButton {
                        icon.source: "qrc:/icons/minus.svg"
                        onClicked: App.settings.locationAccuracyMeters = App.settings.locationAccuracyMeters - 1
                        enabled: App.settings.locationAccuracyMeters > 1
                        autoRepeat: true
                    }
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: row0.height
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
                        onClicked: App.settings.simulateGPS = checkSimulateGPS.checked
                    }
                }

                onClicked: {
                    if (App.config.fullRowCheck) {
                        checkSimulateGPS.checked = !checkSimulateGPS.checked
                        checkSimulateGPS.clicked()
                    }
                }
            }

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: row0.height
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
                        onClicked: {
                            if (App.requestPermissionReadExternalStorage()) {
                                simulationSourcePopup.open()
                            }
                        }
                    }

                    ComboBox {
                        Layout.fillWidth: true
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
                    }
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: row0.height
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
                        onClicked: App.settings.uploadRequiresWiFi = checkUploadRequiresWiFi.checked
                    }
                }

                onClicked: {
                    if (App.config.fullRowCheck) {
                        checkUploadRequiresWiFi.checked = !checkUploadRequiresWiFi.checked
                        checkUploadRequiresWiFi.clicked()
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

                    Image {
                        source: "qrc:/icons/share_variant.svg"
                        layer {
                            enabled: true
                            effect: ColorOverlay {
                                color: Material.foreground
                            }
                        }
                    }

                    C.ChevronRight {}
                }

                onClicked: {
                    if (App.requestPermissionReadExternalStorage()) {
                        appPageStack.push("qrc:/BugReportPage.qml", { useSharingOnMobile: true })
                    }
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

                    Image {
                        source: "qrc:/icons/usb.svg"
                        layer {
                            enabled: true
                            effect: ColorOverlay {
                                color: Material.foreground
                            }
                        }
                    }

                    C.ChevronRight {}
                }

                onClicked: {
                    if (App.requestPermissionReadExternalStorage()) {
                        appPageStack.push("qrc:/BugReportPage.qml", { useSharingOnMobile: false })
                    }
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

    function syncProjects() {
        if (!Utils.networkAvailable()) {
            showToast(qsTr("Offline"))
            return
        }

        App.projectManager.updateAll();
    }
}
