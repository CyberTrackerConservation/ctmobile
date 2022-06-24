INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

QMAKE_IOS_DEPLOYMENT_TARGET = 13.6

# Note for devices: 1=iPhone, 2=iPad, 1,2=Universal.
QMAKE_APPLE_TARGETED_DEVICE_FAMILY = 1,2

QMAKE_ASSET_CATALOGS += $$PWD/Assets.xcassets

QMAKE_INFO_PLIST = $$PWD/Info.plist

APP_ENTITLEMENTS.name = CODE_SIGN_ENTITLEMENTS
APP_ENTITLEMENTS.value = $$PWD/AppDomains.entitlements
QMAKE_MAC_XCODE_SETTINGS += MY_ENTITLEMENTS

app_launch_screen.files = $$files($$PWD/LaunchScreen.storyboard)
QMAKE_BUNDLE_DATA += app_launch_screen

OTHER_FILES += \
    $$APP_ENTITLEMENTS.value \
    $$PWD/LaunchScreen.storyboard

QMAKE_TARGET_BUNDLE_PREFIX = org.cybertracker
QMAKE_BUNDLE = mobile
