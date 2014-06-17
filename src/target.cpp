/*
 * Copyright (C) 2012, 2013, 2014
 * Computer Graphics Group, University of Siegen, Germany.
 * http://www.cg.informatik.uni-siegen.de/
 * All rights reserved.
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>.
 */

#include <cstdio>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <system_error>

#include "target.h"


Target::Target(variant_t v) :
    variant(v),
    // Variant 1 default values
    model_filename(),
    // Variant 2 default values
    number_of_bars(40),
    first_bar_width(0.05f),
    first_bar_height(0.20f),
    first_offset_x(0.075f),
    first_offset_y(0.0f),
    first_offset_z(0.0f),
    next_bar_width_factor(0.75f),
    next_bar_width_offset(0.0f),
    next_bar_height_factor(1.0f),
    next_bar_height_offset(0.0f),
    next_offset_x_factor(0.75f),
    next_offset_x_offset(0.0f),
    next_offset_y_factor(1.0f),
    next_offset_y_offset(0.0f),
    next_offset_z_factor(1.0f),
    next_offset_z_offset(0.0f),
    bar_background_near_side(1),
    bar_background_dist_near(0.0f),
    bar_background_dist_far(0.20f),
    bar_rotation(0.0f),
    // Variant 3 default values
    star_spokes(20),
    star_radius(0.20f),
    star_background_dist_center(0.20f),
    star_background_dist_rim(0.0f),
    // Variant 4 default values
    background_planar_width(0.8f),
    background_planar_height(0.6f),
    background_planar_dist(0.0f)
{
}

void Target::save(const std::string& filename) const
{
    FILE* f = fopen(filename.c_str(), "wb");
    if (!f) {
        throw std::system_error(errno, std::system_category(),
                std::string("Cannot open ").append(filename));
    }
    fprintf(f, "PMDSIM TARGET VERSION 1\n");
    fprintf(f, "variant %d\n", static_cast<int>(variant));
    fprintf(f, "model_filename '%s'\n", model_filename.c_str());
    fprintf(f, "number_of_bars %d\n", number_of_bars);
    fprintf(f, "first_bar_width %.8g\n", first_bar_width);
    fprintf(f, "first_bar_height %.8g\n", first_bar_height);
    fprintf(f, "first_offset_x %.8g\n", first_offset_x);
    fprintf(f, "first_offset_y %.8g\n", first_offset_y);
    fprintf(f, "first_offset_z %.8g\n", first_offset_z);
    fprintf(f, "next_bar_width_factor %.8g\n", next_bar_width_factor);
    fprintf(f, "next_bar_width_offset %.8g\n", next_bar_width_offset);
    fprintf(f, "next_bar_height_factor %.8g\n", next_bar_height_factor);
    fprintf(f, "next_bar_height_offset %.8g\n", next_bar_height_offset);
    fprintf(f, "next_offset_x_factor %.8g\n", next_offset_x_factor);
    fprintf(f, "next_offset_x_offset %.8g\n", next_offset_x_offset);
    fprintf(f, "next_offset_y_factor %.8g\n", next_offset_y_factor);
    fprintf(f, "next_offset_y_offset %.8g\n", next_offset_y_offset);
    fprintf(f, "next_offset_z_factor %.8g\n", next_offset_z_factor);
    fprintf(f, "next_offset_z_offset %.8g\n", next_offset_z_offset);
    fprintf(f, "bar_background_near_side %d\n", bar_background_near_side);
    fprintf(f, "bar_background_dist_near %.8g\n", bar_background_dist_near);
    fprintf(f, "bar_background_dist_far %.8g\n", bar_background_dist_far);
    fprintf(f, "bar_rotation %.8g\n", bar_rotation);
    fprintf(f, "star_spokes %d\n", star_spokes);
    fprintf(f, "star_radius %.8g\n", star_radius);
    fprintf(f, "star_background_dist_center %.8g\n", star_background_dist_center);
    fprintf(f, "star_background_dist_rim %.8g\n", star_background_dist_rim);
    fprintf(f, "background_planar_width %.8g\n", background_planar_width);
    fprintf(f, "background_planar_height %.8g\n", background_planar_height);
    fprintf(f, "background_planar_dist %.8g\n", background_planar_dist);
    fflush(f);
    if (ferror(f) || fclose(f) != 0) {
        throw std::system_error(errno, std::system_category(),
                std::string("Cannot write ").append(filename));
    }
}

void Target::load(const std::string& filename)
{
    Target newtarget;   // new target initialized with default values
    const size_t linebuf_size = 512;
    char linebuf[linebuf_size];
    int fileformat_version = 0;
    int line_index = 1;
    FILE* f = fopen(filename.c_str(), "rb");
    char* p = NULL;
    char* q = NULL;
    int x;
    if (!f) {
        throw std::system_error(errno, std::system_category(),
                std::string("Cannot open ").append(filename));
    }
    if (!fgets(linebuf, linebuf_size, f)
            || (sscanf(linebuf, "PMDSIM TARGET VERSION %d", &fileformat_version) != 1
                && sscanf(linebuf, "PMDSIMTAP TARGET VERSION %d", &fileformat_version) != 1)
            || fileformat_version != 1) {
        throw std::runtime_error(
                std::string("Cannot read ").append(filename).append(": not a valid target description"));
    }
    while (fgets(linebuf, linebuf_size, f)) {
        line_index++;
        // ignore empty lines and comment lines
        if (linebuf[0] == '\r' || linebuf[0] == '\n' || linebuf[0] == '#')
            continue;
        // override default values if defined in the file
        else if (sscanf(linebuf, "variant %d", &x) == 1 && x >= 1 && x <= 4) {
            newtarget.variant = static_cast<variant_t>(x);
            continue;
        }
        else if (strncmp(linebuf, "model_filename", 14) == 0
                && (p = strchr(linebuf + 14, '\''))
                && (q = strrchr(linebuf + 14, '\''))
                && q > p) {
            newtarget.model_filename = std::string(p + 1, q - p - 1);
            continue;
        }
        else if (sscanf(linebuf, "number_of_bars %d", &newtarget.number_of_bars) == 1)
            continue;
        else if (sscanf(linebuf, "first_bar_width %f", &newtarget.first_bar_width) == 1)
            continue;
        else if (sscanf(linebuf, "first_bar_height %f", &newtarget.first_bar_height) == 1)
            continue;
        else if (sscanf(linebuf, "first_offset_x %f", &newtarget.first_offset_x) == 1)
            continue;
        else if (sscanf(linebuf, "first_offset_y %f", &newtarget.first_offset_y) == 1)
            continue;
        else if (sscanf(linebuf, "first_offset_z %f", &newtarget.first_offset_z) == 1)
            continue;
        else if (sscanf(linebuf, "next_bar_width_factor %f", &newtarget.next_bar_width_factor) == 1)
            continue;
        else if (sscanf(linebuf, "next_bar_width_offset %f", &newtarget.next_bar_width_offset) == 1)
            continue;
        else if (sscanf(linebuf, "next_bar_height_factor %f", &newtarget.next_bar_height_factor) == 1)
            continue;
        else if (sscanf(linebuf, "next_bar_height_offset %f", &newtarget.next_bar_height_offset) == 1)
            continue;
        else if (sscanf(linebuf, "next_offset_x_factor %f", &newtarget.next_offset_x_factor) == 1)
            continue;
        else if (sscanf(linebuf, "next_offset_x_offset %f", &newtarget.next_offset_x_offset) == 1)
            continue;
        else if (sscanf(linebuf, "next_offset_y_factor %f", &newtarget.next_offset_y_factor) == 1)
            continue;
        else if (sscanf(linebuf, "next_offset_y_offset %f", &newtarget.next_offset_y_offset) == 1)
            continue;
        else if (sscanf(linebuf, "next_offset_z_factor %f", &newtarget.next_offset_z_factor) == 1)
            continue;
        else if (sscanf(linebuf, "next_offset_z_offset %f", &newtarget.next_offset_z_offset) == 1)
            continue;
        else if (sscanf(linebuf, "bar_background_near_side %d", &newtarget.bar_background_near_side) == 1)
            continue;
        else if (sscanf(linebuf, "bar_background_dist_near %f", &newtarget.bar_background_dist_near) == 1)
            continue;
        else if (sscanf(linebuf, "bar_background_dist_far %f", &newtarget.bar_background_dist_far) == 1)
            continue;
        else if (sscanf(linebuf, "bar_rotation %f", &newtarget.bar_rotation) == 1)
            continue;
        else if (sscanf(linebuf, "star_spokes %d", &newtarget.star_spokes) == 1)
            continue;
        else if (sscanf(linebuf, "star_radius %f", &newtarget.star_radius) == 1)
            continue;
        else if (sscanf(linebuf, "star_background_dist_center %f", &newtarget.star_background_dist_center) == 1)
            continue;
        else if (sscanf(linebuf, "star_background_dist_rim %f", &newtarget.star_background_dist_rim) == 1)
            continue;
        else if (sscanf(linebuf, "background_planar_width %f", &newtarget.background_planar_width) == 1)
            continue;
        else if (sscanf(linebuf, "background_planar_height %f", &newtarget.background_planar_height) == 1)
            continue;
        else if (sscanf(linebuf, "background_planar_dist %f", &newtarget.background_planar_dist) == 1)
            continue;
        else // ignore unknown entries, for future compatibility
            fprintf(stderr, "ignoring %s line %d\n", filename.c_str(), line_index);
    }
    if (ferror(f)) {
        throw std::system_error(errno, std::system_category(),
                std::string("Cannot read ").append(filename));
    }
    *this = newtarget;
}
