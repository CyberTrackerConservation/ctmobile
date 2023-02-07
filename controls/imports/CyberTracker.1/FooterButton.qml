import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12

Item {
    id: root

    property alias icon: button.icon
    property alias text: button.text
    property alias enabled: button.enabled

    property alias checkable: button.checkable
    property bool checked: false

    property bool highlight: false
    property bool highlightBlink: false

    property bool separatorTop: true
    property bool separatorRight: false
    property bool separatorLeft: false

    property var colorOverride

    property double extraScale: 1.0

    signal clicked()

    onCheckedChanged: {
        internal.mutex = true
        try {
            if (button.checkable && root.checked !== button.checked) {
                button.checked = root.checked
            }
        } finally {
            internal.mutex = false
        }
    }

    Component.onCompleted: {
        internal.mutex = true
        try {
            if (button.checkable) {
                button.checked = root.checked
            }
        } finally {
            internal.mutex = false
        }
    }

    QtObject {
        id: internal
        property bool mutex: false
        property bool groupCheck: root.ButtonGroup.group !== null
        property color highlightTextColor: Utils.lightness(root.Material.accentColor) < 128 ? "white" : "black"
        property color highlightColor: root.Material.accentColor
    }

    implicitWidth: findWidth()
    implicitHeight: button.implicitHeight

    SequentialAnimation {
        running: root.enabled && root.highlightBlink
        loops: 3
        PropertyAnimation { target: highlightRectangle; property: "color"; to: root.Material.accentColor; duration: 0 }
        PropertyAnimation { target: highlightRectangle; property: "color"; to: root.Material.accentColor; duration: 250 }
        PropertyAnimation { target: highlightRectangle; property: "color"; to: colorOverride || Style.colorToolbar; duration: 250 }
        PropertyAnimation { target: highlightRectangle; property: "color"; to: colorOverride || Style.colorToolbar; duration: 250 }
        PropertyAnimation { target: highlightRectangle; property: "color"; to: root.Material.accentColor; duration: 250 }
    }

    Rectangle {
        width: parent.width
        height: Style.lineWidth2
        y: -Style.lineWidth2
        color: Style.colorGroove
        visible: root.separatorTop
    }

    Rectangle {
        width: Style.lineWidth2
        height: root.height
        color: Style.colorGroove
        visible: root.separatorLeft
    }

    Rectangle {
        x: root.width - Style.lineWidth2
        width: Style.lineWidth2
        height: root.height
        color: Style.colorGroove
        visible: root.separatorRight
    }

    Rectangle {
        id: highlightRectangle
        anchors.fill: button
        color: internal.highlightColor
        visible: root.highlight
    }

    ToolButton {
        id: button
        anchors.fill: parent
        display: text !== "" && App.settings.footerText ? ToolButton.TextUnderIcon : ToolButton.IconOnly
        font.pixelSize: App.settings.font10 * root.extraScale
        font.capitalization: Font.MixedCase
        icon.width: Style.toolButtonSize * root.extraScale
        icon.height: Style.toolButtonSize * root.extraScale
        Material.accent: root.Material.foreground
        Material.foreground: {
            if (root.colorOverride || false) {
                return root.colorOverride
            }

            if (root.highlight) {
                return internal.highlightTextColor
            }

            return root.Material.foreground
        }

        focusPolicy: Qt.NoFocus
        ButtonGroup.group: root.ButtonGroup.group

        onCheckedChanged: {
            if (internal.mutex || !checkable) {
                return
            }

            button.forceActiveFocus()
            root.checked = checked
            if (checked || !internal.groupCheck) {
                root.clicked()
            }
        }

        onClicked: {
            if (internal.mutex || checkable) {
                return
            }

            button.forceActiveFocus()
            root.clicked()
        }

        opacity: root.colorOverride ? 1.0 : (root.highlight ? 1.0 : (root.checkable ? (root.checked ? 1.0 : 0.5) : 0.5))
    }

    function findWidth() {
        let visibleCount = 0
        for (let i = 0; i < parent.children.length; i++) {
            let b = parent.children[i]
            if (b.visible && b.name === root.name) {
                visibleCount++
            }
        }

        return visibleCount === 0 ? 0 : parent.width / visibleCount
    }
}
