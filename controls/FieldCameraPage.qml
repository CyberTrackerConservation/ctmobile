import QtQuick
import QtQuick.Controls.Material
import CyberTracker as C

ContentPage {
    id: page

    property alias recordUid: camera.recordUid
    property alias fieldUid: camera.fieldUid
    property alias photoIndex: camera.photoIndex
    property alias resolutionX: camera.resolutionX
    property alias resolutionY: camera.resolutionY
    property alias simpleMode: camera.simpleMode

    header: PageHeader {
        text: fieldUid !== "" ? form.getFieldName(recordUid, fieldUid) : qsTr("Photo")
        menuIcon: "qrc:/icons/check.svg"
        menuVisible: true
        formBack: false
        onBackClicked: {
            let filename = camera.getFilename()
            if (filename !== "") {
                App.photoTaken(filename)
            }
            popPageDefault(page)
        }
        onMenuClicked: {
            let filename = camera.getFilename()
            if (filename !== "") {
                camera.commitAttachmentTags(filename)
                App.photoTaken(filename)
            }
            popPageImmediate(page)
        }
        menuEnabled: camera.getFilename() !== ""
    }

    FieldCamera {
        id: camera
        anchors.fill: parent
    }
}
