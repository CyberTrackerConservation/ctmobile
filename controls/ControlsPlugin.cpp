#include "ControlsPlugin.h"
#include "Skyplot.h"
#include "Sketchpad.h"

void registerControls(QQmlEngine* engine)
{
    engine->addImportPath(":/imports");

    engine->rootContext()->setContextProperty("FormViewUrl", "qrc:/imports/CyberTracker.1/FormView.qml");

    qmlRegisterType<Skyplot>("CyberTracker", 1, 0, "Skyplot");
    qmlRegisterType<Sketchpad>("CyberTracker", 1, 0, "Sketchpad");
}
