import QtQuick 2.12
import QtQuick.Controls.Material 2.12
import QtGraphicalEffects 1.0

SquareIcon {
    source: "qrc:/icons/chevron_right.svg"

    opacity: {
        for (let p = this; p !== null; p = p.parent) {
            if (!p.enabled) {
                return 0.25
            }
        }

        return 0.5
    }
}
