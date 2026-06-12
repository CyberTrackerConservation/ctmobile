import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtWebView
import CyberTracker as C

ContentPage {
    id: page

    property var canHome: () => { return true; }
    property var canBack: () => { return true; }
    property var canNext: () => { return true; }
    property var canSave: () => { return true; }

    QtObject {
        id: internal
        property var headerParams
        property var contentParams
        property var footerParams
        property bool autoNext: false
        property bool hasStyle: false
        property bool isActive: true
    }

    Component.onDestruction: {
        internal.isActive = false
    }

    C.FieldBinding {
        id: fieldBinding

        Component.onCompleted: {
            recordUid = form.wizard.lastPageRecordUid
            fieldUid = form.wizard.lastPageFieldUid

            internal.headerParams = form.getFieldParameter(recordUid, fieldUid, "header", {})
            internal.contentParams = form.getFieldParameter(recordUid, fieldUid, "content", {})
            internal.footerParams = form.getFieldParameter(recordUid, fieldUid, "footer", {})
            internal.autoNext = form.getFieldParameter(recordUid, fieldUid, "autoNext", false)
            internal.hasStyle = getParam(internal.contentParams, "style") !== undefined

            contentPane.padding = getParam(internal.contentParams, "frameWidth", App.scaleByFontSize(16))
            page.colorContent = getParamColor(internal.contentParams, "color", page.colorContent)
            page.colorHighlight = getParamColor(internal.contentParams, "colorHighlight", page.colorHighlight)
            page.colorFooter = getParamColor(internal.footerParams, "color", page.colorFooter)

            createPart(headerLoader, headerComponent, "header")
            createPart(footerLoader, footerComponent, "footer")
            createPart(contentLoader, contentComponent, "content")
        }
    }

    header: Loader { id: headerLoader }

    footer: Loader { id: footerLoader }

    Pane {
        id: contentPane
        anchors.fill: parent
        clip: true

        Material.background: colorContent || Style.colorContent

        Loader {
            id: contentLoader
            anchors.fill: parent
        }
    }

    // Header.
    Component {
        id: headerComponent

        WizardHeader {
            width: parent.width
            recordUid: fieldBinding.recordUid
            fieldUid: fieldBinding.fieldUid
            params: internal.headerParams
        }
    }

    // Content.
    Component {
        id: contentComponent

        Column {
            anchors.fill: parent
            spacing: 0

            property bool staticField: form.getFieldType(fieldBinding.fieldUid) === "StaticField"
            property bool acknowledgeField: form.getFieldType(fieldBinding.fieldUid) === "AcknowledgeField"

            Flickable {
                id: hintBlock
                ScrollBar.vertical: ScrollBar { interactive: false }
                width: parent.width
                height: {
                    if (staticField) {
                        return hintLabel.height
                    }

                    if (acknowledgeField) {
                        return Math.min(parent.height - Style.minRowHeight, hintLabel.height)
                    }

                    return Math.min(parent.height / 3, hintLabel.height)
                }
                clip: true
                contentWidth: parent.width
                contentHeight: hintLabel.height
                opacity: 0.75

                Label {
                    id: hintLabel
                    width: parent.width
                    font.pixelSize: App.settings.font16
                    wrapMode: Label.WordWrap
                    textFormat: Label.MarkdownText
                }
            }

            Item {
                id: hintSpacer
                width: parent.width
                height: 8
                visible: hintBlock.visible && !staticField

                // Divider.
                Rectangle {
                    y: parent.height - height
                    width: parent.width
                    height: Style.lineWidth1
                    color: Style.colorGroove
                }
            }

            Loader {
                id: innerLoader
                width: parent.width
                height: parent.height - (hintBlock.visible ? hintBlock.height + hintSpacer.height : 0)
            }

            function validate() {
                if (typeof(innerLoader.item.validate) !== "undefined") {
                    innerLoader.item.validate()
                }
            }

            Component.onCompleted: {
                let hint = fieldBinding.hint

                // Decode hints for records.
                if (fieldBinding.fieldUid === "") {
                    let recordUid = form.getParentRecordUid(fieldBinding.recordUid)
                    let fieldUid = form.getRecordFieldUid(fieldBinding.recordUid)
                    hint = form.getFieldHint(recordUid, fieldUid);
                }

                hintLabel.text = hint
                hintBlock.visible = hint !== ""

                if (fieldBinding.fieldUid === "") {
                    if (internal.hasStyle) {
                        innerLoader.sourceComponent = getRecordFieldComponent(fieldBinding.recordUid, fieldBinding.fieldUid)
                    } else {
                        innerLoader.sourceComponent = groupRecordFieldComponent
                    }
                    return
                }

                if (fieldBinding.field.formula !== "" && fieldBinding.field.readonly) {
                    innerLoader.sourceComponent = staticFormulaComponent
                    return
                }

                switch (fieldBinding.fieldType) {
                case "RecordField":
                    if (internal.hasStyle) {
                        innerLoader.sourceComponent = getRecordFieldComponent(fieldBinding.recordUid, fieldBinding.fieldUid)
                    } else {
                        innerLoader.sourceComponent = recordFieldComponent
                    }
                    break

                case "LocationField":
                    innerLoader.sourceComponent = locationFieldComponent
                    break

                case "LineField":
                case "AreaField":
                    innerLoader.sourceComponent = fieldBinding.field.parameters.embed ? lineFieldEmbedComponent : lineFieldComponent
                    break

                case "CheckField":
                    if (fieldBinding.fieldIsList && internal.hasStyle) {
                        innerLoader.sourceComponent = checkGridFieldComponent
                    } else if (fieldBinding.fieldIsList) {
                        innerLoader.sourceComponent = checkListFieldComponent
                    } else {
                        innerLoader.sourceComponent = checkFieldComponent
                    }
                    break

                case "AcknowledgeField":
                    innerLoader.sourceComponent = acknowledgeFieldComponent
                    break

                case "StaticField":
                    if (fieldBinding.hintLink !== "") {
                        innerLoader.sourceComponent = staticFieldWebComponent
                    } else if (fieldBinding.hintIcon !== "") {
                        innerLoader.sourceComponent = staticFieldImageComponent
                    } else {
                        innerLoader.sourceComponent = staticFieldComponent
                    }
                    break

                case "StringField":
                    if (fieldBinding.field.barcode) {
                        innerLoader.sourceComponent = barcodeFieldComponent
                    } else if (fieldBinding.fieldIsList && fieldBinding.field.parameters.likert) {
                        innerLoader.sourceComponent = likertFieldComponent
                    } else if (fieldBinding.fieldIsList && internal.hasStyle) {
                        innerLoader.sourceComponent = gridFieldComponent
                    } else if (fieldBinding.fieldIsList) {
                        innerLoader.sourceComponent = listFieldComponent
                    } else {
                        innerLoader.sourceComponent = stringFieldComponent
                    }
                    break

                case "NumberField":
                    if (fieldBinding.fieldIsList) {
                        innerLoader.sourceComponent = numberListFieldComponent
                    } else if (fieldBinding.field.parameters.appearance === "range" && internal.hasStyle) {
                        innerLoader.sourceComponent = rangeFieldComponent
                    } else {
                        innerLoader.sourceComponent = numberFieldComponent
                    }
                    break

                case "PhotoField":
                    if (fieldBinding.field.maxCount > 1) {
                        innerLoader.sourceComponent = photoFieldGridComponent
                    } else if (fieldBinding.field.parameters.embed) {
                        innerLoader.sourceComponent = photoFieldEmbedComponent
                    } else {
                        innerLoader.sourceComponent = photoFieldComponent
                    }

                    break

                case "AudioField":
                    innerLoader.sourceComponent = audioFieldComponent
                    break

                case "SketchField":
                    innerLoader.sourceComponent = sketchFieldComponent
                    break

                case "FileField":
                    innerLoader.sourceComponent = fileFieldComponent
                    break

                case "DateField":
                case "TimeField":
                case "DateTimeField":
                    innerLoader.sourceComponent = dateTimeFieldComponent
                    break

                case "CalculateField":
                    innerLoader.sourceComponent = calculateFieldComponent
                    break

                default:
                    console.log("Undefined field type: " + fieldBinding.fieldType)
                }
            }
        }
    }

    // Footer.
    Component {
        id: footerComponent

        WizardFooter {
            width: parent.width
            recordUid: fieldBinding.recordUid
            fieldUid: fieldBinding.fieldUid
            params: internal.footerParams
        }
    }

    // Content components.
    Component {
        id: staticFieldComponent

        Control {
            anchors.fill: parent
            padding: App.scaleByFontSize(16)
        }
    }

    Component {
        id: staticFieldImageComponent

        Image {
            anchors.fill: parent
            source: fieldBinding.hintIcon
            fillMode: Image.PreserveAspectFit
        }
    }

    Component {
        id: staticFieldWebComponent

        WebView {
            id: webView
            anchors.fill: parent
            Component.onCompleted: {
                webView.url = fieldBinding.hintLink
            }
        }
    }

    Component {
        id: staticFormulaComponent

        Label {
            anchors.fill: parent
            wrapMode: Label.Wrap
            horizontalAlignment: Label.AlignHCenter
            verticalAlignment: Label.AlignVCenter
            font.pixelSize: App.settings.font20
            color: fieldBinding.isCalculated ? Style.colorCalculated : Material.foreground
            text: fieldBinding.displayValue
        }
    }

    Component {
        id: stringFieldComponent

        FieldTextEdit {
            anchors.fill: parent
            recordUid: fieldBinding.recordUid
            fieldUid: fieldBinding.fieldUid
            forceFocus: true
        }
    }

    Component {
        id: barcodeFieldComponent

        FieldBarcodePane {
            anchors.fill: parent
            recordUid: fieldBinding.recordUid
            fieldUid: fieldBinding.fieldUid
        }
    }

    Component {
        id: locationFieldComponent

        FieldLocationPane {
            anchors.centerIn: parent
            recordUid: fieldBinding.recordUid
            fieldUid: fieldBinding.fieldUid
            Component.onCompleted: {
                active = fieldBinding.isEmpty
            }
        }
    }

    Component {
        id: lineFieldComponent

        FieldLinePane {
            anchors.fill: parent
            recordUid: fieldBinding.recordUid
            fieldUid: fieldBinding.fieldUid
        }
    }

    Component {
        id: lineFieldEmbedComponent

        FieldLine {
            anchors.fill: parent
            recordUid: fieldBinding.recordUid
            fieldUid: fieldBinding.fieldUid
        }
    }

    Component {
        id: checkFieldComponent

        ListViewV {
            id: checkFieldListView
            anchors.fill: parent
            currentIndex: fieldBinding.value === undefined ? -1 : (fieldBinding.value ? 0 : 1)

            model: [qsTr("Yes"), qsTr("No")]

            delegate: HighlightDelegate {
                width: ListView.view.width
                height: 64
                highlighted: checkFieldListView.currentIndex === model.index

                contentItem: Label {
                    text: modelData
                    font.pixelSize: App.settings.font18
                    verticalAlignment: Label.AlignVCenter
                }

                onClicked: {
                    fieldBinding.setValue(model.index === 0)

                    if (form.wizard.canNext) {
                        form.wizard.next(fieldBinding.recordUid, fieldBinding.fieldUid)
                    }
                }

                HorizontalDivider {}
            }
        }
    }

    Component {
        id: acknowledgeFieldComponent

        ColumnLayout {
            width: parent.width

            FieldCheckBox3 {
                recordUid: fieldBinding.recordUid
                fieldUid: fieldBinding.fieldUid
                size: Style.minRowHeight
                font.pixelSize: App.settings.font16
                text: qsTr("OK")
            }

            Item {
                Layout.fillHeight: true
            }
        }
    }

    Component {
        id: dateTimeFieldComponent

        FieldDateTimePicker {
            anchors.fill: parent
            recordUid: fieldBinding.recordUid
            fieldUid: fieldBinding.fieldUid
        }
    }

    Component {
        id: listFieldComponent

        FieldListView {
            anchors.fill: parent
            recordUid: fieldBinding.recordUid
            fieldUid: fieldBinding.fieldUid
            listElementUid: {
                let value = fieldBinding.value
                if (value === undefined) {
                    return fieldBinding.field.listElementUid
                }

                return form.getElement(value).parentElement.uid
            }

            onItemClicked: function (elementUid) {
                fieldBinding.setValue(elementUid)

                if (form.getElement(elementUid).hasChildren) {
                    listElementUid = elementUid
                    return
                }

                if (!form.wizard.canNext) {
                    return
                }

                if (!internal.autoNext) {
                    return
                }

                form.wizard.next(fieldBinding.recordUid, fieldBinding.fieldUid)
            }

            Component.onCompleted: {
                page.canBack = () => {
                    if (listElementUid === fieldBinding.field.listElementUid) {
                        return true
                    }

                    listElementUid = form.getElement(listElementUid).parentElement.uid
                    return false
                }
            }
        }
    }

    Component {
        id: gridFieldComponent

        FieldGridView {
            anchors.fill: parent
            recordUid: fieldBinding.recordUid
            fieldUid: fieldBinding.fieldUid
            params: internal.contentParams
            sideBorder: contentPane.padding > 0

            onItemClicked: function (elementUid) {
                fieldBinding.setValue(elementUid)
                if (!form.wizard.canNext) {
                    return
                }

                if (!internal.autoNext) {
                    return
                }

                form.wizard.next(fieldBinding.recordUid, fieldBinding.fieldUid)
            }
        }
    }

    Component {
        id: likertFieldComponent

        FieldLikert {
            width: parent.width
            recordUid: fieldBinding.recordUid
            fieldUid: fieldBinding.fieldUid

            onItemClicked: {
                fieldBinding.setValue(elementUid)
                if (!form.wizard.canNext) {
                    return
                }

                if (!internal.autoNext) {
                    return
                }

                form.wizard.next(fieldBinding.recordUid, fieldBinding.fieldUid)
            }
        }
    }

    Component {
        id: numberFieldComponent

        FieldKeypad {
            anchors.fill: parent
            recordUid: fieldBinding.recordUid
            fieldUid: fieldBinding.fieldUid
        }
    }

    Component {
        id: rangeFieldComponent

        FieldRange {
            anchors.fill: parent
            recordUid: fieldBinding.recordUid
            fieldUid: fieldBinding.fieldUid
            params: internal.contentParams
            sideBorder: contentPane.padding > 0

            onItemClicked: (value) => {
                fieldBinding.setValue(value)
                if (!form.wizard.canNext) {
                    return
                }

                if (!internal.autoNext) {
                    return
                }

                form.wizard.next(fieldBinding.recordUid, fieldBinding.fieldUid)
            }
        }
    }

    Component {
        id: checkListFieldComponent

        FieldCheckListView {
            anchors.fill: parent
            recordUid: fieldBinding.recordUid
            fieldUid: fieldBinding.fieldUid
        }
    }

    Component {
        id: checkGridFieldComponent

        FieldCheckGridView {
            anchors.fill: parent
            recordUid: fieldBinding.recordUid
            fieldUid: fieldBinding.fieldUid
            params: internal.contentParams
            sideBorder: contentPane.padding > 0
        }
    }

    Component {
        id: photoFieldComponent

        FieldCameraPane {
            anchors.fill: parent
            recordUid: fieldBinding.recordUid
            fieldUid: fieldBinding.fieldUid
        }
    }

    Component {
        id: photoFieldEmbedComponent

        FieldCamera {
            anchors.fill: parent
            recordUid: fieldBinding.recordUid
            fieldUid: fieldBinding.fieldUid
        }
    }

    Component {
        id: fileFieldComponent

        FieldFilePane {
            anchors.fill: parent
            recordUid: fieldBinding.recordUid
            fieldUid: fieldBinding.fieldUid
        }
    }

    Component {
        id: groupRecordFieldComponent

        FieldListViewEditor {
            anchors.fill: parent
            recordUid: fieldBinding.recordUid
            wizardMode: true
        }
    }

    Component {
        id: recordFieldComponent

        FieldRecordListView {
            anchors.fill: parent
            recordUid: fieldBinding.recordUid
            fieldUid: fieldBinding.fieldUid
            wizardMode: true
        }
    }

    Component {
        id: calculateFieldComponent

        Control {
            anchors.fill: parent
            padding: App.scaleByFontSize(8)

            contentItem: Label {
                text: fieldBinding.displayValue
                font.pixelSize: App.settings.font16
                font.bold: true
                horizontalAlignment: Label.AlignHCenter
                verticalAlignment: Label.AlignVCenter
            }
        }
    }

    Component {
        id: sketchFieldComponent

        FieldSketchPad {
            anchors.fill: parent
            recordUid: fieldBinding.recordUid
            fieldUid: fieldBinding.fieldUid
        }
    }

    Component {
        id: audioFieldComponent

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            FieldEditorDelegate {
                Layout.fillWidth: true
                recordUid: fieldBinding.recordUid
                fieldUid: fieldBinding.fieldUid
                showTitle: false
                HorizontalDivider {}
            }

            Item {
                Layout.fillHeight: true
            }
        }
    }

    Component {
        id: photoGridComponent

        FieldRecordPhotoGridView {
            anchors.fill: parent
            recordUid: fieldBinding.recordUid
            fieldUid: fieldBinding.fieldUid
            params: internal.contentParams
            sideBorder: contentPane.padding > 0
        }
    }

    Component {
        id: photoFieldGridComponent

        FieldPhotoGridView {
            anchors.fill: parent
            recordUid: fieldBinding.recordUid
            fieldUid: fieldBinding.fieldUid
            params: internal.contentParams
            sideBorder: contentPane.padding > 0
        }
    }

    Component {
        id: numberGridComponent

        FieldRecordNumberGridView {
            anchors.fill: parent
            recordUid: fieldBinding.recordUid
            fieldUid: fieldBinding.fieldUid
            params: internal.contentParams
            sideBorder: contentPane.padding > 0
            onItemClicked: function (recordUid, fieldUid) {
                form.wizard.edit(recordUid, fieldUid)
            }
        }
    }

    Connections {
        target: form

        function onHighlightInvalid() {
            if (typeof(contentLoader.item.validate) !== "undefined") {
                contentLoader.item.validate()
            }
        }
    }

    Connections {
        target: form.wizard

        function onRebuildPage(transition) {
            if (transition !== -1) {
                page.rebuild(transition)
            }
        }
    }

    Connections {
        target: App

        function onLocationAcquired(x, y, z, accuracy, timestamp, contextData) {
            if (saveState.load(contextData)) {
                page.saveContinue()
            }
        }
    }

    PopupLoader {
        id: popupConfirmHome

        popupComponent: Component {
            ConfirmPopup {
                text: qsTr("Unsaved data?")
                subText: qsTr("This sighting has not been saved.")
                confirmText: qsTr("Yes, discard it")
                confirmDelay: true
                onConfirmed: {
                    form.wizard.home()
                }
            }
        }
    }

    PopupLoader {
        id: popupImmersiveHome

        popupComponent: Component {
            OptionsPopup {
                icon: {
                    let homeIcon = getParam(internal.footerParams, "homeIcon")
                    if (homeIcon !== undefined && homeIcon !== "") {
                        return App.projectManager.getFileUrl(form.projectUid, homeIcon)
                    }

                    return "qrc:/icons/home.svg"
                }

                model: [
                    { text: qsTr("Change %1").arg(App.alias_project), icon: "qrc:/icons/clipboard_multiple_outline.svg" },
                    { text: qsTr("Update %1").arg(App.alias_project), icon: "qrc:/icons/autorenew.svg", visible: form.project.loggedIn && App.projectManager.canUpdate(form.project.uid) },
                    { text: qsTr("Settings"), icon: "qrc:/icons/settings_outline.svg" },
                    { text: qsTr("Home"), icon: C.Style.homeIconSource }
                ]

                onClicked: function (index) {
                    switch (index) {
                    case 0:
                        form.pushPage("qrc:/ProjectChangePage.qml")
                        break

                    case 1:
                        if (Utils.networkAvailable()) {
                            busyCover.doWork = () => {
                                let result = App.projectManager.update(form.project.uid)
                                if (result.success) {
                                    changeToProject(form.projectUid)
                                }
                            }

                            busyCover.start()
                        } else {
                            showToast(qsTr("Offline"));
                        }
                        break

                    case 2:
                        form.pushPage("qrc:/imports/CyberTracker/FormSettingsPage.qml")
                        break

                    case 3:
                        form.wizard.home()
                        break
                    }
                }
            }
        }
    }

    // Save.
    QtObject {
        id: saveState

        property int stage: -1
        property string locationRecordUid: ""
        property string locationFieldUid: ""
        property string targetFieldUid: ""
        property var commands: ({})
        property bool mutex: false
        
        function reset() {
            stage = -1
            locationRecordUid = locationFieldUid = ""
            targetFieldUid = ""
            commands = ({})
        }

        function save() {
            return ({ 
                type: "saveState",
                stage: stage, 
                locationRecordUid: locationRecordUid, locationFieldUid: locationFieldUid, 
                targetFieldUid: targetFieldUid, 
                commands: commands })
        }

        function load(source) {
            if (source && source.type === "saveState") {
                stage = source.stage
                locationRecordUid = source.locationRecordUid
                locationFieldUid = source.locationFieldUid
                targetFieldUid = source.targetFieldUid
                commands = source.commands
                return true
            }

            return false
        }

        function setLocationField(recordUid, fieldUid) {
            locationRecordUid = recordUid
            locationFieldUid = fieldUid
        }

        function setSaveTarget(fieldUid) {
            targetFieldUid = fieldUid
        }
    }

    PopupLoader {
        id: popupSaveTarget

        popupComponent: Component {
            PopupBase {
                id: saveTargetPopup

                property var saveTargets: ([])

                width: page.width * 0.80

                contentItem: ColumnLayout {
                    width: saveTargetPopup.width
                    spacing: 0

                    Item {
                        Layout.fillWidth: true
                        height: Style.iconSize64

                        SquareIcon {
                            anchors.centerIn: parent
                            source: {
                                let saveIcon = getParam(internal.footerParams, "saveIcon")
                                if (saveIcon !== undefined && saveIcon !== "") {
                                    return App.projectManager.getFileUrl(form.projectUid, saveIcon)
                                }

                                return "qrc:/icons/save.svg"
                            }
                            size: parent.height
                            opacity: 0.5
                            recolor: true
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                        height: 16
                        HorizontalDivider {}
                    }

                    Repeater {
                        Layout.fillWidth: true

                        model: saveTargetPopup.saveTargets

                        DelayButton {
                            id: delayButton

                            property color highlightColor: Utils.changeAlpha(Material.foreground, 64)

                            Layout.fillWidth: true
                            delay: modelData.delay ? (App.desktopOS ? 500 : 1000) : 0
                            hoverEnabled: true

                            background: Rectangle {
                                anchors.fill: delayButton
                                color: delayButton.hovered && delayButton.progress === 0 ? delayButton.highlightColor : "transparent"
                                opacity: 0.5

                                Rectangle {
                                    height: parent.height
                                    width: parent.width * delayButton.progress
                                    color: modelData.delay ? Material.accent : delayButton.highlightColor
                                    visible: delayButton.progress > 0
                                }
                            }

                            contentItem: RowLayout {
                                width: parent.width
                                Item {
                                    height: Style.iconSize48
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: modelData.elementUid !== "" ? form.getElementName(modelData.elementUid) : qsTr("Home")
                                    font.pixelSize: App.settings.font18
                                    wrapMode: Label.Wrap
                                    elide: Label.ElideRight
                                }

                                SquareIcon {
                                    source: modelData.elementUid !== "" ? form.getElementIcon(modelData.elementUid) : Style.homeIconSource
                                    size: Style.iconSize48
                                    recolor: modelData.elementUid === ""
                                    opacity: modelData.elementUid === "" ? 0.5 : 1.0
                                }
                            }

                            property bool isActivated: false

                            onActivated: {
                                isActivated = true
                                delayButton.progress = 0
                                if (saveTargetPopup.opened) {
                                    saveTargetPopup.close()
                                    saveState.setSaveTarget(modelData.fieldUid)
                                    
                                    if (modelData.outputFieldUid !== "") {
                                        form.setFieldValue(form.sighting.rootRecordUid, modelData.outputFieldUid, modelData.elementUid)
                                    }

                                    page.saveContinue()
                                }
                            }

                            onReleased: {
                                if (modelData.delay && !isActivated) {
                                    App.showToast(qsTr("Press and hold to confirm"))
                                }
                                delayButton.progress = 0
                                isActivated = false
                            }

                            HorizontalDivider {}
                        }
                    }
                }
            }
        }
    }

    PopupLoader {
        id: popupLocation

        popupComponent: Component {
            LocationPopup {
                color: page.colorContent
            }
        }
    }

    function createPart(loader, defaultComponent, partName) {
        let params = form.getFieldParameter(fieldBinding.recordUid, fieldBinding.fieldUid, partName)

        // Decode base64 to text.
        if (params && params.qmlBase64) {
            params.qml = Utils.decodeBase64(params.qmlBase64)
            // Fall through.
        } else if (params && params.qmlBase64z) {
            params.qml = Utils.decodeBase64z(params.qmlBase64z)
            // Fall through.
        }

        // QML text.
        if (params && params.qml) {
            // Create a unique filename to avoid system caching issues.
            let filePath = App.tempPath + "/" + Utils.computeHash(params.qml) + ".qml"
            Utils.writeTextToFile(filePath, params.qml)
            loader.setSource("file:///" + filePath, { recordUid: fieldBinding.recordUid, fieldUid: fieldBinding.fieldUid, params: params })
            return
        }

        // QML file.
        if (params && params.qmlFile) {
            if (App.desktopOS) {
                App.clearComponentCache()
            }
            let filePath = App.projectManager.getFileUrl(form.projectUid, params.qmlFile)
            loader.setSource(filePath, { recordUid: fieldBinding.recordUid, fieldUid: fieldBinding.fieldUid, params: params })
            return
        }

        // Default.
        loader.sourceComponent = defaultComponent
    }

    function getRecordFieldComponent(recordUid, fieldUid) {
        // Groups only have a recordUid. Repeats may have no records yet.
        let recordField = fieldUid === "" ? form.getField(form.getRecordFieldUid(recordUid)) : form.getField(fieldUid)

        switch (recordField.onlyFieldType) {
        case "PhotoField":  // Photo grid: works on groups and repeats.
            return photoGridComponent


        case "NumberField": // Number list: works on groups only.
            if (fieldUid === "") {
                return numberGridComponent
            }
            break
        }

        console.log("Error: unexpected style on type")
        return undefined
    }

    function rebuild(transition) {
        form.replaceLastPage(form.wizard.pageUrl, {}, transition)
    }

    function saveStart() {
        if (saveState.mutex) {
            return
        }

        saveState.stage = -1
        saveState.targetFieldUid = ""
        saveState.commands = form.getSaveCommands(fieldBinding.recordUid, fieldBinding.fieldUid)

        saveContinue()
    }

    function saveContinue() {
        if (saveState.mutex) {
            return
        }

        saveState.stage++

        switch (saveState.stage) {
        case 0: // Check the constraint for the current field.
            let error = form.wizard.error(fieldBinding.recordUid, fieldBinding.fieldUid)
            if (error !== "") {
                showError(error)
                saveState.reset()
                return
            }

            saveContinue()
            break

        case 1: // Check if the whole sighting is valid.
            if (!form.wizard.valid) {
                form.wizard.index(fieldBinding.recordUid, fieldBinding.fieldUid, true)
                saveState.reset()
                return
            }

            saveContinue()
            break

        case 2: // Check sighting already saved -> just save and do nothing else.
            if (form.editing || form.isSightingCompleted(form.rootRecordUid)) {
                form.wizard.save()
                saveState.reset()
                return
            }

            saveContinue()
            break;

        case 3: // Check for save target.
            let saveTargets = saveState.commands.saveTargets
            if (saveTargets.length > 1) {
                popupSaveTarget.open({ saveTargets: saveTargets })
                return
            }

            // Advance with single or empty save target.
            saveState.targetFieldUid = saveTargets.length === 0 ? "" : saveTargets[0].fieldUid
            saveContinue()
            break

        case 4: // Check for location.
            let snapLocation = saveState.commands.snapLocation
            if (snapLocation.recordUid && snapLocation.fieldUid && !snapLocation.valid) {
                saveState.setLocationField(snapLocation.recordUid, snapLocation.fieldUid)
                popupLocation.open({fixCount: form.getField(snapLocation.fieldUid).fixCount, contextData: saveState.save()})
                return
            }

            saveContinue()
            break

        case 5:
            // Make sure we cannot start a new save operation while this one is busy.
            saveState.mutex = true
            try {
                // Apply track state changes.
                form.applyTrackCommand()

                // Save with target.
                if (!form.wizard.save(saveState.targetFieldUid)) {
                    App.showError(qsTr("Error saving observation"))
                }
            } finally {
                if (internal.isActive) {
                    saveState.reset()
                    saveState.mutex = false
                }
            }

            break
        }
    }

//    FieldStateMarker {
//        recordUid: fieldBinding.recordUid
//        fieldUid: fieldBinding.fieldUid
//        x: -page.padding - App.scaleByFontSize(2)
//        y: -page.padding - App.scaleByFontSize(2)
//        icon.width: App.scaleByFontSize(10)
//        icon.height: App.scaleByFontSize(10)
//    }
}
