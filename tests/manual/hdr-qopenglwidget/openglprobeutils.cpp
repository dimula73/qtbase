/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "openglprobeutils.h"

#include <QApplication>
#include <QSurfaceFormat>
#include <QOpenGLContext>
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

bool fuzzyCompareColorSpaces(const QSurfaceFormat::ColorSpace &lhs, const QSurfaceFormat::ColorSpace &rhs)
{
    return lhs == rhs ||
        ((lhs == QSurfaceFormat::DefaultColorSpace ||
          lhs == QSurfaceFormat::sRGBColorSpace) &&
         (rhs == QSurfaceFormat::DefaultColorSpace ||
          rhs == QSurfaceFormat::sRGBColorSpace));
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
