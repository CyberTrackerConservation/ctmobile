#!/bin/sh
# create_dmg Frobulator Frobulator.dmg path/to/frobulator/dir [ 'Your Code Sign Identity' ]
set -e

VOLNAME="$1"
DMG="$2"
SRC_DIR="$3"
CODESIGN_IDENTITY="$4"

hdiutil create -srcfolder "$SRC_DIR" \
  -volname "$VOLNAME" \
  -fs HFS+ -fsargs "-c c=64,a=16,e=16" \
  -format UDZO -imagekey zlib-level=9 "$DMG"

if [ -n "$CODESIGN_IDENTITY" ]; then
  codesign -s "$CODESIGN_IDENTITY" -v "$DMG"
fi
