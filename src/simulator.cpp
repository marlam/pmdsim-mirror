/*
 * Copyright (C) 2012, 2013, 2014
 * Computer Graphics Group, University of Siegen, Germany.
 * http://www.cg.informatik.uni-siegen.de/
 * All rights reserved.
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>.
 */

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <system_error>

#ifdef HAVE_GTA
# include <gta/gta.hpp>
#endif

#include "simulator.h"


LightSourceIntensityTable::LightSourceIntensityTable()
{
    reset();
}

LightSourceIntensityTable::~LightSourceIntensityTable()
{
}

void LightSourceIntensityTable::reset()
{
    filename.clear();
    width = 0;
    height = 0;
    table.clear();
}

void LightSourceIntensityTable::load(const std::string& filename)
{
#ifdef HAVE_GTA
    FILE* f = fopen(filename.c_str(), "rb");
    if (!f) {
        throw std::system_error(errno, std::system_category(),
                std::string("Cannot open ").append(filename));
    }
    gta::header hdr;
    hdr.read_from(f);
    if (hdr.dimensions() != 2 || hdr.dimension_size(0) > 4096 || hdr.dimension_size(1) > 4096
            || hdr.components() != 1 || hdr.component_type(0) != gta::float32) {
        throw std::runtime_error(
                std::string("Cannot read ").append(filename).append(": invalid measurement table"));
    }
    std::vector<float> gtadata(hdr.dimension_size(0) * hdr.dimension_size(1));
    hdr.read_data(f, &gtadata[0]);
    fclose(f);

    // get range of table
    const float first_x_sample = -M_PI_2; // TODO: read from GTA tag
    const float last_x_sample = +M_PI_2;  // TODO: read from GTA tag
    const float first_y_sample = -M_PI_2; // TODO: read from GTA tag
    const float last_y_sample = +M_PI_2;  // TODO: read from GTA tag
    float step_x = hdr.dimension_size(0) == 1 ? 0.0f
        : (last_x_sample - first_x_sample) / (hdr.dimension_size(0) - 1);
    float step_y = hdr.dimension_size(1) == 1 ? 0.0f
        : (last_y_sample - first_y_sample) / (hdr.dimension_size(1) - 1);

    // convert from W/sr to mW/sr
    for (size_t i = 0; i < gtadata.size(); i++)
        gtadata[i] *= 1000.0f;

    reset();
    this->filename = filename;
    width = hdr.dimension_size(0);
    height = hdr.dimension_size(1);
    start_x = first_x_sample - 0.5f * step_x;
    end_x = last_x_sample + 0.5f * step_x;
    start_y = first_x_sample - 0.5f * step_y;
    end_y = last_x_sample + 0.5f * step_y;
    table = gtadata;
#else
    throw std::runtime_error(
            std::string("Cannot read ").append(filename).append(": this program was built without libgta"));
#endif
}


Simulator::Simulator() :
    aperture_angle(70.0f),                      // ca 90 deg for 50cm app, ca 70 deg for 70cm app; see notes 2012-12-19
    near_plane(0.05f),                          // 5cm; sensible for 70cm app.
    far_plane(2.0f),                            // 2m; sensible for 70cm app.
    exposure_time_samples(1),                   // temporal supersampling of phase image computation; default: off
    rendering_method(0),                        // Default is plain old rasterization
    material_model(0),                          // Lambertian surfaces
    material_lambertian_reflectivity(0.7f),     // 70% surface reflectivity
    lightsource_model(0),                       // Default: simple model
    lightsource_simple_power(200.0f),           // 200 mW (typical; see notes 2012-12-19)
    lightsource_simple_aperture_angle(90.0f),   // assume a light propagation angle of 90 deg
    lightsource_measured_intensities(),         // table of measured angle-dependent intensities
    lens_aperture_diameter(8.89f),              // 0.889 cm aperture (so that F-number is 1.8)
    lens_focal_length(16.0f),                   // 1.6 cm focal length
    sensor_width(352),                          // CIF; see notes 2012-11-28
    sensor_height(288),                         // CIF; see notes 2012-11-28
    pixel_mask_x(1.3f / 7.2f),                  // see sketch 2012-12-05
    pixel_mask_y(0.0f),                         // see sketch 2012-12-05
    pixel_mask_width(4.6 / 7.2f),               // see sketch 2012-12-05
    pixel_mask_height(0.5f),                    // see sketch 2012-12-05
    pixel_width(7),                             // should probably be <= 15
    pixel_height(7),                            // should probably be <= 15
    pixel_pitch(12.0f),                         // a 12 micrometer pitch is realistic
    readout_time(1 * 1000),                     // see mail from Stefan 2012-12-03
    contrast(0.75f),                            // Default is 0.75f; could also be 1.0f?
    modulation_frequency(10 * 1000 * 1000),     // see mail from Stefan 2012-12-03
    exposure_time(1 * 1000)                     // see mail from Stefan 2012-12-03
{
}

void Simulator::save(const std::string& filename) const
{
    FILE* f = fopen(filename.c_str(), "wb");
    if (!f) {
        throw std::system_error(errno, std::system_category(),
                std::string("Cannot open ").append(filename));
    }
    fprintf(f, "PMDSIM SIMULATOR VERSION 1\n");
    fprintf(f, "aperture_angle %.8g\n", aperture_angle);
    fprintf(f, "near_plane %.8g\n", near_plane);
    fprintf(f, "far_plane %.8g\n", far_plane);
    fprintf(f, "exposure_time_samples %d\n", exposure_time_samples);
    fprintf(f, "rendering_method %d\n", rendering_method);
    fprintf(f, "material_model %d\n", material_model);
    fprintf(f, "material_lambertian_reflectivity %.8g\n", material_lambertian_reflectivity);
    fprintf(f, "lightsource_model %d\n", lightsource_model);
    fprintf(f, "lightsource_simple_power %.8g\n", lightsource_simple_power);
    fprintf(f, "lightsource_simple_aperture_angle %.8g\n", lightsource_simple_aperture_angle);
    fprintf(f, "lightsource_measured_intensities '%s'\n", lightsource_measured_intensities.filename.c_str());
    fprintf(f, "lens_aperture_diameter %.8g\n", lens_aperture_diameter);
    fprintf(f, "lens_focal_length %.8g\n", lens_focal_length);
    fprintf(f, "sensor_width %d\n", sensor_width);
    fprintf(f, "sensor_height %d\n", sensor_height);
    fprintf(f, "pixel_mask_x %.8g\n", pixel_mask_x);
    fprintf(f, "pixel_mask_y %.8g\n", pixel_mask_y);
    fprintf(f, "pixel_mask_width %.8g\n", pixel_mask_width);
    fprintf(f, "pixel_mask_height %.8g\n", pixel_mask_height);
    fprintf(f, "pixel_width %d\n", pixel_width);
    fprintf(f, "pixel_height %d\n", pixel_height);
    fprintf(f, "pixel_pitch %.8g\n", pixel_pitch);
    fprintf(f, "readout_time %d\n", readout_time);
    fprintf(f, "contrast %.8g\n", contrast);
    fprintf(f, "modulation_frequency %d\n", modulation_frequency);
    fprintf(f, "exposure_time %d\n", exposure_time);
    fflush(f);
    if (ferror(f) || fclose(f) != 0) {
        throw std::system_error(errno, std::system_category(),
                std::string("Cannot write ").append(filename));
    }
}

void Simulator::load(const std::string& filename)
{
    Simulator newsim;   // new simulator initialized with default values
    const size_t linebuf_size = 512;
    char linebuf[linebuf_size];
    int fileformat_version = 0;
    int line_index = 1;
    char* p = NULL;
    char* q = NULL;
    FILE* f = fopen(filename.c_str(), "rb");
    if (!f) {
        throw std::system_error(errno, std::system_category(),
                std::string("Cannot open ").append(filename));
    }
    if (!fgets(linebuf, linebuf_size, f)
            || (sscanf(linebuf, "PMDSIM SIMULATOR VERSION %d", &fileformat_version) != 1
                && sscanf(linebuf, "PMDSIMTAP SIMULATOR VERSION %d", &fileformat_version) != 1)
            || fileformat_version != 1) {
        throw std::runtime_error(
                std::string("Cannot read ").append(filename).append(": not a valid simulator description"));
    }
    while (fgets(linebuf, linebuf_size, f)) {
        line_index++;
        // ignore empty lines and comment lines
        if (linebuf[0] == '\r' || linebuf[0] == '\n' || linebuf[0] == '#')
            continue;
        // override default values if defined in the file
        else if (sscanf(linebuf, "aperture_angle %f", &newsim.aperture_angle) == 1)
            continue;
        else if (sscanf(linebuf, "near_plane %f", &newsim.near_plane) == 1)
            continue;
        else if (sscanf(linebuf, "far_plane %f", &newsim.far_plane) == 1)
            continue;
        else if (sscanf(linebuf, "exposure_time_samples %d", &newsim.exposure_time_samples) == 1)
            continue;
        else if (sscanf(linebuf, "rendering_method %d", &newsim.rendering_method) == 1)
            continue;
        else if (sscanf(linebuf, "material_model %d", &newsim.material_model) == 1)
            continue;
        else if (sscanf(linebuf, "material_lambertian_reflectivity %f", &newsim.material_lambertian_reflectivity) == 1)
            continue;
        else if (sscanf(linebuf, "lightsource_model %d", &newsim.lightsource_model) == 1)
            continue;
        else if (sscanf(linebuf, "lightsource_simple_power %f", &newsim.lightsource_simple_power) == 1
                || sscanf(linebuf, "lightsource_power %f", &newsim.lightsource_simple_power) == 1) // for backward compat
            continue;
        else if (sscanf(linebuf, "lightsource_simple_aperture_angle %f", &newsim.lightsource_simple_aperture_angle) == 1)
            continue;
        else if (strncmp(linebuf, "lightsource_measured_intensities", 30) == 0
                && (p = strchr(linebuf + 30, '\''))
                && (q = strrchr(linebuf + 30, '\''))
                && q > p) {
            if (q > p + 1) {
                std::string filename(p + 1, q - p - 1);
                newsim.lightsource_measured_intensities.load(filename);
            }
            continue;
        }
        else if (sscanf(linebuf, "lens_aperture_diameter %f", &newsim.lens_aperture_diameter) == 1)
            continue;
        else if (sscanf(linebuf, "lens_focal_length %f", &newsim.lens_focal_length) == 1)
            continue;
        else if (sscanf(linebuf, "sensor_width %d", &newsim.sensor_width) == 1)
            continue;
        else if (sscanf(linebuf, "sensor_height %d", &newsim.sensor_height) == 1)
            continue;
        else if (sscanf(linebuf, "pixel_mask_x %f", &newsim.pixel_mask_x) == 1)
            continue;
        else if (sscanf(linebuf, "pixel_mask_y %f", &newsim.pixel_mask_y) == 1)
            continue;
        else if (sscanf(linebuf, "pixel_mask_width %f", &newsim.pixel_mask_width) == 1)
            continue;
        else if (sscanf(linebuf, "pixel_mask_height %f", &newsim.pixel_mask_height) == 1)
            continue;
        else if (sscanf(linebuf, "pixel_width %d", &newsim.pixel_width) == 1)
            continue;
        else if (sscanf(linebuf, "pixel_height %d", &newsim.pixel_height) == 1)
            continue;
        else if (sscanf(linebuf, "pixel_pitch %f", &newsim.pixel_pitch) == 1)
            continue;
        else if (sscanf(linebuf, "readout_time %d", &newsim.readout_time) == 1)
            continue;
        else if (sscanf(linebuf, "contrast %f", &newsim.contrast) == 1)
            continue;
        else if (sscanf(linebuf, "modulation_frequency %d", &newsim.modulation_frequency) == 1)
            continue;
        else if (sscanf(linebuf, "exposure_time %d", &newsim.exposure_time) == 1)
            continue;
        else // ignore unknown entries, for future compatibility
            fprintf(stderr, "ignoring %s line %d\n", filename.c_str(), line_index);
    }
    if (ferror(f)) {
        throw std::system_error(errno, std::system_category(),
                std::string("Cannot read ").append(filename));
    }
    *this = newsim;
}
