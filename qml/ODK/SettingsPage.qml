import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import QtGraphicalEffects 1.0
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    header: C.PageHeader {
        text: qsTr("Settings")
    }

    Flickable {
        anchors.fill: parent
        ScrollBar.vertical: ScrollBar { interactive: false }
        contentWidth: parent.width
        contentHeight: column.height

        ColumnLayout {
            id: column
            spacing: 0
            width: parent.width

            // Version and update check.
            ItemDelegate {
                Layout.fillWidth: true

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
                        enabled: form.project.loggedIn
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

                C.HorizontalDivider {}
            }

            // Logged in.
            ItemDelegate {
                Layout.fillWidth: true

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

                C.HorizontalDivider {}
            }

            // Reported by.
            ItemDelegate {
                Layout.fillWidth: true

                contentItem: RowLayout {
                    width: parent.width
                    height: parent.height

                    ColumnLayout {
                        Layout.fillWidth: true

                        Label {
                            Layout.fillWidth: true
                            font.pixelSize: App.settings.font14
                            text: qsTr("Collect as")
                            elide: Text.ElideRight
                            wrapMode: Label.WordWrap
                        }

                        Label {
                            Layout.fillWidth: true
                            font.pixelSize: App.settings.font12
                            opacity: 0.5
                            text: App.settings.username === "" ? "(" + form.project.username + ")" : App.settings.username
                            elide: Text.ElideRight
                            wrapMode: Label.WordWrap
                        }
                     }

                    C.ChevronRight {}
                }

                onClicked: form.pushPage("qrc:/imports/CyberTracker.1/UsernamePage.qml", { required: form.project.username === "" })

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                contentItem: RowLayout {
                    Label {
                        Layout.preferredWidth: page.width / 3
                        text: qsTr("Language")
                        font.pixelSize: App.settings.font14
                        wrapMode: Label.WordWrap
                    }

                    C.FormLanguageComboBox {
                        Layout.fillWidth: true
                    }
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                contentItem: RowLayout {
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Page mode")
                        font.pixelSize: App.settings.font14
                    }

                    Switch {
                        checked: form.project.wizardMode
                        onCheckedChanged: form.project.wizardMode = checked
                    }
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
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
        }
    }

    C.ConfirmPopup {
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
