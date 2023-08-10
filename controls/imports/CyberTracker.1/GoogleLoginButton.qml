import QtQml 2.12
import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtGraphicalEffects 1.0

Rectangle {
    id: root

    signal clicked

    color: App.settings.darkTheme ? "#4285F4" : "white"
    radius: App.scaleByFontSize(1)

    implicitWidth: rowLayout.implicitWidth + App.scaleByFontSize(2)
    implicitHeight: rowLayout.implicitHeight + App.scaleByFontSize(2)

    RowLayout {
        id: rowLayout
        anchors.centerIn: parent
        width: root.width
        spacing: 0

        SquareIcon {
            source: "qrc:/Google/GoogleLogin.svg"
            size: App.scaleByFontSize(30)
        }

        Item { width: App.scaleByFontSize(8) }

        Label {
            Layout.alignment: Qt.AlignVCenter
            Layout.fillWidth: true
            text: qsTr("Sign in with Google")
            font.capitalization: Font.MixedCase
            font.pixelSize: App.settings.font13
            font.bold: true
            color: App.settings.darkTheme ? "#ffffff" : "#757575"
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        Item { width: App.scaleByFontSize(8) }
    }

    layer.enabled: true
    layer.effect: DropShadow {
        verticalOffset: 1
        horizontalOffset: 0
        color: App.settings.darkTheme ? "#40ffffff" : "#40000000"
        radius: 1
        samples: 3
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            root.clicked()
        }
    }
}
