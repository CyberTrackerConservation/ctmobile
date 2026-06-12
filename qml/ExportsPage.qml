import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import CyberTracker as C

C.ContentPage {
    id: page

    property string projectUid: ""

    QtObject {
        id: internal
        property var selectedInfo
    }

    header: C.PageHeader {
        text: qsTr("Exported data")
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
                    text: qsTr("The files listed below are awaiting import. If the import is not working, click on a file and send it using another channel.")
                    font.pixelSize: App.settings.font16
                    Layout.fillWidth: true
                    wrapMode: Label.WordWrap
                    opacity: 0.75
                }
            }
        }

        C.HorizontalDivider {
            Layout.fillWidth: true
            height: 2
        }

        C.ListViewV {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: []
            delegate: ItemDelegate {
                width: listView.width
                contentItem: RowLayout {
                    Label {
                        Layout.fillWidth: true
                        font.pixelSize: App.settings.font14
                        text: modelData.name
                    }

                    Label {
                        font.pixelSize: App.settings.font14
                        text: modelData.size
                    }

                    C.ChevronRight {}
                }

                onClicked: {
                    internal.selectedInfo = modelData
                    if (typeof(formPageStack) !== "undefined" && App.kioskMode) {
                        popupKioskCode.open()
                        return
                    }

                    showDetails()
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
        id: popupKioskCode

        popupComponent: Component {
            C.PasscodePopup {
                passcode: form.project.kioskModePin
                onSuccess: {
                    App.settings.kioskModeProjectUid = ""
                    showDetails()
                }
            }
        }
    }

    Component.onCompleted: {
        rebuild()
    }

    Connections {
        target: App.exportFile

        function onChanged() {
            rebuild()
        }
    }

    function rebuild() {
        App.exportFile.syncWithDownloads()
        listView.model = App.exportFile.buildModel(projectUid)
    }

    function showDetails() {
        pushPageImmediate(page, Qt.resolvedUrl("ExportDetailsPage.qml"), { fileInfo: internal.selectedInfo })
    }
}
