import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import CyberTracker as C

Flickable {
    id: root

    ScrollBar.vertical: ScrollBar { interactive: false }
    contentWidth: width
    contentHeight: column.height

    ColumnLayout {
        id: column
        spacing: 0
        width: parent.width

        ItemDelegate {
            Layout.fillWidth: true
            Layout.minimumHeight: C.Style.minRowHeight
            contentItem: RowLayout {
                Label {
                    Layout.fillWidth: true
                    text: qsTr("System settings")
                    font.pixelSize: App.settings.font14
                    font.capitalization: Font.MixedCase
                    font.bold: false
                    wrapMode: Label.WordWrap
                }

                SquareIcon {
                    source: "qrc:/icons/cogs.svg"
                    opacity: 0.5
                }

                ChevronRight {}
            }

            onClicked: {
                form.pushPage("qrc:/SettingsPage.qml")
            }

            HorizontalDivider {}
        }


        // Version and update check.
        ItemDelegate {
            Layout.fillWidth: true
            Layout.minimumHeight: C.Style.minRowHeight

            contentItem: RowLayout {
                width: parent.width
                height: parent.height

                ColumnLayout {
                    Layout.fillWidth: true

                    Label {
                        Layout.fillWidth: true
                        font.pixelSize: App.settings.font14
                        text: qsTr("Form version")
                        elide: Text.ElideRight
                        wrapMode: Label.WordWrap
                    }

                    Label {
                        Layout.fillWidth: true
                        font.pixelSize: App.settings.font12
                        opacity: 0.5
                        text: {
                            let result = form.schema.version
                            if (result === "") {
                                result = qsTr("Not specified")
                            }
                            return result
                        }
                        elide: Text.ElideRight
                        wrapMode: Label.WordWrap
                    }
                 }

                Button {
                    text: qsTr("Update")
                    font.pixelSize: App.settings.font14
                    font.capitalization: Font.MixedCase
                    icon.source: "qrc:/icons/autorenew.svg"
                    Layout.alignment: Qt.AlignRight
                    enabled: form.project.loggedIn && App.projectManager.canUpdate(form.project.uid)
                    onClicked: {
                        if (Utils.networkAvailable()) {
                            busyCover.doWork = updateProject
                            busyCover.start()
                        } else {
                            showToast(qsTr("Offline"));
                        }
                    }
                 }
            }

            HorizontalDivider {}
        }

        // Logged in.
        ItemDelegate {
            Layout.fillWidth: true
            Layout.minimumHeight: C.Style.minRowHeight
            visible: App.projectManager.canLogin(form.project.uid)

            contentItem: RowLayout {
                width: parent.width
                height: parent.height

                ColumnLayout {
                    Layout.fillWidth: true

                    Label {
                        Layout.fillWidth: true
                        font.pixelSize: App.settings.font14
                        text: form.project.loggedIn ? qsTr("Logged in as") : qsTr("Logged out")
                        elide: Text.ElideRight
                        wrapMode: Label.WordWrap
                    }

                    Label {
                        Layout.fillWidth: true
                        font.pixelSize: App.settings.font12
                        opacity: 0.5
                        text: form.project.username === "" ? "-" : form.project.username
                        elide: Text.ElideRight
                        wrapMode: Label.WordWrap
                    }
                 }

                Button {
                    text: form.project.loggedIn ? qsTr("Logout") : qsTr("Login")
                    font.pixelSize: App.settings.font14
                    icon.source: form.project.loggedIn ? "qrc:/icons/logout.svg" : "qrc:/icons/login.svg"
                    font.capitalization: Font.MixedCase
                    Layout.alignment: Qt.AlignRight
                    onClicked: {
                        if (form.project.loggedIn) {
                           confirmLogout.open()
                        } else {
                            form.triggerLogin()
                        }
                    }
                 }
            }

            HorizontalDivider {}
        }

        ItemDelegate {
            Layout.fillWidth: true
            Layout.minimumHeight: C.Style.minRowHeight
            visible: formLanguageComboBox.model.length > 0

            contentItem: RowLayout {
                Label {
                    Layout.preferredWidth: root.width / 3
                    text: qsTr("Language")
                    font.pixelSize: App.settings.font14
                    wrapMode: Label.WordWrap
                }

                FormLanguageComboBox {
                    id: formLanguageComboBox
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
            }

            HorizontalDivider {}
        }

        ItemDelegate {
            Layout.fillWidth: true
            Layout.minimumHeight: C.Style.minRowHeight

            visible: false
            contentItem: RowLayout {
                spacing: 0

                ColumnLayout {
                    Layout.fillWidth: true

                    Label {
                        Layout.fillWidth: true
                        font.pixelSize: App.settings.font14
                        text: qsTr("Immersive mode")
                        elide: Text.ElideRight
                        wrapMode: Label.WordWrap
                    }

                    Label {
                        Layout.fillWidth: true
                        font.pixelSize: App.settings.font12
                        opacity: 0.5
                        text: qsTr("Hide sighting home page")
                        elide: Text.ElideRight
                        wrapMode: Label.WordWrap
                    }
                }

                Switch {
                    id: checkImmersive
                    Layout.alignment: Qt.AlignRight
                    scale: C.Style.switchScale
                    font.pixelSize: App.settings.font14
                    checked: form.project.immersive
                    onCheckedChanged: {
                        if (form.project.immersive === checkImmersive.checked) {
                            return
                        }

                        form.setImmersive(checkImmersive.checked)
                        changeToProject(form.project.uid)
                    }
                }
            }

            HorizontalDivider {}
        }

        ItemDelegate {
            Layout.fillWidth: true
            Layout.minimumHeight: C.Style.minRowHeight

            visible: form.wizard.immersive === false
            contentItem: RowLayout {
                ColumnLayout {
                    Layout.fillWidth: true

                    Label {
                        Layout.fillWidth: true
                        font.pixelSize: App.settings.font14
                        text: qsTr("Page mode")
                        elide: Text.ElideRight
                        wrapMode: Label.WordWrap
                    }

                    Label {
                        Layout.fillWidth: true
                        font.pixelSize: App.settings.font12
                        opacity: 0.5
                        text: qsTr("One page per question")
                        elide: Text.ElideRight
                        wrapMode: Label.WordWrap
                    }
                }

                Switch {
                    scale: C.Style.switchScale
                    checked: form.project.wizardMode
                    onCheckedChanged: {
                        form.project.wizardMode = checked
                    }
                }
            }

            HorizontalDivider {}
        }

        ItemDelegate {
            Layout.fillWidth: true
            Layout.minimumHeight: C.Style.minRowHeight

            contentItem: RowLayout {
                Label {
                    Layout.preferredWidth: root.width / 3
                    text: qsTr("Auto-submit")
                    font.pixelSize: App.settings.font14
                    wrapMode: Label.WordWrap
                }

                ComboBox {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    font.pixelSize: App.settings.font14

                    model: [
                        { text: qsTr("Off"), interval: 0 },
                        { text: qsTr("On save"), interval: 1 },
                        { text: qsTr("5 minutes"), interval: 5 * 60 },
                        { text: qsTr("10 minutes"), interval: 10 * 60 },
                        { text: qsTr("30 minutes"), interval: 30 * 60 },
                        { text: qsTr("1 hour"), interval: 60 * 60 }
                    ]

                    delegate: ItemDelegate {
                        width: parent.width
                        text: modelData.text
                        font.pixelSize: App.settings.font14
                    }

                    onActivated: function (index) {
                        if (index !== -1) {
                            displayText = model[index].text
                            let newInterval = model[index].interval
                            if (newInterval === form.project.submitInterval) {
                                return
                            }

                            form.project.submitInterval = newInterval
                        }
                    }

                    Component.onCompleted: {
                        for (let i = 0; i < model.length; i++) {
                            if (model[i].interval === form.project.submitInterval) {
                                currentIndex = i
                                displayText = model[i].text
                                return
                            }
                        }

                        displayText = qsTr("Custom interval")
                    }
                }
            }

            HorizontalDivider {}
        }

        ItemDelegate {
            Layout.fillWidth: true
            Layout.minimumHeight: C.Style.minRowHeight
            enabled: form.project.defaultLocationServiceUrl === "" // Enabled if no default specified
            visible: enabled && form.supportLocationTrackService() // Visible if no default and supported
            contentItem: RowLayout {
                spacing: 0
                ColumnLayout {
                    Layout.fillWidth: true

                    Label {
                        Layout.fillWidth: true
                        font.pixelSize: App.settings.font14
                        text: qsTr("Location service URL")
                        elide: Text.ElideRight
                        wrapMode: Label.WordWrap
                    }

                    TextField {
                        id: locationServiceUrl
                        Layout.fillWidth: true
                        font.pixelSize: App.settings.font12
                        text: form.project.locationServiceUrl
                        wrapMode: Label.WordWrap
                        inputMethodHints: Qt.ImhEmailCharactersOnly | Qt.ImhNoAutoUppercase | Qt.ImhPreferLowercase
                        onTextChanged: {
                            form.project.esriLocationServiceState = ({})
                            form.project.locationServiceUrl = locationServiceUrl.text.trim()
                        }
                    }
                }

                ToolButton {
                    icon.source: "qrc:/icons/database_cog.svg"
                    icon.width: Style.toolButtonSize
                    icon.height: Style.toolButtonSize
                    opacity: 0.5
                    onClicked: {
                        form.pushPage(form.provider.locationServicePage)
                    }
                }
            }

            HorizontalDivider {}
        }

        ItemDelegate {
            Layout.fillWidth: true
            Layout.minimumHeight: C.Style.minRowHeight
            enabled: form.project.defaultTrackFieldUid === ""
            visible: enabled && form.supportLocationTrackField()
            contentItem: RowLayout {
                Label {
                    Layout.preferredWidth: root.width / 3
                    text: qsTr("Track file output")
                    font.pixelSize: App.settings.font14
                    wrapMode: Label.WordWrap
                }

                ToolButton {
                    icon.source: "qrc:/icons/info.svg"
                    icon.color: Material.accent
                    icon.width: C.Style.toolButtonSize
                    icon.height: C.Style.toolButtonSize
                    onClicked: {
                        trackFieldPopup.open()
                    }
                }

                ComboBox {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    font.pixelSize: App.settings.font14

                    model: form.trackFieldCandidates()

                    currentIndex: model.indexOf(form.project.trackFieldUid)

                    delegate: ItemDelegate {
                        width: parent.width
                        text: modelData
                        font.pixelSize: App.settings.font14
                    }

                    onActivated: function (index) {
                        if (index !== -1) {
                            form.project.trackFieldUid = model[index]

                            if (form.project.trackFileFormat === "") {
                                form.project.trackFileFormat = "kmz"
                            }
                        }
                    }
                }
            }

            HorizontalDivider {}
        }

        ItemDelegate {
            Layout.fillWidth: true
            Layout.minimumHeight: C.Style.minRowHeight
            enabled: allowTrackCustomization() && form.project.defaultTrackFileFormat === ""
            visible: enabled && form.supportLocationTrackField() && form.project.trackFieldUid !== ""
            contentItem: RowLayout {
                Label {
                    Layout.preferredWidth: root.width / 3
                    text: qsTr("Track format")
                    font.pixelSize: App.settings.font14
                    wrapMode: Label.WordWrap
                }

                ComboBox {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    font.pixelSize: App.settings.font14

                    model: [ "kml", "kmz", "geojson" ]

                    currentIndex: {
                        let result  = model.indexOf(form.project.trackFileFormat)
                        return result === -1 ? 0 : result;
                    }

                    delegate: ItemDelegate {
                        width: parent.width
                        text: modelData
                        font.pixelSize: App.settings.font14
                    }

                    onActivated: function (index) {
                        if (index !== -1) {
                            form.project.trackFileFormat = model[index]
                        }
                    }
                }
            }

            HorizontalDivider {}
        }

        ItemDelegate {
            Layout.fillWidth: true
            Layout.minimumHeight: C.Style.minRowHeight
            enabled: allowTrackCustomization() && form.project.defaultLocationPointInterval === 0
            visible: enabled && form.supportLocationTrackService() && form.project.locationServiceUrl !== ""
            contentItem: RowLayout {
                Label {
                    Layout.preferredWidth: root.width / 3
                    text: qsTr("Autosend location")
                    font.pixelSize: App.settings.font14
                    wrapMode: Label.WordWrap
                }

                ComboBox {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    font.pixelSize: App.settings.font14

                    model: [
                        { text: qsTr("Off"), interval: 0 },
                        { text: qsTr("30 seconds"), interval: 30 },
                        { text: qsTr("5 minutes"), interval: 5 * 60 },
                        { text: qsTr("10 minutes"), interval: 10 * 60 },
                        { text: qsTr("30 minutes"), interval: 30 * 60 },
                        { text: qsTr("1 hour"), interval: 60 * 60 }
                    ]

                    delegate: ItemDelegate {
                        width: parent.width
                        text: modelData.text
                        font.pixelSize: App.settings.font14
                    }

                    onActivated: function (index) {
                        if (index !== -1) {
                            displayText = model[index].text
                            form.project.locationPointInterval = model[index].interval
                        }
                    }

                    Component.onCompleted: {
                        for (let i = 0; i < model.length; i++) {
                            if (model[i].interval === form.project.locationPointInterval) {
                                currentIndex = i
                                displayText = model[i].text
                                return
                            }
                        }

                        displayText = qsTr("Custom interval")
                    }
                }
            }

            HorizontalDivider {}
        }

        ItemDelegate {
            Layout.fillWidth: true
            Layout.minimumHeight: C.Style.minRowHeight
            enabled: allowTrackCustomization() && form.project.defaultLocationTrackInterval === 0
            visible: enabled && ((form.supportLocationTrackService() && form.project.locationServiceUrl !== "") || (form.supportLocationTrackField() && form.project.trackFieldUid !== ""))
            contentItem: RowLayout {
                Label {
                    Layout.preferredWidth: root.width / 3
                    text: qsTr("Track timer")
                    font.pixelSize: App.settings.font14
                    wrapMode: Label.WordWrap
                }

                ComboBox {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    font.pixelSize: App.settings.font14

                    model: [
                        { text: qsTr("Off"), interval: 0 },
                        { text: qsTr("5 seconds"), interval: 5 },
                        { text: qsTr("10 seconds"), interval: 10 },
                        { text: qsTr("30 seconds"), interval: 30 },
                        { text: qsTr("60 seconds"), interval: 60 },
                        { text: qsTr("5 minutes"), interval: 60 * 5 }
                    ]

                    delegate: ItemDelegate {
                        width: parent.width
                        text: modelData.text
                        font.pixelSize: App.settings.font14
                    }

                    onActivated: function (index) {
                        if (index !== -1) {
                            displayText = model[index].text
                            form.project.locationTrackInterval = model[index].interval
                        }
                    }

                    Component.onCompleted: {
                        for (let i = 0; i < model.length; i++) {
                            if (model[i].interval === form.project.locationTrackInterval) {
                                currentIndex = i
                                displayText = model[i].text
                                return
                            }
                        }

                        displayText = qsTr("Custom interval")
                    }
                }
            }

            HorizontalDivider {}
        }

        ItemDelegate {
            Layout.fillWidth: true
            Layout.minimumHeight: C.Style.minRowHeight
            enabled: allowTrackCustomization() && form.project.defaultLocationTrackDistance === 0
            visible: enabled && ((form.supportLocationTrackService() && form.project.locationServiceUrl !== "") || (form.supportLocationTrackField() && form.project.trackFieldUid !== ""))
            contentItem: RowLayout {
                Label {
                    Layout.preferredWidth: root.width / 3
                    text: qsTr("Track minimum distance")
                    font.pixelSize: App.settings.font14
                    wrapMode: Label.WordWrap
                }

                ComboBox {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    font.pixelSize: App.settings.font14

                    model: [
                        { text: qsTr("None"), distance: 0 },
                        { text: qsTr("1 meter"), distance: 1 },
                        { text: qsTr("2 meters"), distance: 2 },
                        { text: qsTr("3 meters"), distance: 3 },
                        { text: qsTr("5 meters"), distance: 5 },
                        { text: qsTr("10 meters"), distance: 10 },
                        { text: qsTr("20 meters"), distance: 20 },
                        { text: qsTr("50 meters"), distance: 50 }
                    ]

                    delegate: ItemDelegate {
                        width: parent.width
                        text: modelData.text
                        font.pixelSize: App.settings.font14
                    }

                    onActivated: function (index) {
                        if (index !== -1) {
                            displayText = model[index].text
                            form.project.locationTrackDistance = model[index].distance
                        }
                    }

                    Component.onCompleted: {
                        for (let i = 0; i < model.length; i++) {
                            if (model[i].distance === form.project.locationTrackDistance) {
                                currentIndex = i
                                displayText = model[i].text
                                return
                            }
                        }

                        displayText = qsTr("Custom interval")
                    }
                }
            }

            HorizontalDivider {}
        }

        FooterSpacer {}
    }

    ConfirmPopup {
        id: confirmLogout
        text: qsTr("Logout?")
        onConfirmed: {
            form.project.logout()
            form.triggerLogin()
        }
    }

    PopupBase {
        id: trackFieldPopup

        contentItem: ColumnLayout {
            Label {
                Layout.fillWidth: true
                text: qsTr("Track file output")
                font.pixelSize: App.settings.font16
                wrapMode: Label.WordWrap
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
            }

            Label {
                Layout.preferredWidth: page.width * 0.7
                text: qsTr("Add a file question to your form. The appearance property should be set to 'hidden'. Once specified, the location logging feature can be enabled.")
                font.pixelSize: App.settings.font12
                wrapMode: Label.WordWrap
            }
        }
    }

    function allowTrackCustomization() {
        return form.project.defaultLocationServiceUrl === "" && form.project.defaultTrackFieldUid === ""
    }

    function updateProject() {
        let result = App.projectManager.update(form.project.uid)
        if (result.success) {
            changeToProject(form.projectUid)
        }
    }
}
