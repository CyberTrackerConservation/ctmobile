import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.0
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    property string recordUid
    property string fieldUid
    property string listElementUid

    C.FieldBinding {
        id: fieldBinding
        recordUid: page.recordUid
        fieldUid: page.fieldUid

        Component.onCompleted: {
            createPart(headerLoader, headerComponent, "header")
            createPart(footerLoader, footerComponent, "footer")
            createPart(contentLoader, contentComponent, "content")
        }
    }

    header: Loader { id: headerLoader }

    footer: Loader { id: footerLoader }

    Pane {
        anchors.fill: parent
        padding: form.getFieldParameter(page.recordUid, page.fieldUid, "contentPadding", 16)

        background: Rectangle {
            anchors.fill: parent
            color: Style.colorContent
        }

        Loader {
            id: contentLoader
            anchors.fill: parent
        }
    }

    function createPart(loader, defaultComponent, partName) {
        let name = form.getFieldParameter(page.recordUid, page.fieldUid, partName)
        if (name) {
            let url = App.projectManager.getFileUrl(form.project.uid, name)
            loader.setSource(url, { recordUid: page.recordUid, fieldUid: page.fieldUid })
        } else {
            loader.sourceComponent = defaultComponent
        }
    }

    // Header.
    Component {
        id: headerComponent

        C.PageHeader {
            topText: form.getRecordName(recordUid)
            text: form.getFieldName(recordUid, fieldUid)
            formBack: false
            backIcon: "qrc:/icons/home_import_outline.svg"
            menuIcon: App.batteryIcon
            menuVisible: true
            menuEnabled: false
            onBackClicked: form.wizard.home()
        }
    }

    // Content.
    Component {
        id: contentComponent

        Column {
            anchors.fill: parent
            spacing: 0

            property bool staticField: form.getFieldType(fieldUid) === "StaticField"
            property bool acknowledgeField: form.getFieldType(fieldUid) === "AcknowledgeField"

            Flickable {
                id: hintBlock
                ScrollBar.vertical: ScrollBar { interactive: false }
                width: parent.width
                height: {
                    if (staticField) {
                        return parent.height
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
                C.HorizontalDivider {}
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
                hintLabel.text = fieldBinding.hint
                hintBlock.visible = fieldBinding.hint !== ""

                if (page.fieldUid === "") {
                    innerLoader.sourceComponent = groupRecordFieldComponent
                    return
                }

                if (fieldBinding.field.formula !== "" && fieldBinding.field.readonly) {
                    innerLoader.sourceComponent = staticFormulaComponent
                    return
                }

                switch (fieldBinding.fieldType) {
                case "LocationField":
                    innerLoader.sourceComponent = locationFieldComponent
                    break

                case "LineField":
                case "AreaField":
                    innerLoader.sourceComponent = lineFieldComponent
                    break

                case "CheckField":
                    innerLoader.sourceComponent = (fieldBinding.fieldIsList === true) ? checkListFieldComponent : checkFieldComponent
                    break

                case "AcknowledgeField":
                    innerLoader.sourceComponent = acknowledgeFieldComponent
                    break

                case "StaticField":
                    innerLoader.sourceComponent = staticFieldComponent
                    break

                case "StringField":
                    if (fieldBinding.field.barcode) {
                        innerLoader.sourceComponent = barcodeFieldComponent
                    } else if (fieldBinding.fieldIsList) {
                        innerLoader.sourceComponent =  fieldBinding.field.parameters.likert ? likertFieldComponent : listFieldComponent
                    } else {
                        innerLoader.sourceComponent = stringFieldComponent
                    }
                    break

                case "NumberField":
                    innerLoader.sourceComponent = (fieldBinding.fieldIsList === true) ? numberListFieldComponent : numberFieldComponent
                    break

                case "PhotoField":
                    innerLoader.sourceComponent = fieldBinding.field.parameters.embed ? photoFieldEmbedComponent : photoFieldComponent
                    break

                case "AudioField":
                    innerLoader.sourceComponent = audioFieldComponent
                    break

                case "SketchField":
                    innerLoader.sourceComponent = sketchFieldComponent
                    break

                case "DateField":
                case "TimeField":
                case "DateTimeField":
                    innerLoader.sourceComponent = dateTimeFieldComponent
                    break

                case "RecordField":
                    innerLoader.sourceComponent = recordFieldComponent
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

        ColumnLayout {
            spacing: 0
            width: parent.width
            visible: true

            Rectangle {
                Layout.fillWidth: true
                height: 2
                color: Material.theme === Material.Dark ? "#FFFFFF" : "#000000"
                opacity: 0.12
            }

            RowLayout {
                id: buttonRow
                spacing: 0
                Layout.fillWidth: true
                property int buttonCount: 3
                property int buttonWidth: form.width / buttonCount
                property var buttonColor: Material.theme === Material.Dark ? Material.foreground : Material.primary

                // Back.
                ToolButton {
                    id: backButton
                    Layout.preferredWidth: buttonRow.buttonWidth
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    font.pixelSize: App.settings.font10
                    font.capitalization: Font.MixedCase
                    display: Button.TextUnderIcon
                    icon.source: "qrc:/icons/arrow_back.svg"
                    icon.width: Style.toolButtonSize
                    icon.height: Style.toolButtonSize
                    Material.foreground: buttonRow.buttonColor
                    focusPolicy: Qt.NoFocus
                    onClicked: {
                        backButton.forceActiveFocus()
                        form.wizard.back()
                    }
                }

                // Index.
                ToolButton {
                    id: indexButton
                    Layout.preferredWidth: buttonRow.buttonWidth
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    font.pixelSize: App.settings.font10
                    font.capitalization: Font.MixedCase
                    display: Button.TextUnderIcon
                    icon.source: "qrc:/icons/file_outline.svg"
                    icon.width: Style.toolButtonSize
                    icon.height: Style.toolButtonSize
                    Material.foreground: buttonRow.buttonColor
                    focusPolicy: Qt.NoFocus
                    onClicked: {
                        indexButton.forceActiveFocus()
                        form.wizard.index(page.recordUid, page.fieldUid)
                    }
                }

                // Next.
                ToolButton {
                    id: nextButton
                    property bool saveMode: form.wizard.canSave && form.wizard.lastPage
                    Layout.preferredWidth: buttonRow.buttonWidth
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    font.pixelSize: App.settings.font10
                    font.capitalization: Font.MixedCase
                    display: Button.TextUnderIcon
                    icon.source: saveMode ? "qrc:/icons/save.svg" : "qrc:/icons/arrow_right.svg"
                    icon.width: Style.toolButtonSize
                    icon.height: Style.toolButtonSize
                    Material.foreground: buttonRow.buttonColor
                    enabled: form.wizard.canNext || saveMode
                    opacity: enabled ? 1.0 : 0.5
                    focusPolicy: Qt.NoFocus

                    onClicked: {
                        nextButton.forceActiveFocus()

                        if (saveMode) {
                            let error = form.wizard.error(page.recordUid, page.fieldUid)
                            if (error !== "") {
                                showError(error)
                                return
                            }

                            if (!form.wizard.save()) {
                                form.wizard.index(page.recordUid, page.fieldUid)
                            }

                            return;
                        }

                        form.wizard.next(page.recordUid, page.fieldUid)
                    }
                }
            }
        }
    }

    // Content components.
    Component {
        id: staticFieldComponent

        Control {
            anchors.fill: parent
            padding: 16
        }
    }

    Component {
        id: staticFormulaComponent

        Control {
            anchors.fill: parent
            padding: 16

            contentItem: Label {
                wrapMode: Label.Wrap
                horizontalAlignment: Label.AlignHCenter
                verticalAlignment: Label.AlignVCenter
                font.pixelSize: App.settings.font20
                color: fieldBinding.isCalculated ? Style.colorCalculated : Material.foreground
                text: fieldBinding.displayValue
            }
        }
    }

    Component {
        id: stringFieldComponent

        Control {
            anchors.fill: parent
            padding: 8

            contentItem: C.FieldTextEdit {
                recordUid: page.recordUid
                fieldUid: page.fieldUid
                placeholderText: ""
                forceFocus: true
            }
        }
    }

    Component {
        id: barcodeFieldComponent

        ItemDelegate {
            id: barcodeFieldDelegate
            anchors.fill: parent
            padding: 8

            contentItem: Item {
                Label {
                    anchors.fill: parent
                    font.pixelSize: App.settings.font16
                    font.bold: true
                    wrapMode: Label.Wrap
                    text: fieldBinding.displayValue
                    visible: !fieldBinding.isEmpty
                    horizontalAlignment: Label.AlignHCenter
                    verticalAlignment: Label.AlignVCenter
                }

                ToolButton {
                    anchors.fill: parent
                    visible: fieldBinding.isEmpty
                    icon.source: "qrc:/icons/qrcode_scan.svg"
                    icon.width: parent.width / 4
                    icon.height: parent.width / 4
                    opacity: 0.5
                    onClicked: barcodeFieldDelegate.clicked()
                }
            }

            onClicked: {
                form.pushPage(Qt.resolvedUrl("FieldBarcodePage.qml"), { recordUid: page.recordUid, fieldUid: page.fieldUid } )
            }
        }
    }

    Component {
        id: locationFieldComponent

        ColumnLayout {
            width: parent.width
            spacing: 4

            Component.onCompleted: {
                autoLocationTimer.running = fieldBinding.isEmpty
            }

            Timer {
                id: autoLocationTimer
                interval: 1
                repeat: false
                running: false
                onTriggered: fieldLocationPane.active = true
            }

            C.FieldLocationPane {
                id: fieldLocationPane
                Layout.fillWidth: true
                recordUid: page.recordUid
                fieldUid: page.fieldUid
            }

            Item {
                Layout.fillHeight: true
            }
        }
    }

    Component {
        id: lineFieldComponent

        ColumnLayout {
            width: parent.width
            spacing: 4

            C.FieldLinePane {
                id: fieldLinePane
                Layout.fillWidth: true
                recordUid: page.recordUid
                fieldUid: page.fieldUid
            }

            Item {
                Layout.fillHeight: true
            }
        }
    }

    Component {
        id: checkFieldComponent

        C.ListViewV {
            id: checkFieldListView
            anchors.fill: parent
            currentIndex: fieldBinding.value === undefined ? -1 : (fieldBinding.value ? 0 : 1)

            model: [qsTr("Yes"), qsTr("No")]

            delegate: ItemDelegate {
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
                        form.wizard.next(page.recordUid, page.fieldUid)
                    }
                }

                C.HorizontalDivider {}
            }
        }
    }

    Component {
        id: acknowledgeFieldComponent

        ColumnLayout {
            width: parent.width

            C.FieldCheckBox {
                recordUid: page.recordUid
                fieldUid: page.fieldUid
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

        C.FieldDateTimePicker {
            anchors.fill: parent
            recordUid: page.recordUid
            fieldUid: page.fieldUid
        }
    }

    Component {
        id: listFieldComponent

        C.FieldListView {
            anchors.fill: parent
            recordUid: page.recordUid
            fieldUid: page.fieldUid
            listElementUid: page.listElementUid !== "" ? page.listElementUid : fieldBinding.field.listElementUid

            onItemClicked: {
                fieldBinding.setValue(elementUid)

                if (!form.wizard.canNext) {
                    return;
                }

                if (form.getElement(elementUid).hasChildren) {
                    form.wizard.next(page.recordUid, page.fieldUid) // BUGBUG
                } else {
                    form.wizard.next(page.recordUid, page.fieldUid)
                }
            }
        }
    }

    Component {
        id: likertFieldComponent

        C.FieldLikert {
            width: parent.width
            recordUid: page.recordUid
            fieldUid: page.fieldUid
        }
    }

    Component {
        id: numberFieldComponent

        C.FieldKeypad {
            anchors.fill: parent
            recordUid: page.recordUid
            fieldUid: page.fieldUid
        }
    }

    Component {
        id: checkListFieldComponent

        C.FieldCheckListView {
            anchors.fill: parent
            recordUid: page.recordUid
            fieldUid: page.fieldUid
        }
    }

    Component {
        id: photoFieldComponent

        ItemDelegate {
            id: photoFieldDelegate
            anchors.fill: parent
            padding: 8

            contentItem: Item {
                Image {
                    anchors.fill: parent
                    fillMode: Image.PreserveAspectFit
                    source: buildPhotoModel()[0] !== "" ? App.getMediaFileUrl(buildPhotoModel()[0]) : ""
                    visible: buildPhotoModel()[0] !== ""
                    autoTransform: true
                    cache: false
                }

                ToolButton {
                    anchors.fill: parent
                    visible: buildPhotoModel()[0] === ""
                    icon.source: "qrc:/icons/camera_alt.svg"
                    icon.width: parent.width / 4
                    icon.height: parent.width / 4
                    opacity: 0.5
                    onClicked: photoFieldDelegate.clicked()
                }
            }

            onClicked: {
                fieldBinding.setValue(buildPhotoModel())
                form.pushPage(Qt.resolvedUrl("FieldCameraPage.qml"), { photoIndex: 0, recordUid: page.recordUid, fieldUid: page.fieldUid } )
            }

            function buildPhotoModel() {
                var v = fieldBinding.value
                if (v === undefined || v === null) {
                    v = [""]
                } else {
                    v = v.filter(e => e !== "")
                    if (v.length < fieldBinding.field.maxCount) {
                        v.push("")
                    }
                }

                return v
            }
        }
    }

    Component {
        id: photoFieldEmbedComponent

        C.FieldCamera {
            anchors.fill: parent
            recordUid: page.recordUid
            fieldUid: page.fieldUid
        }
    }

    Component {
        id: groupRecordFieldComponent

        C.FieldListViewEditor {
            anchors.fill: parent
            recordUid: page.recordUid
            wizardMode: true
        }
    }

    Component {
        id: recordFieldComponent

        C.FieldRecordListView {
            anchors.fill: parent
            recordUid: page.recordUid
            fieldUid: page.fieldUid
            wizardMode: true
        }
    }

    Component {
        id: calculateFieldComponent

        Control {
            anchors.fill: parent
            padding: 8

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

        FieldSketchpad {
            width: page.width
            height: Math.min(page.width * 0.6666, page.height)
            recordUid: page.recordUid
            fieldUid: page.fieldUid
        }
    }

    Component {
        id: audioFieldComponent

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            C.FieldEditorDelegate {
                Layout.fillWidth: true
                recordUid: page.recordUid
                fieldUid: page.fieldUid
                showTitle: false
                C.HorizontalDivider {}
            }

            Item {
                Layout.fillHeight: true
            }
        }
    }

    Component {
        id: saveTargetPopupComponent

        C.PopupBase {
            id: saveTargetPopup
            width: page.width * 0.80
            insets: -12

            contentItem: ColumnLayout {
                width: saveTargetPopup.width
                spacing: 0

                Item {
                    Layout.fillWidth: true
                    height: 64

                    Image {
                        anchors.centerIn: parent
                        source: "qrc:/icons/save.svg"
                        sourceSize.height: 48
                        sourceSize.width: 48
                        width: 48
                        height: 48
                        opacity: 0.5
                        layer {
                            enabled: true
                            effect: ColorOverlay {
                                color: Material.foreground
                            }
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    height: 16
                    C.HorizontalDivider {}
                }

                Repeater {
                    Layout.fillWidth: true

                    model: form.wizard.findSaveTargets()

                    ItemDelegate {
                        Layout.fillWidth: true

                        contentItem: RowLayout {
                            Item {
                                height: 48
                            }

                            Label {
                                Layout.fillWidth: true
                                text: form.getElementName(modelData.elementUid)
                                font.pixelSize: App.settings.font18
                                elide: Label.ElideRight
                            }

                            Image {
                                source: form.getElementIcon(modelData.elementUid)
                                sourceSize.height: 48
                                sourceSize.width: 48
                                width: 48
                                visible: source !== ""
                            }
                        }

                        onClicked: {
                            if (!form.wizard.save(modelData.fieldUid)) {
                                form.wizard.index(page.recordUid, page.fieldUid)
                            }
                        }

                        C.HorizontalDivider {}
                    }
                }
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

//    FieldStateMarker {
//        recordUid: page.recordUid
//        fieldUid: page.fieldUid
//        x: -page.padding - 12
//        y: -page.padding - 12
//        icon.width: 16
//        icon.height: 16
//    }
}
