// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef KISGLIMAGEWIDGET_H
#define KISGLIMAGEWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QTransform>
#include "KisGLImageF16.h"


class KisGLImageWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    KisGLImageWidget(QWidget *parent = nullptr);
    KisGLImageWidget(const QColorSpace &colorSpace,
                     QWidget *parent = nullptr);

    void initializeGL() override;
    void paintGL() override;

    void loadImage(const KisGLImageF16 &image);

    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    QSize sizeHint() const override;

    KisGLImageF16 image() const;

public Q_SLOTS:

private:
    void updateVerticesBuffer(const QRect &rect);

private:
    KisGLImageF16 m_sourceImage;

    QOpenGLShaderProgram m_shader;
    QOpenGLVertexArrayObject m_vao;
    QOpenGLBuffer m_verticesBuffer;
    QOpenGLBuffer m_textureVerticesBuffer;
    QOpenGLTexture m_texture;
};

#endif // KISGLIMAGEWIDGET_H
