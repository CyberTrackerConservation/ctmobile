win32 {
    include (Win/Win.pri)
}

mac {
    cache()
}

macx {
    include (Mac/Mac.pri)
}

ios {
    include (iOS/iOS.pri)
}

android {
    include (Android/Android.pri)
}
