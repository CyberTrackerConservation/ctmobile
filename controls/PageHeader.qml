import QtQuick
import QtQuick.Controls.Material

Rectangle {
    id: root

    property alias topText: topLabel.text
    property alias text: titleLabel.text
    property bool menuVisible: false
    property alias menuEnabled: menuButton.enabled
    property alias menuIcon: menuButton.icon.source
    property bool formBack: true
    property alias backIcon: backButton.icon.source
    property alias backEnabled: backButton.enabled
    property alias backVisible: backButton.visible
    property string textColor: Utils.lightness(Material.primary) < 130 ? "white" : "black"
    property bool fontBold: false

    property bool backPrompt: false
    property string backPromptText: ""
    property string backPromptConfirmText: ""

    color: Material.primary

    implicitHeight: Math.max(toolbarInternal.implicitHeight, titleRow.implicitHeight + App.scaleByFontSize(topLabel.visible ? 2 : 0))

    signal backClicked(var androidBack)
    signal menuClicked()

    function sendBackClick(backConfirmed = false, androidBack = false) {
        Qt.inputMethod.hide()

        if (backPrompt && !backConfirmed) {
            confirmBackPopup.open()
            return
        }

        if (formBack) {
            popPageDefault(root)
        } else {
            backClicked(androidBack)
        }
    }

    ToolBar {
        id: toolbarInternal
        visible: false
    }

    Row {
        id: titleRow
        width: parent.width
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
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width - backButton.width - menuButton.width
            spacing: 0

            Label {
                id: topLabel
                width: parent.width
                font.pixelSize: App.settings.font16
                font.bold: root.fontBold
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
                font.bold: root.fontBold
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

    ConfirmPopup {
        id: confirmBackPopup
        text: root.backPromptText
        confirmDelay: false
        confirmText: root.backPromptConfirmText
        onConfirmed: {
            sendBackClick(true)
        }
    }
}
