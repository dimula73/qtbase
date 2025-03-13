// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QGuiApplication>
#include <QSurfaceFormat>
#include <QOpenGLContext>
#include <QCommandLineParser>
#include <QCommandLineOption>


#include "glwindow.h"

// This example demonstrates easy, cross-platform usage of OpenGL ES 3.0 functions via
// QOpenGLExtraFunctions in an application that works identically on desktop platforms
// with OpenGL 3.3 and mobile/embedded devices with OpenGL ES 3.0.

// The code is always the same, with the exception of two places: (1) the OpenGL context
// creation has to have a sufficiently high version number for the features that are in
// use, and (2) the shader code's version directive is different.

int main(int argc, char *argv[])
{
    QCoreApplication::setApplicationName("Qt Hello GLES 3 Example");
    QCoreApplication::setOrganizationName("QtProject");
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::applicationName());
    parser.addHelpOption();
    parser.addVersionOption();

#ifdef Q_OS_WIN
    QCommandLineOption openglApiOption("api", "Choose openGL API to use", "opengl|opengles");
    openglApiOption.setDefaultValue("opengles");
    parser.addOption(openglApiOption);

    {
        QStringList arguments;
        arguments.reserve(argc);
        for (int i = 0; i < argc; ++i) {
            arguments.append(QString::fromLatin1(argv[i]));
        }
        parser.process(arguments);
    }

    if (parser.isSet(openglApiOption)) {
        if (!QStringList({"opengl", "opengles"}).contains(parser.value(openglApiOption))) {
            qWarning() << "Unsupported openGL API flavour:" << parser.value(openglApiOption);
            return 1;
        }
        if (parser.value(openglApiOption) == "opengles") {
            QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);
        }
    }
#endif
    
    QGuiApplication app(argc, argv);

    QSurfaceFormat fmt;
    fmt.setDepthBufferSize(24);

    // Request OpenGL 3.3 core or OpenGL ES 3.0.
    if (QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGL) {
        qDebug("Requesting 3.3 core context");
        fmt.setVersion(3, 3);
        fmt.setProfile(QSurfaceFormat::CoreProfile);
    } else {
        qDebug("Requesting 3.0 context");
        fmt.setVersion(3, 0);
    }

    QSurfaceFormat::setDefaultFormat(fmt);

    GLWindow glWindow;
    glWindow.showMaximized();

    return app.exec();
}
