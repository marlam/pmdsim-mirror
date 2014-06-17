/*
 * Copyright (C) 2012, 2013, 2014
 * Computer Graphics Group, University of Siegen, Germany.
 * http://www.cg.informatik.uni-siegen.de/
 * All rights reserved.
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>.
 */

#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <cstdio>

#include <GL/glew.h>

#include <QGLWidget>

#include "simulator.h"


class GLWidget : public QGLWidget
{
    Q_OBJECT

private:
    GLEWContext _glewctx;

protected:
    Simulator _simulator;

    GLEWContext* glewGetContext()
    {
        return &_glewctx;
    }

public:
    GLWidget(QGLWidget* sharing_widget);

public:
    // Disable Qt GL handling
    virtual void initializeGL() {}
    virtual void paintGL() {}
    virtual void resizeGL(int, int) {}

public slots:
    void update_simulator(const Simulator&);

protected:
    #include "glhelper.inl"
};

#endif
