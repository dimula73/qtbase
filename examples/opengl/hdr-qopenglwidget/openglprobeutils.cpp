// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "openglprobeutils.h"

#include <QApplication>
#include <QSurfaceFormat>
#include <QOpenGLContext>
#include <QColorSpace>
#include <QDebug>
#include <QWindow>

namespace OpenGLProbeUtils {

namespace {

struct AppAttributeSetter
{
    AppAttributeSetter(Qt::ApplicationAttribute attribute, bool useOpenGLES)
        : m_attribute(attribute),
          m_oldValue(QCoreApplication::testAttribute(attribute))
    {
        QCoreApplication::setAttribute(attribute, useOpenGLES);
    }

    ~AppAttributeSetter() {
        QCoreApplication::setAttribute(m_attribute, m_oldValue);
    }

private:
    Qt::ApplicationAttribute m_attribute;
    bool m_oldValue = false;
};

struct SurfaceFormatSetter
{
    SurfaceFormatSetter(const QSurfaceFormat &format)
        : m_oldFormat(QSurfaceFormat::defaultFormat())
    {
        QSurfaceFormat::setDefaultFormat(format);
    }

    ~SurfaceFormatSetter() {
        QSurfaceFormat::setDefaultFormat(m_oldFormat);
    }

private:
    QSurfaceFormat m_oldFormat;
};

}

bool fuzzyCompareColorSpaces(const QColorSpace &lhs, const QColorSpace &rhs)
{
    return lhs == rhs ||
        ((lhs == QColorSpace() ||
          lhs == QColorSpace::SRgb) &&
         (rhs == QColorSpace() ||
          rhs == QColorSpace::SRgb));
}


bool probeFormat(const QSurfaceFormat &format, bool adjustGlobalState)
{
    QScopedPointer<AppAttributeSetter> sharedContextSetter;
    QScopedPointer<AppAttributeSetter> glSetter;
    QScopedPointer<AppAttributeSetter> glesSetter;
    QScopedPointer<SurfaceFormatSetter> formatSetter;
    QScopedPointer<QApplication> application;

    if (adjustGlobalState) {
        sharedContextSetter.reset(new AppAttributeSetter(Qt::AA_ShareOpenGLContexts, false));

        if (format.renderableType() != QSurfaceFormat::DefaultRenderableType) {
            glSetter.reset(new AppAttributeSetter(Qt::AA_UseDesktopOpenGL, format.renderableType() != QSurfaceFormat::OpenGLES));
            glesSetter.reset(new AppAttributeSetter(Qt::AA_UseOpenGLES, format.renderableType() == QSurfaceFormat::OpenGLES));
        }

        formatSetter.reset(new SurfaceFormatSetter(format));

        int argc = 1;
        QByteArray data("krita");
        char *argv = data.data();
        application.reset(new QApplication(argc, &argv));
    }

    QWindow surface;
    surface.setFormat(format);
    surface.setSurfaceType(QSurface::OpenGLSurface);
    surface.create();

    QOpenGLContext context;
    context.setFormat(format);


    if (!context.create()) {
        qCritical() << "OpenGL context cannot be created";
        return false;
    }
    if (!context.isValid()) {
        qCritical() << "OpenGL context is not valid while checking Qt's OpenGL status";
        return false;
    }
    if (!context.makeCurrent(&surface)) {
        qCritical() << "OpenGL context cannot be made current";
        return false;
    }

    if (!fuzzyCompareColorSpaces(context.format().colorSpace(), format.colorSpace())) {
        qCritical() << "Failed to create an OpenGL context with requested color space. Requested:" << format.colorSpace() << "Actual:" << context.format().colorSpace();
        return false;
    }

    return true;
}

}
