import QtQuick 2.12
import QtQuick.Controls.Material 2.12
import QtGraphicalEffects 1.0

Image {
    source: "qrc:/icons/chevron_right.svg"
    sourceSize.width: 32
    sourceSize.height: 32

    opacity: {
        for (let p = this; p !== null; p = p.parent) {
            if (!p.enabled) {
                return 0.25
            }
        }

        return 0.5
    }

    layer {
        enabled: true
        effect: ColorOverlay {
            color: Material.foreground
        }
    }
}
