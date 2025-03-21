// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef KISGLIMAGEF16_H
#define KISGLIMAGEF16_H

#include <QSharedDataPointer>
#include <QFloat16>

class QSize;

class KisGLImageF16
{
public:
    KisGLImageF16();
    KisGLImageF16(const QSize &size, bool clearPixels = false);
    KisGLImageF16(int width, int height, bool clearPixels = false);
    KisGLImageF16(const KisGLImageF16 &rhs);
    KisGLImageF16& operator=(const KisGLImageF16 &rhs);

    friend bool operator==(const KisGLImageF16 &lhs, const KisGLImageF16 &rhs);
    friend bool operator!=(const KisGLImageF16 &lhs, const KisGLImageF16 &rhs);

    ~KisGLImageF16();

    void clearPixels();
    void resize(const QSize &size, bool clearPixels = false);

    const qfloat16* constData() const;
    qfloat16* data();

    QSize size() const;
    int width() const;
    int height() const;

    bool isNull() const;

private:
    struct Private;
    QSharedDataPointer<Private> m_d;
};

#endif // KISGLIMAGEF16_H
