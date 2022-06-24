import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import QtGraphicalEffects 1.0
import CyberTracker 1.0 as C

C.ContentPage {
    header: C.PageHeader {
        text: qsTr("Reported by")
        menuIcon: "qrc:/icons/check.svg"
        menuVisible: true
        onMenuClicked: form.popPage(StackView.Immediate)
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
                    font.bold: modelData.uid === form.provider.reportedBy
                    font.pixelSize: App.settings.font14
                    verticalAlignment: Text.AlignVCenter
                    text: modelData.name
                }

                Image {
                    visible: modelData.uid === form.provider.reportedBy
                    Layout.alignment: Qt.AlignRight
                    source: "qrc:/icons/ok.svg"
                    sourceSize.width: 24
                    layer {
                        enabled: true
                        effect: ColorOverlay {
                            color: Material.foreground
                        }
                    }
                }
            }

            onClicked: {
                form.provider.reportedBy = modelData.uid
                form.popPage(StackView.Immediate)
            }

            C.HorizontalDivider {}
        }
    }
}
