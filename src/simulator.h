/*
 * Copyright (C) 2012, 2013, 2014
 * Computer Graphics Group, University of Siegen, Germany.
 * http://www.cg.informatik.uni-siegen.de/
 * All rights reserved.
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>.
 */

#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <string>
#include <vector>

/**
 * \file simulator.h
 * \brief The simulator description.
 *
 * This file documents the Simulator class which
 * describes a simulator.
 */

/**
 * \brief The LightSourceIntensityTable class.
 *
 * This class holds a table with measured light source intensities in milliwatt/steradian.
 */
class LightSourceIntensityTable
{
public:
    /** \brief File from which this table was loaded */
    std::string filename;
    /** \brief Width of the table */
    int width;
    /** \brief Height of the table */
    int height;
    /** \brief Measurement angle at left border of table */
    float start_x;
    /** \brief Measurement angle at right border of table */
    float end_x;
    /** \brief Measurement angle at top border of table */
    float start_y;
    /** \brief Measurement angle at bottom border of table */
    float end_y;
    /** \brief The table */
    std::vector<float> table;

    /** \brief Constructor: creates an empty table */
    LightSourceIntensityTable();
    /** \brief Destructor */
    ~LightSourceIntensityTable();
    /** \brief Resets this to an empty table with no associated file */
    void reset();
    /** \brief Loads a table from a file in .gta format. On failure, an
     * exception is thrown and the existing table is not modified. */
    void load(const std::string& filename);
};

/**
 * \brief The Simulator class.
 *
 * This class describes a simulator through a number of parameters.
 * The parameter default values are defined in the class implementation.
 */
class Simulator
{
public:
    static const int c = 299792458; /**< \brief Speed of light in m/s */
    constexpr static const float e = 0.1602176565; /**< \brief Elementary charge in Attocoulomb (1e-18 C). */

    /** \name Rasterization parameters */

    /*@{*/
    /** \brief Aperture angle in degrees */
    float aperture_angle;
    /** \brief Near clip plane in meters */
    float near_plane;
    /** \brief Far clip plane in meters */
    float far_plane;
    /** \brief Number of phase image samples taken during exposure time */
    int exposure_time_samples;
    /** \brief Rendering method; currently only 0=default */
    int rendering_method;
    /*@}*/

    /** \name Material parameters */
    /** \brief Material model to use:
     *  0=lambertian (perfect lambertion reflection) */
    int material_model;
    /** \brief Material model Lambertian: Reflectivity in [0,1] */
    float material_lambertian_reflectivity;

    /** \name Light source parameters */

    /*@{*/
    /** \brief Light source model to use:
     *  0=simple (homogeneous power over aperture angle) or
     *  1=measured (measured power depending on angle) */
    int lightsource_model;
    /** \brief Light source model 0: light source power in milliwatt */
    float lightsource_simple_power;
    /** \brief Light source model 0: light source aperture angle in degrees */
    float lightsource_simple_aperture_angle;
    /** \brief Light source model 1: the table of measured intensities */
    LightSourceIntensityTable lightsource_measured_intensities;;
    /*@}*/

    /** \name Lens parameters */

    /*@{*/
    /** \brief Aperture of lens, in millimeters */
    float lens_aperture_diameter;
    /** \brief Focal length of lens, in millimeters */
    float lens_focal_length;
    /*@}*/

    /** \name Physical sensor model parameters */

    /*@{*/
    /** \brief Number of pixel columns */
    int sensor_width;
    /** \brief Number of pixel rows */
    int sensor_height;
    /** \brief Horizontal start point of the area that is sensitive to incoming photons, in [0,1].
     * The reference area [0,1]x[0,1] is the whole pixel in the 2-Tap approach. In the 1-Tap approach,
     * this area is between the two readout areas, and therefore covers only half the pixel. */
    float pixel_mask_x;
    /** \brief Vertical start point of the area that is sensitive to incoming photons, in [0,1].
     * The reference area [0,1]x[0,1] is the whole pixel in the 2-Tap approach. In the 1-Tap approach,
     * this area is between the two readout areas, and therefore covers only half the pixel. */
    float pixel_mask_y;
    /** \brief Width of the area that is sensitive to incoming photons, in [0,1].
     * The reference area [0,1]x[0,1] is the whole pixel in the 2-Tap approach. In the 1-Tap approach,
     * this area is between the two readout areas, and therefore covers only half the pixel. */
    float pixel_mask_width;
    /** \brief Height of the area that is sensitive to incoming photons, in [0,1].
     * The reference area [0,1]x[0,1] is the whole pixel in the 2-Tap approach. In the 1-Tap approach,
     * this area is between the two readout areas, and therefore covers only half the pixel. */
    float pixel_mask_height;
    /** \brief Width of the discretized pixel mask, in subpixels. This must be an odd number for
     * precise results. (The reason is that we need to read the center of the mask in some situations,
     * and since we use nearest neighbor reading for performance reasons, the center of the mask must
     * correspond with the center of a subpixel.) */
    int pixel_width;
    /** \brief Height of the discretized pixel mask, in subpixels. This must be an odd number for
     * precise results. (The reason is that we need to read the center of the mask in some situations,
     * and since we use nearest neighbor reading for performance reasons, the center of the mask must
     * correspond with the center of a subpixel.) */
    int pixel_height;
    /** \brief Pixel pitch in micrometer */
    float pixel_pitch;
    /** \brief Read-Out time in microseconds */
    int readout_time;
    /** \brief Contrast achieved by one pixel, in [0,1] */
    float contrast;
    /*@}*/

    /** \name User-changeable sensor parameters */

    /*@{*/
    /** \brief Modulation frequency in Hz */
    int modulation_frequency;
    /** \brief Exposure time in microseconds */
    int exposure_time;
    /*@}*/

public:
    /** \brief Constructor
     *
     * Fills in default values, defined in the implementation.
     */
    Simulator();

    /** \brief Return width of energy_a/energy_b map. */
    int map_width() const
    {
        return sensor_width * pixel_width;
    }

    /** \brief Return height of energy_a/energy_b map. */
    int map_height() const
    {
        return sensor_height * pixel_height;
    }

    /** \brief Return aspect ratio of the whole sensor. */
    float aspect_ratio() const
    {
        return static_cast<float>(sensor_width) / sensor_height; // this assumes square pixels!
    }

    /** \brief Return aspect ratio of the energy_a/energy_b map. */
    float map_aspect_ratio() const
    {
        return aspect_ratio();
    }

    /** \brief Save simulator description to a file
     *
     * This throws a std::exception on failure. */
    void save(const std::string& filename) const;

    /** \brief Load simulator description from a file
     *
     * This throws a std::exception on failure. */
    void load(const std::string& filename);
};

#endif
