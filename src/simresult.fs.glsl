/*
 * Copyright (C) 2012, 2013, 2014
 * Computer Graphics Group, University of Siegen, Germany.
 * http://www.cg.informatik.uni-siegen.de/
 * All rights reserved.
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>.
 */

#version 120

uniform sampler2D phase_texs[4];

// Simulator properties
uniform float frac_c_modfreq;

// Constants
const float pi = 3.14159265358979323846;

void main(void)
{
    // Values energy_a and energy_b from the four phase images
    float A[4], B[4];
    // The difference between energy_a and energy_b;
    float D[4];

    vec2 tc = gl_TexCoord[0].xy;
    for (int i = 0; i < 4; i++) {
        vec2 P = texture2D(phase_texs[i], tc).rg;
        A[i] = P[0];
        B[i] = P[1];
        D[i] = A[i] - B[i];
    }

    float pmd_depth;
    if (abs(D[0] - D[2]) <= 0.0 && abs(D[1] - D[3]) <= 0.0) {
        pmd_depth = 0.0;
    } else {
        /*
        float phase_shift = atan(D[0] - D[2], D[1] - D[3]) - pi / 2.0;
        if (phase_shift < 0.0)
            phase_shift += 2.0 * pi;
        */
        float phase_shift = atan(D[3] - D[1], D[0] - D[2]);
        pmd_depth = frac_c_modfreq * phase_shift / (4.0 * pi);
    }
    float pmd_amp = sqrt((D[0] - D[2]) * (D[0] - D[2]) + (D[1] - D[3]) * (D[1] - D[3])) * pi / 2.0;
    float pmd_intensity = (D[0] + D[1] + D[2] + D[3]) / 2.0;

    gl_FragColor = vec4(pmd_depth, pmd_amp, pmd_intensity, 0.0);
}
