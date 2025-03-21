// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QColorSpace>
#include "window.h"

#include "openglprobeutils.h"
#include <QDebug>

QSurfaceFormat generateSurfaceFormat(QSurfaceFormat::RenderableType renderer,
                                     const QColorSpace &colorSpace,
                                     int bitDepth)
{
    QSurfaceFormat format;
#ifdef Q_OS_MACOS
    format.setVersion(3, 2);
    format.setProfile(QSurfaceFormat::CoreProfile);
#else
    format.setVersion(3, 0);
    format.setProfile(QSurfaceFormat::CoreProfile);
#endif
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);

    switch (bitDepth) {
    case 8:
        format.setRedBufferSize(8);
        format.setGreenBufferSize(8);
        format.setBlueBufferSize(8);
        format.setAlphaBufferSize(8);
        break;
    case 10:
        format.setRedBufferSize(10);
        format.setGreenBufferSize(10);
        format.setBlueBufferSize(10);
        format.setAlphaBufferSize(2);
        break;
    case 16:
        format.setRedBufferSize(16);
        format.setGreenBufferSize(16);
        format.setBlueBufferSize(16);
        format.setAlphaBufferSize(16);
        break;
    default:
        qFatal("Unsupported surface bit depth %d", bitDepth);
    }

    format.setRenderableType(renderer);
    format.setColorSpace(colorSpace);

    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format.setSwapInterval(0); // Disable vertical refresh syncing

    return format;
}

int main(int argc, char *argv[])
{
    qputenv("QT_WIDGETS_RHI", "1");
    qputenv("QT_WIDGETS_RHI_BACKEND", "opengl");
    qputenv("QSG_RHI_BACKEND", "opengl");

    QVector<QSurfaceFormat> allFormats;

    QVector<QSurfaceFormat::RenderableType> availableRenderers;
    availableRenderers << QSurfaceFormat::OpenGL;
    availableRenderers << QSurfaceFormat::OpenGLES;

    for (QSurfaceFormat::RenderableType renderer : availableRenderers) {
        allFormats << generateSurfaceFormat(renderer, QColorSpace::SRgb, 8);
        allFormats << generateSurfaceFormat(renderer, QColorSpace::Bt2100Pq, 8);
        allFormats << generateSurfaceFormat(renderer, QColorSpace::SRgb, 10);
        allFormats << generateSurfaceFormat(renderer, QColorSpace::Bt2100Pq, 10);
        allFormats << generateSurfaceFormat(renderer, QColorSpace::SRgbLinear, 16);
    }

    for (QSurfaceFormat format : allFormats) {
        qDebug() << "Probing: " << format;
        bool result = OpenGLProbeUtils::probeFormat(format, true);
        qDebug() << "    result:" << result;
    }


    if (argc > 1 && !strcmp(argv[1], "--sharecontext")) {
        qDebug("Requesting all contexts to share");
        QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    }

    QApplication a(argc, argv);

    QSurfaceFormat::RenderableType renderer = QSurfaceFormat::OpenGLES;
    QColorSpace colorSpace = QColorSpace::SRgbLinear;
    int bitDepth = 16;


    if (QCoreApplication::arguments().contains(QLatin1String("--scrgb"))) {
        colorSpace = QColorSpace::SRgbLinear;
        bitDepth = 16;
    } else if (QCoreApplication::arguments().contains(QLatin1String("--bt2020pq"))) {
        colorSpace = QColorSpace::Bt2100Pq;
        bitDepth = 10;
    } else if (QCoreApplication::arguments().contains(QLatin1String("--srgb"))) {
        colorSpace = QColorSpace::SRgb;
        bitDepth = 8;
    }

    if (QCoreApplication::arguments().contains(QLatin1String("--opengl"))) {
        renderer = QSurfaceFormat::OpenGL;
    } else if (QCoreApplication::arguments().contains(QLatin1String("--opengles"))) {
        renderer = QSurfaceFormat::OpenGLES;
    }

    QSurfaceFormat format = generateSurfaceFormat(renderer, colorSpace, bitDepth);

    if (QCoreApplication::arguments().contains(QLatin1String("--multisample"))) {
        format.setSamples(4);
    }

    if (format.renderableType() == QSurfaceFormat::OpenGL) {
        QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL, true);
    } else if (format.renderableType() == QSurfaceFormat::OpenGLES) {
        QCoreApplication::setAttribute(Qt::AA_UseOpenGLES, true);
    }

    qDebug() << "Requesting" << format.renderableType() << format;
    QSurfaceFormat::setDefaultFormat(format);

    Window window;
    window.show();

    return a.exec();
}
