import QtQml 2.12
import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    property var forms: []
    property var getFormName
    property var getFormSubText
    property var connectForms

    title: qsTr("Select forms")

    header: C.PageHeader {
        id: formsPageHeader
        text: page.title
        menuIcon: "qrc:/icons/check.svg"
        menuVisible: true
        menuEnabled: getAnySelected()
        onMenuClicked: {
            busyCover.doWork = page.connectForms
            busyCover.start()
        }
    }

    footer: RowLayout {
        spacing: 0

        C.FooterButton {
            text: qsTr("All")
            icon.source: "qrc:/icons/checkbox_multiple_marked.svg"
            onClicked: {
                for (let i = 0; i < internal.forms.length; i++) {
                    page.forms[i].__selected = true
                }

                formsListView.model = page.forms
                formsPageHeader.menuEnabled = true
            }
        }

        C.FooterButton {
            text: qsTr("None")
            icon.source: "qrc:/icons/checkbox_multiple_blank_outline.svg"
            onClicked: {
                for (let i = 0; i < internal.forms.length; i++) {
                    page.forms[i].__selected = false
                }

                formsListView.model = page.forms
                formsPageHeader.menuEnabled = false
            }
        }
    }

    C.ListViewV {
        id: formsListView
        anchors.fill: parent
        model: page.forms
        delegate: C.ListRowDelegate {
            width: ListView.view.width
            text: page.getFormName(modelData)
            subText: page.getFormSubText(modelData)
            checked: modelData.__selected === true
            checkable: true
            onCheckedChanged: {
                page.forms[model.index].__selected = checked
                formsPageHeader.menuEnabled = getAnySelected()
            }
        }
    }

    function getAnySelected() {
        for (let i = 0; i < page.forms.length; i++) {
            if (page.forms[i].__selected) {
                return true
            }
        }
        return false
    }
}
