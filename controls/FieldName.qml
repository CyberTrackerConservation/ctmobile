import QtQuick
import QtQuick.Controls.Material

Label {
    id: root

    property string recordUid
    property string fieldUid

    opacity: 0.6
    wrapMode: Label.Wrap
    font.pixelSize: App.settings.font16

    onFieldUidChanged: {
        text = form.getFieldName(recordUid, fieldUid)
    }
}
