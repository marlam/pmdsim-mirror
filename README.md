# pmdsim

This is a simulator for continuous-wave Time-of-Flight sensors, specifically
PMD sensors.

It implements the illumination and sensor models described in this paper:
M. Lambers, S. Hoberg, A. Kolb: [Simulation of Time-of-Flight Sensors for Evaluation of
Chip Layout Variants](http://ieeexplore.ieee.org/xpl/articleDetails.jsp?reload=true&arnumber=7054461).
In IEEE Sensors Journal, 15(7), 2015, pages 4019-4026.

This software was developed at the [Computer Graphics Group, University of
Siegen](http://www.cg.informatik.uni-siegen.de),
in collaboration with [pmdtec](http://pmdtec.com).

PMDSim is free software, licensed under the terms of the GNU GPL version 3 or
later.

**Please note that PMDSim is superseded by [CamSim](https://marlam.de/camsim).**

## Building 

This software should build and run on any operating system.
The build system is based on [CMake](http://www.cmake.org/).
You need the following libraries:
- [Qt](https://www.qt.io/) version 4 (version 5 is not supported)
- [GLEW](http://glew.sourceforge.net/)
- [OpenSceneGraph](http://www.openscenegraph.com/) with the OSG/Qt module

If [Doxygen](http://www.stack.nl/~dimitri/doxygen/) is available, the HTML
documentation will be generated as well.

## Using

Starting the program gives you this screen:
![GUI screen shot](https://marlam.de/pmdsim/pmdsim-screenshot.png)

In the top left view, you see the current scene, consisting of a *target*
(default: a Siemens star) and a *background* (default: nothing). You can use
mouse and space bar to navigate within the scene. You can load different targets
and backgrounds from OBJ files, or create predefined geometries, using the
Target and Background menus.

In the top middle view, you see the Ground Truth range information. This is
what an ideal camera would give you.

In the top right view, you see the four phase images that the simulated PMD
sensor produces for the scene. You can change simulation parameters from the
Simulator menu.

In the bottom row, you see the simulated results: range image (left), amplitude
image (middle), and intensity image (right). Depending on simulation parameters,
the range image shows the typical errors (flying pixels, motion artefacts, ...).

The scene is static by default, but you can animate it, either by using the
mouse in the top left view to generate motion in the scene, or by loading an
animation file from the Animation menu.

You can export the current frame or all frames of the current animation from
the File menu. For each frame, you get the following files:
- Ideal depth for phase images 0, 1, 2, 3: `raw-depth-*.csv`
- Ideal energy that reaches each PMD pixel for phase images 0, 1, 2, 3: `raw-energy-*.csv`
- Simulated phase images 0, 1, 2, 3, A tap and B tap: `sim-phase-a-*.csv` and `sim-phase-b-*.csv`
- Simulated depth, amplitude, and intensity: `sim-depth.csv`, `sim-amplitude.csv`, `sim-intensity.csv`

The `pmdsim` executable supports the following command line options
for automated tests and evaluations:
- `--simulator=FILE.TXT`: load a simulator specification
- `--background=FILE.TXT`: load a background specification
- `--target=FILE.TXT`: load a target specification
- `--animation=FILE.TXT`: load an animation specification
- `--export-dir=DIR`: export file to the given directory
- `--export-animation`: export all frames of the animation and quit
- `--export-frame=TIMESTAMP`: export the frame nearest to the given timestamp (in seconds) and quit
- `--minimize`: start with minimized window and without progress dialogues.
