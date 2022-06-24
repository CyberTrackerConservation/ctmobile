import QtQuick 2.12

Item {
    property url url
    readonly property int loadProgress: 0
    readonly property bool loading: false

    Text {
        anchors.fill: parent

        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter

        color: colors.white1
        font.pixelSize: 140 * dp
        text: "Browser unavailable"
    }
}
