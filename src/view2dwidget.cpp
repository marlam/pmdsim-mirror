/*
 * Copyright (C) 2012, 2013, 2014
 * Computer Graphics Group, University of Siegen, Germany.
 * http://www.cg.informatik.uni-siegen.de/
 * All rights reserved.
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>.
 */

#include <GL/glew.h>

#include "view2dwidget.h"

#include "view2d.fs.glsl.h"


View2DWidget::View2DWidget(QGLWidget* sharing_widget) : GLWidget(sharing_widget), _prg(0)
{
}

void View2DWidget::view(GLuint tex, float ar, int channel, float minval, float maxval, bool high_dynamic_range)
{
    makeCurrent();

    if (_prg == 0) {
        GLuint fshader = xglCompileShader(GL_FRAGMENT_SHADER, VIEW2D_FS_GLSL_STR, XGL_HERE);
        _prg = xglCreateProgram(0, 0, fshader);
        xglLinkProgram(_prg);
    }

    int viewport[4];
    float clearcolor[3];
    get_viewport(ar, viewport, clearcolor);

    glViewport(0, 0, width(), height());
    glClearColor(clearcolor[0], clearcolor[1], clearcolor[2], 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

    glUseProgram(_prg);
    glUniform1i(glGetUniformLocation(_prg, "channel"), channel);
    glUniform1f(glGetUniformLocation(_prg, "minval"), minval);
    glUniform1f(glGetUniformLocation(_prg, "maxval"), maxval);
    glUniform1i(glGetUniformLocation(_prg, "dynamic_range_reduction"), high_dynamic_range ? 1 : 0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(-1.0f, -1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(1.0f, -1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(1.0f, 1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(-1.0f, 1.0f);
    glEnd();

    swapBuffers();
}
