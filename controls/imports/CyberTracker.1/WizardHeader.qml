import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

PageHeader {
    id: root

    property string recordUid
    property string fieldUid
    property var params

    Component.onCompleted: {
        if (getParam(params, "hidden", false)) {
            visible = false
            height = 0
        }
    }

    color: getParamColor(params, "color", Material.primary)
    textColor: getParamColor(params, "textColor", Utils.lightness(color) < 128 ? "white" : "black")

    topText: {
        if (params === undefined) {
            if (recordUid === form.wizard.recordUid) {
                return form.wizard.header
            }

            return form.getRecordName(recordUid)
        }

        return getParam(params, "topText", "")
    }

    text: getParam(params, "text", form.getFieldName(root.recordUid, root.fieldUid))

    formBack: false
    backIcon: form.editing ? findIcon("cancel", Style.cancelIconSource) : findIcon("home", Style.homeIconSource)
    backVisible: form.editing || !getParam(params, "hideHome", false)

    enum MiscButtonType { None, Save, Track, Battery }

    property int buttonType: {
        if (form.editing) {
            return WizardHeader.MiscButtonType.Save
        }

        let button = getParam(params, "button")
        if (button !== undefined) {
            switch (button.toString().toLowerCase()) {
            case "track":
                return WizardHeader.MiscButtonType.Track

            case "battery":
                return WizardHeader.MiscButtonType.Battery

            default:
                console.log("Error: unknown button type - " + button.toString())
            }
        }

        return WizardHeader.MiscButtonType.None
    }

    menuVisible: buttonType !== WizardHeader.MiscButtonType.None

    menuIcon: {
        switch (buttonType) {
        case WizardHeader.MiscButtonType.Save:
            return findIcon("confirm", Style.okIconSource)

        case WizardHeader.MiscButtonType.Track:
            return form.trackStreamer.rateIcon

        case WizardHeader.MiscButtonType.Battery:
            return App.batteryIcon

        default:
            return ""
        }
    }

    onBackClicked: {
        if (form.wizard.immersive && form.editing) {
            form.popPagesToParent()
            return
        }

        if (!form.wizard.immersive && !form.wizard.autoSave && form.hasRecordChanged(form.wizard.recordUid)) {
            popupConfirmHome.open()
            return
        }

        form.wizard.home()
    }

    onMenuClicked: {
        switch (buttonType) {
        case WizardHeader.MiscButtonType.None:
            break

        case WizardHeader.MiscButtonType.Save:
            saveStart()
            break

        case WizardHeader.MiscButtonType.Track:
            App.showToast(form.trackStreamer.rateFullText)
            break

        case WizardHeader.MiscButtonType.Battery:
            App.showToast(App.batteryText)
            break

        default:
            console.log("Error: unknown button type");
        }
    }

    function findIcon(name, defaultIcon) {
        let result = getParam(params, name + "Icon", undefined)
        if (result === undefined) {
            return defaultIcon
        }

        return App.projectManager.getFileUrl(form.projectUid, result)
    }
}
