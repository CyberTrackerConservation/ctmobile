import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

ToolButton {
    anchors.fill: parent
    icon.source: "qrc:/SMART/logo.svg"
    icon.width: 128
    icon.height: 128
    Material.foreground: Material.accent
    display: Button.TextUnderIcon
    text: qsTr("Tap to add project")
    font.pixelSize: App.settings.font18
    font.capitalization: Font.MixedCase
    onClicked: appPageStack.push("qrc:/SMART/ConnectCollectPage.qml")
}
