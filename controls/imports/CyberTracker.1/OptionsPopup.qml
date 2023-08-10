import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import QtGraphicalEffects 1.0

PopupBase {
    id: popup

    property string text: ""
    property string subText: ""
    property string icon: ""
    property color iconColor: Material.foreground
    property var model: []

    signal clicked(var index)

    width: parent.width * 0.75

    contentItem: ColumnLayout {
        width: popup.width
        spacing: 0

        Component.onCompleted: {
            if (typeof(formPageStack) !== "undefined") {
                formPageStack.setProjectColors(popup)
            }
        }

        Pane {
            id: pane
            Layout.fillWidth: true
            Binding { target: pane.background; property: "color"; value: "transparent" }
            contentItem: ColumnLayout {
                Image {
                    Layout.alignment: Image.AlignHCenter
                    Layout.preferredHeight: Style.iconSize48
                    source: popup.icon
                    sourceSize.width: Style.iconSize48
                    sourceSize.height: Style.iconSize48
                    layer {
                        enabled: true
                        effect: ColorOverlay {
                            color: popup.iconColor
                        }
                    }
                    visible: popup.icon !== ""
                }

                Label {
                    Layout.fillWidth: true
                    Layout.maximumHeight: Style.iconSize64
                    text: popup.text
                    font.pixelSize: App.settings.font16
                    font.bold: true
                    wrapMode: Label.Wrap
                    verticalAlignment: Label.AlignVCenter
                    horizontalAlignment: Label.AlignHCenter
                    visible: popup.text !== ""
                    elide: Label.ElideRight
                }

                Label {
                    Layout.fillWidth: true
                    text: popup.subText
                    font.pixelSize: App.settings.font12
                    wrapMode: Label.Wrap
                    visible: popup.subText !== ""
                    elide: Label.ElideRight
                }
            }
        }

        Item {
            Layout.fillWidth: true
            height: 8
            HorizontalDivider {}
        }

        Item {
            height: 8
        }

        Repeater {
            Layout.fillWidth: true

            model: popup.model

            DelayButton {
                id: delayButton

                property color highlightColor: Utils.changeAlpha(Material.foreground, 64)

                Layout.fillWidth: true
                delay: modelData.delay ? (App.desktopOS ? 500 : 1000) : 0
                hoverEnabled: true
                enabled: modelData.enabled === undefined || modelData.enabled === true
                visible: modelData.visible === undefined || modelData.visible === true

                background: Rectangle {
                    anchors.fill: delayButton
                    color: delayButton.hovered && delayButton.progress === 0 ? delayButton.highlightColor : "transparent"
                    opacity: 0.5

                    Rectangle {
                        height: parent.height
                        width: parent.width * delayButton.progress
                        color: modelData.delay ? Material.accent : delayButton.highlightColor
                        visible: delayButton.progress > 0
                    }
                }

                contentItem: RowLayout {
                    width: parent.width

                    Label {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        text: modelData.text
                        font.pixelSize: App.settings.font18
                        wrapMode: Label.Wrap
                        verticalAlignment: Label.AlignVCenter
                    }

                    SquareIcon {
                        Layout.preferredWidth: Style.iconSize24
                        source: modelData.icon
                        recolor: true
                        opacity: 0.5
                    }
                }

                onActivated: {
                    if (popup.opened) {
                        popup.close()
                        popup.clicked(model.index)
                    }
                }
            }
        }
    }
}
