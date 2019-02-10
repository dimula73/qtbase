QT += widgets widgets-private gui-private core-private

TARGET = hdr-openglwidget
TEMPLATE = app

SOURCES += main.cpp \
        #hdr-openglwidget.cpp \
        openglprobeutils.cpp \
        KisGLImageWidget.cpp \
        KisGLImageF16.cpp \
        window.cpp

HEADERS  += \
#hdr-openglwidget.h \
    openglprobeutils.h \
    KisGLImageWidget.h \
    KisGLImageF16.h \
    window.h

RESOURCES += kis_gl_image_widget.qrc
