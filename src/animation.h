/*
 * Copyright (C) 2012, 2013, 2014
 * Computer Graphics Group, University of Siegen, Germany.
 * http://www.cg.informatik.uni-siegen.de/
 * All rights reserved.
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>.
 */

#ifndef ANIMATION_H
#define ANIMATION_H

/**
 * \file animation.h
 * \brief The animation description.
 *
 * This file documents the Animation class which
 * describes an animation, i.e. a movement of a target over time.
 *
 * \page animation-example.txt Example animation definition
 * \include animation-example.txt
 */

#include <cassert>
#include <vector>

/**
 * \brief The Animation class.
 *
 * This class describes an animation through a set key frames, which are points
 * in time at which position and orientation of a target is known.
 *
 * Position and orientation at arbitrary points in time are interpolated from
 * these key frames.
 *
 * Positions are interpolated linearly, and orientations are interpolated via
 * spherical linear interpolation (slerp).
 *
 * The position of a target is measured relative to the camera, and its
 * orientation is given by a rotation angle around a rotation axis. The camera
 * always looks in direction of the negative z axis, and the "up" direction is
 * always the positive y axis.
 *
 * Examples:
 * - Target position (0, 0, -0.2) means that the origin of the target is
 *   directly in front of the camera at 20cm distance.
 * - Zero rotation means that the target is upright, while a rotation of 180
 *   degrees around the z axis means that it is upside down.
 *
 * Example animation definition file: \link animation-example.txt \endlink
 */
class Animation
{
public:
    /**
     * \brief The Keyframe class.
     *
     * This class describes one key frame, consisting of a point in time and
     * position and orientation of a target at this point in time.
     */
    class Keyframe
    {
    public:
        long long t;            /**< \brief Key frame time in microseconds. */
        float pos[3];           /**< \brief Position of the target at time t, relative to the camera, in meters. */
        float rot[4];           /**< \brief Rotation of the target at time t, relative to the upright orientation, given as
                                  a quaternion. (The angle/axis representation is only used in the file format!) */
    };

    /**
     * \brief
     *
     * The list of keyframes, sorted by ascending time.
     */
    std::vector<Keyframe> keyframes;

public:
    /** \brief Constructor
     *
     * Constructs an empty animation (no keyframes).
     */
    Animation();

    /** \brief Test validity of this animation.
     *
     * Returns true if this animation is valid, i.e. if it contains at least
     * one keyframe.
     */
    bool is_valid() const
    {
        return keyframes.size() > 0;
    }

    /** \brief Get start time of animation.
     *
     * Returns the time of the first keyframe.
     * The animation must be valid!
     */
    long long start_time() const
    {
        assert(is_valid());
        return keyframes.front().t;
    }

    /** \brief Get end time of animation.
     *
     * Returns the time of the first keyframe.
     * The animation must be valid!
     */
    long long end_time() const
    {
        assert(is_valid());
        return keyframes.back().t;
    }

    /** \brief Interpolate position and orientation.
     *
     * \param t         Point in time, in microseconds
     * \param pos       The position, in meters
     * \param rot       The orientation as a quaternion
     *
     * Returns position and orientation at the given point in time.
     * The animation must be valid!
     */
    void interpolate(long long t, float pos[3], float rot[4]);

    /** \brief Load animation description from a file
     *
     * This throws a std::exception on failure. */
    void load(const std::string& filename);
};

#endif
