/*
 * Copyright (C) 2012, 2013, 2014
 * Computer Graphics Group, University of Siegen, Germany.
 * http://www.cg.informatik.uni-siegen.de/
 * All rights reserved.
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>.
 */

void get_viewport(float aspect_ratio, int viewport[4], float clearcolor[3]) const
{
    float widget_aspect_ratio = static_cast<float>(width()) / height();
    if (widget_aspect_ratio > aspect_ratio) {
        // Do not use full width
        int w = height() * aspect_ratio;
        viewport[0] = (width() - w) / 2;
        viewport[1] = 0;
        viewport[2] = w;
        viewport[3] = height();
    } else {
        // Do not use full height
        int h = width() / aspect_ratio;
        viewport[0] = 0;
        viewport[1] = (height() - h) / 2;
        viewport[2] = width();
        viewport[3] = h;
    }
    clearcolor[0] = 0.4f;
    clearcolor[1] = 0.4f;
    clearcolor[2] = 0.4f;
}
