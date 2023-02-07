import QtQuick 2.15
import QtWebEngine 1.10 as WebEngine

// Some websites (https://youtube.com/leanback, for example) can grab focus forever
// https://bugreports.qt.io/browse/QTBUG-46251
WebEngine.WebEngineView {
    focus: true
    //settings.focusOnNavigationEnabled: false
    settings.pluginsEnabled: true
}
