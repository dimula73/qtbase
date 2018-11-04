# custom tests

defineTest(qtConfLibrary_freetype) {
    TRY_INCLUDEPATHS = $$EXTRA_INCLUDEPATH $$QMAKE_INCDIR_X11
    haiku: TRY_INCLUDEPATHS += /system/develop/headers
    TRY_INCLUDEPATHS += $$QMAKE_DEFAULT_INCDIR
    for (p, TRY_INCLUDEPATHS) {
        includedir = $$p/freetype2
        exists($$includedir) {
            $${1}.includedir = "$$val_escape(includedir)"
            export($${1}.includedir)
            return(true)
        }
    }
    return(true)
}

# For MSVC everything DirectX related is included in Windows Kit >= 8,
# so we do not do any magic in this case.
# For MinGW we need the shader compiler (fxc.exe), which
# are not part of MinGW. They can either be obtained from a DirectX SDK
# (keep the old approach working) or Windows Kit (>= 8).
defineTest(qtConfTest_directX) {
    $$qtConfEvaluate("features.sse2") {
        ky = $$size($${1}.files._KEYS_)
        $${1}.files._KEYS_ += $$ky
        # Not present on MinGW-32
        $${1}.files.$${ky} = "intrin.h"
    }

    qtConfTest_files($${1}): return(true)
    return(false)
}

defineTest(qtConfTest_fxc) {
    !mingw {
        fxc = $$qtConfFindInPath("fxc.exe")
    } else {
        dxdir = $$getenv("DXSDK_DIR")
        winkitdir = $$getenv("WindowsSdkDir")
        !isEmpty(dxdir) {
            equals(QT_ARCH, x86_64): \
                fxc = $$dxdir/Utilities/bin/x64/fxc.exe
            else: \
                fxc = $$dxdir/Utilities/bin/x86/fxc.exe
        } else: !isEmpty(winkitdir) {
            equals(QT_ARCH, x86_64): \
                fxc = $$winkitdir/bin/x64/fxc.exe
            else: \
                fxc = $$winkitdir/bin/x86/fxc.exe

            !exists($$fxc) {
                binsubdirs = $$files($$winkitdir/bin/*)
                for (dir, binsubdirs) {
                    equals(QT_ARCH, x86_64): \
                        finalBinDir = $$dir/x64
                    else: \
                        finalBinDir = $$dir/x86

                    fxc = $${finalBinDir}/fxc.exe
                    exists($$fxc) {
                        break()
                    }
                }
            }
       }
    }

    !isEmpty(fxc):exists($$fxc) {
        $${1}.value = $$fxc
        export($${1}.value)
        $${1}.cache += value
        export($${1}.cache)
        return(true)
    }
    return(false)
}

defineTest(qtConfTest_qpaDefaultPlatform) {
    name =
    !isEmpty(config.input.qpa_default_platform): name = $$config.input.qpa_default_platform
    else: !isEmpty(QT_QPA_DEFAULT_PLATFORM): name = $$QT_QPA_DEFAULT_PLATFORM
    else: winrt: name = winrt
    else: win32: name = windows
    else: android: name = android
    else: macos: name = cocoa
    else: if(ios|tvos): name = ios
    else: watchos: name = minimal
    else: qnx: name = qnx
    else: integrity: name = integrityfb
    else: haiku: name = haiku
    else: name = xcb

    $${1}.value = $$name
    $${1}.plugin = q$$name
    $${1}.name = "\"$$name\""
    export($${1}.value)
    export($${1}.plugin)
    export($${1}.name)
    $${1}.cache += value plugin name
    export($${1}.cache)
    return(true)
}
