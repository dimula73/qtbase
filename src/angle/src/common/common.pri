# static builds should still link ANGLE dynamically when dynamic GL is enabled
include($$OUT_PWD/../../../gui/qtgui-config.pri)
static:qtConfig(dynamicgl) {
    CONFIG -= static
    CONFIG += shared
}

CONFIG += installed
include (../config.pri)

INCLUDEPATH += \
    $$ANGLE_DIR/src \
    $$ANGLE_DIR/include

LIBS_PRIVATE = $$QMAKE_LIBS_GUI

TR_EXCLUDE += $$ANGLE_DIR/src/*

lib_replace.match = $$[QT_INSTALL_LIBS/get]
lib_replace.replace = \$\$\$\$[QT_INSTALL_LIBS]
lib_replace.CONFIG = path
QMAKE_PRL_INSTALL_REPLACE += lib_replace

FXC = $$QMAKE_FXC_LOCATION

static: DEFINES *= LIBGLESV2_EXPORT_H_ ANGLE_EXPORT=
