import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import QtGraphicalEffects 1.0
import CyberTracker 1.0 as C

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
                    recolor: true
                    visible: modelData.uid === form.project.reportedBy || modelData.uid === form.project.reportedByDefault
                    source: modelData.uid === form.project.reportedBy ? "qrc:/icons/ok.svg" : "qrc:/icons/account.svg"
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
