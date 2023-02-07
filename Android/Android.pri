INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

ANDROID_ABIS = arm64-v8a armeabi-v7a #x86

# Pick up the version information.
ANDROID_VERSION_NAME = $${VERSION}
ANDROID_VERSION_CODE = $$sprintf("%1%2%3%4", \
    $$format_number($${VER_MAJ}, width=2 zeropad), \
    $$format_number($${VER_MIN}, width=2 zeropad), \
    $$format_number($${VER_PAT}, width=2 zeropad), \
    0)

HEADERS += \
    $$PWD/../cpp/LocationAndroid.h

SOURCES += \
    $$PWD/../cpp/LocationAndroid.cpp

# Files.
ANDROID_PACKAGE_SOURCE_DIR += $$PWD

DISTFILES += \
    $$ANDROID_PACKAGE_SOURCE_DIR/AndroidManifest.xml\
    Android/gradle/wrapper/gradle-wrapper.jar \
    Android/gradle/wrapper/gradle-wrapper.properties \
    Android/gradlew \
    Android/gradlew.bat \
    Android/res/values/libs.xml

OTHER_FILES += \
    $$ANDROID_PACKAGE_SOURCE_DIR/build.gradle \
    $$ANDROID_PACKAGE_SOURCE_DIR/src/org/cybertracker/mobile/MainActivity.java \
    $$ANDROID_PACKAGE_SOURCE_DIR/src/org/cybertracker/mobile/LocationUpdateService.java \
    $$ANDROID_PACKAGE_SOURCE_DIR/src/org/cybertracker/mobile/LocationWakeupService.java \
    $$ANDROID_PACKAGE_SOURCE_DIR/src/org/cybertracker/mobile/AlarmJobService.java \
    $$ANDROID_PACKAGE_SOURCE_DIR/src/org/cybertracker/mobile/AdminReceiver.java \
    $$ANDROID_PACKAGE_SOURCE_DIR/src/org/cybertracker/mobile/QtPositioning.java \
    $$ANDROID_PACKAGE_SOURCE_DIR/src/org/cybertracker/mobile/Utils.java \

# Default.
OTHER_FILES += \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable/splashscreen.xml \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/values/strings.xml \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/values/theme.xml \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/xml/file_paths.xml \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/xml/device_admin.xml \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable-hdpi/track_service_icon.png \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable-mdpi/track_service_icon.png \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable-xhdpi/track_service_icon.png \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable-xxhdpi/track_service_icon.png \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable-xxxhdpi/track_service_icon.png \

# CT.
OTHER_FILES += \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable/ct_splashscreen.xml \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable-anydpi-v26/ct_launcher.xml \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable-anydpi-v26/ct_launcher_round.xml \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable-hdpi/ct_launcher.png \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable-hdpi/ct_launcher_foreground.png \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable-hdpi/ct_launcher_round.png \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable-mdpi/ct_launcher.png \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable-mdpi/ct_launcher_foreground.png \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable-mdpi/ct_launcher_round.png \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable-xhdpi/ct_launcher.png \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable-xhdpi/ct_launcher_foreground.png \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable-xhdpi/ct_launcher_round.png \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable-xxhdpi/ct_launcher.png \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable-xxhdpi/ct_launcher_foreground.png \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable-xxhdpi/ct_launcher_round.png \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable-xxxhdpi/ct_launcher.png \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable-xxxhdpi/ct_launcher_foreground.png \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable-xxxhdpi/ct_launcher_round.png \

# SM.
OTHER_FILES += \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable/sm_splashscreen.xml \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable-anydpi-v26/sm_launcher.xml \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable-anydpi-v26/sm_launcher_round.xml \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable-hdpi/sm_launcher.png \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable-hdpi/sm_launcher_foreground.png \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable-hdpi/sm_launcher_round.png \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable-mdpi/sm_launcher.png \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable-mdpi/sm_launcher_foreground.png \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable-mdpi/sm_launcher_round.png \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable-xhdpi/sm_launcher.png \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable-xhdpi/sm_launcher_foreground.png \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable-xhdpi/sm_launcher_round.png \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable-xxhdpi/sm_launcher.png \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable-xxhdpi/sm_launcher_foreground.png \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable-xxhdpi/sm_launcher_round.png \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable-xxxhdpi/sm_launcher.png \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable-xxxhdpi/sm_launcher_foreground.png \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/drawable-xxxhdpi/sm_launcher_round.png \

# OpenSSL.
include ($$ANDROID_PACKAGE_SOURCE_DIR/android_openssl/openssl.pri)
