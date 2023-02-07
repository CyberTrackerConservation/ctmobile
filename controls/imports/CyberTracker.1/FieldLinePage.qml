import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import Esri.ArcGISRuntime 100.15 as Esri
import QtGraphicalEffects 1.0
import QtPositioning 5.12
import CyberTracker 1.0 as C

ContentPage {
    property alias recordUid: fieldLine.recordUid
    property alias fieldUid: fieldLine.fieldUid

    header: PageHeader {
        menuIcon: "qrc:/icons/check.svg"
        menuVisible: true
        onMenuClicked: form.popPage(StackView.Immediate)
        text: {
            if (fieldUid === "") {
                return ""
            }

            switch (form.getFieldType(fieldUid))
            {
            case "LineField":
                return qsTr("Line")
            case "AreaField":
                return qsTr("Area")

            default:
                return ""
            }
        }
    }

    FieldLine {
        id: fieldLine
        anchors.fill: parent
    }
}
