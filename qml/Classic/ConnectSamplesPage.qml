import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    header: C.PageHeader {
        text: qsTr("Samples")
    }

    C.ListViewV {
        anchors.fill: parent
        model: [
            {
                text: "Simple",
                subText: qsTr("Shows what data capture is like for simple cases"),
                iconId: "qrc:/Classic/Simple.png",
                sampleId: "Simple"
            },
            {
                text: "Photo notes",
                subText: qsTr("Shows recording notes, photos and audio"),
                iconId: "qrc:/Classic/PhotoNotes.png",
                sampleId: "PhotoNotes"
            },
            {
                text: "Biodiversity monitoring",
                subText: qsTr("More advanced sample showing track timer and a plant key"),
                iconId: "qrc:/Classic/BioSample.png",
                sampleId: "BioSample"
            },
        ]

        delegate: C.ListRowDelegate {
            id: listRow
            width: ListView.view.width
            text: modelData.text
            subText: modelData.subText
            iconSource: modelData.iconId
            wrapSubText: true
            onClicked: {
                if (App.requestPermissionReadExternalStorage() && App.requestPermissionWriteExternalStorage()) {
                    busyCover.doWork = connectClassicSample
                    busyCover.params = { sampleId: modelData.sampleId }
                    busyCover.start()
                }
            }
        }
    }

    function connectClassicSample() {
        App.projectManager.bootstrap("Classic", busyCover.params)
        postPopToRoot()
    }
}
