import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import QtGraphicalEffects 1.0
import CyberTracker 1.0 as C

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

                C.SquareIcon {
                    source: "qrc:/icons/cogs.svg"
                    opacity: 0.5
                }

                C.ChevronRight {}
            }

            onClicked: {
                form.pushPage("qrc:/SettingsPage.qml")
            }

            C.HorizontalDivider {}
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
                            let result = form.fieldManager.rootField.version
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

                    onActivated: {
                        if (index != -1) {
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

            C.HorizontalDivider {}
        }

        ItemDelegate {
            Layout.fillWidth: true
            Layout.minimumHeight: C.Style.minRowHeight
            visible: form.supportLocationPoint
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

                    onActivated: {
                        if (index != -1) {
                            displayText = model[index].text
                            let newInterval = model[index].interval
                            if (newInterval === form.project.sendLocationInterval) {
                                return
                            }

                            form.project.sendLocationInterval = newInterval
                            if (newInterval === 0) {
                                form.pointStreamer.stop()
                            } else {
                                form.pointStreamer.start(newInterval * 1000)
                            }
                        }
                    }

                    Component.onCompleted: {
                        for (let i = 0; i < model.length; i++) {
                            if (model[i].interval === form.project.sendLocationInterval) {
                                currentIndex = i
                                displayText = model[i].text
                                return
                            }
                        }

                        displayText = qsTr("Custom interval")
                    }
                }
            }

            C.HorizontalDivider {}
        }
    }

    ConfirmPopup {
        id: confirmLogout
        text: qsTr("Logout?")
        onConfirmed: {
            form.project.logout()
            form.triggerLogin()
        }
    }

    function updateProject() {
        let result = App.projectManager.update(form.project.uid)
        if (result.success) {
            changeToProject(form.projectUid)
        }
    }
}
