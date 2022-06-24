import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12

Rectangle {
    id: root

    property alias topText: topLabel.text
    property alias text: titleLabel.text
    property alias textStyle: titleLabel.style
    property alias textStyleColor: titleLabel.styleColor
    property bool menuVisible: false
    property alias menuEnabled: menuButton.enabled
    property alias menuIcon: menuButton.icon.source
    property bool formBack: true
    property alias backIcon: backButton.icon.source
    property alias backEnabled: backButton.enabled
    property alias backVisible: backButton.visible

    property var textColor: Utils.lightness(Material.primary) < 128 ? "white" : "black"
    color: Material.primary

    implicitHeight: Math.max(toolbarInternal.implicitHeight, titleColumn.implicitHeight + 8)

    signal backClicked()
    signal menuClicked()

    function sendBackClick() {
        if (formBack) {
            Qt.inputMethod.hide()
            if (typeof(formPageStack) !== "undefined") {
                form.popPage()
            } else {
                appPageStack.pop()
            }

        } else {
            backClicked()
        }
    }

    ToolBar {
        id: toolbarInternal
        visible: false
    }

    Row {
        anchors.fill: parent
        spacing: 0

        ToolButton {
            id: backButton
            icon.source: Style.backIconSource
            icon.width: Style.toolButtonSize
            icon.height: Style.toolButtonSize
            icon.color: root.textColor
            opacity: enabled ? 1.0 : 0.5
            onClicked: sendBackClick()
        }

        Item {
            width: backButton.width
            height: backButton.height
            visible: !backButton.visible
        }

        Column {
            id: titleColumn
            width: parent.width - backButton.width - menuButton.width
            spacing: 0
            y: (root.implicitHeight - titleColumn.implicitHeight) / 2

            Label {
                id: topLabel
                width: parent.width
                font.pixelSize: App.settings.font16
                wrapMode: Label.WordWrap
                horizontalAlignment: Qt.AlignHCenter
                elide: Label.ElideRight
                clip: true
                color: root.textColor
                visible: text !== "" && text != titleLabel.text
            }

            Label {
                id: titleLabel
                width: parent.width
                height: Math.min(implicitHeight, 128)
                font.pixelSize: App.settings.font20
                font.preferShaping: !text.includes("Cy") // Hack to fix iOS bug
                wrapMode: Label.WordWrap
                horizontalAlignment: Qt.AlignHCenter
                elide: Label.ElideRight
                clip: true
                color: root.textColor
            }
        }

        ToolButton {
            id: menuButton
            icon.width: Style.toolButtonSize
            icon.height: Style.toolButtonSize
            icon.color: root.textColor
            visible: menuVisible
            opacity: enabled ? 1.0 : 0.5
            onClicked: menuClicked()
        }
    }
}
