#!/bin/sh

QTDIR=$HOME/"Qt/6.10.2/macos/bin"
CTAPP=$HOME/"github/ct/build/Qt_6_10_2_for_macOS-Release/ct.app"
CTQML=$HOME/"github/ct/qml"
ARCGIS=$HOME/"ArcGIS_SDKs/qt200.8.0"

cd $QTDIR

# run macdeployqt with options
./macdeployqt $CTAPP -executable=$CTAPP/Contents/MacOS/ct -qmldir=$CTQML

# remove unneeded SQL driver plugins with missing dependencies
rm -f $CTAPP/Contents/PlugIns/sqldrivers/libqsqlodbc.dylib
rm -f $CTAPP/Contents/PlugIns/sqldrivers/libqsqlpsql.dylib
rm -f $CTAPP/Contents/PlugIns/sqldrivers/libqsqlmimer.dylib

# copy in dependent library libEsriCommonQt.dylib
cp $ARCGIS/sdk/macOS/universal/lib/libEsriCommonQt.dylib $CTAPP/Contents/Frameworks 

# copy in dependent library libruntimecore.dylib
cp $ARCGIS/sdk/macOS/universal/lib/libruntimecore.dylib $CTAPP/Contents/Frameworks 

# fix the ct executable to find ArcGIS libraries
cd $CTAPP/Contents/MacOS
install_name_tool -change libEsriCommonQt.dylib @executable_path/../Frameworks/libEsriCommonQt.dylib ct
install_name_tool -change libruntimecore.dylib @executable_path/../Frameworks/libruntimecore.dylib ct
otool -L ct

# run install_name_tool on the ArcGISRuntime plugin dylib to find EsriCommonQt.dylib
cd $CTAPP/Contents/Plugins/quick

install_name_tool -change $ARCGIS/sdk/macOS/universal/lib/libEsriCommonQt.dylib @executable_path/../Frameworks/libEsriCommonQt.dylib libArcGISRuntimePlugin.dylib

# check that all references now are relative and not hardcoded to install path
otool -L libArcGISRuntimePlugin.dylib

# run install_name_tool on the EsriCommonQt dylib to find runtimecore.dylib
cd $CTAPP/Contents/Frameworks

install_name_tool -change $ARCGIS/sdk/macOS/universal/lib/libruntimecore.dylib @executable_path/../Frameworks/libruntimecore.dylib libEsriCommonQt.dylib

# check that all references now are relative and not hardcoded to install path
otool -L libEsriCommonQt.dylib

cd $CTAPP/..
rm -rf CyberTracker.app
mv ct.app CyberTracker.app
codesign --force --deep --sign - CyberTracker.app
rm -rf "CT-build-$1-mac.dmg"
hdiutil create -srcfolder "CyberTracker.app" \
  -volname "CyberTracker" \
  -fs HFS+ -fsargs "-c c=64,a=16,e=16" \
  -format UDZO -imagekey zlib-level=9 "CT-build-$1-mac.dmg"
mv CyberTracker.app ct.app
