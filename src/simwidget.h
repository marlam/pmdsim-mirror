/*
 * Copyright (C) 2012, 2013, 2014
 * Computer Graphics Group, University of Siegen, Germany.
 * http://www.cg.informatik.uni-siegen.de/
 * All rights reserved.
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>.
 */

#ifndef SIMWIDGET_H
#define SIMWIDGET_H

#include <GL/glew.h>

#include "glwidget.h"
#include "trianglepatch.h"


class SimWidget : public GLWidget
{
    Q_OBJECT

private:
    GLuint _fbo, _depthbuffer;

    float _pixel_mask_x, _pixel_mask_y, _pixel_mask_w, _pixel_mask_h;
    int _pixel_map_w, _pixel_map_h;
    GLuint _pixel_map_tex;

    GLuint _oversampled_map_tex;
    int _oversampled_map_width, _oversampled_map_height;

    GLuint _simple_prg;
    std::string _simple_prg_current_table;
    GLuint _simple_prg_table;
    void render_oversampled_map(const std::vector<TrianglePatch>& scene, int phase_index);

    GLuint _reduction_prg;
    int _map_width, _map_height;
    GLuint _map_tex;

    GLuint _phase_add_prg;
    int _phase_w, _phase_h;
    GLuint _phase_texs[4][2]; // four phase images, with ping-pong buffers
    int _phase_texs_index[4]; // index of most recently written ping-pong buffer (0 or 1)

    GLuint _result_prg;
    int _result_w, _result_h;
    GLuint _result_tex;

public:
    SimWidget();

    GLuint get_map() const;
    GLuint get_phase(int index) const;
    GLuint get_result() const;

    void render_map(const std::vector<TrianglePatch>& scene, int phase_index);
    void simulate_phase_img(int phase_index, int exposure_time_sample_index);
    void simulate_result();
};

#endif
