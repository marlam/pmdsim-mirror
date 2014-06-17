/*
 * Copyright (C) 2013, 2014
 * Computer Graphics Group, University of Siegen, Germany.
 * http://www.cg.informatik.uni-siegen.de/
 * All rights reserved.
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>.
 */

#version 120

uniform sampler2D phase_tex_0; // First phase tex
uniform sampler2D phase_tex_1; // Second phase tex

uniform bool have_phase_tex_0;

void main(void)
{
    if (have_phase_tex_0) {
        vec4 p0 = texture2D(phase_tex_0, gl_TexCoord[0].xy);
        vec4 p1 = texture2D(phase_tex_1, gl_TexCoord[0].xy);
        // Contents: energy_a, energy_b, raw_depth, raw_energy.
        // We can add all components except for raw_depth: this must be unchanged from
        // the original value.
        gl_FragColor = vec4(p0.x + p1.x, p0.y + p1.y, p1.z, p0.w + p1.w);
    } else {
        // In the initial step, there's nothing to add yet; just copy
        gl_FragColor = texture2D(phase_tex_1, gl_TexCoord[0].xy);
    }
}
