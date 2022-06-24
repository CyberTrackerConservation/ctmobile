import QtQuick 2.12
import QtQuick.Controls 2.12
import CyberTracker 1.0 as C

ContentPage {
    property alias recordUid: camera.recordUid
    property alias fieldUid: camera.fieldUid
    property alias photoIndex: camera.photoIndex
    property alias resolutionX: camera.resolutionX
    property alias resolutionY: camera.resolutionY

    header: C.PageHeader {
        text: fieldUid !== "" ? form.getFieldName(recordUid, fieldUid) : qsTr("Photo")
        menuIcon: "qrc:/icons/check.svg"
        menuVisible: true
        formBack: false
        onBackClicked: menuClicked()
        onMenuClicked: {
            let filename = camera.getFilename()
            if (filename !== "") {
                App.photoTaken(filename)
            }
            form.popPage(StackView.Immediate)
        }
    }

    C.FieldCamera {
        id: camera
        anchors.fill: parent
    }
}
