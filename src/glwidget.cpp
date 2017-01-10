/*
 * Copyright (C) 2012, 2013, 2014, 2017
 * Computer Graphics Group, University of Siegen, Germany.
 * http://www.cg.informatik.uni-siegen.de/
 * All rights reserved.
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>.
 */

#include <cstdlib>

#include <GL/glew.h>

#include <QMessageBox>

#include "glwidget.h"


GLWidget::GLWidget(QGLWidget* sharing_widget) : QGLWidget(NULL, sharing_widget)
{
    setAutoBufferSwap(false);
    setMinimumSize(32, 32);
    QGLWidget::updateGL();      // force initialization of the GL context
    if (!context()->isValid()) {
        QMessageBox::critical(this, "Error", "Cannot get valid OpenGL context.");
        std::exit(1);
    }
    makeCurrent();
    glewInit();
}

void GLWidget::update_simulator(const Simulator& simulator)
{
    _simulator = simulator;
}
