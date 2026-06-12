import QtQuick
import QtQuick.Controls.Material
import CyberTracker as C

C.ContentPage {
    id: page

    property int controlId: 0

    QtObject {
        id: internal
        property bool mutex: true
    }

    header: C.PageHeader {
        text: qsTr("Text Editor")
        menuIcon: "qrc:/icons/check.svg"
        menuVisible: true
        onMenuClicked: {
            form.provider.classicView.setTextEditValue(page.controlId, textArea.text)
            Qt.inputMethod.hide()
            popPageImmediate(page)
        }
    }

    TextArea {
        id: textArea
        anchors.fill: parent
        padding: App.scaleByFontSize(4)
        font.pixelSize: App.settings.font16
        wrapMode: Text.Wrap
        focus: true
        onTextChanged: {
            if (internal.mutex) {
                return
            }

            form.provider.classicView.setTextEditValue(page.controlId, textArea.text)
        }
    }

    Component.onCompleted: {
        Qt.callLater(() => {
            textArea.text = form.provider.classicView.getTextEditValue(page.controlId)
            Qt.inputMethod.show()
            textArea.forceActiveFocus()
            internal.mutex = false
        })
    }
}
