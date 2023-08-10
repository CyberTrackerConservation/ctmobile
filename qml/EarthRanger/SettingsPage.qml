import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.0
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ContentPage {
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
            width: parent.width
            spacing: 0

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
                            text: qsTr("Last updated")
                            elide: Text.ElideRight
                            wrapMode: Label.WordWrap
                        }

                        Label {
                            Layout.fillWidth: true
                            font.pixelSize: App.settings.font12
                            opacity: 0.5
                            text: form.project.lastUpdate
                            elide: Text.ElideRight
                            wrapMode: Label.WordWrap
                            visible: text !== ""
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
                             elide: Text.ElideRight
                             wrapMode: Label.WordWrap
                             text: form.project.loggedIn ? qsTr("Logged in as") : qsTr("Logged out")
                         }

                         Label {
                             Layout.fillWidth: true
                             color: Material.foreground
                             opacity: 0.5
                             font.pixelSize: App.settings.font12
                             elide: Text.ElideRight
                             text: form.project.username === "" ? "-" : form.project.username
                             wrapMode: Label.WordWrap
                         }
                     }

                     Button {
                         text: form.project.loggedIn ? qsTr("Logout") : qsTr("Login")
                         font.pixelSize: App.settings.font14
                         font.capitalization: Font.MixedCase
                         icon.source: form.project.loggedIn ? "qrc:/icons/logout.svg" : "qrc:/icons/login.svg"
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

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: C.Style.minRowHeight

                contentItem: RowLayout {
                    width: parent.width
                    height: parent.height

                    ColumnLayout {
                        Layout.fillWidth: true

                        Label {
                            text: qsTr("Reported by")
                            font.pixelSize: App.settings.font14
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                            wrapMode: Label.WordWrap
                        }

                        Label {
                            property string reportedBy: form.project.reportedBy
                            property bool reportedByValid: reportedBy !== ""
                            text: reportedByValid ? form.getElementName(reportedBy) : qsTr("Unknown")
                            color: reportedByValid ? Material.foreground : "red"
                            opacity: reportedByValid ? 0.5 : 1.0
                            font.italic: !reportedByValid
                            font.pixelSize: App.settings.font12
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }

                    C.ChevronRight {}
                }

               onClicked: form.pushPage("qrc:/EarthRanger/ReportedByPage.qml")

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: C.Style.minRowHeight

                contentItem: RowLayout {
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Merge categories")
                        font.pixelSize: App.settings.font14
                        wrapMode: Label.WordWrap
                    }

                    Switch {
                        id: checkMergeCategories
                        Layout.alignment: Qt.AlignRight
                        checked: form.getSetting("mergeCategories", false)
                        onCheckedChanged: {
                            form.setSetting("mergeCategories", checked)
                        }
                    }
                }

                onClicked: {
                    checkMergeCategories.checked = !checkMergeCategories.checked
                    checkMergeCategories.clicked()
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: C.Style.minRowHeight

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

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: C.Style.minRowHeight

                contentItem: RowLayout {
                    Label {
                        id: l
                        Layout.fillWidth: true
                        text: qsTr("Visible reports")
                        font.pixelSize: App.settings.font14
                        wrapMode: Label.WordWrap
                    }

                    C.ChevronRight {}
                }

                onClicked: form.pushPage("qrc:/EarthRanger/VisibleReportsPage.qml")

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
            return
        }
    }
}
