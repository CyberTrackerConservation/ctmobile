import QtQuick 2.0

Item {
    id: root

    property var params
    property var popupComponent

    QtObject {
        id: internal
        property var popup: null
    }

    function open(params) {
        close()

        internal.popup = root.popupComponent.createObject(appWindow, params === undefined ? ({}) : params)
        internal.popup.open()
    }

    function close() {
        if (internal.popup !== null) {
            internal.popup.close()
            internal.popup = null
        }
    }

    Connections {
        target: internal.popup

        function onClosed() {
            internal.popup = null
        }
    }
}
