/*
 * Copyright (C) 2012, 2013, 2014
 * Computer Graphics Group, University of Siegen, Germany.
 * http://www.cg.informatik.uni-siegen.de/
 * All rights reserved.
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>.
 */

#ifndef TARGET_H
#define TARGET_H

#include <string>

/**
 * \file target.h
 * \brief The target description.
 *
 * This file documents the Target class which
 * describes a target.
 */

/**
 * \brief The Target class.
 *
 * This class describes a target through either the name of a file that
 * contains the target model, or through a number of parameters.
 * The parameter default values are defined in the class implementation.
 */
class Target
{
public:
    /** \brief Target variants. */
    typedef enum {
        variant_model = 1,              /**< \brief Target is defined by 3D model file. */
        variant_bars = 2,               /**< \brief Target is a bar pattern. Its front is centered in the x/y-plane, and
                                          it extents in the direction of the negative z axis. */
        variant_star = 3,               /**< \brief Target is a Siemens star. Its front is centered in the x/y-plane,
                                          and it extents in the direction of the negative z axis. */
        variant_background_planar = 4   /**< \brief Target is a planar background, centered in the x/y-plane. */
    } variant_t;
    variant_t variant;                  /**< \brief The target variant. */

    /** \name Variant 1: model file. */

    /*@{*/
    /** \brief Name of a model file with arbitrary content, to be used as target. */
    std::string model_filename;
    /*@}*/

    /** \name Variant 2: a pattern of 2D bars. */

    /*@{*/
    int number_of_bars;                 /**< \brief Number of bars in the pattern. */
    float first_bar_width;              /**< \brief Width of the first bar, in meters. */
    float first_bar_height;             /**< \brief Height of the first bar, in meters. */
    float first_offset_x;               /**< \brief Offset from first bar to next bar, x component, in meters. */
    float first_offset_y;               /**< \brief Offset from first bar to next bar, y component, in meters. */
    float first_offset_z;               /**< \brief Offset from first bar to next bar, z component, in meters. */
    float next_bar_width_factor;        /**< \brief Factor to calculate the width of the next bar from the width of the current bar. */
    float next_bar_width_offset;        /**< \brief Offset to calculate the width of the next bar from the width of the current bar. */
    float next_bar_height_factor;       /**< \brief Factor to calculate the height of the next bar from the height of the current bar. */
    float next_bar_height_offset;       /**< \brief Offset to calculate the height of the next bar from the height of the current bar. */
    float next_offset_x_factor;         /**< \brief Factor to calculate the x component of the next offset from the x component of the current offset. */
    float next_offset_x_offset;         /**< \brief Offset to calculate the x component of the next offset from the x component of the current offset. */
    float next_offset_y_factor;         /**< \brief Factor to calculate the x component of the next offset from the x component of the current offset. */
    float next_offset_y_offset;         /**< \brief Offset to calculate the x component of the next offset from the x component of the current offset. */
    float next_offset_z_factor;         /**< \brief Factor to calculate the x component of the next offset from the x component of the current offset. */
    float next_offset_z_offset;         /**< \brief Offset to calculate the x component of the next offset from the x component of the current offset. */
    int bar_background_near_side;       /**< \brief Side of the background that is near. -1=disabled, 0=left, 1=top, 2=right, 3=bottom. The opposite side will be far. */
    float bar_background_dist_near;     /**< \brief Distance of the background in meters, measured at the bottom of the farthest bar. */
    float bar_background_dist_far;      /**< \brief Distance of the background in meters, measured at the left of the farthest bar. */
    float bar_rotation;                 /**< \brief Rotation of the complete target around the view direction. */
    /*@}*/

    /** \name Variant 3: a Siemens star. */

    /*@{*/
    int star_spokes;                    /**< \brief Number of spokes in the star. */
    float star_radius;                  /**< \brief Radius of the star, in meters. */
    float star_background_dist_center;  /**< \brief Distance to the star background at the star center, in meters. Ignored if < 0. */
    float star_background_dist_rim;     /**< \brief Distance to the star background at the star rim, in meters. Ignored if < 0. */
    /*@}*/

    /** \name Variant 4: a planar background. */

    /*@{*/
    float background_planar_width;      /**< \brief Width of the plane, in meters. */
    float background_planar_height;     /**< \brief Height of the plane, in meters. */
    float background_planar_dist;       /**< \brief Distance of the plane, in meters. */
    /*@}*/

public:
    /** \brief Constructor
     *
     * Fills in default values, defined in the implementation.
     */
    Target(variant_t v = variant_star);

    /** \brief Save target description to a file
     *
     * This throws a std::exception on failure. */
    void save(const std::string& filename) const;

    /** \brief Load target description from a file
     *
     * This throws a std::exception on failure. */
    void load(const std::string& filename);
};

#endif
