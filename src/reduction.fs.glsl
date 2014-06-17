/*
 * Copyright (C) 2012, 2013, 2014
 * Computer Graphics Group, University of Siegen, Germany.
 * http://www.cg.informatik.uni-siegen.de/
 * All rights reserved.
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>.
 */

#version 120

uniform sampler2D oversampled_map_tex; // The oversampled map
uniform sampler2D pixel_map_tex;       // The pixel map (size pixel_width x pixel_height)

uniform int pixel_width;    // width of a sensor pixel, in subpixels
uniform int pixel_height;   // height of a sensor pixel, in subpixels
uniform vec2 subpixel_size; // = 1 / map_tex size

void main(void)
{
    // The raw depth of the complete sensor pixel.
    // This must not be averaged over subpixels; instead, we need the center value.
    float pixel_depth = texture2D(oversampled_map_tex, gl_TexCoord[0].xy).z;
    // Loop over all subpixels to compute the remaining values.
    float pixel_energy_a = 0.0;
    float pixel_energy_b = 0.0;
    float pixel_energy = 0.0;
    for (int y = 0; y < pixel_height; y++) {
        for (int x = 0; x < pixel_width; x++) {
            // Get photon-sensitive area of this subpixel from pixel map
            float active_area_fraction = texture2D(pixel_map_tex,
                    vec2((float(x) + 0.5) / float(pixel_width), (float(y) + 0.5) / float(pixel_height))).r;
            // Get information from the map for this subpixel
            vec2 subpixel_center = gl_TexCoord[0].xy
                + subpixel_size * vec2(x - pixel_width / 2, y - pixel_height / 2);
            vec4 mapval = texture2D(oversampled_map_tex, subpixel_center).xyzw;
            float energy_a = mapval.x;
            float energy_b = mapval.y;
            float energy = mapval.w;
            // Add subpixel info to pixel
            pixel_energy_a += active_area_fraction * energy_a;
            pixel_energy_b += active_area_fraction * energy_b;
            pixel_energy += active_area_fraction * energy;
        }
    }

    gl_FragColor = vec4(pixel_energy_a, pixel_energy_b, pixel_depth, pixel_energy);
}
