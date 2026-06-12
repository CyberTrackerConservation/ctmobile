import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Dialogs
import QtCore
import CyberTracker as C

C.ContentPage {
    id: page

    header: C.PageHeader {
        text: qsTr("Activity log")
    }

    footer: RowLayout {
        spacing: 0

        C.FooterButton {
            text: qsTr("Share")
            icon.source: "qrc:/icons/share_variant_outline.svg"
            enabled: listView.model.length !== 0
            onClicked: {
                busyCover.doWork = shareAudit
                busyCover.start()
            }
        }

        C.FooterButton {
            text: qsTr("Reset")
            icon.source: "qrc:/icons/delete_outline.svg"
            enabled: listView.model.length !== 0
            onClicked: {
                confirmClearLog.open()
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Pane {
            Layout.fillWidth: true
            padding: App.scaleByFontSize(12)

            background: Rectangle {
                anchors.fill: parent
                color: C.Style.colorContent
            }

            contentItem: ColumnLayout {
                spacing: 0

                Label {
                    text: qsTr("The activity log tracks activities and events within the application.")
                    font.pixelSize: App.settings.font16
                    Layout.fillWidth: true
                    wrapMode: Label.WordWrap
                    opacity: 0.75
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
                contentItem: ColumnLayout {
                    spacing: App.scaleByFontSize(4)

                    Label {
                        Layout.fillWidth: true
                        font.pixelSize: App.settings.font10
                        text: modelData.projectTitle
                        elide: Label.ElideRight
                        visible: text !== ""
                    }

                    Label {
                        Layout.fillWidth: true
                        font.pixelSize: App.settings.font14
                        text: modelData.description
                        elide: Label.ElideRight
                    }

                    Label {
                        font.pixelSize: App.settings.font12
                        text: modelData.timestamp
                    }
                }

                C.HorizontalDivider {}
            }

            Label {
                anchors.centerIn: parent
                visible: listView.model.length === 0
                font.pixelSize: App.settings.font16
                text: qsTr("No entries")
                opacity: 0.5
            }
        }
    }

    C.PopupLoader {
        id: confirmClearLog

        popupComponent: Component {
            C.ConfirmPopup {
                text: qsTr("Clear activity log?")
                subText: qsTr("This will permanently remove all activity log entries.")
                confirmText: qsTr("Yes, clear it")
                confirmDelay: true
                onConfirmed: {
                    App.audit.clear()
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

    function rebuild() {
        listView.model = App.audit.buildModel()
    }

    function shareAudit() {
        let csvFilePath = App.audit.createCSV()
        if (csvFilePath === "") {
            App.showError(qsTr("Failed to export activity log."))
            return
        }

        App.sendFile(csvFilePath, qsTr("Activity log"))
    }
} 
