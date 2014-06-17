/*
 * Copyright (C) 2012, 2013, 2014
 * Computer Graphics Group, University of Siegen, Germany.
 * http://www.cg.informatik.uni-siegen.de/
 * All rights reserved.
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>.
 */

#version 120

uniform float lightsource_intensity;    // light source intensity in milliwatt/steradian
uniform sampler2D lightsource_intensity_table;
uniform float lightsource_intensity_table_start_x;
uniform float lightsource_intensity_table_end_x;
uniform float lightsource_intensity_table_start_y;
uniform float lightsource_intensity_table_end_y;
uniform float lambertian_reflectivity;  // material reflection coefficient in [0,1]
uniform float frac_apdiam_foclen;       // lens: aperture diameter / focal length
uniform float frac_modfreq_c;           // modulation_frequency / speed of light
uniform float exposure_time;            // exposure time in microseconds
uniform float pixel_area;               // area of a sensor pixel in micrometer²
uniform int pixel_width;                // width of a sensor pixel, counted in subpixels
uniform int pixel_height;               // height of a sensor pixel, counted in subpixels
uniform float contrast;                 // achievable demodulation contrast, in [0,1]

uniform float tau;                      // Phase i: tau=i*pi/2

const float pi = 3.14159265358979323846;

varying vec3 vp;    // Position in eye space
varying vec3 vn;    // Normal in eye space, not normalized


void main(void)
{
    vec3 n = normalize(vn);
    vec3 l = normalize(-vp);
    vec3 p = normalize(vp);
    float depth = length(vp);
    float cos_theta_surface = clamp(dot(l, n), 0.0, 1.0);
    float cos_theta_sensor = clamp(dot(p, vec3(0.0, 0.0, -1.0)), 0.0, 1.0);

    float li = lightsource_intensity;
    if (li < 0.0f) { // this means we have to read a measured value
        vec3 p_xz = normalize(vec3(p.x, 0.0, p.z));
        float theta_sensor_x = acos(clamp(dot(p_xz, vec3(0.0, 0.0, -1.0)), 0.0, 1.0));
        if (p.x < 0.0)
            theta_sensor_x = -theta_sensor_x;
        float theta_sensor_y = acos(clamp(dot(p, p_xz), 0.0, 1.0));
        if (p.y < 0.0)
            theta_sensor_y = -theta_sensor_y;
        float table_ind_x = (theta_sensor_x - lightsource_intensity_table_start_x) /
            (lightsource_intensity_table_end_x - lightsource_intensity_table_start_x);
        float table_ind_y = (theta_sensor_y - lightsource_intensity_table_start_y) /
            (lightsource_intensity_table_end_y - lightsource_intensity_table_start_y);
        // swap directions, necessary for plausible orientation of the table
        table_ind_x = 1.0 - table_ind_x;
        table_ind_y = 1.0 - table_ind_y;
        li = texture2D(lightsource_intensity_table, vec2(table_ind_x, table_ind_y)).r;
    }
    // li: [mW/sr]

    float irradiance_surface = li * cos_theta_surface / (depth * depth); // [mW/m²]
    float radiosity_surface = lambertian_reflectivity * irradiance_surface; // [mW/m²]
    float radiance_to_sensor = radiosity_surface / pi; // [mW/sr/m²]
    float irradiance_sensor = (pi / 4.0) * (frac_apdiam_foclen * frac_apdiam_foclen)
        * (cos_theta_sensor * cos_theta_sensor * cos_theta_sensor * cos_theta_sensor)
        * radiance_to_sensor; // [mW/m²]

    float power_sensor = irradiance_sensor * (pixel_area / float(pixel_width * pixel_height));
        // [(mW/m²) * (1e-6m)²] = [1e-3 * 1e-6 * 1e-6 W] = [1e-15 W] = [Femtowatt]

    float energy = power_sensor * exposure_time;
        // [1e-15 W * 1e-6 s] = [1e-21 J] = [Zeptojoule]

    // You can compute e.g. the total accumulated charge from this if you want.

    float phase_shift = 2.0 * pi * (2.0 * depth) * frac_modfreq_c;
    float energy_a = energy / 2.0 * (1.0 + contrast * cos(tau + phase_shift));
    float energy_b = energy / 2.0 * (1.0 - contrast * cos(tau + phase_shift));

    gl_FragColor = vec4(energy_a, energy_b, depth, energy);
}
