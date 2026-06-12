import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import CyberTracker as C
import ".."

C.ContentPage {
    id: page

    property var accountName

    QtObject {
        id: internal

        property var account: page.accountName !== undefined ? App.config.accounts[page.accountName] : undefined
        property string landscape: ""
        property string token
        property var forms: []
    }

    header: C.PageHeader {
        text: qsTr("Connect to %1").arg(internal.account.title)
    }

    ColumnLayout {
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width * 0.8
        spacing: 0

        Item { height: 16 }

        C.SquareIcon {
            Layout.alignment: Qt.AlignHCenter
            size: parent.width * 0.2
            source: internal.account.icon
        }

        Item { height: 8 }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Enter user name")
            font.pixelSize: App.settings.font14
            font.bold: true
        }

        Item { height: 4 }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Use %1 for templates").arg("'" + page.accountName + "'")
            font.pixelSize: App.settings.font10
            visible: page.accountName !== ""
        }

        Item { height: 4 }

        TextField {
            id: textFieldLandscape
            Layout.fillWidth: true
            font.pixelSize: App.settings.font14
            horizontalAlignment: TextField.AlignHCenter
            focus: true
            inputMethodHints: Qt.ImhNoAutoUppercase
        }

        Item { height: 8 }

        C.ColorButton {
            Layout.fillWidth: true
            Layout.preferredHeight: C.Style.minRowHeight
            text: qsTr("Choose forms")
            font.pixelSize: App.settings.font12
            font.bold: true
            font.capitalization: Font.MixedCase
            color: Material.accent
            enabled: textFieldLandscape.text.trim() !== ""
            onClicked: {
                internal.landscape = textFieldLandscape.text.trim()
                busyCover.doWork = getToken
                busyCover.start()
            }
        }
    }

    Component {
        id: formsPage

        ConnectFormPage {
            forms: internal.forms
            setFormSelected: function(index, value) { internal.forms[index].__selected = value }
            getFormName: function(form) { return form.name; }
            getFormSubText: function(form) { return App.formatDateTime(form.date_modified); }
            connectForms: page.connectForms
        }
    }

    function getToken() {
        let response = Utils.httpGet(internal.account.server + "/token/?format=json", internal.account.username, internal.account.password)

        if (response.status === 200) {
            internal.token = JSON.parse(response.data).token
            if (internal.token.length > 0) {
                getForms()
                return
            }
        }

        showError(qsTr("Connection failed"))
    }

    function getForms() {
        internal.forms = Utils.getKoBoForms(internal.account.server, internal.token, internal.landscape)
        if (internal.forms.length > 0) {
            appPageStack.push(formsPage)
            return
        }

        showError(qsTr("No forms found"))
    }

    function connectForms() {
        for (let i = 0; i < internal.forms.length; i++) {
            let kform = internal.forms[i]
            if (kform.__selected !== true) {
                continue
            }

            let params = {
                account: page.accountName,
                token: internal.token,
                formId: kform.uid,
                formhubUuid: kform.formhubUuid
            }

            let result = App.projectManager.bootstrap("KoBo", params, false)
            if (!result.success && !result.expected) {
                showError(result.errorString)
                return
            }
        }

        App.projectManager.projectsChanged()

        postPopToRoot()
    }
}
