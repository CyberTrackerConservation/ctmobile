import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import CyberTracker as C

ListViewV {
    id: listView

    property alias sightingUid: sightingModel.sightingUid
    property alias sighting: sightingModel.sighting

    model: C.SightingModel {
        id: sightingModel
    }

    delegate: ItemDelegate {
        id: d
        width: ListView.view.width
        implicitHeight: wrapDelegate.implicitHeight

        Component {
            id: textDelegateComponent
            Label {
                anchors.fill: parent
                text: modelData.caption
                font.pixelSize: App.settings.font14
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                opacity: 0.5
            }
        }

        Component {
            id: groupDelegateComponent
            Label {
                anchors.fill: parent
                text: modelData.caption
                font.bold: true
                font.pixelSize: App.settings.font14
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }
        }

        Component {
            id: elementDelegateComponent
            RowLayout {
                Label {
                    id: controlLabel
                    Layout.fillWidth: true
                    wrapMode: Label.WordWrap
                    elide: Label.ElideRight
                    text: modelData.caption
                    font.bold: true
                    font.pixelSize: App.settings.font14
                    verticalAlignment: Text.AlignVCenter
                }

                SquareIcon {
                    Layout.alignment: Qt.AlignRight
                    source: modelData.icon
                    size: Style.minRowHeight
                }
            }
        }

        Component {
            id: timestampDelegateComponent
            Label {
                anchors.fill: parent
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                font.bold: true
                font.pixelSize: App.settings.font14
                wrapMode: Label.WordWrap
                elide: Label.ElideRight
                text: modelData.createTime === "" ? qsTr("Unsaved") : App.formatDateTime(modelData.createTime)
            }
        }

        Component {
            id: locationDelegateComponent
            RowLayout {
                anchors.fill: parent
                Item {
                    width: buttonLocation.width
                    height: buttonLocation.height
                }

                Label {
                    Layout.fillWidth: true
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    font.bold: true
                    font.pixelSize: App.settings.font14
                    wrapMode: Label.WordWrap
                    elide: Label.ElideRight
                    text: App.getLocationText(modelData.value.x, modelData.value.y, modelData.value.z)
                }

                ToolButton {
                    id: buttonLocation
                    icon.source: "qrc:/icons/map_empty.svg"
                    icon.width: Style.toolButtonSize
                    icon.height: Style.toolButtonSize
                    opacity: 0.5
                    onClicked: {
                        let points = [[modelData.value.x, modelData.value.y]]
                        let params = {
                            title: qsTr("Location"),
                            footerText: App.getLocationText(modelData.value.x, modelData.value.y, modelData.value.z),
                            hideDataLayers: true,
                            mode: C.MapController.ModeField,
                            initialPanMode: C.MapController.AutoPanModeOff,
                            initialExtent: Utils.computeExtent(points),
                            fieldValue: { fieldType: "LocationField", fieldValue: points },
                        }

                        pushPageImmediate(listView, "qrc:/imports/CyberTracker/EsriMapViewPage.qml", params)
                    }
                }
            }
        }

        Component {
            id: recordHeaderDelegateComponent
            Label {
                anchors.fill: parent
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                font.bold: true
                font.pixelSize: App.settings.font14
                fontSizeMode: Label.Fit
                text: modelData.text
            }
        }

        Component {
            id: fieldDelegateComponent

            RowLayout {
                spacing: 0

                RowLayout {
                    spacing: App.scaleByFontSize(4)
                    Layout.preferredWidth: parent.width / 2
                    Layout.maximumWidth: parent.width / 2

                    Item {
                        width: App.scaleByFontSize(8)
                    }

                    SquareIcon {
                        size: Style.iconSize24
                        source: modelData.fieldIcon
                        visible: modelData.fieldIcon !== ""
                    }

                    Label {
                        Layout.fillWidth: true
                        font.pixelSize: App.settings.font14
                        wrapMode: Label.WordWrap
                        elide: Label.ElideRight
                        text: modelData.fieldName
                    }
                }

                Label {
                    Layout.preferredWidth: parent.width / 2
                    Layout.maximumWidth: parent.width / 2
                    font.pixelSize: App.settings.font14
                    wrapMode: Label.WordWrap
                    elide: Label.ElideRight
                    text: modelData.fieldValue
                }
            }
        }

        Component {
            id: lineDelegateComponent
            RowLayout {
                spacing: 0

                RowLayout {
                    spacing: App.scaleByFontSize(4)
                    Layout.preferredWidth: parent.width / 2
                    Layout.maximumWidth: parent.width / 2

                    Item {
                        width: App.scaleByFontSize(8)
                    }

                    SquareIcon {
                        size: Style.iconSize24
                        source: modelData.fieldIcon
                        visible: modelData.fieldIcon !== ""
                    }

                    Label {
                        Layout.fillWidth: true
                        font.pixelSize: App.settings.font14
                        wrapMode: Label.WordWrap
                        elide: Label.ElideRight
                        text: modelData.fieldName
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Image {
                        Layout.fillWidth: true
                        Layout.maximumHeight: C.Style.iconSize64
                        source: modelData.value ? "data:image/svg+xml;utf8," + Utils.renderPointsToSvg(modelData.value, "black", 5, modelData.fieldType === "AreaField") : ""
                        fillMode: Image.PreserveAspectFit
                        horizontalAlignment: Image.AlignLeft
                        layer.enabled: true
                        layer.effect: SimpleColorOverlay {
                            color: Material.foreground
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        font.pixelSize: App.settings.font12
                        wrapMode: Label.WordWrap
                        elide: Label.ElideRight
                        text: modelData.fieldValue
                        opacity: 0.5
                    }
                }
            }
        }

        Component {
            id: sketchDelegateComponent
            RowLayout {
                spacing: 0

                RowLayout {
                    spacing: App.scaleByFontSize(4)
                    Layout.preferredWidth: parent.width / 2
                    Layout.maximumWidth: parent.width / 2

                    Item {
                        width: App.scaleByFontSize(8)
                    }

                    SquareIcon {
                        size: Style.iconSize24
                        source: modelData.fieldIcon
                        visible: modelData.fieldIcon !== ""
                    }

                    Label {
                        Layout.fillWidth: true
                        font.pixelSize: App.settings.font14
                        wrapMode: Label.WordWrap
                        elide: Label.ElideRight
                        text: modelData.fieldName
                    }
                }

                Image {
                    Layout.fillWidth: true
                    Layout.maximumHeight: C.Style.iconSize64
                    source: modelData.value ? "data:image/svg+xml;utf8," + Utils.renderSketchToSvg(modelData.value.sketch, "black", 5) : ""
                    fillMode: Image.PreserveAspectFit
                    horizontalAlignment: Image.AlignLeft
                    layer.enabled: true
                    layer.effect: SimpleColorOverlay {
                        color: Material.foreground
                    }
                }
            }
        }

        Component {
            id: trackDelegateComponent
            ColumnLayout {
                anchors.fill: parent
                Image {
                    Layout.fillWidth: true
                    Layout.maximumHeight: C.Style.iconSize64
                    source: "data:image/svg+xml;utf8," + Utils.renderPointsToSvg(modelData.points, "black", 5)
                    fillMode: Image.PreserveAspectFit
                    horizontalAlignment: Image.AlignHCenter
                    layer {
                        enabled: true
                        effect: SimpleColorOverlay {
                            color: Material.foreground
                        }
                    }
                }

                Label {
                    Layout.fillWidth: true
                    horizontalAlignment: Label.AlignHCenter
                    font.pixelSize: App.settings.font12
                    wrapMode: Label.WordWrap
                    elide: Label.ElideRight
                    text: modelData.summary
                    opacity: 0.5
                }
            }
        }

        Component {
            id: audioDelegateComponent
            RowLayout {
                spacing: 0

                RowLayout {
                    spacing: App.scaleByFontSize(4)
                    Layout.preferredWidth: parent.width / 2
                    Layout.maximumWidth: parent.width / 2

                    Item {
                        width: App.scaleByFontSize(8)
                    }

                    SquareIcon {
                        size: Style.iconSize24
                        source: modelData.fieldIcon
                        visible: modelData.fieldIcon !== ""
                    }

                    Label {
                        Layout.fillWidth: true
                        font.pixelSize: App.settings.font14
                        wrapMode: Label.WordWrap
                        elide: Label.ElideRight
                        text: modelData.fieldName
                    }
                }

                FieldAudio {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    fieldValue: modelData.value
                }
            }
        }

        Component {
            id: classicPhotoDelegateComponent
            RowLayout {
                spacing: 0

                RowLayout {
                    spacing: App.scaleByFontSize(4)
                    Layout.preferredWidth: parent.width / 2
                    Layout.maximumWidth: parent.width / 2

                    Item {
                        width: App.scaleByFontSize(8)
                    }

                    SquareIcon {
                        size: Style.iconSize24
                        source: modelData.fieldIcon
                        visible: modelData.fieldIcon !== ""
                    }

                    Label {
                        Layout.fillWidth: true
                        font.pixelSize: App.settings.font14
                        wrapMode: Label.WordWrap
                        elide: Label.ElideRight
                        text: modelData.fieldName
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: classicPhotoImage.sourceSize.height > 0 ? App.scaleByFontSize(96) : 0

                    Image {
                        id: classicPhotoImage
                        anchors.fill: parent
                        fillMode: Image.PreserveAspectFit
                        horizontalAlignment: Image.AlignLeft
                        source: {
                            let photos = modelData.value.filter(function(p) { return p !== "" })
                            return photos.length > 0 ? App.getMediaFileUrl(photos[0]) : ""
                        }
                        sourceSize.width: App.scaleByFontSize(96)
                        sourceSize.height: App.scaleByFontSize(96)
                        autoTransform: true

                        MouseArea {
                            anchors.fill: parent
                            enabled: classicPhotoImage.source !== ""
                            onClicked: {
                                pushPageImmediate(listView, Qt.resolvedUrl("ImagePage.qml"), { text: modelData.fieldName, source: classicPhotoImage.source })
                            }
                        }
                    }
                }
            }
        }

        Component {
            id: photosDelegateComponent
            Control {
                anchors.fill: parent
                implicitHeight: imageSize
                property int imageSize: App.scaleByFontSize(96)

                ListViewH {
                    id: listViewPhotos

                    property var photoTags: modelData.photoTags

                    anchors.fill: parent
                    leftMargin: spacing
                    spacing: App.scaleByFontSize(8)
                    model: modelData.value.filter(function(photo) { return photo !== "" });
                    delegate: ItemDelegate {
                        width: imageSize
                        height: imageSize
                        padding: 0
                        contentItem: Item {
                            Image {
                                id: image
                                anchors.fill: parent
                                fillMode: Image.PreserveAspectFit
                                source: modelData !== "" ? App.getMediaFileUrl(modelData) : ""
                                sourceSize.width: imageSize
                                sourceSize.height: imageSize
                                autoTransform: true
                            }
                        }
                        enabled: modelData !== ""
                        onClicked: {
                            pushPageImmediate(listView, Qt.resolvedUrl("ImagePage.qml"), { text: qsTr("Photo"), source: image.source, tags: listViewPhotos.photoTags[modelData] })
                        }

                        Rectangle {
                            color: "blue"
                            x: 0
                            y: 0
                            width: parent.height / 12
                            height: width
                            visible: listViewPhotos.photoTags[modelData].length > 0
                        }
                    }
                }
            }
        }

        ItemDelegate {
            id: wrapDelegate
            anchors.fill: parent

            padding: App.scaleByFontSize(8)

            function getComponent() {
                backColor.color = "transparent"
                switch (modelData.delegate) {
                case "text":
                    return textDelegateComponent

                case "group":
                    backColor.color = Material.primary
                    return groupDelegateComponent

                case "element":
                    return elementDelegateComponent

                case "timestamp":
                    return timestampDelegateComponent

                case "trackFile":
                    return trackDelegateComponent

                case "field":
                    switch (modelData.fieldType) {
                    case "PhotoField":
                        return modelData.classicPhoto ? classicPhotoDelegateComponent : photosDelegateComponent

                    case "LocationField":
                        return modelData.depth > 0 ? fieldDelegateComponent : locationDelegateComponent

                    case "LineField":
                    case "AreaField":
                        return lineDelegateComponent

                    case "SketchField":
                        return sketchDelegateComponent

                    case "AudioField":
                        return audioDelegateComponent
                    }

                    return fieldDelegateComponent

                case "recordHeader":
                    return recordHeaderDelegateComponent

                default:
                    return undefined
                }
            }

            contentItem: modelData ? getComponent().createObject(wrapDelegate) : null

            Rectangle {
                x: leftPadding
                y: parent.height - 1
                width: contentItem.width
                height: 1
                color: C.Style.colorGroove
                visible: modelData.divider === true
            }

            onClicked: {
                if (modelData.delegate === "trackFile") {
                    let params = {
                        title: qsTr("Track"),
                        footerText: modelData.summary,
                        hideDataLayers: true,
                        mode: C.MapController.ModeField,
                        initialPanMode: C.MapController.AutoPanModeOff,
                        initialExtent: Utils.computeExtent(modelData.points),
                        fieldValue: { fieldType: "TrackFile", fieldValue: modelData.points },
                    }

                    pushPageImmediate(listView, Qt.resolvedUrl("EsriMapViewPage.qml"), params)
                    return
                }

                if (modelData.fieldType === "LineField" || modelData.fieldType === "AreaField") {
                    let params = {
                        title: modelData.fieldName,
                        footerText: modelData.fieldValue,
                        hideDataLayers: true,
                        mode: C.MapController.ModeField,
                        initialPanMode: C.MapController.AutoPanModeOff,
                        initialExtent: Utils.computeExtent(modelData.value),
                        fieldValue: { fieldType: modelData.fieldType, fieldValue: modelData.value },
                    }

                    pushPageImmediate(listView, Qt.resolvedUrl("EsriMapViewPage.qml"), params)
                    return
                }
            }
        }

        background: Rectangle {
            id: backColor
            anchors.fill: parent
            opacity: 0.5
        }
    }
}
