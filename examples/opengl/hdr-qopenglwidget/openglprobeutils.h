// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef OPENGLPROBEUTILS_H
#define OPENGLPROBEUTILS_H

#include <QSurfaceFormat>

namespace OpenGLProbeUtils
{

bool fuzzyCompareColorSpaces(const QColorSpace &lhs, const QColorSpace &rhs);
bool probeFormat(const QSurfaceFormat &format, bool adjustGlobalState);

};

#endif // OPENGLPROBEUTILS_H
