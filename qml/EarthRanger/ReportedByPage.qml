import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import CyberTracker as C

C.ContentPage {
    header: C.PageHeader {
        text: qsTr("Reported by")
    }

    C.ListViewV {
        anchors.fill: parent

        model: C.ElementListModel {
            elementUid: "__reportedBy"
        }

        delegate: ItemDelegate {
            width: ListView.view.width

            contentItem: RowLayout {
                Label {
                    Layout.fillWidth: true
                    wrapMode: Label.WordWrap
                    font.bold: modelData.uid === form.project.reportedBy
                    font.pixelSize: App.settings.font14
                    verticalAlignment: Text.AlignVCenter
                    text: modelData.name
                }

                C.SquareIcon {
                    Layout.alignment: Qt.AlignRight
                    visible: modelData.uid === form.project.reportedBy
                    source: "qrc:/icons/ok.svg"
                }
            }

            onClicked: {
                form.project.reportedBy = modelData.uid
                if (form.getPageStack().length === 1) {
                    form.replaceLastPage(form.provider.startPage)
                    return
                }

                form.popPage(StackView.Immediate)
            }

            C.HorizontalDivider {}
        }
    }
}
