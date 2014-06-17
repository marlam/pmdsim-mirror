/*
 * Copyright (C) 2013, 2014
 * Computer Graphics Group, University of Siegen, Germany.
 * http://www.cg.informatik.uni-siegen.de/
 * All rights reserved.
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>.
 */

#ifndef TRIANGLEPATCH_H
#define TRIANGLEPATCH_H

#include <vector>

/**
 * \file trianglepatch.h
 * \brief A triangle patch that represents part of the scene.
 *
 * This file documents the TrianglePatch class. A triangle patch is a set of
 * triangles that share a common transformation matrix (e.g. one object in
 * the scene).
 */

/**
 * \brief The TrianglePatch class.
 *
 * A triangle patch is given by vertices with associated normals, texture
 * coordinates, and colors. A texture can be associated with a triangle patch.
 * All triangles in the patch share the same transformation matrix.
 *
 * If a vertex attribute is not present, the corresponding array is empty. Otherwise,
 * the attribute must be available for each vertex.
 */
class TrianglePatch
{
public:
    std::vector<float> vertex_array;            /**< \brief Vertex information (x, y, z) */
    std::vector<float> normal_array;            /**< \brief Vertex attribute: normals (n.x, n.y, n.z) */
    std::vector<float> color_array;             /**< \brief Vertex attribute: color (r, g, b, a) */
    std::vector<float> texcoord_array;          /**< \brief Vertex attribute: texture coordinates (s, t) */
    std::vector<unsigned int> index_array;      /**< \brief Indices into the arrays, for rendering */
    float transformation[16];                   /**< \brief Transformation matrix, 4x4 column-major */
    unsigned int texture;                       /**< \brief Texture associated with this patch, or zero. */

public:
    /** \brief Constructor
     *
     * Constructs an empty patch.
     */
    TrianglePatch();
};

#endif
