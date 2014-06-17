/*
 * Copyright (C) 2012, 2013, 2014
 * Computer Graphics Group, University of Siegen, Germany.
 * http://www.cg.informatik.uni-siegen.de/
 * All rights reserved.
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>.
 */

#include <cassert>
#include <stdexcept>
#include <system_error>

#include <GL/glew.h>

#include "simwidget.h"

#include "render-simple.vs.glsl.h"
#include "render-simple.fs.glsl.h"
#include "reduction.fs.glsl.h"
#include "simphaseadd.fs.glsl.h"
#include "simresult.fs.glsl.h"


SimWidget::SimWidget() : GLWidget(NULL),
    _fbo(0), _depthbuffer(0),
    _pixel_map_w(0), _pixel_map_h(0),
    _pixel_map_tex(0),
    _oversampled_map_tex(0), _oversampled_map_width(-1), _oversampled_map_height(-1),
    _simple_prg(0),
    _simple_prg_current_table(), _simple_prg_table(0),
    _reduction_prg(0),
    _map_width(-1), _map_height(-1), _map_tex(0),
    _phase_add_prg(0),
    _phase_w(0), _phase_h(0),
    _result_prg(0),
    _result_w(0), _result_h(0),
    _result_tex(0)
{
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 2; j++)
            _phase_texs[i][j] = 0;
        _phase_texs_index[i] = -1;
    }
}

GLuint SimWidget::get_map() const
{
    return _map_tex;
}

GLuint SimWidget::get_phase(int index) const
{
    assert(index >= 0 && index <= 4);
    assert(_phase_texs_index[index] >= 0 && _phase_texs_index[index] <= 1);
    return _phase_texs[index][_phase_texs_index[index]];
}

GLuint SimWidget::get_result() const
{
    return _result_tex;
}

static std::string replace(const std::string &str, const std::string &s, const std::string &r)
{
    // replace all occurences of 's' in str with r, and return result
    std::string ts(str);
    size_t s_len = s.length();
    size_t r_len = r.length();
    size_t p = 0;

    while ((p = ts.find(s, p)) != std::string::npos) {
        ts.replace(p, s_len, r);
        p += r_len;
    }
    return ts;
}

static GLuint create_tex2d(GLint internal_format, int w, int h)
{
    GLuint t;
    glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    return t;
}

void render_one_to_one(float tl = 0.0f, float tr = 1.0f)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glBegin(GL_QUADS);
    glTexCoord2f(tl, 0.0f);
    glVertex2f(-1.0f, -1.0f);
    glTexCoord2f(tr, 0.0f);
    glVertex2f(1.0f, -1.0f);
    glTexCoord2f(tr, 1.0f);
    glVertex2f(1.0f, 1.0f);
    glTexCoord2f(tl, 1.0f);
    glVertex2f(-1.0f, 1.0f);
    glEnd();
}

void SimWidget::render_oversampled_map(const std::vector<TrianglePatch>& scene, int phase_index)
{
    if (_simple_prg == 0) {
        GLuint vshader = xglCompileShader(GL_VERTEX_SHADER, RENDER_SIMPLE_VS_GLSL_STR, XGL_HERE);
        GLuint fshader = xglCompileShader(GL_FRAGMENT_SHADER, RENDER_SIMPLE_FS_GLSL_STR, XGL_HERE);
        _simple_prg = xglCreateProgram(vshader, 0, fshader);
        xglLinkProgram(_simple_prg);
        assert(xglCheckError(XGL_HERE));
    }

    // Set shader parameters from simulation parameters
    glUseProgram(_simple_prg);
    if (_simulator.lightsource_model == 0) {
        // simple light source model
        float lightsource_simple_aperture_angle = static_cast<float>(M_PI) / 180.0f
            * _simulator.lightsource_simple_aperture_angle;
        float lightsource_simple_solid_angle = 2.0f * static_cast<float>(M_PI)
            * (1.0f - std::cos(lightsource_simple_aperture_angle / 2.0f));
        glUniform1f(glGetUniformLocation(_simple_prg, "lightsource_intensity"),
                _simulator.lightsource_simple_power / lightsource_simple_solid_angle);
    } else {
        // measured light source
        glUniform1f(glGetUniformLocation(_simple_prg, "lightsource_intensity"), -1.0f);
        glUniform1i(glGetUniformLocation(_simple_prg, "lightsource_intensity_table"), 0);
        if (_simple_prg_current_table != _simulator.lightsource_measured_intensities.filename) {
            glDeleteTextures(1, &_simple_prg_table);
            _simple_prg_table = create_tex2d(GL_R32F,
                    _simulator.lightsource_measured_intensities.width,
                    _simulator.lightsource_measured_intensities.height);
            glBindTexture(GL_TEXTURE_2D, _simple_prg_table);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F,
                    _simulator.lightsource_measured_intensities.width,
                    _simulator.lightsource_measured_intensities.height, 0,
                    GL_RED, GL_FLOAT, &_simulator.lightsource_measured_intensities.table[0]);
            _simple_prg_current_table = _simulator.lightsource_measured_intensities.filename;
        }
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _simple_prg_table);
        glUniform1f(glGetUniformLocation(_simple_prg, "lightsource_intensity_table_start_x"),
                _simulator.lightsource_measured_intensities.start_x);
        glUniform1f(glGetUniformLocation(_simple_prg, "lightsource_intensity_table_end_x"),
                _simulator.lightsource_measured_intensities.end_x);
        glUniform1f(glGetUniformLocation(_simple_prg, "lightsource_intensity_table_start_y"),
                _simulator.lightsource_measured_intensities.start_y);
        glUniform1f(glGetUniformLocation(_simple_prg, "lightsource_intensity_table_end_y"),
                _simulator.lightsource_measured_intensities.end_y);
    }
    glUniform1f(glGetUniformLocation(_simple_prg, "frac_modfreq_c"),
            static_cast<double>(_simulator.modulation_frequency) / Simulator::c);
    glUniform1f(glGetUniformLocation(_simple_prg, "frac_apdiam_foclen"),
            _simulator.lens_aperture_diameter / _simulator.lens_focal_length);

    glUniform1f(glGetUniformLocation(_simple_prg, "exposure_time"), _simulator.exposure_time
            / _simulator.exposure_time_samples);
    glUniform1f(glGetUniformLocation(_simple_prg, "pixel_area"), _simulator.pixel_pitch * _simulator.pixel_pitch);
    glUniform1i(glGetUniformLocation(_simple_prg, "pixel_width"), _simulator.pixel_width);
    glUniform1i(glGetUniformLocation(_simple_prg, "pixel_height"), _simulator.pixel_height);
    glUniform1f(glGetUniformLocation(_simple_prg, "contrast"), _simulator.contrast);
    glUniform1f(glGetUniformLocation(_simple_prg, "tau"), phase_index * static_cast<float>(M_PI_2));
    assert(_simulator.material_model == 0);
    glUniform1f(glGetUniformLocation(_simple_prg, "lambertian_reflectivity"),
            _simulator.material_lambertian_reflectivity);

    assert(xglCheckError(XGL_HERE));

    // Now render.
    // This uses simple vertex buffer rendering.
    // TODO: Performance optimization: cache the data on the GPU; do not transfer it every frame.
    glMatrixMode(GL_MODELVIEW);
    for (unsigned int i = 0; i < scene.size(); i++) {
        const TrianglePatch& tp = scene[i];
        if (tp.vertex_array.empty())
            continue;
        glLoadMatrixf(tp.transformation);
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, 0, &(tp.vertex_array[0]));
        assert(!tp.normal_array.empty());
        glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(GL_FLOAT, 0, &(tp.normal_array[0]));
        if (tp.color_array.empty()) {
            glDisableClientState(GL_COLOR_ARRAY);
        } else {
            glEnableClientState(GL_COLOR_ARRAY);
            glColorPointer(4, GL_FLOAT, 0, &(tp.color_array[0]));
        }
        if (tp.texcoord_array.empty()) {
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        } else {
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            glTexCoordPointer(2, GL_FLOAT, 0, &(tp.texcoord_array[0]));
        }
        glDrawElements(GL_TRIANGLES, tp.index_array.size(), GL_UNSIGNED_INT, &(tp.index_array[0]));
    }
}

void SimWidget::render_map(const std::vector<TrianglePatch>& scene, int phase_index)
{
    makeCurrent();
    glClearColor(0.0, 0.0, 0.0, 0.0);

    // First, make sure that the oversampled map is correct
    if (_oversampled_map_width != _simulator.map_width()
            || _oversampled_map_height != _simulator.map_height()) {
        glDeleteTextures(1, &_oversampled_map_tex);
        _oversampled_map_tex = create_tex2d(GL_RGBA32F, _simulator.map_width(), _simulator.map_height());
        if (_depthbuffer == 0)
            glGenRenderbuffers(1, &_depthbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, _depthbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, _simulator.map_width(), _simulator.map_height());
        _oversampled_map_width = _simulator.map_width();
        _oversampled_map_height = _simulator.map_height();
    }
    // Set up framebuffer, viewport, and projection matrix
    if (_fbo == 0)
        glGenFramebuffers(1, &_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _oversampled_map_tex, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthbuffer);
    assert(xglCheckFBO(XGL_HERE));
    glViewport(0, 0, _simulator.map_width(), _simulator.map_height());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(_simulator.aperture_angle, _simulator.map_aspect_ratio(),
            _simulator.near_plane, _simulator.far_plane);
    // Initialize OpenGL stuff
    glClampColorARB(GL_CLAMP_READ_COLOR_ARB, GL_FALSE);
    glClampColorARB(GL_CLAMP_FRAGMENT_COLOR_ARB, GL_FALSE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // Now render the scene into the oversampled map
    render_oversampled_map(scene, phase_index);

    // Reduce spatially oversampled map to sensor resolution
    if (_pixel_map_w != _simulator.pixel_width || _pixel_map_h != _simulator.pixel_height
            || _pixel_mask_x < _simulator.pixel_mask_x || _pixel_mask_x > _simulator.pixel_mask_x
            || _pixel_mask_y < _simulator.pixel_mask_y || _pixel_mask_y > _simulator.pixel_mask_y
            || _pixel_mask_w < _simulator.pixel_mask_width || _pixel_mask_w > _simulator.pixel_mask_width
            || _pixel_mask_h < _simulator.pixel_mask_height || _pixel_mask_h > _simulator.pixel_mask_height) {
        // Recreate pixel map. For each map entry (= subpixel), calculate the subarea that is covered
        // by the photon-sensitive pixel mask.
        _pixel_map_w = _simulator.pixel_width;
        _pixel_map_h = _simulator.pixel_height;
        _pixel_mask_x = _simulator.pixel_mask_x;
        _pixel_mask_y = _simulator.pixel_mask_y;
        _pixel_mask_w = _simulator.pixel_mask_width;
        _pixel_mask_h = _simulator.pixel_mask_height;
        glDeleteTextures(1, &_pixel_map_tex);
        _pixel_map_tex = create_tex2d(GL_R32F, _pixel_map_w, _pixel_map_h);
        float* pixel_map = new float[_pixel_map_w * _pixel_map_h];
        float subpixel_w = 1.0f / _pixel_map_w;
        float subpixel_h = 1.0f / _pixel_map_h;
        for (int y = 0; y < _pixel_map_h; y++) {
            for (int x = 0; x < _pixel_map_w; x++) {
                int i = y * _pixel_map_w + x;
                float subpixel_x = x * subpixel_w;
                float subpixel_y = y * subpixel_h;
                float sx = std::max(subpixel_x, _pixel_mask_x);
                float sy = std::max(subpixel_y, _pixel_mask_y);
                float sw = std::min(subpixel_x + subpixel_w, _pixel_mask_x + _pixel_mask_w) - sx;
                float sh = std::min(subpixel_y + subpixel_h, _pixel_mask_y + _pixel_mask_h) - sy;
                float subarea = (sw > 0.0f && sh > 0.0f) ? sw * sh : 0.0f;
                subarea *= _pixel_map_w * _pixel_map_h;
                pixel_map[i] = subarea;
            }
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, _pixel_map_w, _pixel_map_h, 0,
                GL_RED, GL_FLOAT, pixel_map);
        delete[] pixel_map;
    }
    if (_map_width != _simulator.sensor_width || _map_height != _simulator.sensor_height) {
        glDeleteTextures(1, &_map_tex);
        _map_tex = create_tex2d(GL_RGBA32F, _simulator.sensor_width, _simulator.sensor_height);
        _map_width = _simulator.sensor_width;
        _map_height = _simulator.sensor_height;
    }
    if (_reduction_prg == 0) {
        GLuint fshader = xglCompileShader(GL_FRAGMENT_SHADER, REDUCTION_FS_GLSL_STR, XGL_HERE);
        _reduction_prg = xglCreateProgram(0, 0, fshader);
        xglLinkProgram(_reduction_prg);
        assert(xglCheckError(XGL_HERE));
    }
    glUseProgram(_reduction_prg);
    glUniform1i(glGetUniformLocation(_reduction_prg, "oversampled_map_tex"), 0);
    glUniform1i(glGetUniformLocation(_reduction_prg, "pixel_map_tex"), 1);
    glUniform1i(glGetUniformLocation(_reduction_prg, "pixel_width"), _simulator.pixel_width);
    glUniform1i(glGetUniformLocation(_reduction_prg, "pixel_height"), _simulator.pixel_height);
    glUniform2f(glGetUniformLocation(_reduction_prg, "subpixel_size"),
            1.0f / _simulator.map_width(), 1.0f / _simulator.map_height());
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _map_tex, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
    glViewport(0, 0, _simulator.map_width() / _simulator.pixel_width, _simulator.map_height() / _simulator.pixel_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    assert(xglCheckFBO(XGL_HERE));
    assert(xglCheckError(XGL_HERE));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _oversampled_map_tex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, _pixel_map_tex);
    render_one_to_one();
    assert(xglCheckError(XGL_HERE));
}

void SimWidget::simulate_phase_img(int phase_index, int exposure_time_sample_index)
{
    assert(phase_index >= 0 && phase_index < 4);
    assert(exposure_time_sample_index >= 0);

    makeCurrent();
    assert(_fbo != 0); // must have been created in render_map()

    if (_phase_w != _simulator.sensor_width || _phase_h != _simulator.sensor_height) {
        _phase_w = _simulator.sensor_width;
        _phase_h = _simulator.sensor_height;
        for (int i = 0; i < 4; i++) {
            glDeleteTextures(3, _phase_texs[i]);
            for (int j = 0; j < 2; j++)
                _phase_texs[i][j] = create_tex2d(GL_RGBA32F, _phase_w, _phase_h);
        }
    }
    if (_phase_add_prg == 0) {
        GLuint fshader = xglCompileShader(GL_FRAGMENT_SHADER, SIMPHASEADD_FS_GLSL_STR, XGL_HERE);
        _phase_add_prg = xglCreateProgram(0, 0, fshader);
        xglLinkProgram(_phase_add_prg);
        glUseProgram(_phase_add_prg);
        glUniform1i(glGetUniformLocation(_phase_add_prg, "phase_tex_0"), 0);
        glUniform1i(glGetUniformLocation(_phase_add_prg, "phase_tex_1"), 1);
    }

    /* Add the most recent map to the accumulated phase image using the ping-pong buffer */
    int pp_prv = (exposure_time_sample_index == 0 ? 1 : _phase_texs_index[phase_index]); // previously written ping-pong buffer
    int pp_cur = (pp_prv == 1 ? 0 : 1);          // currently written ping-pong buffer
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _phase_texs[phase_index][pp_cur], 0);
    glViewport(0, 0, _simulator.sensor_width, _simulator.sensor_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _phase_texs[phase_index][pp_prv]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, _map_tex);
    glUseProgram(_phase_add_prg);
    glUniform1i(glGetUniformLocation(_phase_add_prg, "have_phase_tex_0"),
            (exposure_time_sample_index == 0 ? 0 : 1));
    assert(xglCheckFBO(XGL_HERE));
    assert(xglCheckError(XGL_HERE));
    render_one_to_one();
    assert(xglCheckError(XGL_HERE));
    _phase_texs_index[phase_index] = pp_cur;
}

void SimWidget::simulate_result()
{
    makeCurrent();
    assert(_fbo != 0);  // must have been initialized by simulate_phase()
    if (_result_prg == 0) {
        GLuint fshader = xglCompileShader(GL_FRAGMENT_SHADER, SIMRESULT_FS_GLSL_STR, XGL_HERE);
        _result_prg = xglCreateProgram(0, 0, fshader);
        xglLinkProgram(_result_prg);
        glUseProgram(_result_prg);
        GLint phase_tex_vals[4] = { 0, 1, 2, 3 };
        glUniform1iv(glGetUniformLocation(_result_prg, "phase_texs"), 4, phase_tex_vals);
        assert(xglCheckError(XGL_HERE));
    }
    if (_result_w != _simulator.sensor_width || _result_h != _simulator.sensor_height) {
        glDeleteTextures(1, &_result_tex);
        _result_tex = create_tex2d(GL_RGB32F, _simulator.sensor_width, _simulator.sensor_height);
        _result_w = _simulator.sensor_width;
        _result_h = _simulator.sensor_height;
        assert(xglCheckError(XGL_HERE));
    }

    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _result_tex, 0);
    glViewport(0, 0, _simulator.sensor_width, _simulator.sensor_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, get_phase(0));
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, get_phase(1));
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, get_phase(2));
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, get_phase(3));
    glUseProgram(_result_prg);
    glUniform1f(glGetUniformLocation(_result_prg, "frac_c_modfreq"),
            static_cast<double>(Simulator::c) / _simulator.modulation_frequency);

    assert(xglCheckFBO(XGL_HERE));
    assert(xglCheckError(XGL_HERE));
    render_one_to_one();
    assert(xglCheckError(XGL_HERE));
}
