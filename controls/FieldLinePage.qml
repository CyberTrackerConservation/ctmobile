import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtPositioning
import CyberTracker as C

ContentPage {
    property alias recordUid: fieldLine.recordUid
    property alias fieldUid: fieldLine.fieldUid

    header: PageHeader {
        menuIcon: "qrc:/icons/check.svg"
        menuVisible: true
        onMenuClicked: form.popPage(StackView.Immediate)

        backPrompt: fieldLine.backPrompt
        backPromptText: fieldLine.backPromptText
        backPromptConfirmText: fieldLine.backPromptConfirmText

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
