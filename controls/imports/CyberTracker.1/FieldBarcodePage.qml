import QtQuick 2.12
import QtQuick.Controls 2.12
import CyberTracker 1.0 as C

ContentPage {
    property alias recordUid: barcode.recordUid
    property alias fieldUid: barcode.fieldUid

    header: PageHeader {
        text: fieldUid !== "" ? form.getFieldName(recordUid, fieldUid) : qsTr("Scan")
    }

    C.FieldBarcode {
        id: barcode
        anchors.fill: parent

        onTagFound: {
            if (fieldUid === "") {
                App.barcodeScan(tag)
            }

            form.popPage(StackView.Immediate)
        }
    }
}
