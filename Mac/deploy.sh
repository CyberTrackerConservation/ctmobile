#!/bin/sh

QTDIR=$HOME/"Qt/5.15.2/clang_64/bin"
CTAPP=$HOME/"github/build-ct-Desktop_Qt_5_15_2_clang_64bit-Release/CyberTracker.app"
CTQML=$HOME/"github/ct/qml"
ARCGIS=$HOME/"ArcGIS_SDKs/Qt100.15.0"

cd $QTDIR

# run macdeployqt with options
./macdeployqt $CTAPP -qmlimport=$ARCGIS/sdk/macOS/x64/qml -executable=$CTAPP/Contents/MacOS/CyberTracker -qmldir=$CTQML

# copy in dependent library libEsriCommonQt.dylib
cp $ARCGIS/sdk/macOS/x64/lib/libEsriCommonQt.dylib $CTAPP/Contents/Frameworks 

# copy in dependent library libruntimecore.dylib
cp $ARCGIS/sdk/macOS/x64/lib/libruntimecore.dylib $CTAPP/Contents/Frameworks 

# run install_name_tool on the ArcGISRuntime plugin dylib to find EsriCommonQt.dylib
cd $CTAPP/Contents/Plugins/quick

install_name_tool -change $ARCGIS/sdk/macOS/x64/lib/libEsriCommonQt.dylib @executable_path/../Frameworks/libEsriCommonQt.dylib libArcGISRuntimePlugin.dylib

# check that all references now are relative and not hardcoded to install path
otool -L libArcGISRuntimePlugin.dylib

# run install_name_tool on the EsriCommonQt dylib to find runtimecore.dylib
cd $CTAPP/Contents/Frameworks

install_name_tool -change $ARCGIS/sdk/macOS/x64/lib/libruntimecore.dylib @executable_path/../Frameworks/libruntimecore.dylib libEsriCommonQt.dylib

# check that all references now are relative and not hardcoded to install path
otool -L libEsriCommonQt.dylib

cd $CTAPP/..
rm -rf CyberTracker.dmg
hdiutil create -srcfolder "CyberTracker.app" \
  -volname "CyberTracker" \
  -fs HFS+ -fsargs "-c c=64,a=16,e=16" \
  -format UDZO -imagekey zlib-level=9 "CT-build-$1-mac.dmg"
