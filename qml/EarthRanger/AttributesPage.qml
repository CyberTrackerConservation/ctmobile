import QtQuick
import QtQuick.Controls.Material
import CyberTracker as C

C.ContentPage {
    id: page

    property string elementUid: form.getFieldValue("reportUid")
    property string banner: form.isSightingCompleted(form.rootRecordUid) ? form.provider.banner() : ""

    Component.onCompleted: {
        if (page.banner !== "") {
            listView.highlightRangeMode = ListView.NoHighlightRange
            listView.positionViewAtBeginning()
        }
    }

    header: C.PageHeader {
        text: form.getElementName(elementUid)
        menuIcon: "qrc:/icons/check.svg"
        menuVisible: true
        onMenuClicked: {
            if (listView.validate()) {
                let f = form
                f.saveSighting()
                f.markSightingCompleted()
                if (f.editing) {
                    f.popPagesToParent()
                    return
                }

                f.popPagesToStart()
                f.newSighting()
            }
        }
    }

    C.FieldListViewEditor {
        id: listView
        anchors.fill: parent
        recordUid: form.rootRecordUid
        filterElementUid: page.elementUid

        header: Rectangle {
            width: page.width
            height: page.banner === "" ? 0 : page.header.height
            color: Material.color(Material.Green, Material.Shade300)

            Label {
                anchors.fill: parent
                font.pixelSize: App.settings.font14
                fontSizeMode: Label.Fit
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
                text: page.banner
            }
        }
    }
}
