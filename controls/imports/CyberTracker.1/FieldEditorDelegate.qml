import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import QtGraphicalEffects 1.0
import CyberTracker 1.0 as C

SwipeDelegate {
    id: root

    property string recordUid
    property string fieldUid
    property bool showTitle: true
    property var minRowHeight: Style.minRowHeight
    property var highlightInvalid: false
    property var callbackEnabled
    property bool autoLocation: true
    property bool enableSwipeReset: true
    property bool wizardMode: false

    width: parent ? parent.width : undefined
    opacity: enabled ? 1.0 : 0.5

    // Hack to prevent doing another layout when the listview goes offscreen.
    height: parent ? (visible || !parent.visible ? implicitHeight : 0) : undefined

    C.FieldBinding {
        id: fieldBinding
        recordUid: root.recordUid
        fieldUid: root.fieldUid

        Component.onCompleted: {
            loadedItem.sourceComponent = getComponent()
            hintButton.visible = fieldBinding.hint !== "" || fieldBinding.hintLink.toString() !== ""
        }
    }

    Binding { target: background; property: "color"; value: C.Style.colorContent }

    Component {
        id: recordFieldComponent

        RowLayout {
            Item { height: root.minRowHeight }

            Column {
                Layout.fillWidth: true
                spacing: 5 // default value for ColumnLayout
                height: recordsTitle.implicitHeight + recordsValue.implicitHeight

                C.FieldName {
                    id: recordsTitle
                    width: parent.width
                    recordUid: fieldBinding.recordUid
                    fieldUid: fieldBinding.fieldUid
                    color: titleColor(fieldBinding)
                    font.bold: titleFontBold(fieldBinding)
                    visible: titleVisible(fieldBinding)
                }

                Label {
                    id: recordsValue
                    width: parent.width
                    color: valueColor(fieldBinding)
                    wrapMode: Label.Wrap
                    font.pixelSize: App.settings.font16
                    font.bold: true
                    text: getRecordsText()
                    elide: Label.ElideRight
                    visible: text !== ""
                }
            }

            Image {
                fillMode: Image.PreserveAspectFit
                source: fieldBinding.fieldIcon
                sourceSize.width: root.minRowHeight
                sourceSize.height: root.minRowHeight
                visible: fieldBinding.fieldIcon.toString() !== ""
                verticalAlignment: Image.AlignVCenter
            }

            Connections {
                target: root

                function onClicked() {
                    if (fieldBinding.field.group) {
                        // Group.
                        if (root.wizardMode) {
                            form.wizard.edit(fieldBinding.value[0])
                        } else {
                            form.pushPage(Qt.resolvedUrl("FieldRecordEditorPage.qml"), { title: recordsTitle.text, recordUid: fieldBinding.value[0], fieldUid: fieldBinding.fieldUid, highlightInvalid: root.highlightInvalid })
                        }

                    } else {
                        // Repeat.
                        if (root.wizardMode) {
                            form.wizard.edit(fieldBinding.recordUid, fieldBinding.fieldUid)
                        } else {
                            form.pushPage(Qt.resolvedUrl("FieldRecordListViewPage.qml"), { title: recordsTitle.text, recordUid: fieldBinding.recordUid, fieldUid: fieldBinding.fieldUid, highlightInvalid: root.highlightInvalid })
                        }
                    }
                }
            }

            Connections {
                target: form

                function onFieldValueChanged(recordUid, fieldUid, oldValue, newValue) {
                    recordsValue.text = getRecordsText()
                }
            }

            function getRecordsText() {
                if (fieldBinding.value === undefined) {
                    return ""
                }

                if (fieldBinding.field.group) {
                    return form.getRecordSummary(fieldBinding.value[0])
                }

                var records = fieldBinding.value
                var count = records !== undefined ? records.length : 0

                // Disable if a master field is specified, but none are visible.
                root.enabled = count !== 0 || fieldBinding.field.masterFieldUid === ""

                return fieldBinding.displayValue
            }
        }
    }

    Component {
        id: locationFieldComponent

        RowLayout {
            property int iconSize: !fieldBinding.field.allowManual ? Style.minRowHeight : Style.minRowHeight * 0.75

            Component.onCompleted: {
                autoLocationTimer.running = (fieldBinding.value === undefined || !fieldBinding.isValid) && root.autoLocation && root.visible
                root.autoLocation = false
            }

            Timer {
                id: autoLocationTimer
                interval: 1
                repeat: false
                running: false
                onTriggered: locationValue.active = true
            }

            Item { height: root.minRowHeight }

            Column {
                Layout.fillWidth: true
                spacing: 5 // default value for ColumnLayout
                height: locationTitle.implicitHeight + locationValue.implicitHeight

                C.FieldName {
                    id: locationTitle
                    width: parent.width
                    recordUid: fieldBinding.recordUid
                    fieldUid: fieldBinding.fieldUid
                    color: titleColor(fieldBinding)
                    font.bold: titleFontBold(fieldBinding)
                    visible: titleVisible(fieldBinding)
                }

                C.FieldLocation {
                    id: locationValue
                    recordUid: fieldBinding.recordUid
                    fieldUid: fieldBinding.fieldUid
                    color: valueColor(fieldBinding)
                    width: parent.width
                    font.pixelSize: App.settings.font16
                    font.bold: true
                    placeholderText: titleVisible(fieldBinding) ? "" : qsTr("Location")
                    visible: titleVisible(fieldBinding) ? fieldBinding.isValid && !fieldBinding.isEmpty : true
                    onFixCounterChanged: locationProgress.progressPercent = (locationValue.fixCounter * 100) / fieldBinding.field.fixCount
                }
            }

            ToolButton {
                icon.source: "qrc:/icons/edit_location_outline.svg"
                icon.width: parent.iconSize
                icon.height: parent.iconSize
                opacity: locationValue.active ? 0.0 : 0.75
                icon.color: Material.foreground
                visible: root.enabled && fieldBinding.field.allowManual
                onClicked: {
                    // Disable with warning if waiting for time correction.
                    if (form.useGPSTime && !App.timeManager.corrected) {
                        showToast(qsTr("Waiting for time correction"))
                        return
                    }

                    if (!wizardMode) {
                        form.pushPage(Qt.resolvedUrl("FieldLocationPage.qml"), { recordUid: fieldBinding.recordUid, fieldUid: fieldBinding.fieldUid } )
                    } else {
                        form.wizard.edit(fieldBinding.recordUid, fieldBinding.fieldUid)
                    }
                }
            }

            ToolButton {
                icon.source: "qrc:/icons/cancel.svg"
                icon.width: parent.iconSize
                icon.height: parent.iconSize
                opacity: 0.75
                icon.color: Material.foreground
                visible: locationValue.active
                onClicked: locationValue.active = false
            }

            ToolButton {
                icon.source: busyIndicator.running ? "qrc:/icons/blank.svg" : "qrc:/icons/gps_fixed.svg"
                icon.width: parent.iconSize
                icon.height: parent.iconSize
                opacity: locationValue.active ? 1.0 : 0.75
                icon.color: Material.foreground
                visible: root.enabled

                C.Skyplot {
                    id: locationProgress
                    anchors.fill: parent
                    miniMode: true
                    progressVisible: true
                    active: visible
                    visible: locationValue.active
                }

                BusyIndicator {
                    anchors.fill: parent
                    id: busyIndicator
                    running: locationValue.active
                    visible: false
                }

                onClicked: locationValue.active = true
            }

            Image {
                fillMode: Image.PreserveAspectFit
                source: fieldBinding.fieldIcon
                sourceSize.width: root.minRowHeight
                sourceSize.height: root.minRowHeight
                visible: fieldBinding.fieldIcon.toString() !== ""
                verticalAlignment: Image.AlignVCenter
            }
        }
    }

    Component {
        id: checkFieldComponent

        RowLayout {
            Item { height: root.minRowHeight }

            C.FieldName {
                Layout.fillWidth: true
                recordUid: fieldBinding.recordUid
                fieldUid: fieldBinding.fieldUid
                color: titleColor(fieldBinding)
                font.bold: titleFontBold(fieldBinding)
                visible: titleVisible(fieldBinding)
            }

            C.FieldCheckBox {
                id: checkBox
                recordUid: fieldBinding.recordUid
                fieldUid: fieldBinding.fieldUid
                size: Style.minRowHeight
                font.pixelSize: App.settings.font16
            }

            Image {
                fillMode: Image.PreserveAspectFit
                source: fieldBinding.fieldIcon
                sourceSize.width: root.minRowHeight
                sourceSize.height: root.minRowHeight
                visible: fieldBinding.fieldIcon.toString() !== ""
                verticalAlignment: Image.AlignVCenter
            }

            Connections {
                target: root

                function onClicked() {
                    if (App.config.fullRowCheck) {
                        checkBox.checked = !checkBox.checked
                        checkBox.clicked()
                    }
                }
            }
        }
    }

    Component {
        id: staticFieldComponent

        RowLayout {
            Item { height: root.minRowHeight }

            C.FieldName {
                Layout.fillWidth: true
                recordUid: fieldBinding.recordUid
                fieldUid: fieldBinding.fieldUid
                font.bold: titleFontBold(fieldBinding)
                visible: titleVisible(fieldBinding)
                opacity: 1.0
            }

            Image {
                fillMode: Image.PreserveAspectFit
                source: fieldBinding.fieldIcon
                sourceSize.width: root.minRowHeight
                sourceSize.height: root.minRowHeight
                visible: fieldBinding.fieldIcon.toString() !== ""
                verticalAlignment: Image.AlignVCenter
            }
        }
    }

    Component {
        id: stringFieldComponent

        RowLayout {
            Item { height: root.minRowHeight }

            Column {
                Layout.fillWidth: true
                spacing: 5 // default value for ColumnLayout
                height: stringTitle.implicitHeight + stringValue.implicitHeight

                C.FieldName {
                    id: stringTitle
                    width: parent.width
                    recordUid: fieldBinding.recordUid
                    fieldUid: fieldBinding.fieldUid
                    color: titleColor(fieldBinding)
                    font.bold: titleFontBold(fieldBinding)
                    visible: titleVisible(fieldBinding)
                }

                Label {
                    id: stringValue
                    width: parent.width
                    color: valueColor(fieldBinding)
                    font.pixelSize: App.settings.font16
                    font.bold: true
                    wrapMode: Label.Wrap
                    text: fieldBinding.displayValue
                    visible: !fieldBinding.isEmpty
                }

                Label {
                    width: parent.width
                    color: titleColor(fieldBinding)
                    font.pixelSize: App.settings.font16
                    font.bold: true
                    wrapMode: Label.Wrap
                    text: qsTr("Tap to scan")
                    visible: !titleVisible(fieldBinding) && fieldBinding.isEmpty && fieldBinding.field.barcode
                    horizontalAlignment: Label.AlignHCenter
                    opacity: 0.5
                }
            }

            Image {
                fillMode: Image.PreserveAspectFit
                source: fieldBinding.fieldIcon
                sourceSize.width: root.minRowHeight
                sourceSize.height: root.minRowHeight
                visible: fieldBinding.fieldIcon.toString() !== ""
                verticalAlignment: Image.AlignVCenter
            }

            Connections {
                target: root

                function onClicked() {
                    if (!wizardMode) {
                        form.pushPage(fieldBinding.field.barcode ? Qt.resolvedUrl("FieldBarcodePage.qml") : Qt.resolvedUrl("FieldTextEditPage.qml"), { recordUid: fieldBinding.recordUid, fieldUid: fieldBinding.fieldUid } )
                    } else {
                        form.wizard.edit(fieldBinding.recordUid, fieldBinding.fieldUid)
                    }
                }
            }
        }
    }

    Component {
        id: listFieldComponent

        RowLayout {
            property bool likertMode: fieldBinding.field.parameters.likert !== undefined

            Item { height: root.minRowHeight }

            Column {
                Layout.fillWidth: true
                spacing: 5 // default value for ColumnLayout
                height: listTitle.implicitHeight + likertMode ? likertRow.implicitHeight : listValue.implicitHeight

                C.FieldName {
                    id: listTitle
                    recordUid: fieldBinding.recordUid
                    fieldUid: fieldBinding.fieldUid
                    width: parent.width
                    color: titleColor(fieldBinding)
                    font.bold: titleFontBold(fieldBinding)
                    visible: titleVisible(fieldBinding)
                }

                Label {
                    id: listValue
                    width: parent.width
                    color: valueColor(fieldBinding)
                    text: fieldBinding.valueElementName
                    wrapMode: Label.Wrap
                    visible: !likertMode && text !== ""
                    font.pixelSize: App.settings.font16
                    font.bold: true
                }

                C.FieldLikert {
                    id: likertRow
                    width: parent.width
                    recordUid: fieldBinding.recordUid
                    fieldUid: fieldBinding.fieldUid
                    visible: likertMode
                }
            }

            Image {
                fillMode: Image.PreserveAspectFit
                source: fieldBinding.fieldIcon
                sourceSize.width: root.minRowHeight
                sourceSize.height: root.minRowHeight
                visible: fieldBinding.isEmpty && fieldBinding.fieldIcon.toString() !== ""
                verticalAlignment: Image.AlignVCenter
            }

            Image {
                fillMode: Image.PreserveAspectFit
                source: fieldBinding.valueElementIcon
                sourceSize.width: root.minRowHeight
                sourceSize.height: root.minRowHeight
                visible: fieldBinding.valueElementIcon.toString() !== ""
                verticalAlignment: Image.AlignVCenter
            }

            Connections {
                target: root

                function onClicked() {
                    if (!likertMode) {
                        if (!wizardMode) {
                            form.pushPage(Qt.resolvedUrl("FieldListViewPage.qml"), { recordUid: fieldBinding.recordUid, fieldUid: fieldBinding.fieldUid } )
                        } else {
                            form.wizard.edit(fieldBinding.recordUid, fieldBinding.fieldUid)
                        }
                    }
                }
            }
        }
    }

    Component {
        id: checkListFieldComponent

        RowLayout {
            Item { height: root.minRowHeight }

            Column {
                Layout.fillWidth: true
                spacing: 5 // default value for ColumnLayout
                height: checkListTitle.implicitHeight + checkListValue.implicitHeight

                C.FieldName {
                    id: checkListTitle
                    width: parent.width
                    recordUid: fieldBinding.recordUid
                    fieldUid: fieldBinding.fieldUid
                    color: titleColor(fieldBinding)
                    font.bold: titleFontBold(fieldBinding)
                    visible: titleVisible(fieldBinding)
                }

                Label {
                    id: checkListValue
                    width: parent.width
                    color: valueColor(fieldBinding)
                    wrapMode: Label.Wrap
                    font.pixelSize: App.settings.font16
                    font.bold: true
                    text: fieldBinding.displayValue
                    visible: !fieldBinding.isEmpty
                }
            }

            Image {
                fillMode: Image.PreserveAspectFit
                source: fieldBinding.fieldIcon
                sourceSize.width: root.minRowHeight
                sourceSize.height: root.minRowHeight
                visible: fieldBinding.fieldIcon.toString() !== ""
                verticalAlignment: Image.AlignVCenter
            }

            Connections {
                target: root

                function onClicked() {
                    if (!wizardMode) {
                        form.pushPage(Qt.resolvedUrl("FieldCheckListViewPage.qml"), { recordUid: fieldBinding.recordUid, fieldUid: fieldBinding.fieldUid } )
                    } else {
                        form.wizard.edit(fieldBinding.recordUid, fieldBinding.fieldUid)
                    }
                }
            }
        }
    }

    Component {
        id: numberListFieldComponent

        RowLayout {
            Item { height: root.minRowHeight }

            Column {
                Layout.fillWidth: true
                spacing: 5 // default value for ColumnLayout
                height: numberListTitle.implicitHeight + numberListValue.implicitHeight

                C.FieldName {
                    id: numberListTitle
                    width: parent.width
                    recordUid: fieldBinding.recordUid
                    fieldUid: fieldBinding.fieldUid
                    color: titleColor(fieldBinding)
                    font.bold: titleFontBold(fieldBinding)
                    visible: titleVisible(fieldBinding)
                }

                Label {
                    id: numberListValue
                    width: parent.width
                    color: valueColor(fieldBinding)
                    font.pixelSize: App.settings.font16
                    font.bold: true
                    wrapMode: Label.Wrap
                    text: fieldBinding.displayValue
                    visible: !fieldBinding.isEmpty
                }
            }

            Image {
                fillMode: Image.PreserveAspectFit
                source: fieldBinding.fieldIcon
                sourceSize.width: root.minRowHeight
                sourceSize.height: root.minRowHeight
                visible: fieldBinding.fieldIcon.toString() !== ""
                verticalAlignment: Image.AlignVCenter
            }

            Connections {
                target: root

                function onClicked() {
                    if (!wizardMode) {
                        form.pushPage(Qt.resolvedUrl("FieldNumberListViewPage.qml"), { recordUid: fieldBinding.recordUid, fieldUid: fieldBinding.fieldUid } )
                    } else {
                        form.wizard.edit(fieldBinding.recordUid, fieldBinding.fieldUid)
                    }
                }
            }
        }
    }

    Component {
        id: numberFieldComponent

        RowLayout {
            Item { height: root.minRowHeight }

            property bool changeMutex: false

            Component.onCompleted: {
                changeMutex = true
                numberEditor.text = fieldBinding.displayValue
                changeMutex = false
            }

            Column {
                Layout.fillWidth: true
                spacing: 5 // default value for ColumnLayout
                height: numberTitle.implicitHeight + (numberValue.visible ? numberValue.implicitHeight : 0) + (numberEditor.visible ? numberEditor.implicitHeight : 0)

                C.FieldName {
                    id: numberTitle
                    width: parent.width
                    recordUid: fieldBinding.recordUid
                    fieldUid: fieldBinding.fieldUid
                    color: titleColor(fieldBinding)
                    font.bold: titleFontBold(fieldBinding)
                    visible: titleVisible(fieldBinding)
                }

                Label {
                    id: numberValue
                    width: parent.width
                    color: valueColor(fieldBinding)
                    font.pixelSize: App.settings.font16
                    font.bold: true
                    text: fieldBinding.displayValue
                    visible: text !== "" && fieldBinding.field.parameters.embed === undefined
                }

                TextField {
                    id: numberEditor
                    width: parent.width
                    visible: fieldBinding.field.parameters.embed !== undefined
                    font.pixelSize: App.settings.font16
                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                    onTextChanged: {
                        if (changeMutex) {
                            return
                        }

                        let newValue = parseFloat(text)
                        if (isNaN(newValue) || newValue === Number.POSITIVE_INFINITY || newValue === Number.NEGATIVE_INFINITY) {
                            fieldBinding.resetValue()
                        } else {
                            fieldBinding.setValue(newValue)
                        }
                    }
                }

                Connections {
                    target: root

                    function onClicked() {
                        if (!numberEditor.visible) {
                            if (!wizardMode) {
                                form.pushPage(Qt.resolvedUrl("FieldKeypadPage.qml"), { recordUid: fieldBinding.recordUid, fieldUid: fieldBinding.fieldUid } )
                            } else {
                                form.wizard.edit(fieldBinding.recordUid, fieldBinding.fieldUid)
                            }
                        }
                    }
                }
            }

            Image {
                fillMode: Image.PreserveAspectFit
                source: fieldBinding.fieldIcon
                sourceSize.width: root.minRowHeight
                sourceSize.height: root.minRowHeight
                visible: fieldBinding.fieldIcon.toString() !== ""
                verticalAlignment: Image.AlignVCenter
            }
        }
    }

    Component {
        id: photoFieldComponent

        RowLayout {
            Item { height: root.minRowHeight }

            Column {
                Layout.fillWidth: true
                spacing: 5 // default value for ColumnLayout
                height: photoTitle.implicitHeight + photoListView.implicitHeight

                C.FieldName {
                    id: photoTitle
                    width: parent.width
                    recordUid: fieldBinding.recordUid
                    fieldUid: fieldBinding.fieldUid
                    color: titleColor(fieldBinding)
                    font.bold: titleFontBold(fieldBinding)
                    visible: titleVisible(fieldBinding)
                }

                C.FieldPhotoListView {
                    id: photoListView
                    recordUid: fieldBinding.recordUid
                    fieldUid: fieldBinding.fieldUid
                    imageSize: root.minRowHeight * 2.2
                    highlightInvalid: root.highlightInvalid
                    width: parent.width
                    implicitHeight: imageSize
                }
            }

            Image {
                fillMode: Image.PreserveAspectFit
                source: fieldBinding.fieldIcon
                sourceSize.width: root.minRowHeight
                sourceSize.height: root.minRowHeight
                visible: fieldBinding.fieldIcon.toString() !== ""
                verticalAlignment: Image.AlignVCenter
            }
        }
    }

    Component {
        id: audioFieldComponent

        RowLayout {
            Item { height: root.minRowHeight }

            Column {
                Layout.fillWidth: true
                spacing: 5 // default value for ColumnLayout
                height: audioTitle.implicitHeight + audioValue.implicitHeight

                C.FieldName {
                    id: audioTitle
                    width: parent.width
                    recordUid: fieldBinding.recordUid
                    fieldUid: fieldBinding.fieldUid
                    color: titleColor(fieldBinding)
                    font.bold: titleFontBold(fieldBinding)
                    visible: titleVisible(fieldBinding)
                }

                C.FieldAudio {
                    id: audioValue
                    width: parent.width
                    recordUid: fieldBinding.recordUid
                    fieldUid: fieldBinding.fieldUid
                }
            }

            Image {
                fillMode: Image.PreserveAspectFit
                source: fieldBinding.fieldIcon
                sourceSize.width: root.minRowHeight
                sourceSize.height: root.minRowHeight
                visible: fieldBinding.fieldIcon.toString() !== ""
                verticalAlignment: Image.AlignVCenter
            }
        }
    }

    Component {
        id: sketchFieldComponent

        RowLayout {
            Item { height: root.minRowHeight }

            Column {
                Layout.fillWidth: true
                spacing: 5 // default value for ColumnLayout
                height: stringTitle.implicitHeight + imageValue.implicitHeight

                C.FieldName {
                    id: stringTitle
                    width: parent.width
                    recordUid: fieldBinding.recordUid
                    fieldUid: fieldBinding.fieldUid
                    color: titleColor(fieldBinding)
                    font.bold: titleFontBold(fieldBinding)
                    visible: titleVisible(fieldBinding)
                }

                Image {
                    id: imageValue
                    height: 64
                    source: fieldBinding.value ? "data:image/svg+xml;utf8," + Utils.renderSketchToSvg(fieldBinding.value.sketch, "black", 5) : ""
                    visible: !fieldBinding.isEmpty
                    fillMode: Image.PreserveAspectFit
                    layer {
                        enabled: true
                        effect: ColorOverlay {
                            color: Material.foreground
                        }
                    }
                }
            }

            Image {
                fillMode: Image.PreserveAspectFit
                source: fieldBinding.fieldIcon
                sourceSize.width: root.minRowHeight
                sourceSize.height: root.minRowHeight
                visible: fieldBinding.fieldIcon.toString() !== ""
                verticalAlignment: Image.AlignVCenter
            }

            Connections {
                target: root

                function onClicked() {
                    if (!wizardMode) {
                        form.pushPage(Qt.resolvedUrl("FieldSketchpadPage.qml"), { recordUid: fieldBinding.recordUid, fieldUid: fieldBinding.fieldUid } )
                    } else {
                        form.wizard.edit(fieldBinding.recordUid, fieldBinding.fieldUid)
                    }
                }
            }
        }
    }

    Component {
        id: lineFieldComponent

        RowLayout {
            Item { height: root.minRowHeight }

            Column {
                Layout.fillWidth: true
                spacing: 5 // default value for ColumnLayout
                height: stringTitle.implicitHeight + imageValue.implicitHeight

                C.FieldName {
                    id: stringTitle
                    width: parent.width
                    recordUid: fieldBinding.recordUid
                    fieldUid: fieldBinding.fieldUid
                    color: titleColor(fieldBinding)
                    font.bold: titleFontBold(fieldBinding)
                    visible: titleVisible(fieldBinding)
                }

                Label {
                    font.pixelSize: App.settings.font14
                    text: fieldBinding.displayValue
                    visible: !fieldBinding.isEmpty
                }

                Image {
                    id: imageValue
                    height: 64
                    source: fieldBinding.value ? "data:image/svg+xml;utf8," + Utils.renderPointsToSvg(fieldBinding.value, "black", 5, fieldBinding.fieldType === "AreaField") : ""
                    visible: !fieldBinding.isEmpty
                    fillMode: Image.PreserveAspectFit
                    layer {
                        enabled: true
                        effect: ColorOverlay {
                            color: Material.foreground
                        }
                    }
                }
            }

            Image {
                fillMode: Image.PreserveAspectFit
                source: fieldBinding.fieldIcon
                sourceSize.width: root.minRowHeight
                sourceSize.height: root.minRowHeight
                visible: fieldBinding.fieldIcon.toString() !== ""
                verticalAlignment: Image.AlignVCenter
            }

            Connections {
                target: root

                function onClicked() {
                    if (!wizardMode) {
                        form.pushPage(Qt.resolvedUrl("FieldLinePage.qml"), { recordUid: fieldBinding.recordUid, fieldUid: fieldBinding.fieldUid } )
                    } else {
                        form.wizard.edit(fieldBinding.recordUid, fieldBinding.fieldUid)
                    }
                }
            }
        }
    }

    Component {
        id: dateTimeFieldComponent

        RowLayout {
            Item { height: root.minRowHeight }

            Column {
                Layout.fillWidth: true
                spacing: 5 // default value for ColumnLayout
                height: dateTimeTitle.implicitHeight + dateTimeValue.implicitHeight

                C.FieldName {
                    id: dateTimeTitle
                    width: parent.width
                    recordUid: fieldBinding.recordUid
                    fieldUid: fieldBinding.fieldUid
                    color: titleColor(fieldBinding)
                    font.bold: titleFontBold(fieldBinding)
                    visible: titleVisible(fieldBinding)
                }

                Label {
                    id: dateTimeValue
                    width: parent.width
                    color: valueColor(fieldBinding)
                    font.pixelSize: App.settings.font16
                    font.bold: true
                    wrapMode: Label.Wrap
                    text: fieldBinding.displayValue
                    visible: !fieldBinding.isEmpty
                }

                Connections {
                    target: root

                    function onClicked() {
                        if (!wizardMode) {
                            form.pushPage(Qt.resolvedUrl("FieldDateTimePickerPage.qml"), { recordUid: fieldBinding.recordUid, fieldUid: fieldBinding.fieldUid } )
                        } else {
                            form.wizard.edit(fieldBinding.recordUid, fieldBinding.fieldUid)
                        }
                    }
                }
            }

            Image {
                fillMode: Image.PreserveAspectFit
                source: fieldBinding.fieldIcon
                sourceSize.width: root.minRowHeight
                sourceSize.height: root.minRowHeight
                visible: fieldBinding.fieldIcon.toString() !== ""
                verticalAlignment: Image.AlignVCenter
            }
        }
    }

    Component {
        id: calculateFieldComponent

        RowLayout {
            Item { height: root.minRowHeight }

            Column {
                Layout.fillWidth: true
                spacing: 5 // default value for ColumnLayout
                height: calculateTitle.implicitHeight + calculateValue.implicitHeight

                C.FieldName {
                    id: calculateTitle
                    width: parent.width
                    recordUid: fieldBinding.recordUid
                    fieldUid: fieldBinding.fieldUid
                    color: titleColor(fieldBinding)
                    font.bold: titleFontBold(fieldBinding)
                    visible: titleVisible(fieldBinding)
                }

                Label {
                    id: calculateValue
                    width: parent.width
                    color: valueColor(fieldBinding)
                    font.pixelSize: App.settings.font16
                    font.bold: true
                    wrapMode: Label.Wrap
                    text: fieldBinding.displayValue
                    visible: !fieldBinding.isEmpty
                }
            }

            Image {
                fillMode: Image.PreserveAspectFit
                source: fieldBinding.fieldIcon
                sourceSize.width: root.minRowHeight
                sourceSize.height: root.minRowHeight
                visible: fieldBinding.fieldIcon.toString() !== ""
                verticalAlignment: Image.AlignVCenter
            }
        }
    }

    enabled: {
        if (fieldBinding.field !== null && fieldBinding.field.readonly) {
            return false
        }

        if (root.callbackEnabled !== undefined) {
            return root.callbackEnabled(fieldBinding)
        }

        return true
    }

    contentItem: Loader {
        id: loadedItem
        width: parent.width - parent.leftPadding - parent.rightPadding
    }

    swipe.enabled: enableSwipeReset
    swipe.right: Rectangle {
        width: parent.width
        height: parent.height
        color: C.Style.colorGroove

        RowLayout {
            anchors.fill: parent

            Label {
                text: qsTr("Reset data?")
                leftPadding: 20
                font.pixelSize: App.settings.font16
                fontSizeMode: Label.Fit
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
            }

            ToolButton {
                flat: true
                text: qsTr("Yes")
                onClicked: {
                    root.swipe.close()
                    fieldBinding.resetValue()
                }
                Layout.alignment: Qt.AlignRight
                Layout.preferredWidth: parent.height
                Layout.maximumWidth: parent.width / 5
                font.pixelSize: App.settings.font14
            }

            ToolButton {
                flat: true
                text: qsTr("No")
                onClicked: root.swipe.close()
                Layout.alignment: Qt.AlignRight
                Layout.preferredWidth: parent.height
                Layout.maximumWidth: parent.width / 5
                font.pixelSize: App.settings.font14
            }
        }
    }

    C.FieldStateMarker {
        recordUid: root.recordUid
        fieldUid: root.fieldUid
        x: -root.padding + 4
        y: -root.padding + 4
        icon.width: 12
        icon.height: 12
        icon.color: titleColor(fieldBinding)
    }

    ToolButton {
        id: hintButton
        anchors.right: root.right
        anchors.top: root.top
        anchors.rightMargin: -root.padding + 6
        anchors.topMargin: -root.padding + 6
        icon.source: "qrc:/icons/info.svg"
        icon.width: 20
        icon.height: 20
        opacity: 0.75
        clip: true
        icon.color: Material.foreground
        visible: false
        onClicked: form.pushPage(Qt.resolvedUrl("HintPage.qml"), { title: fieldBinding.fieldName, elementUid: fieldBinding.field.hintElementUid, link: fieldBinding.hintLink })
    }

    onPressAndHold: {
        if (hintButton.visible) {
            hintButton.clicked()
        }
    }

    function titleColor(fieldBinding) {
        return (highlightInvalid && !fieldBinding.isValid) ? Style.colorInvalid : Material.foreground
    }

    function titleFontBold(fieldBinding) {
        return highlightInvalid && !fieldBinding.isValid ? true : false
    }

    function titleVisible(fieldBinding) {
        return root.showTitle
    }

    function valueColor(fieldBinding) {
        return fieldBinding.isCalculated ? Style.colorCalculated : Material.foreground
    }

    function getComponent() {
        switch (fieldBinding.fieldType) {
         case "LocationField":
             return locationFieldComponent

         case "LineField":
         case "AreaField":
             return lineFieldComponent

         case "CheckField":
             return (fieldBinding.fieldIsList === true) ? checkListFieldComponent : checkFieldComponent

         case "AcknowledgeField":
             return checkFieldComponent

         case "StaticField":
             return staticFieldComponent

         case "StringField":
             if (fieldBinding.fieldIsList) {
                 return fieldBinding.field.parameters.checkbox !== undefined ? checkFieldComponent : listFieldComponent
             }

             return stringFieldComponent

         case "NumberField":
             return (fieldBinding.fieldIsList === true) ? numberListFieldComponent : numberFieldComponent

         case "PhotoField":
             return photoFieldComponent

         case "AudioField":
             return audioFieldComponent

         case "SketchField":
             return sketchFieldComponent

         case "DateField":
         case "TimeField":
         case "DateTimeField":
             return dateTimeFieldComponent

         case "RecordField":
             return recordFieldComponent

         case "CalculateField":
             return calculateFieldComponent

         default: console.log(fieldBinding.fieldType)
         }

         return undefined
    }
}
