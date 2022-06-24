#include "ClassicProvider.h"
#include "ctSession.h"

ClassicProvider::ClassicProvider(QObject *parent) : Provider(parent)
{
    update_name(CLASSIC_PROVIDER);
}

int ClassicProvider::version()
{
    return RESOURCE_VERSION;
}

QUrl ClassicProvider::getStartPage()
{
    return QUrl("qrc:/Classic/StartPage.qml");
}
