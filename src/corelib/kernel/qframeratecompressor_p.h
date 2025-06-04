/*
 *  SPDX-FileCopyrightText: 2025 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef QFRAMERATECOMPRESSOR_H
#define QFRAMERATECOMPRESSOR_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Qt translation tools.  This header file may change from version
// to version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>
#include <QtCore/qobject.h>
#include <QtCore/qelapsedtimer.h>

class QTimer;

#ifndef QT_NO_QOBJECT

QT_BEGIN_NAMESPACE

/**
 * QFrameRateCompressor: Like KisSignalCompressor in FIRST_ACTIVE mode, but emits
 * timeout() only on timer ticks (or on the very first invocation). It guarantees
 * that the framerate is always stable
 */
class Q_CORE_EXPORT QFrameRateCompressor : public QObject
{
    Q_OBJECT

public:
    explicit QFrameRateCompressor(QObject *parent = nullptr);
    QFrameRateCompressor(int delay, QObject *parent = nullptr);
    ~QFrameRateCompressor() override;

    bool isActive() const;
    int delay() const;

public Q_SLOTS:
    void setDelay(int delay);
    void start();
    void stop();

private Q_SLOTS:
    void slotTimerExpired();

Q_SIGNALS:
    void timeout();

private:
    QTimer *m_timer = nullptr;
    bool m_signalsPending = false;
    int m_numTicksWithoutEmission = 0;
};

QT_END_NAMESPACE

#endif // QT_NO_QOBJECT

#endif // QFRAMERATECOMPRESSOR_H
