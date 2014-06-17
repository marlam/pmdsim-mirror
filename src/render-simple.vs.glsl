/*
 * Copyright (C) 2012, 2013, 2014
 * Computer Graphics Group, University of Siegen, Germany.
 * http://www.cg.informatik.uni-siegen.de/
 * All rights reserved.
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>.
 */

#version 120

varying vec3 vp;    // Position in eye space
varying vec3 vn;    // Normal in eye space, not normalized

void main(void)
{
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

    // The modelview matrix gives us camera space.
    // The camera is thus always in (0,0,0), and we assume
    // the light source is also always in (0,0,0).

    vp = (gl_ModelViewMatrix * gl_Vertex).xyz;
    vn = gl_NormalMatrix * gl_Normal;
}
