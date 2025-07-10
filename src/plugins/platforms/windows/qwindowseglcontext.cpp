// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowseglcontext.h"
#include "qwindowscontext.h"
#include "qwindowswindow.h"

#include <QtCore/qdebug.h>
#include <QtGui/qopenglcontext.h>

#include <EGL/eglext.h>
#include <VersionHelpers.h>

#include <string_view>

using namespace Qt::Literals::StringLiterals;
using namespace std::string_view_literals;

#include <QtGui/private/qeglconvenience_p.h>

QT_BEGIN_NAMESPACE

void APIENTRY angleDebugMessagesCallback(EGLenum error, const char *command, EGLint messageType,
                                         EGLLabelKHR threadLabel, EGLLabelKHR objectLabel,
                                         const char *message)
{
    Q_UNUSED(threadLabel)
    Q_UNUSED(objectLabel)

    const QLatin1StringView severity([=] () {
        if (messageType == EGL_DEBUG_MSG_CRITICAL_KHR) {
            return "critical";
        } else if (messageType == EGL_DEBUG_MSG_ERROR_KHR) {
            return "error";
        } else if (messageType == EGL_DEBUG_MSG_WARN_KHR) {
            return "warn";
        } else if (messageType == EGL_DEBUG_MSG_INFO_KHR) {
            return "info";
        } else {
            return "unknown";
        }
    }());

    qDebug().noquote().nospace() << "ANGLE (" << severity << ") " << command << ": \"" << message
                                 << "\" (code: " << Qt::hex << Qt::showbase << error << ")";
}

struct IndirectFunctions : QEglConfigFunctions
{
    IndirectFunctions(QWindowsLibEGL *libEGLArg) : libEGL(libEGLArg) {}
    QWindowsLibEGL *libEGL;
    EGLBoolean eglChooseConfig(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs,
                               EGLint config_size, EGLint *num_config) override
    {
        return libEGL->eglChooseConfig(dpy, attrib_list, configs, config_size, num_config);
    }
    EGLBoolean eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config, EGLint attribute,
                                  EGLint *value) override
    {
        return libEGL->eglGetConfigAttrib(dpy, config, attribute, value);
    }
    const char *eglQueryString(EGLDisplay dpy, EGLint name) override
    {
        return libEGL->eglQueryString(dpy, name);
    }
    QFunctionPointer eglGetProcAddress(const char *procname) override
    {
        return reinterpret_cast<QFunctionPointer>(libEGL->eglGetProcAddress(procname));
    }
    EGLint eglGetError() override { return libEGL->eglGetError(); }
};

/*!
    \class QWindowsEGLStaticContext
    \brief Static data for QWindowsEGLContext.

    Keeps the display. The class is shared via QSharedPointer in the windows, the
    contexts and in QWindowsIntegration. The display will be closed if the last instance
    is deleted.

    No EGL or OpenGL functions are called directly. Instead, they are resolved
    dynamically. This works even if the plugin links directly to libegl/libglesv2 so
    there is no need to differentiate between dynamic or Angle-only builds in here.

    \internal
*/

QWindowsLibEGL QWindowsEGLStaticContext::libEGL;
QWindowsLibGLESv2 QWindowsEGLStaticContext::libGLESv2;
std::unique_ptr<QEglConfigFunctions> QWindowsEGLStaticContext::eglConfigFunctions;

#ifdef Q_CC_MINGW
static inline void *resolveFunc(HMODULE lib, const char *name)
{
    const auto baseNameStr{ QString::fromLatin1(name) };
    QString nameStr;
    void *proc = 0;

    // Play nice with 32-bit mingw: Try func first, then func@0, func@4,
    // func@8, func@12, ..., func@64. The def file does not provide any aliases
    // in libEGL and libGLESv2 in these builds which results in exporting
    // function names like eglInitialize@12. This cannot be fixed without
    // breaking binary compatibility. So be flexible here instead.

    int argSize = -1;
    while (!proc && argSize <= 64) {
        nameStr = baseNameStr;
        if (argSize >= 0)
            nameStr += u'@' + QString::number(argSize);
        argSize = argSize < 0 ? 0 : argSize + 4;
        proc = reinterpret_cast<void *>(::GetProcAddress(lib, nameStr.toLatin1().constData()));
    }
    return proc;
}
#else
static inline void *resolveFunc(HMODULE lib, const char *name)
{
    return reinterpret_cast<void *>(::GetProcAddress(lib, name));
}
#endif // Q_CC_MINGW

void *QWindowsLibEGL::resolve(const char *name)
{
    return m_lib ? resolveFunc(m_lib, name) : nullptr;
}

#define GETPROC(name) reinterpret_cast<decltype(name)>(resolve(#name))

#define RESOLVE(name) name = GETPROC(name)

bool QWindowsLibEGL::init()
{
    static constexpr LPCWSTR dllName{ L"libEGL" };

    qCDebug(lcQpaGl) << "Qt: Using EGL from" << dllName;

    m_lib = ::LoadLibraryW(dllName);
    if (!m_lib) {
        qErrnoWarning(::GetLastError(), "Failed to load %s", dllName);
        return false;
    }

    RESOLVE(eglGetError);
    RESOLVE(eglGetDisplay);
    RESOLVE(eglInitialize);
    RESOLVE(eglTerminate);
    RESOLVE(eglChooseConfig);
    RESOLVE(eglGetConfigAttrib);
    RESOLVE(eglQueryContext);
    RESOLVE(eglCreateWindowSurface);
    RESOLVE(eglCreatePbufferSurface);
    RESOLVE(eglDestroySurface);
    RESOLVE(eglBindAPI);
    RESOLVE(eglSwapInterval);
    RESOLVE(eglCreateContext);
    RESOLVE(eglDestroyContext);
    RESOLVE(eglMakeCurrent);
    RESOLVE(eglGetCurrentContext);
    RESOLVE(eglGetCurrentSurface);
    RESOLVE(eglGetCurrentDisplay);
    RESOLVE(eglSwapBuffers);
    RESOLVE(eglQueryString);
    RESOLVE(eglWaitNative);
    RESOLVE(eglSurfaceAttrib);
    RESOLVE(eglGetProcAddress);

    if (!eglGetError || !eglGetDisplay || !eglInitialize || !eglGetProcAddress || !eglQueryString)
        return false;

    eglGetPlatformDisplayEXT = nullptr;
    eglDebugMessageControlKHR = nullptr;

#ifdef EGL_ANGLE_platform_angle

    eglGetPlatformDisplayEXT = reinterpret_cast<decltype(eglGetPlatformDisplayEXT)>(
            eglGetProcAddress("eglGetPlatformDisplayEXT"));
    RESOLVE(eglDebugMessageControlKHR);
#endif

    return true;
}

void *QWindowsLibGLESv2::resolve(const char *name)
{
    return m_lib ? resolveFunc(m_lib, name) : nullptr;
}

bool QWindowsLibGLESv2::init()
{
    static constexpr LPCWSTR dllName{ L"libGLESv2" };

    qCDebug(lcQpaGl) << "Qt: Using OpenGL ES 2.0 from" << dllName;

    m_lib = ::LoadLibraryW(dllName);
    if (!m_lib) {
        qErrnoWarning(int(GetLastError()), "Failed to load %s", dllName);
        return false;
    }

    void(APIENTRY * glBindTexture)(GLenum target, GLuint texture){ nullptr };
    GLuint(APIENTRY * glCreateShader)(GLenum type){ nullptr };
    void(APIENTRY * glClearDepthf)(GLclampf depth){ nullptr };
    RESOLVE(glBindTexture);
    RESOLVE(glCreateShader);
    RESOLVE(glClearDepthf);
    RESOLVE(glGetString);

    return glBindTexture && glCreateShader && glClearDepthf;
}

QWindowsEGLStaticContext::QWindowsEGLStaticContext(EGLDisplay display, bool isYUpInNDC)
    : m_display(display),
      m_hasSRGBColorSpaceSupport(false),
      m_hasSCRGBColorSpaceSupport(false),
      m_hasBt2020PQColorSpaceSupport(false),
      m_hasPixelFormatFloatSupport(false),
      m_isYUpInNDC(isYUpInNDC),
      m_manuallyUpdateSurfaceSize(false)
{
    m_hasSRGBColorSpaceSupport = q_hasEglExtension(display, "EGL_KHR_gl_colorspace", eglConfigFunctions.get());
    m_hasSCRGBColorSpaceSupport = q_hasEglExtension(display, "EGL_EXT_gl_colorspace_scrgb_linear", eglConfigFunctions.get());
    m_hasBt2020PQColorSpaceSupport = q_hasEglExtension(display, "EGL_EXT_gl_colorspace_bt2020_pq", eglConfigFunctions.get());
    m_hasPixelFormatFloatSupport = q_hasEglExtension(display, "EGL_EXT_pixel_format_float", eglConfigFunctions.get());
    if (m_hasSCRGBColorSpaceSupport && !m_hasPixelFormatFloatSupport) {
        qWarning("%s: EGL_EXT_gl_colorspace_scrgb_linear supported but EGL_EXT_pixel_format_float "
                 "not available!", __FUNCTION__);
        m_hasSCRGBColorSpaceSupport = false;
    }
#ifdef EGL_ANGLE_platform_angle
    m_manuallyUpdateSurfaceSize = qEnvironmentVariableIntValue("QT_ANGLE_MANUALLY_UPDATE_SURFACE_SIZE");
    qInfo() << "INFO: manually update surface size:" << m_manuallyUpdateSurfaceSize;
#endif
}

bool QWindowsEGLStaticContext::initializeAngle(QWindowsOpenGLTester::Renderers preferredType,
                                               HDC dc, EGLDisplay *display, EGLint *major,
                                               EGLint *minor, QWindowsOpenGLTester::Renderer *resultRenderer)
{
#ifdef EGL_ANGLE_platform_angle
    if (libEGL.eglGetPlatformDisplayEXT
        && (preferredType & QWindowsOpenGLTester::AngleBackendMask)) {
        static constexpr std::array<std::array<EGLint, 8>, 5> anglePlatformAttributes{ {
                { EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE, EGL_NONE },
                { EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_D3D9_ANGLE, EGL_NONE },
                { EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
                  EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE,
                  EGL_PLATFORM_ANGLE_DEVICE_TYPE_D3D_WARP_ANGLE, EGL_NONE },
                { EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
                  EGL_PLATFORM_ANGLE_D3D11ON12_ANGLE, EGL_TRUE, EGL_NONE },
                { EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_OPENGL_ANGLE, EGL_NONE },
        } };
        const EGLint *attributes = nullptr;
        if (preferredType & QWindowsOpenGLTester::AngleRendererD3d11) {
            attributes = anglePlatformAttributes[0].data();
            *resultRenderer = QWindowsOpenGLTester::AngleRendererD3d11;
        } else if (preferredType & QWindowsOpenGLTester::AngleRendererD3d9) {
            attributes = anglePlatformAttributes[1].data();
            *resultRenderer = QWindowsOpenGLTester::AngleRendererD3d9;
        } else if (preferredType & QWindowsOpenGLTester::AngleRendererD3d11Warp) {
            attributes = anglePlatformAttributes[2].data();
            *resultRenderer = QWindowsOpenGLTester::AngleRendererD3d11Warp;
        } else if (preferredType & QWindowsOpenGLTester::AngleRendererD3d11On12) {
            if (IsWindows10OrGreater()) {
                attributes = anglePlatformAttributes[3].data();
                *resultRenderer = QWindowsOpenGLTester::AngleRendererD3d11On12;
            } else {
                qWarning("%s: Attempted to use D3d11on12 in an unsupported version of windows. "
                         "Retargeting for D3d11Warp",
                         __FUNCTION__);
                attributes = anglePlatformAttributes[2].data();
                *resultRenderer = QWindowsOpenGLTester::AngleRendererD3d11Warp;
            }
        } else if (preferredType & QWindowsOpenGLTester::AngleRendererOpenGL) {
            attributes = anglePlatformAttributes[4].data();
            *resultRenderer = QWindowsOpenGLTester::AngleRendererOpenGL;
        }
        if (attributes) {
#  ifdef EGL_ANGLE_platform_angle
            {
                EGLint result =
                        libEGL.eglDebugMessageControlKHR(&angleDebugMessagesCallback, nullptr);
                if (result != EGL_SUCCESS) {
                    qWarning() << "WARNING: failed to install ANGLE debug handler, error:"
                               << Qt::hex << Qt::showbase << result;
                }
            }
#  endif

            *display = libEGL.eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE, dc, attributes);
            if (!libEGL.eglInitialize(*display, major, minor)) {
                qWarning("%s: Unable to initialize ANGLE: error 0x%x", __FUNCTION__,
                         libEGL.eglGetError());
                libEGL.eglTerminate(*display);
                *display = EGL_NO_DISPLAY;
                *major = *minor = 0;
                return false;
            }
        }
    }
#else // EGL_ANGLE_platform_angle
#   warning "Building QWindowsEGLStaticContext without ANGLE is not officially supported"
    Q_UNUSED(preferredType);
    Q_UNUSED(dc);
    Q_UNUSED(display);
    Q_UNUSED(major);
    Q_UNUSED(minor);
    Q_UNUSED(resultRenderer);
#endif
    return true;
}

QWindowsEGLStaticContext *
QWindowsEGLStaticContext::create(QWindowsOpenGLTester::Renderers preferredType)
{
    const HDC dc{ QWindowsContext::instance()->displayContext() };
    if (!dc) {
        qWarning("%s: No Display", __FUNCTION__);
        return nullptr;
    }

    if (!libEGL.init()) {
        qWarning("%s: Failed to load and resolve libEGL functions", __FUNCTION__);
        return nullptr;
    }
    if (!libGLESv2.init()) {
        qWarning("%s: Failed to load and resolve libGLESv2 functions", __FUNCTION__);
        return nullptr;
    }
    if (!eglConfigFunctions) {
        eglConfigFunctions.reset(new IndirectFunctions(&libEGL));
    }

    EGLDisplay display{ EGL_NO_DISPLAY };
    EGLint major{ 0 };
    EGLint minor{ 0 };

    QWindowsOpenGLTester::Renderer resultRenderer = QWindowsOpenGLTester::InvalidRenderer;

    if (!initializeAngle(preferredType, dc, &display, &major, &minor, &resultRenderer)
        && (preferredType & QWindowsOpenGLTester::AngleRendererD3d11)) {
        preferredType &= ~QWindowsOpenGLTester::AngleRendererD3d11;
        initializeAngle(preferredType, dc, &display, &major, &minor, &resultRenderer);
    }

    if (display == EGL_NO_DISPLAY)
        display = libEGL.eglGetDisplay(dc);
    if (!display) {
        qWarning("%s: Could not obtain EGL display", __FUNCTION__);
        return nullptr;
    }

    if (!major && !libEGL.eglInitialize(display, &major, &minor)) {
        const auto err{ libEGL.eglGetError() };
        qWarning("%s: Could not initialize EGL display: error 0x%x", __FUNCTION__, err);
        if (err == EGL_NOT_INITIALIZED)
            qWarning("%s: When using ANGLE, check if d3dcompiler_4x.dll is available",
                     __FUNCTION__);
        return nullptr;
    }

    qCDebug(lcQpaGl) << __FUNCTION__ << "Created EGL display" << display << 'v' << major << '.'
                     << minor;
    
    /**
     * When openGL backend is activated in ANGLE, then Y axis is flipped. Hence we should
     * notify all the users about it.
     */
    const bool isYUpInNDC = resultRenderer != QWindowsOpenGLTester::AngleRendererOpenGL;
    return new QWindowsEGLStaticContext(display, isYUpInNDC);
}

QWindowsEGLStaticContext::~QWindowsEGLStaticContext()
{
    qCDebug(lcQpaGl) << __FUNCTION__ << "Releasing EGL display " << m_display;
    libEGL.eglTerminate(m_display);

#ifdef EGL_ANGLE_platform_angle
    {
        EGLint result = libEGL.eglDebugMessageControlKHR(nullptr, nullptr);
        if (result != EGL_SUCCESS) {
            qWarning() << "WARNING: failed to de-install ANGLE debug handler, error:" << Qt::hex
                       << Qt::showbase << result;
        }
    }
#endif
}

QWindowsOpenGLContext *QWindowsEGLStaticContext::createContext(QOpenGLContext *context)
{
    return new QWindowsEGLContext(this, context->format(), context->shareHandle());
}

QWindowsOpenGLContext *QWindowsEGLStaticContext::createContext(EGLContext context, EGLDisplay display, QOpenGLContext *shareContext)
{
    return new QWindowsEGLContext(this, context, display, shareContext);
}

void *QWindowsEGLStaticContext::createWindowSurface(void *nativeWindow, void *nativeConfig,
                                                    const QColorSpace &colorSpace, const QSize &size, int *err)
{
    *err = 0;

    EGLint eglColorSpace{ EGL_GL_COLORSPACE_LINEAR_KHR };
    bool colorSpaceSupported{ colorSpace.isValid() };

    if (colorSpace == QColorSpace::SRgb) {
        colorSpaceSupported = m_hasSRGBColorSpaceSupport;
        eglColorSpace = EGL_GL_COLORSPACE_SRGB_KHR;
    } else if (colorSpace == QColorSpace::SRgbLinear) {
        colorSpaceSupported = m_hasSCRGBColorSpaceSupport;
        eglColorSpace = EGL_GL_COLORSPACE_SCRGB_LINEAR_EXT;
    } else if (colorSpace == QColorSpace::Bt2100Pq) {
        colorSpaceSupported = m_hasBt2020PQColorSpaceSupport;
        eglColorSpace = EGL_GL_COLORSPACE_BT2020_PQ_EXT;
    }

    std::vector<EGLint> attributes;

    if (colorSpaceSupported) {
        attributes.emplace_back(EGL_GL_COLORSPACE);
        attributes.emplace_back(eglColorSpace);
    }

    if (!m_isYUpInNDC) {
        attributes.emplace_back(EGL_SURFACE_ORIENTATION_ANGLE);
        attributes.emplace_back(EGL_SURFACE_ORIENTATION_INVERT_Y_ANGLE);
    }

#ifdef EGL_ANGLE_platform_angle
    if (m_manuallyUpdateSurfaceSize) {
        attributes.emplace_back(EGL_FIXED_SIZE_ANGLE);
        attributes.emplace_back(EGL_TRUE);
        attributes.emplace_back(EGL_WIDTH);
        attributes.emplace_back(size.width());
        attributes.emplace_back(EGL_HEIGHT);
        attributes.emplace_back(size.height());
    }
#endif

    attributes.emplace_back(EGL_NONE);

    if (!colorSpaceSupported && colorSpace.isValid())
        qWarning("%s: Requested color space is not supported by EGL implementation: %s %s (egl: 0x%x)",
                 __FUNCTION__,
                 QMetaEnum::fromType<QColorSpace::Primaries>().valueToKey(int(colorSpace.primaries())),
                 QMetaEnum::fromType<QColorSpace::TransferFunction>().valueToKey(int(colorSpace.transferFunction())),
                 eglColorSpace);

    EGLSurface surface{ libEGL.eglCreateWindowSurface(
            m_display, nativeConfig, static_cast<EGLNativeWindowType>(nativeWindow),
            attributes.data()) };
    if (surface == EGL_NO_SURFACE) {
        *err = libEGL.eglGetError();
        qWarning("%s: Could not create the EGL window surface: 0x%x", __FUNCTION__, *err);
    }

    return surface;
}

void QWindowsEGLStaticContext::destroyWindowSurface(void *nativeSurface)
{
    libEGL.eglDestroySurface(m_display, nativeSurface);
}

void QWindowsEGLStaticContext::updateWindowSurfaceSize(void * nativeSurface, const QSize & size)
{
#ifdef EGL_ANGLE_platform_angle
    if (m_manuallyUpdateSurfaceSize) {
        libEGL.eglSurfaceAttrib(m_display, nativeSurface, EGL_WIDTH, size.width());
        libEGL.eglSurfaceAttrib(m_display, nativeSurface, EGL_HEIGHT, size.height());
    }
#endif
}

/*!
    \class QWindowsEGLContext
    \brief Open EGL context.

    \section1 Using QWindowsEGLContext for Desktop with ANGLE
    \section2 Build Instructions
    \list
    \o Install the Direct X SDK
    \o Checkout and build ANGLE (SVN repository) as explained here:
       \l{https://chromium.googlesource.com/angle/angle/+/master/README.md}
       When building for 64bit, de-activate the "WarnAsError" option
       in every project file (as otherwise integer conversion
       warnings will break the build).
    \o Run configure.exe with the options "-opengl es2".
    \o Build qtbase and test some examples.
    \endlist

    \internal
*/
QWindowsEGLContext::QWindowsEGLContext(QWindowsEGLStaticContext *staticContext,
                                       const QSurfaceFormat &format, QPlatformOpenGLContext *share)
    : m_staticContext(staticContext), m_eglDisplay(staticContext->display())
{
    if (!m_staticContext)
        return;

    m_eglConfig = q_configFromGLFormat(m_eglDisplay, format, false, EGL_WINDOW_BIT, m_staticContext->eglConfigFunctions.get());
    m_format = q_glFormatFromConfig(m_eglDisplay, m_eglConfig, format, m_staticContext->eglConfigFunctions.get());
    m_shareContext = [&]() -> EGLContext {
        if (!share)
            return nullptr;
        if (const auto realShare = dynamic_cast<QWindowsEGLContext *>(share))
            return realShare->m_eglContext;
        return nullptr;
    }();

    const EGLint major{ m_format.majorVersion() };
    const EGLint minor{ m_format.minorVersion() };
    if (major > 3 || (major == 3 && minor > 0))
        qWarning("QWindowsEGLContext: ANGLE only partially supports OpenGL ES > 3.0");
    const std::array<EGLint, 5> contextAttrs{
        EGL_CONTEXT_MAJOR_VERSION, major, EGL_CONTEXT_MINOR_VERSION, minor, EGL_NONE,
    };

    QWindowsEGLStaticContext::libEGL.eglBindAPI(m_api);
    m_eglContext = QWindowsEGLStaticContext::libEGL.eglCreateContext(
            m_eglDisplay, m_eglConfig, m_shareContext, contextAttrs.data());
    if (m_eglContext == EGL_NO_CONTEXT && m_shareContext != EGL_NO_CONTEXT) {
        m_shareContext = nullptr;
        m_eglContext = QWindowsEGLStaticContext::libEGL.eglCreateContext(
                m_eglDisplay, m_eglConfig, nullptr, contextAttrs.data());
    }

    if (m_eglContext == EGL_NO_CONTEXT) {
        const auto err{ QWindowsEGLStaticContext::libEGL.eglGetError() };
        qWarning("QWindowsEGLContext: Failed to create context, eglError: %x, this: %p", err, this);
        // ANGLE gives bad alloc when it fails to reset a previously lost D3D device.
        // A common cause for this is disabling the graphics adapter used by the app.
        if (err == EGL_BAD_ALLOC)
            qWarning("QWindowsEGLContext: Graphics device lost. (Did the adapter get disabled?)");
        return;
    }

    // Make the context current to ensure the GL version query works. This needs a surface too.
    static constexpr std::array<EGLint, 7> pbufferAttributes{
        EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_LARGEST_PBUFFER, EGL_FALSE, EGL_NONE
    };
    EGLSurface pbuffer{ QWindowsEGLStaticContext::libEGL.eglCreatePbufferSurface(
            m_eglDisplay, m_eglConfig, pbufferAttributes.data()) };
    if (pbuffer == EGL_NO_SURFACE)
        return;

    EGLDisplay prevDisplay{ QWindowsEGLStaticContext::libEGL.eglGetCurrentDisplay() };
    if (prevDisplay == EGL_NO_DISPLAY) // when no context is current
        prevDisplay = m_eglDisplay;
    EGLContext prevContext{ QWindowsEGLStaticContext::libEGL.eglGetCurrentContext() };
    EGLSurface prevSurfaceDraw{ QWindowsEGLStaticContext::libEGL.eglGetCurrentSurface(EGL_DRAW) };
    EGLSurface prevSurfaceRead{ QWindowsEGLStaticContext::libEGL.eglGetCurrentSurface(EGL_READ) };

    if (QWindowsEGLStaticContext::libEGL.eglMakeCurrent(m_eglDisplay, pbuffer, pbuffer,
                                                        m_eglContext)) {
        const GLubyte *s{ QWindowsEGLStaticContext::libGLESv2.glGetString(GL_VERSION) };
        if (s) {
            const QByteArray version(reinterpret_cast<const char *>(s));
            int major{};
            int minor{};
            if (QPlatformOpenGLContext::parseOpenGLVersion(version, major, minor)) {
                m_format.setMajorVersion(major);
                m_format.setMinorVersion(minor);
            }
        }
        m_format.setProfile(QSurfaceFormat::NoProfile);
        m_format.setOptions(QSurfaceFormat::FormatOptions());
        QWindowsEGLStaticContext::libEGL.eglMakeCurrent(prevDisplay, prevSurfaceDraw,
                                                        prevSurfaceRead, prevContext);
    }
    QWindowsEGLStaticContext::libEGL.eglDestroySurface(m_eglDisplay, pbuffer);
}

QWindowsEGLContext::QWindowsEGLContext(QWindowsEGLStaticContext *staticContext, 
                                       EGLContext context, EGLDisplay display, QOpenGLContext *shareContext)
    : m_staticContext(staticContext), m_eglDisplay(display)
{
    if (!m_staticContext)
        return;

    m_eglContext = context;

    m_shareContext = [&]() -> EGLContext {
        if (!shareContext)
            return nullptr;
        if (const auto realShare = dynamic_cast<QWindowsEGLContext *>(shareContext))
            return realShare->m_eglContext;
        return nullptr;
    }();

    QWindowsEGLStaticContext::libEGL.eglBindAPI(m_api);

    // Figure out the EGLConfig.
    EGLint value = 0;
    if (!QWindowsEGLStaticContext::libEGL.eglQueryContext(m_eglDisplay, m_eglContext, EGL_CONFIG_ID, &value)) {
        qWarning("QWindowsEGLContext: failed to query framebuffer configuration ID from a native context");
        m_eglContext = EGL_NO_CONTEXT;
        return;
    }

    EGLint n = 0;
    EGLConfig cfg;
    const EGLint attribs[] = { EGL_CONFIG_ID, value, EGL_NONE };
    if (QWindowsEGLStaticContext::libEGL.eglChooseConfig(m_eglDisplay, attribs, &cfg, 1, &n) && n == 1) {
        m_eglConfig = cfg;
        m_format = q_glFormatFromConfig(m_eglDisplay, m_eglConfig, {}, m_staticContext->eglConfigFunctions.get());
    } else {
        qWarning("QWindowsEGLContext: Failed to get framebuffer configuration for context");
        m_eglContext = EGL_NO_CONTEXT;
    }
}

QWindowsEGLContext::~QWindowsEGLContext()
{
    if (m_eglContext != EGL_NO_CONTEXT) {
        QWindowsEGLStaticContext::libEGL.eglDestroyContext(m_eglDisplay, m_eglContext);
        m_eglContext = EGL_NO_CONTEXT;
    }
}

bool QWindowsEGLContext::makeCurrent(QPlatformSurface *surface)
{
    Q_ASSERT(surface->surface()->supportsOpenGL());

    QWindowsEGLStaticContext::libEGL.eglBindAPI(m_api);

    auto *window{ dynamic_cast<QWindowsWindow *>(surface) };
    Q_ASSERT(window);
    int err{};
    auto eglSurface{ static_cast<EGLSurface>(window->surface(m_eglConfig, &err)) };
    if (eglSurface == EGL_NO_SURFACE) {
        if (err == EGL_CONTEXT_LOST) {
            m_eglContext = EGL_NO_CONTEXT;
            qCDebug(lcQpaGl) << "Got EGL context lost in createWindowSurface() for context" << this;
        } else if (err == EGL_BAD_ACCESS) {
            // With ANGLE this means no (D3D) device and can happen when disabling/changing graphics
            // adapters.
            qCDebug(lcQpaGl) << "Bad access (missing device?) in createWindowSurface() for context"
                             << this;
        } else if (err == EGL_BAD_MATCH) {
            qCDebug(lcQpaGl) << "Got bad match in createWindowSurface() for context" << this
                             << "Check color space configuration.";
        }
        // Simulate context loss as the context is useless.
        QWindowsEGLStaticContext::libEGL.eglDestroyContext(m_eglDisplay, m_eglContext);
        m_eglContext = EGL_NO_CONTEXT;
        return false;
    }

    // shortcut: on some GPUs, eglMakeCurrent is not a cheap operation
    if (QWindowsEGLStaticContext::libEGL.eglGetCurrentContext() == m_eglContext
        && QWindowsEGLStaticContext::libEGL.eglGetCurrentDisplay() == m_eglDisplay
        && QWindowsEGLStaticContext::libEGL.eglGetCurrentSurface(EGL_READ) == eglSurface
        && QWindowsEGLStaticContext::libEGL.eglGetCurrentSurface(EGL_DRAW) == eglSurface) {
        return true;
    }

    const auto ok{ QWindowsEGLStaticContext::libEGL.eglMakeCurrent(m_eglDisplay, eglSurface,
                                                                   eglSurface, m_eglContext) };
    if (ok) {
        const auto requestedSwapInterval{ surface->format().swapInterval() };
        if (requestedSwapInterval >= 0 && m_swapInterval != requestedSwapInterval) {
            m_swapInterval = requestedSwapInterval;
            QWindowsEGLStaticContext::libEGL.eglSwapInterval(m_staticContext->display(),
                                                             m_swapInterval);
        }
    } else {
        err = QWindowsEGLStaticContext::libEGL.eglGetError();
        // EGL_CONTEXT_LOST (loss of the D3D device) is not necessarily fatal.
        // Qt Quick is able to recover for example.
        if (err == EGL_CONTEXT_LOST) {
            m_eglContext = EGL_NO_CONTEXT;
            qCDebug(lcQpaGl) << "Got EGL context lost in makeCurrent() for context" << this;
            // Drop the surface. Will recreate on the next makeCurrent.
            window->invalidateSurface();
        } else {
            qWarning("%s: Failed to make surface current. eglError: %x, this: %p", __FUNCTION__,
                     err, this);
        }
    }

    return ok;
}

void QWindowsEGLContext::doneCurrent()
{
    QWindowsEGLStaticContext::libEGL.eglBindAPI(m_api);
    const auto ok{ QWindowsEGLStaticContext::libEGL.eglMakeCurrent(
            m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) };
    if (!ok)
        qWarning("%s: Failed to make no context/surface current. eglError: %d, this: %p",
                 __FUNCTION__, QWindowsEGLStaticContext::libEGL.eglGetError(), this);
}

void QWindowsEGLContext::swapBuffers(QPlatformSurface *surface)
{
    QWindowsEGLStaticContext::libEGL.eglBindAPI(m_api);
    auto *window{ dynamic_cast<QWindowsWindow *>(surface) };
    Q_ASSERT(window);
    int err{};
    auto eglSurface{ static_cast<EGLSurface>(window->surface(m_eglConfig, &err)) };
    if (eglSurface == EGL_NO_SURFACE) {
        if (err == EGL_CONTEXT_LOST) {
            m_eglContext = EGL_NO_CONTEXT;
            qCDebug(lcQpaGl) << "Got EGL context lost in createWindowSurface() for context" << this;
        }
        return;
    }

    const auto ok{ QWindowsEGLStaticContext::libEGL.eglSwapBuffers(m_eglDisplay, eglSurface) };
    if (!ok) {
        err = QWindowsEGLStaticContext::libEGL.eglGetError();
        if (err == EGL_CONTEXT_LOST) {
            m_eglContext = EGL_NO_CONTEXT;
            qCDebug(lcQpaGl) << "Got EGL context lost in eglSwapBuffers()";
        } else {
            qWarning("%s: Failed to swap buffers. eglError: %d, this: %p", __FUNCTION__, err, this);
        }
    }
}

QFunctionPointer QWindowsEGLContext::getProcAddress(const char *procName)
{
    QWindowsEGLStaticContext::libEGL.eglBindAPI(m_api);

    QFunctionPointer procAddress{ nullptr };

    // Special logic for ANGLE extensions for blitFramebuffer and
    // renderbufferStorageMultisample. In version 2 contexts the extensions
    // must be used instead of the suffixless, version 3.0 functions.
    if (m_format.majorVersion() < 3) {
        std::string_view procNameView{ procName };
        if (procNameView == "glBlitFramebuffer"sv
            || procNameView == "glRenderbufferStorageMultisample"sv) {
            std::string extName{ procNameView };
            extName += "ANGLE"sv;
            procAddress = reinterpret_cast<QFunctionPointer>(
                    QWindowsEGLStaticContext::libEGL.eglGetProcAddress(procNameView.data()));
        }
    }

    if (!procAddress)
        procAddress = reinterpret_cast<QFunctionPointer>(
                QWindowsEGLStaticContext::libEGL.eglGetProcAddress(procName));

    // We support AllGLFunctionsQueryable, which means this function must be able to
    // return a function pointer for standard GLES2 functions too. These are not
    // guaranteed to be queryable via eglGetProcAddress().
    if (!procAddress)
        procAddress = reinterpret_cast<QFunctionPointer>(
                QWindowsEGLStaticContext::libGLESv2.resolve(procName));

    if (QWindowsContext::verbose > 1)
        qCDebug(lcQpaGl) << __FUNCTION__ << procName
                         << QWindowsEGLStaticContext::libEGL.eglGetCurrentContext() << "returns"
                         << reinterpret_cast<void *>(procAddress);

    return procAddress;
}

void QWindowsEGLContext::beginFrame()
{
    // The D3D backend checks for buffer resizes inside eglWaitNative, so we should
    // call this function before every frame to avoid window flicker
    EGLBoolean result = QWindowsEGLStaticContext::libEGL.eglWaitNative(EGL_CORE_NATIVE_ENGINE);
    if (result == EGL_FALSE) {
        qCWarning(lcQpaGl, "QWindowsEGLContext::beforeCompose: eglWaitNative failed");
    }
}

bool QWindowsEGLContext::isYUpInNDC() const
{
    return m_staticContext->isYUpInNDC();
}

QT_END_NAMESPACE
