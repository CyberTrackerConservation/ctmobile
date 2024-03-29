import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import QtGraphicalEffects 1.0
import CyberTracker 1.0 as C

ContentPage {
    id: page

    property bool required: false
    property bool backOnSet: true

    header: PageHeader {
        id: pageHeader
        text: qsTr("Collect as")
    }

    ColumnLayout {
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width - 32
        y: 8
        spacing: 0

        SquareIcon {
            Layout.alignment: Qt.AlignHCenter
            size: parent.width * 0.2
            source: "qrc:/icons/account.svg"
            opacity: 0.5
        }

        TextField {
            id: username
            Layout.fillWidth: true
            font.pixelSize: App.settings.font16
            text: App.settings.username
        }

        ColorButton {
            Layout.fillWidth: true
            Layout.preferredHeight: Style.minRowHeight
            text: qsTr("Set name")
            font.pixelSize: App.settings.font12
            font.bold: true
            font.capitalization: Font.MixedCase
            color: Material.accent
            enabled: !page.required || username.text.trim() !== ""
            onClicked: {
                App.settings.username = username.text.trim()

                if (backOnSet) {
                    pageHeader.sendBackClick()
                } else {
                    Qt.inputMethod.hide()
                }
            }
        }

        Item { height: 8 }

        Label {
            Layout.fillWidth: true
            font.pixelSize: App.settings.font14
            wrapMode: Label.WordWrap
            text: qsTr("Data will be tagged with this name. This setting applies to all %1.").arg(App.alias_projects)
            opacity: 0.5
        }
    }
}
