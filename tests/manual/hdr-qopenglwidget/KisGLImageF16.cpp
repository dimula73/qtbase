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

#include "KisGLImageF16.h"

#include <QByteArray>
#include <QSize>

struct KisGLImageF16::Private : public QSharedData
{
    QSize size;
    QByteArray data;
};

KisGLImageF16::KisGLImageF16()
    : m_d(new Private)
{
}

KisGLImageF16::KisGLImageF16(const QSize &size, bool clearPixels)
    : m_d(new Private)
{
    resize(size, clearPixels);
}

KisGLImageF16::KisGLImageF16(int width, int height, bool clearPixels)
    : KisGLImageF16(QSize(width, height), clearPixels)
{
}

KisGLImageF16::KisGLImageF16(const KisGLImageF16 &rhs)
    : m_d(rhs.m_d)
{
}

KisGLImageF16 &KisGLImageF16::operator=(const KisGLImageF16 &rhs)
{
    m_d = rhs.m_d;
}

bool operator==(const KisGLImageF16 &lhs, const KisGLImageF16 &rhs)
{
    return lhs.m_d == rhs.m_d;
}

bool operator!=(const KisGLImageF16 &lhs, const KisGLImageF16 &rhs)
{
    return !(lhs == rhs);
}

KisGLImageF16::~KisGLImageF16()
{
}

void KisGLImageF16::clearPixels()
{
    if (!m_d->data.isEmpty()) {
        m_d->data.fill(0);
    }
}

void KisGLImageF16::resize(const QSize &size, bool clearPixels)
{
    const int pixelSize = 2 * 4;

    m_d->size = size;
    m_d->data.resize(size.width() * size.height() * pixelSize);

    if (clearPixels) {
        m_d->data.fill(0);
    }
}

const qfloat16 *KisGLImageF16::constData() const
{
    Q_ASSERT(!m_d->data.isNull());
    return reinterpret_cast<const qfloat16*>(m_d->data.data());
}

qfloat16 *KisGLImageF16::data()
{
    m_d->data.detach();
    Q_ASSERT(!m_d->data.isNull());

    return reinterpret_cast<qfloat16*>(m_d->data.data());
}

QSize KisGLImageF16::size() const
{
    return m_d->size;
}

int KisGLImageF16::width() const
{
    return m_d->size.width();
}

int KisGLImageF16::height() const
{
    return m_d->size.height();
}

bool KisGLImageF16::isNull() const
{
    return m_d->data.isNull();
}
