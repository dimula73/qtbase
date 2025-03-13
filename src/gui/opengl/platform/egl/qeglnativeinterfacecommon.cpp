// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtGui/private/qtguiglobal_p.h>

#if QT_CONFIG(opengl)
#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformopenglcontext.h>
#include <qpa/qplatformintegration.h>
#endif

/*!
    \class QNativeInterface::QEGLContext
    \since 6.0
    \brief Native interface to an EGL context.

    Accessed through QOpenGLContext::nativeInterface().

    \inmodule QtGui
    \inheaderfile QOpenGLContext
    \ingroup native-interfaces
    \ingroup native-interfaces-qopenglcontext
*/

/*!
    \fn QOpenGLContext *QNativeInterface::QEGLContext::fromNative(EGLContext context, EGLDisplay display, QOpenGLContext *shareContext = nullptr)

    \brief Adopts an EGLContext \a context.

    The same \c EGLDisplay passed to \c eglCreateContext must be passed as the \a display argument.

    Ownership of the created QOpenGLContext \a shareContext is transferred
    to the caller.
*/

/*!
    \fn EGLContext QNativeInterface::QEGLContext::nativeContext() const

    \return the underlying EGLContext.
*/

/*!
    \fn EGLConfig QNativeInterface::QEGLContext::config() const
    \since 6.3
    \return the EGLConfig associated with the underlying EGLContext.
*/

/*!
    \fn EGLDisplay QNativeInterface::QEGLContext::display() const
    \since 6.3
    \return the EGLDisplay associated with the underlying EGLContext.
*/


/*!
    \fn void QNativeInterface::QEGLContext::invalidateContext()
    \since 6.5
    \brief Marks the context as invalid

    If this context is used by the Qt Quick scenegraph, this will trigger the
    SceneGraph to destroy this context and create a new one.

    Similarly to QPlatformWindow::invalidateSurface(),
    this function can only be expected to have an effect on certain platforms,
    such as eglfs.

    \sa QOpenGLContext::isValid(), QPlatformWindow::invalidateSurface()
*/

QT_BEGIN_NAMESPACE

#ifndef QT_NO_OPENGL

using namespace QNativeInterface::Private;

QT_DEFINE_NATIVE_INTERFACE(QEGLContext);
QT_DEFINE_PRIVATE_NATIVE_INTERFACE(QEGLIntegration);

QOpenGLContext *QNativeInterface::QEGLContext::fromNative(EGLContext context, EGLDisplay display, QOpenGLContext *shareContext)
{
    return QGuiApplicationPrivate::platformIntegration()->call<
        &QEGLIntegration::createOpenGLContext>(context, display, shareContext);
}

#endif // QT_NO_OPENGL

QT_END_NAMESPACE