import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import CyberTracker as C

C.ContentPage {
    id: page

    header: C.PageHeader {
        text: qsTr("Manage backups")
    }

    footer: RowLayout {
        spacing: 0

        C.FooterButton {
            text: qsTr("Share")
            icon.source: "qrc:/icons/share_variant_outline.svg"
            enabled: listView.model.length !== 0
            onClicked: {
                busyCover.doWork = shareBackupFiles
                busyCover.start()
            }
        }

        C.FooterButton {
            text: qsTr("Reset")
            icon.source: "qrc:/icons/delete_outline.svg"
            enabled: listView.model.length !== 0
            onClicked: {
                confirmClearBackups.open()
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Pane {
            Layout.fillWidth: true
            leftPadding: App.scaleByFontSize(8)
            rightPadding: App.scaleByFontSize(8)

            background: Rectangle {
                anchors.fill: parent
                color: C.Style.colorContent
            }

            contentItem: RowLayout {
                id: backupDays
                spacing: 0

                property var modelNumbers: [0, 7, 14, 30, 60, 90, -1]

                Label {
                    Layout.fillWidth: true
                    text: qsTr("Keep data for")
                    font.pixelSize: App.settings.font14
                    wrapMode: Label.WordWrap
                }

                function getText(index) {
                    if (index === -1) {
                        return ""
                    }

                    let value = backupDays.modelNumbers[index]
                    switch (value) {
                    case -1:
                        return qsTr("Indefinite")

                    case 0:
                        return qsTr("Do not keep")

                    default:
                        return qsTr("%1 days").arg(value.toString())
                    }
                }

                function getIndex() {
                    return backupDays.modelNumbers.indexOf(App.settings.backupFileQuotaDays)
                }

                ComboBox {
                    Layout.preferredWidth: page.width / 2
                    Layout.fillHeight: true
                    font.pixelSize: App.settings.font14
                    model: backupDays.modelNumbers.length
                    currentIndex: backupDays.getIndex()
                    onActivated: function (index) {
                        if (index !== -1) {
                            App.settings.backupFileQuotaDays = backupDays.modelNumbers[index]
                        }
                        displayText = backupDays.getText(index)
                    }

                    displayText: backupDays.getText(backupDays.getIndex())

                    delegate: ItemDelegate {
                        width: parent.width
                        font.pixelSize: App.settings.font14
                        text: backupDays.getText(model.index)
                    }
                }
            }
        }

        C.HorizontalDivider {
            Layout.fillWidth: true
            height: App.scaleByFontSize(2)
        }

        C.ListViewV {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: []
            delegate: ItemDelegate {
                width: listView.width
                opacity: 0.75

                contentItem: RowLayout {
                    width: parent.width
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: App.scaleByFontSize(4)

                        Label {
                            Layout.fillWidth: true
                            font.pixelSize: App.settings.font10
                            text: modelData.projectTitle
                        }

                        Label {
                            Layout.fillWidth: true
                            font.pixelSize: App.settings.font12
                            text: modelData.timestamp
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    Label {
                        font.pixelSize: App.settings.font12
                        text: modelData.size
                    }
                }

                C.HorizontalDivider {}
            }

            Label {
                anchors.centerIn: parent
                visible: listView.model.length === 0
                font.pixelSize: App.settings.font16
                text: qsTr("No files")
                opacity: 0.5
            }
        }
    }

    C.PopupLoader {
        id: confirmClearBackups

        popupComponent: Component {
            C.ConfirmPopup {
                text: qsTr("Delete backup files?")
                subText: qsTr("This will permanently remove all backup files.")
                confirmText: qsTr("Yes, clear them")
                confirmDelay: true
                onConfirmed: {
                    App.backupFile.reset()
                    rebuild()
                }
            }
        }
    }

    C.BusyCover {
        id: busyCover
    }

    Component.onCompleted: {
        rebuild()
    }

    Connections {
        target: App.backupFile

        function onChanged() {
            rebuild()
        }
    }

    function rebuild() {
        listView.model = App.backupFile.buildModel()
    }

    function shareBackupFiles() {
        // Create a zip file of all backup files
        let backupFile = App.backupFile.createZip()
        if (backupFile === "") {
            App.showError(qsTr("Failed to create backup"))
            return
        }

        App.sendFile(backupFile, "Data backup")
    }
} 
