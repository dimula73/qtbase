/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QApplication>
#include "window.h"

#include "openglprobeutils.h"
#include <QDebug>

QSurfaceFormat generateSurfaceFormat(QSurfaceFormat::RenderableType renderer,
                                     QSurfaceFormat::ColorSpace colorSpace,
                                     int bitDepth)
{
    QSurfaceFormat format;
#ifdef Q_OS_OSX
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
    QVector<QSurfaceFormat> allFormats;

    QVector<QSurfaceFormat::RenderableType> availableRenderers;
    availableRenderers << QSurfaceFormat::OpenGL;
    availableRenderers << QSurfaceFormat::OpenGLES;

    for (QSurfaceFormat::RenderableType renderer : availableRenderers) {
        allFormats << generateSurfaceFormat(renderer, QSurfaceFormat::sRGBColorSpace, 8);
        allFormats << generateSurfaceFormat(renderer, QSurfaceFormat::bt2020PQColorSpace, 8);
        allFormats << generateSurfaceFormat(renderer, QSurfaceFormat::sRGBColorSpace, 10);
        allFormats << generateSurfaceFormat(renderer, QSurfaceFormat::bt2020PQColorSpace, 10);
        allFormats << generateSurfaceFormat(renderer, QSurfaceFormat::scRGBColorSpace, 16);
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
    QSurfaceFormat::ColorSpace colorSpace = QSurfaceFormat::scRGBColorSpace;
    int bitDepth = 16;


    if (QCoreApplication::arguments().contains(QLatin1String("--scrgb"))) {
        colorSpace = QSurfaceFormat::scRGBColorSpace;
        bitDepth = 16;
    } else if (QCoreApplication::arguments().contains(QLatin1String("--bt2020pq"))) {
        colorSpace = QSurfaceFormat::bt2020PQColorSpace;
        bitDepth = 10;
    } else if (QCoreApplication::arguments().contains(QLatin1String("--srgb"))) {
        colorSpace = QSurfaceFormat::sRGBColorSpace;
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
