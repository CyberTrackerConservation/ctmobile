ARCGIS_RUNTIME_VERSION = 100.15.0
include ($$PWD/arcgisruntime.pri)

include ($$PWD/miniz/miniz.pri)

include ($$PWD/geographiclib/geographiclib.pri)

include ($$PWD/geomag/geomag.pri)

include ($$PWD/quazip/quazip.pri)

CONFIG += qzxing_multimedia
CONFIG += enable_encoder_qr_code
DEFINES += QZXING_QML
include ($$PWD/qzxing/QZXing.pri)

include ($$PWD/zonedetect/ZoneDetect.pri)

include ($$PWD/classic/ClassicLib.pri)

include ($$PWD/duperagent/com_cutehacks_duperagent.pri)

include ($$PWD/qxlnt/Qxlnt.pri)

DEFINES += QTCSV_STATIC_LIB
include ($$PWD/qtcsv/qtcsv.pri)

include ($$PWD/qtkeychain/qtkeychain.pri)
