import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import CyberTracker as C

C.ContentPage {
    id: page

    header: C.PageHeader {
        text: form.project.title
        menuIcon: "qrc:/icons/settings.svg"
        menuVisible: true
        onMenuClicked: {
            form.pushPage("qrc:/imports/CyberTracker/FormSettingsPage.qml")
        }
    }

    footer: RowLayout {
        spacing: 0
        width: page.width

        C.FooterButton {
            text: qsTr("Submit")
            enabled: listView.canSubmitData
            icon.source: "qrc:/icons/upload_multiple.svg"
            onClicked: listView.submit()
        }

        C.FooterButton {
            text: qsTr("Map")
            icon.source: "qrc:/icons/map_outline.svg"
            onClicked: form.pushPage("qrc:/MapsPage.qml")
        }

        C.FooterButton {
            text: qsTr("New")
            icon.source: "qrc:/icons/plus.svg"
            enabled: form.provider.parserError === ""
            onClicked: {
                form.newSighting()
                form.snapCreateTime()
                form.saveSighting()
                if (form.project.wizardMode) {
                    form.pushWizardPage(form.rootRecordUid, {})
                } else {
                    form.pushPage("qrc:/ODK/FormPage.qml")
                }
            }
        }
    }

    // Watermark.
    C.SquareIcon {
        anchors.centerIn: parent
        source: form.project.icon !== "" ? App.projectManager.getFileUrl(form.project.uid, form.project.icon) : ""
        size: Math.min(parent.width, parent.height) / 2
        opacity: 0.15
        recolor: false
    }

    C.SightingsListView {
        id: listView
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            bottom: locationTrackingState.visible ? locationTrackingState.top : parent.bottom
        }

        active: true
        showNoData: false
        filter.projectUid: form.project.uid
        filter.stateSpace: form.stateSpace

        onClicked: function (sighting, index) {
            if (sighting.trackOnly) {
                showTrackFile(page, sighting.trackFileName, sighting.trackFileSummary)
                return
            }

            editSighting(sighting)
        }
    }

    ColumnLayout {
        id: locationTrackingState
        spacing: 0
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }

        visible: form.connected ? (form.trackStreamer.running || form.pointStreamer.running || form.allowUserTrackChange) : false

        Rectangle {
            Layout.fillWidth: true
            height: App.scaleByFontSize(2)
            color: Material.theme === Material.Dark ? "#FFFFFF" : "#000000"
            opacity: 0.125
        }

        RowLayout {
            Layout.fillHeight: true
            Layout.fillWidth: true
            spacing: 0

            Item {
                Layout.preferredWidth: 0
                Layout.preferredHeight: Math.max(1.25, C.Style.switchScale) * checkLocationStreamingEnabled.height * C.Style.switchScale
            }

            Switch {
                id: checkLocationStreamingEnabled
                Layout.alignment: Qt.AlignVCenter
                Layout.leftMargin: App.scaleByFontSize(4) * C.Style.switchScale
                scale: C.Style.switchScale
                visible: form.allowUserTrackChange
                checked: form.project.locationStreamingEnabled
                onCheckedChanged: {
                    form.project.locationStreamingEnabled = checked
                }
            }

            Item {
                Layout.preferredWidth: checkLocationStreamingEnabled.width
                Layout.preferredHeight: checkLocationStreamingEnabled.height
                visible: !checkLocationStreamingEnabled.visible
            }

            Label {
                Layout.fillWidth: true
                verticalAlignment: Qt.AlignVCenter
                horizontalAlignment: Qt.AlignHCenter
                font.pixelSize: App.settings.font14
                elide: Text.ElideRight
                text: {
                    if (form.trackStreamer.running || form.pointStreamer.running) {
                        return form.trackStreamer.running ? form.trackStreamer.rateFullText : form.pointStreamer.rateFullText
                    } else {
                        return qsTr("Location stopped")
                    }
                }
                opacity: 0.5
            }

            C.SquareIcon {
                id: streamerStateIcon
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredWidth: checkLocationStreamingEnabled.width
                size: C.Style.iconSize24
                source: form.trackStreamer.running || form.pointStreamer.running ? App.locationLogger.stateIcon : "qrc:/icons/gps_off.svg"
                opacity: 0.5
            }
        }

        Item {
            height: App.scaleByFontSize(2)
        }
    }

    Component.onCompleted: {
        if (form.provider.parserError !== "") {
            showError(qsTr("Form error") + ": " + form.provider.parserError)
        }
    }

    function editSighting(sighting) {
        form.loadSighting(sighting.sightingUid)

        // Resume the wizard where we left off.
        if (form.project.wizardMode && form.sighting.wizardRecordUid !== "" && form.sighting.wizardPageStack.length > 0) {
            form.pushWizardPage(form.sighting.wizardRecordUid, {})
            return
        }

        // Restart the wizard or page mode from the index.
        form.setPageStack([{ pageUrl: form.startPage }])

        form.wizard.reset()
        if (form.project.wizardMode) {
            form.pushWizardIndexPage(form.rootRecordUid, {})
        } else {
            form.pushPage("qrc:/ODK/FormPage.qml")
        }
    }
}
