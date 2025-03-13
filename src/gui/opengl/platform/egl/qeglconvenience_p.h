// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QEGLCONVENIENCE_H
#define QEGLCONVENIENCE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/qsurfaceformat.h>
#include <QtCore/qlist.h>
#include <QtCore/qsize.h>

#include <QtGui/private/qt_egl_p.h>

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QEglConfigFunctions
{
public:
    virtual ~QEglConfigFunctions();
    virtual EGLBoolean eglChooseConfig(EGLDisplay dpy, const EGLint *attrib_list,
                                       EGLConfig *configs, EGLint config_size,
                                       EGLint *num_config) = 0;
    virtual EGLBoolean eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config, EGLint attribute,
                                          EGLint *value) = 0;
    virtual const char * eglQueryString(EGLDisplay dpy, EGLint name) = 0;
    virtual QFunctionPointer eglGetProcAddress(const char *procname) = 0;
    virtual EGLint eglGetError() = 0;
};

Q_GUI_EXPORT QEglConfigFunctions* q_resolveEglConfigFunctions(QEglConfigFunctions *func);

Q_GUI_EXPORT QList<EGLint> q_createConfigAttributesFromFormat(const QSurfaceFormat &format);

Q_GUI_EXPORT bool q_reduceConfigAttributes(QList<EGLint> *configAttributes);

Q_GUI_EXPORT EGLConfig q_configFromGLFormat(EGLDisplay display,
                                               const QSurfaceFormat &format,
                                               bool highestPixelFormat = false,
                                               int surfaceType = EGL_WINDOW_BIT,
                                               QEglConfigFunctions *func = nullptr);

Q_GUI_EXPORT QSurfaceFormat q_glFormatFromConfig(EGLDisplay display, const EGLConfig config,
                                                    const QSurfaceFormat &referenceFormat = {},
                                                    QEglConfigFunctions *func = nullptr);

Q_GUI_EXPORT bool q_hasEglExtension(EGLDisplay display,const char* extensionName, QEglConfigFunctions *func = nullptr);

Q_GUI_EXPORT void q_printEglConfig(EGLDisplay display, EGLConfig config, QEglConfigFunctions *func = nullptr);

#ifdef Q_OS_UNIX
Q_GUI_EXPORT QSizeF q_physicalScreenSizeFromFb(int framebufferDevice,
                                                  const QSize &screenSize = {});

Q_GUI_EXPORT  QSize q_screenSizeFromFb(int framebufferDevice);

Q_GUI_EXPORT int q_screenDepthFromFb(int framebufferDevice);

Q_GUI_EXPORT  qreal q_refreshRateFromFb(int framebufferDevice);

#endif

class Q_GUI_EXPORT QEglConfigChooser
{
public:
    QEglConfigChooser(EGLDisplay display, QEglConfigFunctions *func = nullptr);
    virtual ~QEglConfigChooser();

    EGLDisplay display() const { return m_display; }

    void setSurfaceType(EGLint surfaceType) { m_surfaceType = surfaceType; }
    EGLint surfaceType() const { return m_surfaceType; }

    void setSurfaceFormat(const QSurfaceFormat &format) { m_format = format; }
    QSurfaceFormat surfaceFormat() const { return m_format; }

    void setIgnoreColorChannels(bool ignore) { m_ignore = ignore; }
    bool ignoreColorChannels() const { return m_ignore; }

    EGLConfig chooseConfig();

protected:
    virtual bool filterConfig(EGLConfig config) const;

    QEglConfigFunctions *m_func = nullptr;

    QSurfaceFormat m_format;
    EGLDisplay m_display;
    EGLint m_surfaceType;
    bool m_ignore;

    int m_confAttrRed;
    int m_confAttrGreen;
    int m_confAttrBlue;
    int m_confAttrAlpha;
};


QT_END_NAMESPACE

#endif //QEGLCONVENIENCE_H
