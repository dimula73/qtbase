// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QWINDOWSOPENGLCONTEXT_H
#define QWINDOWSOPENGLCONTEXT_H

#include <QtGui/qopenglcontext.h>
#include <qpa/qplatformopenglcontext.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_OPENGL

class QWindowsOpenGLContext;

class QWindowsStaticOpenGLContext
{
    Q_DISABLE_COPY_MOVE(QWindowsStaticOpenGLContext)
public:
    static QWindowsStaticOpenGLContext *create();
    virtual ~QWindowsStaticOpenGLContext() = default;

    virtual QWindowsOpenGLContext *createContext(QOpenGLContext *context) = 0;
    virtual void *moduleHandle() const = 0;
    virtual QOpenGLContext::OpenGLModuleType moduleType() const = 0;
    virtual bool supportsThreadedOpenGL() const { return false; }

    // If the windowing system interface needs explicitly created window surfaces (like EGL),
    // reimplement these.
    virtual void *createWindowSurface(void * /*nativeWindow*/, void * /*nativeConfig*/,
                                      const QColorSpace & /*colorSpace*/,
                                      const QSize &/*size*/,
                                      int * /*err*/)
    {
        return nullptr;
    }
    virtual void destroyWindowSurface(void * /*nativeSurface*/) { }
    virtual void updateWindowSurfaceSize(void * /*nativeSurface*/, const QSize & /*size*/) { }

protected:
    QWindowsStaticOpenGLContext() = default;

private:
    static QWindowsStaticOpenGLContext *doCreate();
};

class QWindowsOpenGLContext : public QPlatformOpenGLContext
{
    Q_DISABLE_COPY_MOVE(QWindowsOpenGLContext)
protected:
    QWindowsOpenGLContext() = default;
};

#endif // QT_NO_OPENGL

QT_END_NAMESPACE

#endif // QWINDOWSOPENGLCONTEXT_H
