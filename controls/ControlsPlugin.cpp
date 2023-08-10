#include "ControlsPlugin.h"
#include "OAuth2.h"
#include "Skyplot.h"
#include "Sketchpad.h"
#include "SquareImage.h"

void registerControls(QQmlEngine* engine)
{
    engine->addImportPath(":/imports");

    engine->rootContext()->setContextProperty("FormViewUrl", "qrc:/imports/CyberTracker.1/FormView.qml");

    qmlRegisterType<OAuth2>("CyberTracker", 1, 0, "OAuth2");
    qmlRegisterType<Skyplot>("CyberTracker", 1, 0, "Skyplot");
    qmlRegisterType<Sketchpad>("CyberTracker", 1, 0, "Sketchpad");
    qmlRegisterType<SquareImage>("CyberTracker", 1, 0, "SquareImage");
}
