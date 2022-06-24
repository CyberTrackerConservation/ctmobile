TEMPLATE = app
TARGET = CyberTracker

CONFIG += c++11
CONFIG += plugin static

QT += core gui opengl network positioning sensors qml quick xml sql websockets concurrent multimedia

#-------------------------------------------------------------------------------

VER_MAJ = $$(CT_VERSION_MAJOR)
isEmpty(VER_MAJ) { VER_MAJ = 1 }
VER_MIN = $$(CT_VERSION_MINOR)
isEmpty(VER_MIN) { VER_MIN = 0 }
VER_PAT = $$(CT_VERSION_PATCH)
isEmpty(VER_PAT) { VER_PAT = 1 }
VERSION = $$sprintf("%1.%2.%3",$$VER_MAJ,$$VER_MIN,$$VER_PAT)

PRODUCT_SELECT = $$(CT_PRODUCT_SELECT)
isEmpty(PRODUCT_SELECT) { PRODUCT_SELECT = ct }

DEFINES += CT_PRODUCT_SELECT=\\\"$$PRODUCT_SELECT\\\"
DEFINES += CT_VERSION=\\\"$$VERSION\\\"

#-------------------------------------------------------------------------------

include ($$PWD/lib/Lib.pri)
include ($$PWD/controls/Controls.pri)

#-------------------------------------------------------------------------------

*msvc* {
PRECOMPILED_HEADER = cpp/pch.h
CONFIG += precompile_header
}

INCLUDEPATH += \
    $$PWD/cpp \

HEADERS += \
    cpp/pch.h \
    cpp/App.h \
    cpp/AppLink.h \
    cpp/Compass.h \
    cpp/Connector.h \
    cpp/Database.h \
    cpp/Element.h \
    cpp/ElementListModel.h \
    cpp/ElementTreeModel.h \
    cpp/Evaluator.h \
    cpp/EvaluatorFunctions.h \
    cpp/Field.h \
    cpp/FieldBinding.h \
    cpp/FieldListModel.h \
    cpp/Form.h \
    cpp/Goto.h \
    cpp/Location.h \
    cpp/MapLayerListModel.h \
    cpp/MBTilesReader.h \
    cpp/Project.h \
    cpp/ProjectListModel.h \
    cpp/PropertyHelpers.h \
    cpp/ProviderInterface.h \
    cpp/Record.h \
    cpp/RecordListModel.h \
    cpp/Register.h \
    cpp/Satellite.h \
    cpp/Settings.h \
    cpp/Sighting.h \
    cpp/SightingListModel.h \
    cpp/Task.h \
    cpp/Telemetry.h \
    cpp/TimeManager.h \
    cpp/Utils.h \
    cpp/UtilsShare.h \
    cpp/VariantListModel.h \
    cpp/WaveFileRecorder.h \
    cpp/Wizard.h \
    cpp/XlsForm.h \
    cpp/Classic/pch.h \
    cpp/Classic/ClassicConnector.h \
    cpp/Classic/ClassicProvider.h \
    cpp/EarthRanger/pch.h \
    cpp/EarthRanger/EarthRangerConnector.h \
    cpp/EarthRanger/EarthRangerProvider.h \
    cpp/Esri/pch.h \
    cpp/Esri/EsriConnector.h \
    cpp/Esri/EsriProvider.h \
    cpp/KoBo/pch.h \
    cpp/KoBo/KoBoConnector.h \
    cpp/Native/pch.h \
    cpp/Native/NativeConnector.h \
    cpp/Native/NativeProvider.h \
    cpp/ODK/ODKConnector.h \
    cpp/ODK/ODKProvider.h \
    cpp/SMART/pch.h \
    cpp/SMART/SMARTConnector.h \
    cpp/SMART/SMARTProvider.h \

SOURCES += \
    cpp/App.cpp \
    cpp/AppLink.cpp \
    cpp/Compass.cpp \
    cpp/Connector.cpp \
    cpp/Database.cpp \
    cpp/Element.cpp \
    cpp/ElementListModel.cpp \
    cpp/ElementTreeModel.cpp \
    cpp/Evaluator.cpp \
    cpp/Field.cpp \
    cpp/FieldBinding.cpp \
    cpp/FieldListModel.cpp \
    cpp/Form.cpp \
    cpp/Goto.cpp \
    cpp/Location.cpp \
    cpp/Main.cpp \
    cpp/MapLayerListModel.cpp \
    cpp/MBTilesReader.cpp \
    cpp/Project.cpp \
    cpp/ProjectListModel.cpp \
    cpp/ProviderInterface.cpp \
    cpp/Record.cpp \
    cpp/RecordListModel.cpp \
    cpp/Register.cpp \
    cpp/Satellite.cpp \
    cpp/Settings.cpp \
    cpp/Sighting.cpp \
    cpp/SightingListModel.cpp \
    cpp/Task.cpp \
    cpp/Telemetry.cpp \
    cpp/TimeManager.cpp \
    cpp/Utils.cpp \
    cpp/UtilsEsri.cpp \
    cpp/VariantListModel.cpp \
    cpp/WaveFileRecorder.cpp \
    cpp/Wizard.cpp \
    cpp/XlsForm.cpp \
    cpp/Classic/ClassicConnector.cpp \
    cpp/Classic/ClassicProvider.cpp \
    cpp/EarthRanger/EarthRangerConnector.cpp \
    cpp/EarthRanger/EarthRangerProvider.cpp \
    cpp/Esri/EsriConnector.cpp \
    cpp/Esri/EsriProvider.cpp \
    cpp/KoBo/KoBoConnector.cpp \
    cpp/Native/NativeConnector.cpp \
    cpp/Native/NativeProvider.cpp \
    cpp/ODK/ODKConnector.cpp \
    cpp/ODK/ODKProvider.cpp \
    cpp/SMART/SMARTConnector.cpp \
    cpp/SMART/SMARTProvider.cpp \

lupdate_only {
    SOURCES += \
        $$PWD/qml/*.qml \
        $$PWD/qml/Classic/*.qml \
        $$PWD/qml/EarthRanger/*.qml \
        $$PWD/qml/Esri/*.qml \
        $$PWD/qml/KoBo/*.qml \
        $$PWD/qml/ODK/*.qml \
        $$PWD/qml/SMART/*.qml \
}

RESOURCES += \
    config.json \
    languages.json \
    assets/assets.qrc \
    qml/qml.qrc \
    $$files($$PWD/i18n/*.qm) \

OTHER_FILES += \
    AUTHOR.md \
    LICENSE \
    README.md \
    cpp/LocationPlugin.json

#-------------------------------------------------------------------------------

include ($$PWD/Platforms.pri)

#-------------------------------------------------------------------------------

TRANSLATIONS = \
    i18n/ct_af.ts \
    i18n/ct_am.ts \
    i18n/ct_ar.ts \
    i18n/ct_az.ts \
    i18n/ct_be.ts \
    i18n/ct_bg.ts \
    i18n/ct_bn.ts \
    i18n/ct_bs.ts \
    i18n/ct_ca.ts \
    i18n/ct_ceb.ts \
    i18n/ct_co.ts \
    i18n/ct_cs.ts \
    i18n/ct_cy.ts \
    i18n/ct_da.ts \
    i18n/ct_de.ts \
    i18n/ct_el.ts \
    i18n/ct_en.ts \
    i18n/ct_eo.ts \
    i18n/ct_es.ts \
    i18n/ct_et.ts \
    i18n/ct_eu.ts \
    i18n/ct_fa.ts \
    i18n/ct_fi.ts \
    i18n/ct_fr.ts \
    i18n/ct_fy.ts \
    i18n/ct_ga.ts \
    i18n/ct_gd.ts \
    i18n/ct_gl.ts \
    i18n/ct_gu.ts \
    i18n/ct_ha.ts \
    i18n/ct_haw.ts \
    i18n/ct_hi.ts \
    i18n/ct_hmn.ts \
    i18n/ct_hr.ts \
    i18n/ct_ht.ts \
    i18n/ct_hu.ts \
    i18n/ct_hy.ts \
    i18n/ct_id.ts \
    i18n/ct_ig.ts \
    i18n/ct_iku.ts \
    i18n/ct_is.ts \
    i18n/ct_it.ts \
    i18n/ct_iw.ts \
    i18n/ct_ja.ts \
    i18n/ct_jw.ts \
    i18n/ct_ka.ts \
    i18n/ct_kk.ts \
    i18n/ct_km.ts \
    i18n/ct_kn.ts \
    i18n/ct_ko.ts \
    i18n/ct_ku.ts \
    i18n/ct_ky.ts \
    i18n/ct_la.ts \
    i18n/ct_lb.ts \
    i18n/ct_lo.ts \
    i18n/ct_lt.ts \
    i18n/ct_lv.ts \
    i18n/ct_mg.ts \
    i18n/ct_mi.ts \
    i18n/ct_mk.ts \
    i18n/ct_ml.ts \
    i18n/ct_mn.ts \
    i18n/ct_mr.ts \
    i18n/ct_ms.ts \
    i18n/ct_mt.ts \
    i18n/ct_my.ts \
    i18n/ct_ne.ts \
    i18n/ct_nl.ts \
    i18n/ct_no.ts \
    i18n/ct_ny.ts \
    i18n/ct_pa.ts \
    i18n/ct_pl.ts \
    i18n/ct_prs.ts \
    i18n/ct_ps.ts \
    i18n/ct_pt.ts \
    i18n/ct_ro.ts \
    i18n/ct_ru.ts \
    i18n/ct_sd.ts \
    i18n/ct_si.ts \
    i18n/ct_sk.ts \
    i18n/ct_sl.ts \
    i18n/ct_sm.ts \
    i18n/ct_sn.ts \
    i18n/ct_so.ts \
    i18n/ct_sq.ts \
    i18n/ct_sr.ts \
    i18n/ct_st.ts \
    i18n/ct_su.ts \
    i18n/ct_sv.ts \
    i18n/ct_sw.ts \
    i18n/ct_ta.ts \
    i18n/ct_te.ts \
    i18n/ct_tg.ts \
    i18n/ct_th.ts \
    i18n/ct_tl.ts \
    i18n/ct_tr.ts \
    i18n/ct_uk.ts \
    i18n/ct_ur.ts \
    i18n/ct_uz.ts \
    i18n/ct_vi.ts \
    i18n/ct_xh.ts \
    i18n/ct_yi.ts \
    i18n/ct_yo.ts \
    i18n/ct_zh.ts \
    i18n/ct_zh-TW.ts \
    i18n/ct_zu.ts
