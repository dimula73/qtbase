# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## hdr-openglwidget Binary:
#####################################################################

cmake_minimum_required(VERSION 3.16)
project(hdr-openglwidget LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/opengl/hdr-openglwidget")


find_package(Qt6 REQUIRED COMPONENTS Core Gui OpenGL OpenGLWidgets)

qt_standard_project_setup()

qt_add_executable(hdr-openglwidget
    main.cpp
    openglprobeutils.cpp openglprobeutils.h
    KisGLImageWidget.cpp KisGLImageWidget.h
    KisGLImageF16.cpp KisGLImageF16.h
    window.cpp window.h
)

set_target_properties(hdr-openglwidget PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(hdr-openglwidget PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::OpenGL
    Qt6::Widgets
    Qt6::OpenGLWidgets
)

# Resources:
set(shaders_resource_files
    kis_gl_image_widget.frag
    kis_gl_image_widget.vert
)

qt_add_resources(hdr-openglwidget "shaders"
    PREFIX
        "/"
    FILES
        ${shaders_resource_files}
)

install(TARGETS hdr-openglwidget
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)