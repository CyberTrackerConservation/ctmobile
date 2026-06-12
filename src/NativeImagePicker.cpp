#include "NativeImagePicker.h"
#include "App.h"

using namespace CyberTracker;

#if !defined(Q_OS_IOS)
NativeImagePicker::NativeImagePicker(QObject *parent) : QObject(parent)
{
    connect(App::instance(), &App::imagePickComplete, this, [&](const QString& fileUrl)
    {
        if (!fileUrl.isEmpty())
        {
            emit accepted(fileUrl);
        }
        else
        {
            emit rejected();
        }
    });
}

void NativeImagePicker::open()
{
#if defined(Q_OS_ANDROID)
    QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<void>("startImagePicker", "()V");
#else
    emit rejected();
#endif
}
#endif
