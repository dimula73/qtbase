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

#include "window.h"

#include "KisGLImageWidget.h"
#include "KisGLImageF16.h"

#include <cmath>

#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QAction>
#include <QDebug>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

#include <cmath>


Window::Window()
{
    setWindowTitle("16 bit float QOpenGLWidget test");
    QMenu *menu = menuBar()->addMenu("File");
    QToolBar *tb = addToolBar("File");

    m_quitAction = new QAction("Quit", this);
    connect(m_quitAction, SIGNAL(triggered(bool)), this, SLOT(close()));
    menu->addAction(m_quitAction);
    tb->addAction(m_quitAction);

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);

    QHBoxLayout *hLayout = new QHBoxLayout(centralWidget);

    m_imageWidget = new KisGLImageWidget(QSurfaceFormat::scRGBColorSpace, this);
    m_imageWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    hLayout->addWidget(m_imageWidget, 0, Qt::AlignLeft);

    m_imageWidgetSdr = new KisGLImageWidget(QSurfaceFormat::scRGBColorSpace, this);
    m_imageWidgetSdr->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    hLayout->addWidget(m_imageWidgetSdr, 0, Qt::AlignLeft);

    QImage image(QSize(255,255), QImage::Format_ARGB32);
    image.fill(Qt::red);

    QLabel *label = new QLabel(this);
    label->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    hLayout->addWidget(label, 0, Qt::AlignLeft);

    m_imageWidget->loadImage(initializeImage(false));
    m_imageWidgetSdr->loadImage(initializeImage(true));
    label->setPixmap(QPixmap::fromImage(convertToQImage(m_imageWidget->image())));

    layout->addLayout(hLayout);

    m_lblContextInfo = new QLabel(this);
    layout->addWidget(m_lblContextInfo);

    QLabel *lblNotes = new QLabel(this);
    lblNotes->setWordWrap(true);
    lblNotes->setText("* In SDR display mode all three images should look exactly the same\n"
                      "* In HDR display mode: image 1 should look brighter than the others "
                      "(it is HDR), images 2 and 3 should have exactly the same brightness and look");

    layout->addWidget(lblNotes);

    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);
}

inline qfloat16 floatToFloat16(float x) {
    qfloat16 result;
    qFloatToFloat16(&result, &x, 1);
    return result;
}

inline float float16ToFloat(qfloat16 x) {
    float result;
    qFloatFromFloat16(&result, &x, 1);
    return result;
}


KisGLImageF16 Window::initializeImage(bool cropRange) const
{
    const int size = 256;
    KisGLImageF16 image(size, size);
    image.clearPixels();
    qfloat16 *pixelPtr = image.data();

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            qfloat16 *pxl = reinterpret_cast<qfloat16*>(pixelPtr);

            float hdrRedValue = 25.0f * std::pow(float(x) / size, 5.0f);

            if (cropRange) {
                hdrRedValue = qMin(hdrRedValue, 1.0f);
            }

            pxl[0] = floatToFloat16(hdrRedValue);

            if (y > size / 2) {
                const float portion = (float(y) / size - 0.5f) * 2.0f;
                const float value = qMin(1.0f, 0.2f + 1.8f * portion);

                pxl[1] = floatToFloat16(value);
                pxl[2] = floatToFloat16(value);
            } else {
                pxl[1] = floatToFloat16(0.2);
                pxl[2] = floatToFloat16(0.2);
            }

            pxl[3] = floatToFloat16(1.0);

            pixelPtr += 4;
        }
    }

    return image;
}

inline float linearToSRGB(float value)
{
    if (value <= 0.0f) {
        value = 0.0f;
    } else if (value < 0.0031308f) {
        value = value * 12.92f;
    } else if (value < 1.0f) {
        value = std::pow(value, 0.41666f) * 1.055f - 0.055f;
    } else {
        value = 1.0f;
    }
    return value;
}

QImage Window::convertToQImage(const KisGLImageF16 &image) const
{
    const QSize size = image.size();
    const qfloat16 *pixelPtr = image.constData();

    QImage qimage(size, QImage::Format_ARGB32);
    quint8 *qimagePixelPtr = qimage.bits();


    for (int y = 0; y < size.height(); y++) {
        for (int x = 0; x < size.width(); x++) {
            const qfloat16 *srcPxl = pixelPtr;
            quint8 *dstPxl = qimagePixelPtr;

            auto convertChannel = [] (qfloat16 x) {
                float value = float16ToFloat(x);
                return quint8(linearToSRGB(value) * 255.0f);
            };

            dstPxl[0] =  convertChannel(srcPxl[2]);
            dstPxl[1] =  convertChannel(srcPxl[1]);
            dstPxl[2] =  convertChannel(srcPxl[0]);
            dstPxl[3] =  convertChannel(srcPxl[3]);

            pixelPtr += 4;
            qimagePixelPtr += 4;
        }
    }

    return qimage;
}

void Window::updateSurfaceInfo()
{
    const QSurfaceFormat format = m_imageWidget->context()->format();

    m_lblContextInfo->setText(
        QString("renderer: %1\ncolorSpace: %2\n\n")
                .arg(format.renderableType() == QSurfaceFormat::OpenGL ? "openGL" : "openGL ES")
                .arg(format.colorSpace() == QSurfaceFormat::sRGBColorSpace ? "sRGB" :
                     format.colorSpace() == QSurfaceFormat::scRGBColorSpace ? "scRGB" :
                     format.colorSpace() == QSurfaceFormat::bt2020PQColorSpace ? "Bt. 2020 PQ" :
                     "unknown"));
}

void Window::showEvent(QShowEvent *ev)
{
    QMainWindow::showEvent(ev);

    if (m_lblContextInfo->text().isEmpty()) {
        updateSurfaceInfo();
    }
}
