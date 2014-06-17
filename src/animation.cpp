/*
 * Copyright (C) 2012, 2013, 2014
 * Computer Graphics Group, University of Siegen, Germany.
 * http://www.cg.informatik.uni-siegen.de/
 * All rights reserved.
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>.
 */

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <system_error>

#include "animation.h"


static const float epsilon = 0.0001f;

Animation::Animation()
{
}

static void copy_keyframe(const Animation::Keyframe& keyframe, float pos[3], float rot[4])
{
    pos[0] = keyframe.pos[0];
    pos[1] = keyframe.pos[1];
    pos[2] = keyframe.pos[2];
    rot[0] = keyframe.rot[0];
    rot[1] = keyframe.rot[1];
    rot[2] = keyframe.rot[2];
    rot[3] = keyframe.rot[3];
}

static void slerp(const float quat_a[4], const float quat_b[4], float alpha, float quat[4])
{
    const float* qa = quat_a;
    float qb[4] = { quat_b[0], quat_b[1], quat_b[2], quat_b[3] };

    float cos_w = qa[0] * qb[0] + qa[1] * qb[1] + qa[2] * qb[2] + qa[3] + qb[3];
    if (cos_w < 0.0f) {
        cos_w = -cos_w;
        qb[0] = -qb[0];
        qb[1] = -qb[1];
        qb[2] = -qb[2];
        qb[3] = -qb[3];
    }
    float t_a, t_b;
    if (1.0f - cos_w > epsilon) {
        float w = std::acos(cos_w);   // 0 <= w <= pi
        float sin_w = std::sin(w);
        t_a = std::sin(alpha * w) / sin_w;
        t_b = std::sin((1.0f - alpha) * w) / sin_w;
    } else {
        // the difference is very small, so use simple linear interpolation
        t_a = alpha;
        t_b = 1.0f - alpha;
    }
    quat[0] = t_a * qa[0] + t_b * qb[0];
    quat[1] = t_a * qa[1] + t_b * qb[1];
    quat[2] = t_a * qa[2] + t_b * qb[2];
    quat[3] = t_a * qa[3] + t_b * qb[3];
}

void Animation::interpolate(long long t, float pos[3], float rot[4])
{
    // Catch corner cases
    if (keyframes.size() == 0) {
        return;
    } else if (t <= keyframes[0].t) {
        copy_keyframe(keyframes[0], pos, rot);
        return;
    } else if (t >= keyframes[keyframes.size() - 1].t) {
        copy_keyframe(keyframes[keyframes.size() - 1], pos, rot);
        return;
    }

    // Binary search for the two nearest keyframes.
    // At this point we know that if we have only one keyframe, then its
    // time stamp is the requested time stamp so we have an exact match below.
    int a = 0;
    int b = keyframes.size() - 1;
    while (b >= a) {
        int c = (a + b) / 2;
        if (keyframes[c].t < t) {
            a = c + 1;
        } else if (keyframes[c].t > t) {
            b = c - 1;
        } else {
            // exact match
            copy_keyframe(keyframes[c], pos, rot);
            return;
        }
    }
    // Now we have two keyframes: b is the lower one and a is the higher one.
    // Swap them so that a is the lower one and b the higher one, to avoid confusion.
    std::swap(a, b);
    // Compute alpha value for interpolation.
    float alpha = static_cast<float>(keyframes[b].t - t) / (keyframes[b].t - keyframes[a].t);

    // Interpolate position
    pos[0] = alpha * keyframes[a].pos[0] + (1.0f - alpha) * keyframes[b].pos[0];
    pos[1] = alpha * keyframes[a].pos[1] + (1.0f - alpha) * keyframes[b].pos[1];
    pos[2] = alpha * keyframes[a].pos[2] + (1.0f - alpha) * keyframes[b].pos[2];

    // Interpolate rotation
    slerp(keyframes[a].rot, keyframes[b].rot, alpha, rot);
}

static float to_rad(float deg)
{
    return static_cast<float>(M_PI) / 180.0f * deg;
}

static float dot(const float v[3], const float w[3])
{
    return v[0] * w[0] + v[1] * w[1] + v[2] * w[2];
}

static void cross(const float v[3], const float w[3], float c[3])
{
    c[0] = v[1] * w[2] - v[2] * w[1];
    c[1] = v[2] * w[0] - v[0] * w[2];
    c[2] = v[0] * w[1] - v[1] * w[0];
}

static void angle_axis_to_quat(float angle, const float axis[3], float quat[4])
{
    float axis_length = std::sqrt(dot(axis, axis));
    if (std::abs(angle) <= epsilon || axis_length <= epsilon) {
        quat[0] = 0.0f;
        quat[1] = 0.0f;
        quat[2] = 0.0f;
        quat[3] = 1.0f;
    } else {
        float sin_a = std::sin(angle / 2.0f);
        quat[0] = axis[0] / axis_length * sin_a;
        quat[1] = axis[1] / axis_length * sin_a;
        quat[2] = axis[2] / axis_length * sin_a;
        quat[3] = std::cos(angle / 2.0f);
    }
}

static void oldnew_to_quat(const float o[3], const float n[3], float quat[4])
{
    float angle;
    float axis[3];
    angle = std::acos(dot(o, n) / std::sqrt(dot(o, o) * dot(n, n)));
    cross(o, n, axis);
    angle_axis_to_quat(angle, axis, quat);
}

static void quat_mult(const float a[4], const float b[4], float q[4])
{
    q[0] = a[3] * b[0] + a[0] * b[3] + a[1] * b[2] - a[2] * b[1];
    q[1] = a[3] * b[1] + a[1] * b[3] + a[2] * b[0] - a[0] * b[2];
    q[2] = a[3] * b[2] + a[2] * b[3] + a[0] * b[1] - a[1] * b[0];
    q[3] = a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2];
}

void Animation::load(const std::string& filename)
{
    Animation newanimation;
    const size_t linebuf_size = 512;
    char linebuf[linebuf_size];
    int fileformat_version = 0;
    int line_index = 1;
    FILE* f = fopen(filename.c_str(), "rb");
    if (!f) {
        throw std::system_error(errno, std::system_category(),
                std::string("Cannot open ").append(filename));
    }
    if (!fgets(linebuf, linebuf_size, f)
            || (sscanf(linebuf, "PMDSIM ANIMATION VERSION %d", &fileformat_version) != 1
                && sscanf(linebuf, "PMDSIMTAP ANIMATION VERSION %d", &fileformat_version) != 1)
            || (fileformat_version != 1 && fileformat_version != 2)) {
        throw std::runtime_error(
                std::string("Cannot read ").append(filename).append(": not a valid animation description"));
    }
    if (fileformat_version == 1) {
        while (fgets(linebuf, linebuf_size, f)) {
            Keyframe keyframe;
            float tmpt;
            float tmprot[4];
            line_index++;
            if (linebuf[0] == '\r' || linebuf[0] == '\n' || linebuf[0] == '#') {
                // ignore empty lines and comment lines
                continue;
            } else if (sscanf(linebuf, "%f %f %f %f %f %f %f %f", &tmpt,
                        &keyframe.pos[0], &keyframe.pos[1], &keyframe.pos[2],
                        &tmprot[0], &tmprot[1], &tmprot[2], &tmprot[3]) == 8) {
                keyframe.t = tmpt * 1e6f;
                angle_axis_to_quat(to_rad(tmprot[0]), tmprot + 1, keyframe.rot);
                // Insert new keyframe at correct position. We use stupid inefficient
                // linear search for simplicity; performance does not matter here.
                size_t i = 0;
                while (i < newanimation.keyframes.size() && newanimation.keyframes[i].t < keyframe.t)
                    i++;
                if (i < newanimation.keyframes.size() && newanimation.keyframes[i].t == keyframe.t) {
                    fprintf(stderr, "%s line %d: overwriting previously defined keyframe\n", filename.c_str(), line_index);
                    newanimation.keyframes[i] = keyframe;
                } else {
                    newanimation.keyframes.insert(newanimation.keyframes.begin() + i, keyframe);
                }
                continue;
            } else {
                // ignore unknown entries, for future compatibility
                fprintf(stderr, "ignoring %s line %d\n", filename.c_str(), line_index);
            }
        }
    } else {    // fileformat_version == 2
        while (fgets(linebuf, linebuf_size, f)) {
            line_index++;
            if (linebuf[0] == '\r' || linebuf[0] == '\n' || linebuf[0] == '#') {
                // ignore empty lines and comment lines
                continue;
            }
            // parse into tokens
            std::vector<std::string> tokens;
            {
                bool in_paren = false;
                int i = 0;
                tokens.push_back(std::string());
                while (linebuf[i]) {
                    if (linebuf[i] == '(') {
                        in_paren = true;
                        tokens[tokens.size() - 1].push_back(linebuf[i]);
                    } else if (linebuf[i] == ')') {
                        in_paren = false;
                        tokens[tokens.size() - 1].push_back(linebuf[i]);
                    } else if (linebuf[i] == ' ' || linebuf[i] == '\t') {
                        while (linebuf[i + 1] == ' ' || linebuf[i + 1] == '\t')
                            i++;
                        if (!in_paren)
                            tokens.push_back(std::string());
                    } else if (linebuf[i] == '\r' || linebuf[i] == '\n') {
                        break;
                    } else {
                        tokens[tokens.size() - 1].push_back(linebuf[i]);
                    }
                    i++;
                }
                if (tokens[tokens.size() - 1].empty())
                    tokens.resize(tokens.size() - 1);
            }
            float tmpt;
            int tmppos_type;    // 0=cart, 1=cyl, 2=sphere
            float tmppos[3];
            int tmprot_type;    // 0=angle_axis, 1=oldnew
            float tmprot[6];
            bool camrel_rot;    // rotation is relative to camera-facing rotation
            bool valid = (tokens.size() == 4);
            if (valid) {
                valid = (sscanf(tokens[0].c_str(), "%f", &tmpt) == 1);
            }
            if (valid) {
                if (sscanf(tokens[1].c_str(), "cart(%f,%f,%f)",
                            tmppos + 0, tmppos + 1, tmppos + 2) == 3) {
                    tmppos_type = 0;
                } else if (sscanf(tokens[1].c_str(), "cyl(%f,%f,%f)",
                            tmppos + 0, tmppos + 1, tmppos + 2) == 3) {
                    tmppos_type = 1;
                } else if (sscanf(tokens[1].c_str(), "sph(%f,%f,%f)",
                            tmppos + 0, tmppos + 1, tmppos + 2) == 3) {
                    tmppos_type = 2;
                } else {
                    valid = false;
                }
            }
            if (valid) {
                if (tokens[2] == std::string("abs_rot")) {
                    camrel_rot = false;
                } else if (tokens[2] == std::string("camrel_rot")) {
                    camrel_rot = true;
                } else {
                    valid = false;
                }
            }
            if (valid) {
                if (sscanf(tokens[3].c_str(), "angle_axis(%f,%f,%f,%f)",
                            tmprot + 0, tmprot + 1, tmprot + 2, tmprot + 3) == 4) {
                    tmprot_type = 0;
                } else if (sscanf(tokens[3].c_str(), "oldnew(%f,%f,%f,%f,%f,%f)",
                            tmprot + 0, tmprot + 1, tmprot + 2, tmprot + 3, tmprot + 4, tmprot + 5) == 6) {
                    tmprot_type = 1;
                } else {
                    valid = false;
                }
            }
            if (valid) {
                Keyframe keyframe;
                keyframe.t = tmpt * 1e6f;
                if (tmppos_type == 0) {         // cartesian coordinates
                    keyframe.pos[0] = tmppos[0];
                    keyframe.pos[1] = tmppos[1];
                    keyframe.pos[2] = tmppos[2];
                } else if (tmppos_type == 1) {  // cylindrical coordinates
                    keyframe.pos[0] = tmppos[0] * std::sin(to_rad(-tmppos[1]));
                    keyframe.pos[1] = tmppos[2];
                    keyframe.pos[2] = -tmppos[0] * std::cos(to_rad(-tmppos[1]));
                } else /* tmppos_type == 2 */ { // spherical coordinates
                    keyframe.pos[0] = tmppos[0] * std::cos(to_rad(tmppos[2])) * std::sin(to_rad(-tmppos[1]));
                    keyframe.pos[1] = tmppos[0] * std::sin(to_rad(tmppos[2]));
                    keyframe.pos[2] = -tmppos[0] * std::cos(to_rad(tmppos[2])) * std::cos(to_rad(-tmppos[1]));
                }
                if (tmprot_type == 0) {         // angle_axis rotation
                    angle_axis_to_quat(to_rad(tmprot[0]), tmprot + 1, keyframe.rot);
                } else /* tmprot_type == 1 */ { // old point / new point rotation
                    oldnew_to_quat(tmprot + 0, tmprot + 3, keyframe.rot);
                }
                if (camrel_rot) {
                    // create quaternion that represents camera-facing rotation
                    float o[3] = { 0.0f, 0.0f, -1.0f };
                    float q[4];
                    oldnew_to_quat(o, keyframe.pos, q);
                    // combine this with the given rotation
                    float oq[4] = { keyframe.rot[0], keyframe.rot[1], keyframe.rot[2], keyframe.rot[3] };
                    quat_mult(q, oq, keyframe.rot);
                }
                // Insert new keyframe at correct position. We use stupid inefficient
                // linear search for simplicity; performance does not matter here.
                size_t i = 0;
                while (i < newanimation.keyframes.size() && newanimation.keyframes[i].t < keyframe.t)
                    i++;
                if (i < newanimation.keyframes.size() && newanimation.keyframes[i].t == keyframe.t) {
                    fprintf(stderr, "%s line %d: overwriting previously defined keyframe\n", filename.c_str(), line_index);
                    newanimation.keyframes[i] = keyframe;
                } else {
                    newanimation.keyframes.insert(newanimation.keyframes.begin() + i, keyframe);
                }
                continue;
            } else {
                // ignore unknown entries, for future compatibility
                fprintf(stderr, "ignoring %s line %d\n", filename.c_str(), line_index);
            }
        }
    }
    if (ferror(f)) {
        throw std::system_error(errno, std::system_category(),
                std::string("Cannot read ").append(filename));
    }
    *this = newanimation;
}
