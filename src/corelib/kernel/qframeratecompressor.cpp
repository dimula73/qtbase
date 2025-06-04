/*
 *  SPDX-FileCopyrightText: 2025 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qframeratecompressor_p.h"

#include <QtCore/qtimer.h>
#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcFrameRateCompressor, "qt.core.frameratecompressor", QtWarningMsg);

QFrameRateCompressor::QFrameRateCompressor(QObject *parent)
    : QObject(parent),
      m_timer(new QTimer(this)),
      m_signalsPending(false)
{
    m_timer->setSingleShot(false);
    m_timer->setTimerType(Qt::PreciseTimer);
    connect(m_timer, &QTimer::timeout, this, &QFrameRateCompressor::slotTimerExpired);
}

QFrameRateCompressor::QFrameRateCompressor(int delay, QObject *parent)
    : QObject(parent),
      m_timer(new QTimer(this)),
      m_signalsPending(false)
{
    m_timer->setSingleShot(false);
    m_timer->setInterval(delay);
    m_timer->setTimerType(Qt::PreciseTimer);
    connect(m_timer, &QTimer::timeout, this, &QFrameRateCompressor::slotTimerExpired);
}

QFrameRateCompressor::~QFrameRateCompressor()
{
    if (m_timer->isActive()) {
        m_timer->stop();
    }
}

void QFrameRateCompressor::setDelay(int delay)
{
    if (m_timer->isActive()) {
        m_timer->stop();
        m_timer->setInterval(delay);
        m_timer->start();
    } else {
        m_timer->setInterval(delay);
    }
}

void QFrameRateCompressor::start()
{
    if (!m_timer->isActive()) {
        qCInfo(lcFrameRateCompressor)
                << this << "Starting framerate timer:" << m_timer->interval() << "ms";
        m_signalsPending = true;
        slotTimerExpired();
        m_timer->start();
    } else {
        m_signalsPending = true;
    }
}

void QFrameRateCompressor::stop()
{
    m_timer->stop();
    m_signalsPending = false;
}

bool QFrameRateCompressor::isActive() const
{
    return m_signalsPending && m_timer->isActive();
}

int QFrameRateCompressor::delay() const
{
    return m_timer->interval();
}

void QFrameRateCompressor::slotTimerExpired()
{
    if (m_signalsPending) {
        m_signalsPending = false;
        m_numTicksWithoutEmission = 0;
        Q_EMIT timeout();
    } else {
        m_numTicksWithoutEmission++;
        if (m_numTicksWithoutEmission > 10) {
            m_numTicksWithoutEmission = 0;
            qCInfo(lcFrameRateCompressor)
                    << this << "Stopping framerate timer:" << m_timer->interval() << "ms";
            m_timer->stop();
        }
    }
}

QT_END_NAMESPACE
