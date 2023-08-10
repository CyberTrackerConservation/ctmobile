import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

Item {
    id: root

    property string recordUid
    property string fieldUid
    property alias icon: marker.icon

    width: marker.implicitWidth
    height: marker.implicitHeight
    visible: fieldBinding.complete ? fieldBinding.required || !fieldBinding.isValid : false

    C.FieldBinding {
        id: fieldBinding
        recordUid: root.recordUid
        fieldUid: root.fieldUid
    }

    ToolButton {
        id: marker
        anchors.fill: parent
        icon.source: "qrc:/icons/asterisk.svg"
        icon.width: App.scaleByFontSize(12)
        icon.height: App.scaleByFontSize(12)
        icon.color: Material.foreground
        opacity: 0.6
        enabled: false
    }
}
