import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import CyberTracker as C

C.ContentPage {
    id: page

    property var forms: []
    property var setFormSelected
    property var getFormName
    property var getFormSubText
    property var connectForms
    property bool backToRoot: false

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
        formBack: !page.backToRoot
        onBackClicked: {
            postPopToRoot()
        }
    }

    footer: RowLayout {
        spacing: 0

        C.FooterButton {
            text: qsTr("All")
            icon.source: "qrc:/icons/checkbox_multiple_marked.svg"
            onClicked: {
                for (let i = 0; i < page.forms.length; i++) {
                    setSelected(i, true)
                }

                formsListView.model = page.forms
                formsPageHeader.menuEnabled = true
            }
        }

        C.FooterButton {
            text: qsTr("None")
            icon.source: "qrc:/icons/checkbox_multiple_blank_outline.svg"
            onClicked: {
                for (let i = 0; i < forms.length; i++) {
                    setSelected(i, false)
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
                setSelected(model.index, checked)
                formsPageHeader.menuEnabled = getAnySelected()
            }
        }
    }

    function setSelected(formIndex, selected) {
        page.setFormSelected(formIndex, selected)
        let f = page.forms[formIndex]
        f.__selected = selected
        page.forms[formIndex] = f
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
