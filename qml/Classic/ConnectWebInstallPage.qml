import QtQml 2.12
import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    header: C.PageHeader {
        text: qsTr("Web Install")
    }

    Component.onCompleted: {
        Qt.inputMethod.show()
    }

    ColumnLayout {
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width * 0.8
        spacing: App.scaleByFontSize(4)

        Item { height: parent.spacing }

        C.SquareIcon {
            Layout.alignment: Qt.AlignHCenter
            source: "qrc:/Classic/logo.svg"
            size: C.Style.iconSize64
        }

        TextField {
            id: server
            Layout.fillWidth: true
            horizontalAlignment: TextField.AlignHCenter
            font.pixelSize: App.settings.font14
            inputMethodHints: Qt.ImhUrlCharactersOnly
            placeholderText: qsTr("Enter web address")
        }

        C.ColorButton {
            Layout.fillWidth: true
            text: qsTr("Install")
            font.pixelSize: App.settings.font12
            font.bold: true
            font.capitalization: Font.MixedCase
            color: Material.accent
            enabled: server.text !== ""
            onClicked: {
                busyCover.doWork = connectWebInstall
                busyCover.start()
            }
        }
    }

    function connectWebInstall() {
        let result = App.projectManager.webInstall(server.text)
        if (!result.success) {
            showError(result.errorString)
            return // Allow user to retry.
        }

        postPopToRoot()
    }
}
