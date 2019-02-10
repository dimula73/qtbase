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
