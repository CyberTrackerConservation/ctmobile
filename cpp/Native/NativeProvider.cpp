#include "NativeProvider.h"

NativeProvider::NativeProvider(QObject *parent) : Provider(parent)
{
    update_name(NATIVE_PROVIDER);
}

QUrl NativeProvider::getStartPage()
{
    return getProjectFileUrl("StartPage.qml");
}

void NativeProvider::getElements(ElementManager* elementManager)
{
    elementManager->loadFromQmlFile(getProjectFilePath("Elements.qml"));
}

void NativeProvider::getFields(FieldManager* fieldManager)
{
    fieldManager->loadFromQmlFile(getProjectFilePath("Fields.qml"));
}
