/*
 * Copyright (C) 2012, 2013, 2014
 * Computer Graphics Group, University of Siegen, Germany.
 * http://www.cg.informatik.uni-siegen.de/
 * All rights reserved.
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>.
 */

#version 120

uniform sampler2D tex;

uniform float minval;
uniform float maxval;
uniform int channel;
uniform int dynamic_range_reduction;

void main()
{
    float v = texture2D(tex, gl_TexCoord[0].xy)[channel];
    v = clamp(v, minval, maxval);
    v = (v - minval) / (maxval - minval);

    if (bool(dynamic_range_reduction)) {
        // Schlick Uniform Rational Quantization.
        // See HDRI book by Reinhard, Ward, Pattanaik, Debevec
        float p = 10.0 * (maxval - minval); // Automatic parameter setting; works well here
        v = (p * v) / ((p - 1.0) * v + 1.0);
        v = clamp(v, 0.0, 1.0);
    }

    gl_FragColor = vec4(v, v, v, 1.0);
}
