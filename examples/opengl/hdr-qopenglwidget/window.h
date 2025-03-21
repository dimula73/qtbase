// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef WINDOW_H
#define WINDOW_H

#include <QMainWindow>

class QAction;

class GLWidget;
class KisGLImageWidget;
class KisGLImageF16;
class QLabel;

class Window : public QMainWindow
{
    Q_OBJECT

public:
    Window();

    void showEvent(QShowEvent *ev) override;

public Q_SLOTS:


private:
    KisGLImageF16 initializeImage(bool cropRange) const;
    QImage convertToQImage(const KisGLImageF16 &image) const;

    void updateSurfaceInfo();

private:
    GLWidget *m_glWidget {0};
    QAction *m_openAction {0};
    QAction *m_quitAction {0};
    KisGLImageWidget *m_imageWidget;
    KisGLImageWidget *m_imageWidgetSdr;
    QLabel *m_lblContextInfo;

};

#endif
