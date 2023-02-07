import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    property string elementUid
    property string banner: ""

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
                f.popPagesToStart()
                f.newSighting()
            }
        }
    }

    C.FieldListViewEditor {
        id: listView

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
