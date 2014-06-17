/*
 * Copyright (C) 2012, 2013, 2014
 * Computer Graphics Group, University of Siegen, Germany.
 * http://www.cg.informatik.uni-siegen.de/
 * All rights reserved.
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>.
 */

#ifndef VIEW2DWIDGET_H
#define VIEW2DWIDGET_H

#include "glwidget.h"


class View2DWidget : public GLWidget
{
    Q_OBJECT

private:
    GLuint _prg;

public:
    View2DWidget(QGLWidget* sharing_widget);

public:
    void view(GLuint tex, float ar = 1.0f, int channel = 0,
            float minval = 0.0f, float maxval = 1.0f,
            bool high_dynamic_range = false);

private:
    #include "simviewhelper.inl"
};

#endif
