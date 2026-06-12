import QtQuick
import QtQuick.Controls.Material

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
