import QtQuick
import QtQuick.Controls.Material
import CyberTracker as C

Calendar {
    id: root

    property alias recordUid: fieldBinding.recordUid
    property alias fieldUid: fieldBinding.fieldUid

    locale: App.locale
    frameVisible: false

    Component.onCompleted: {
        if (fieldBinding.field.minDate !== "") {
            root.minimumDate = fieldBinding.field.minDate
        }

        if (fieldBinding.field.maxDate !== "") {
            root.maximumDate = fieldBinding.field.maxDate
        }

        if (fieldBinding.value !== undefined) {
            root.selectedDate = Date.fromLocaleDateString(root.locale, fieldBinding.value, "yyyy-MM-dd")
        }
    }

    C.FieldBinding {
        id: fieldBinding
    }

    onSelectedDateChanged: {
        fieldBinding.setValue(Qt.formatDate(root.selectedDate, "yyyy-MM-dd"))
    }

    style: CalendarStyle {
        background: Rectangle {
            anchors.fill: parent
            color: Material.background
        }
    }
}
