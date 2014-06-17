/**
 * \mainpage PMDSim Documentation
 *
 * \section overview Source Code Overview
 *
 * The simulator pipeline starts with a 3D representation of the current target
 * as seen by the sensor. This scene is managed by OpenSceneGraph (OSG). OSG is
 * not used in any simulation steps; instead, the current state of the scene is
 * extracted from OSG into triangle patches (see <code>trianglepatch.*</code>).
 *
 * The triangle patches are then rendered into a map using spatial
 * oversampling: each sensor pixel is represented by a number of subpixels,
 * for accurate simulation of flying pixels and similar effects.
 *
 * The map stores the energies energy_a and energy_b and some additional information.
 * The oversampled energy_a/energy_b map is then reduced to sensor resolution using
 * averaging and subpixel sensor geometry information.
 *
 * Several of these reduced energy_a/energy_b maps, representing different points in time,
 * are then averaged to obtain one phase image that is oversampled in the time
 * domain, for accurate simulation of motion artefacts.
 *
 * Once four phase images are computed, the final PMD output frame is computed
 * from them. This frame contains depth, amplitude, and intensity values.
 *
 * All of the above steps are implemented in <code>simwidget.*</code>.
 *
 * The main simulation loop is implemented in <code>MainWindow::simulation_step()</code>
 * in <code>mainwindow.cpp</code>. This loop is triggered by a Qt timer.
 *
 * All OpenGL widgets use sharing contexts, so that the simulation input and output
 * are available to all of them without duplication, while each widget can still alter
 * its own OpenGL state.
 *
 * The central classes that define the simulation process are defined in
 * <code>simulator.cpp</code>, <code>target.cpp</code>, and <code>animation.cpp</code>.
 *
 * There are a few additional source files that implement helper functions, e.g.
 * for viewing the simulated data.
 *
 *
 * \section build Building and Installing
 *
 * The build system is based on <a href="http://www.cmake.org/">CMake</a>.
 * Required libraries are <a href="http://qt-project.org/">Qt</a> version 4,
 * <a href="http://glew.sourceforge.net/">GLEW</a> (MX variant), and
 * <a href="http://www.openscenegraph.com/">OpenSceneGraph</a>.
 * If <a href="http://www.stack.nl/~dimitri/doxygen/">Doxygen</a> is available,
 * then HTML documentation will be generated as part of the build process.
 *
 * <ul>
 * <li>Linux
 *
 * On a current Debian or Ubuntu system, the following packages should be
 * installed to build PMDSim: g++ make cmake cmake-curses-gui libqt4-dev
 * libglewmx-dev libopenscenegraph-dev doxygen.
 *
 * \code
 * $ tar xvf pmdsim-0.1.tar.xz
 * $ cd pmdsim-0.1
 * $ mkdir build
 * $ cd build
 * $ cmake ..
 * $ make
 * $ ./pmdsim
 * \endcode
 *
 * </li>
 * <li>Visual Studio
 *
 * Visual Studio 10 is tested. Note that the precompiled OpenSceneGraph
 * packages lack support for the OSG/Qt module, so it might be necessary to
 * recompile OpenSceneGraph after installing Qt. *
 *
 * </li></ul>
 *
 *
 * \section sta Simulator, Target, and Animation
 *
 * See the documentation of the \link Simulator \endlink, \link Target \endlink,
 * and \link Animation \endlink classes for a description of parameters.
 *
 *
 * \section export Export
 *
 * All simulation data can be exported to CSV files. To convert the data to
 * different file formats (e.g. TIFF, PFS, Matlab .mat),
 * <a href="http://gta.nongnu.org/gtatool.html">gtatool</a> can be
 * used.
 *
 * The export functions in the File menu export the data to a set of files in a
 * directory, so you must choose a directory (and not a file) in their dialogs.
 *
 * The set of files for a single frame are the following:
 * - raw-depth-0.csv, raw-depth-1.csv, raw-depth-2.csv, raw-depth-3.csv\n
 *   Ideal depth for phases 0, 1, 2, 3.
 * - raw-energy-0.csv, raw-energy-1.csv, raw-energy-2.csv, raw-energy-3.csv\n
 *   Ideal energy that reaches a PMD pixel for phases 0, 1, 2, 3.
 *   Note that this includes masked areas of the PMD pixel.
 * - sim-phase-a-0.csv, sim-phase-a-1.csv, sim-phase-a-2.csv, sim-phase-a-3.csv,
 *   sim-phase-b-0.csv, sim-phase-b-1.csv, sim-phase-b-2.csv, sim-phase-b-3.csv\n
 *   Simulated phase images, energy_a and energy_b parts.
 * - sim-depth.csv, sim-amplitude.csv, sim-intensity.csv\n
 *   Simulated depth, amplitude, and intensity data.
 *
 * For static scenes, the file sets for the four phase images are identical.
 *
 * When animations are exported, the file structure is the same, but each file name
 * is preprended with the frame number.
 *
 *
 * \section scripting Scripting
 *
 * The <code>pmdsim</code> executable supports the following command line options
 * for automated tests and evaluations. Note that the '=' character is required!
 *
 * <ul>
 * <li><code>-</code><code>-simulator=&lt;FILE.TXT&gt;</code><br>
 *     Load the specified simulator description file.</li>
 * <li><code>-</code><code>-background=&lt;FILE.TXT&gt;</code><br>
 *     Load the specified background description file.</li>
 * <li><code>-</code><code>-target=&lt;FILE.TXT&gt;</code><br>
 *     Load the specified target description file.</li>
 * <li><code>-</code><code>-animation=&lt;FILE.TXT&gt;</code><br>
 *     Load the specified animation description file.</li>
 * <li><code>-</code><code>-export-dir=&lt;DIR&gt;</code><br>
 *     Use the given directory for export files. The default is the current directory.</li>
 * <li><code>-</code><code>-export-animation</code><br>
 *     Export all frames of the animation, and quit.</li>
 * <li><code>-</code><code>-export-frame=&lt;TIMESTAMP&gt;</code><br>
 *     Export only the frame nearest to the given timestamp (in seconds), and quit.</li>
 * <li><code>-</code><code>-minimize</code><br>
 *     Start with the window minimized, and without showing progress dialogs.
 *     Useful if you don't want your work interrupted by pmdsim instances starting
 *     from a script.</li>
 * </ul>
 */
