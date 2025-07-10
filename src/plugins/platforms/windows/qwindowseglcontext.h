// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSEGLCONTEXT_H
#define QWINDOWSEGLCONTEXT_H

#include "qwindowsopenglcontext.h"
#include "qwindowsopengltester.h"
#include <EGL/egl.h>

QT_BEGIN_NAMESPACE

class QEglConfigFunctions;

struct QWindowsLibEGL
{
    bool init();

    EGLint(EGLAPIENTRY *eglGetError)(void);
    EGLDisplay(EGLAPIENTRY *eglGetDisplay)(EGLNativeDisplayType display_id);
    EGLBoolean(EGLAPIENTRY *eglInitialize)(EGLDisplay dpy, EGLint *major, EGLint *minor);
    EGLBoolean(EGLAPIENTRY *eglTerminate)(EGLDisplay dpy);
    EGLBoolean(EGLAPIENTRY *eglChooseConfig)(EGLDisplay dpy, const EGLint *attrib_list,
                                             EGLConfig *configs, EGLint config_size,
                                             EGLint *num_config);
    EGLBoolean(EGLAPIENTRY *eglGetConfigAttrib)(EGLDisplay dpy, EGLConfig config, EGLint attribute,
                                                EGLint *value);
    EGLBoolean(EGLAPIENTRY *eglQueryContext)(EGLDisplay dpy, EGLContext ctx, EGLint attribute,
                                             EGLint *value);
    EGLSurface(EGLAPIENTRY *eglCreateWindowSurface)(EGLDisplay dpy, EGLConfig config,
                                                    EGLNativeWindowType win,
                                                    const EGLint *attrib_list);
    EGLSurface(EGLAPIENTRY *eglCreatePbufferSurface)(EGLDisplay dpy, EGLConfig config,
                                                     const EGLint *attrib_list);
    EGLBoolean(EGLAPIENTRY *eglDestroySurface)(EGLDisplay dpy, EGLSurface surface);
    EGLBoolean(EGLAPIENTRY *eglBindAPI)(EGLenum api);
    EGLBoolean(EGLAPIENTRY *eglSwapInterval)(EGLDisplay dpy, EGLint interval);
    EGLContext(EGLAPIENTRY *eglCreateContext)(EGLDisplay dpy, EGLConfig config,
                                              EGLContext share_context, const EGLint *attrib_list);
    EGLBoolean(EGLAPIENTRY *eglDestroyContext)(EGLDisplay dpy, EGLContext ctx);
    EGLBoolean(EGLAPIENTRY *eglMakeCurrent)(EGLDisplay dpy, EGLSurface draw, EGLSurface read,
                                            EGLContext ctx);
    EGLContext(EGLAPIENTRY *eglGetCurrentContext)(void);
    EGLSurface(EGLAPIENTRY *eglGetCurrentSurface)(EGLint readdraw);
    EGLDisplay(EGLAPIENTRY *eglGetCurrentDisplay)(void);
    EGLBoolean(EGLAPIENTRY *eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
    const char *(EGLAPIENTRY *eglQueryString)(EGLDisplay dpy, EGLint name);
    QFunctionPointer(EGLAPIENTRY *eglGetProcAddress)(const char *procname);
    EGLBoolean (EGLAPIENTRY * eglWaitNative)(EGLint engine);
    EGLBoolean (EGLAPIENTRY * eglSurfaceAttrib)(EGLDisplay display, EGLSurface surface, EGLint attribute, EGLint value);

    EGLDisplay(EGLAPIENTRY *eglGetPlatformDisplayEXT)(EGLenum platform, void *native_display,
                                                      const EGLint *attrib_list);

    typedef void *EGLObjectKHR;
    typedef void *EGLLabelKHR;
    typedef void(APIENTRY *EGLDEBUGPROCKHR)(EGLenum error, const char *command, EGLint messageType,
                                            EGLLabelKHR threadLabel, EGLLabelKHR objectLabel,
                                            const char *message);
    EGLint(EGLAPIENTRY *eglDebugMessageControlKHR)(EGLDEBUGPROCKHR callback,
                                                   const EGLAttrib *attrib_list);

private:
    void *resolve(const char *name);
    HMODULE m_lib;
};

struct QWindowsLibGLESv2
{
    bool init();

    void *moduleHandle() const { return m_lib; }

    const GLubyte *(APIENTRY *glGetString)(GLenum name);

    void *resolve(const char *name);

private:
    HMODULE m_lib;
};

class QWindowsEGLStaticContext : public QWindowsStaticOpenGLContext
{
    Q_DISABLE_COPY_MOVE(QWindowsEGLStaticContext)

public:
    static QWindowsEGLStaticContext *create(QWindowsOpenGLTester::Renderers preferredType);
    ~QWindowsEGLStaticContext() override;

    EGLDisplay display() const { return m_display; }

    QWindowsOpenGLContext *createContext(QOpenGLContext *context) override;
    QWindowsOpenGLContext *createContext(EGLContext context, EGLDisplay display, QOpenGLContext *shareContext);

    void *moduleHandle() const override { return libGLESv2.moduleHandle(); }
    QOpenGLContext::OpenGLModuleType moduleType() const override { return QOpenGLContext::LibGLES; }

    void *createWindowSurface(void *nativeWindow, void *nativeConfig, const QColorSpace &colorSpace,
                              const QSize &size,
                              int *err) override;
    void destroyWindowSurface(void *nativeSurface) override;
    void updateWindowSurfaceSize(void * nativeSurface, const QSize & size) override;

    QSurfaceFormat formatFromConfig(EGLDisplay display, EGLConfig config,
                                    const QSurfaceFormat &referenceFormat);

    bool hasPixelFormatFloatSupport() const { return m_hasPixelFormatFloatSupport; }
    bool isYUpInNDC() const { return m_isYUpInNDC; }

    static QWindowsLibEGL libEGL;
    static QWindowsLibGLESv2 libGLESv2;
    static std::unique_ptr<QEglConfigFunctions> eglConfigFunctions;

private:
    explicit QWindowsEGLStaticContext(EGLDisplay display, bool isYUpInNDC);
    static bool initializeAngle(QWindowsOpenGLTester::Renderers preferredType, HDC dc,
                                EGLDisplay *display, EGLint *major, EGLint *minor,
                                QWindowsOpenGLTester::Renderer *resultRenderer);

    const EGLDisplay m_display;
    bool m_hasSRGBColorSpaceSupport;
    bool m_hasSCRGBColorSpaceSupport;
    bool m_hasBt2020PQColorSpaceSupport;
    bool m_hasPixelFormatFloatSupport;
    bool m_isYUpInNDC;
    bool m_manuallyUpdateSurfaceSize;
};

class QWindowsEGLContext : public QWindowsOpenGLContext, public QNativeInterface::QEGLContext
{
public:
    explicit QWindowsEGLContext(QWindowsEGLStaticContext *staticContext,
                                const QSurfaceFormat &format, QPlatformOpenGLContext *share);
    explicit QWindowsEGLContext(QWindowsEGLStaticContext *staticContext, 
                                EGLContext context, EGLDisplay display, QOpenGLContext *shareContext);
    ~QWindowsEGLContext() override;

    void beginFrame() override;
    bool makeCurrent(QPlatformSurface *surface) override;
    void doneCurrent() override;
    void swapBuffers(QPlatformSurface *surface) override;
    QFunctionPointer getProcAddress(const char *procName) override;

    QSurfaceFormat format() const override { return m_format; }
    bool isSharing() const override { return m_shareContext != EGL_NO_CONTEXT; }
    bool isValid() const override { return m_eglContext != EGL_NO_CONTEXT && !m_markedInvalid; }
    bool isYUpInNDC() const override;

    EGLContext nativeContext() const override { return m_eglContext; }
    EGLDisplay display() const override { return m_eglDisplay; }
    EGLConfig config() const override { return m_eglConfig; }

    virtual void invalidateContext() override { m_markedInvalid = true; }

private:
    QWindowsEGLStaticContext *m_staticContext;
    EGLContext m_eglContext;
    EGLContext m_shareContext;
    EGLDisplay m_eglDisplay;
    EGLConfig m_eglConfig;
    QSurfaceFormat m_format;
    EGLenum m_api = EGL_OPENGL_ES_API;
    int m_swapInterval = -1;

    bool m_markedInvalid = false;
};

QT_END_NAMESPACE

#endif // QWINDOWSEGLCONTEXT_H
