/*
 * Copyright (C) 2012, 2013, 2014
 * Computer Graphics Group, University of Siegen, Germany.
 * http://www.cg.informatik.uni-siegen.de/
 * All rights reserved.
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>.
 */

#include <stdexcept>
#include <system_error>
#include <cerrno>
#include <cstring>
#include <clocale>
#include <cmath>
#include <cstdio>

#include <GL/glew.h>

#include <QMainWindow>
#include <QCloseEvent>
#include <QSettings>
#include <QGridLayout>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QTimer>
#include <QGLFormat>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QApplication>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QComboBox>
#include <QGroupBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QLabel>
#include <QElapsedTimer>
#include <QProgressDialog>
#include <QThread>
#include <QLineEdit>
#include <QtCore>

#ifdef HAVE_GTA
#  include <gta/gta.hpp>
#endif

#include "mainwindow.h"
#include "simwidget.h"
#include "osgwidget.h"
#include "view2dwidget.h"
#include "animwidget.h"


MainWindow::MainWindow(
            QString script_simulator_file,
            QString script_background_file,
            QString script_target_file,
            QString script_animation_file,
            QString script_export_dir,
            bool script_export_animation,
            double script_export_frame,
            bool script_minimize_window) :
    QMainWindow(NULL)
{
    bool script_mode = (!script_simulator_file.isEmpty()
            || !script_background_file.isEmpty()
            || !script_target_file.isEmpty()
            || !script_animation_file.isEmpty()
            || !script_export_dir.isEmpty()
            || script_export_animation
            || std::isfinite(script_export_frame)
            || script_minimize_window);

    // Set application properties
    setWindowTitle("PMDSim");
    setWindowIcon(QIcon(":appicon.png"));
    _settings = new QSettings();

    // Restore window settings
    if (script_mode && script_minimize_window) {
        // XXX: We don't want the window to steal focus from the foreground application,
        // but none of the following seem to prevent that...
        setFocusPolicy(Qt::NoFocus);
        setAttribute(Qt::WA_ShowWithoutActivating);
        setAttribute(Qt::WA_X11DoNotAcceptFocus);
        setWindowFlags(Qt::WindowStaysOnBottomHint);
    } else {
        _settings->beginGroup("MainWindow");
        restoreGeometry(_settings->value("geometry").toByteArray());
        restoreState(_settings->value("window_state").toByteArray());
        _settings->endGroup();
    }

    // Create widgets
    _scene_id = 0;
    QGLFormat fmt(QGL::DoubleBuffer | QGL::DepthBuffer | QGL::Rgba | QGL::DirectRendering
            | QGL::NoSampleBuffers | QGL::NoAlphaChannel | QGL::NoAccumBuffer
            | QGL::NoStencilBuffer | QGL::NoStereoBuffers | QGL::NoOverlay | QGL::NoSampleBuffers);
    fmt.setSwapInterval(0);
    QGLFormat::setDefaultFormat(fmt);
    connect(this, SIGNAL(update_scene(const Target&, const Target&)), this, SLOT(reset_scene()));
    _sim_widget = new SimWidget();
    connect(this, SIGNAL(update_simulator(const Simulator&)), _sim_widget, SLOT(update_simulator(const Simulator&)));
    _osg_widget = new OSGWidget(_sim_widget);
    connect(this, SIGNAL(update_simulator(const Simulator&)), _osg_widget, SLOT(update_simulator(const Simulator&)));
    connect(this, SIGNAL(update_scene(const Target&, const Target&)), _osg_widget, SLOT(update_scene(const Target&, const Target&)));
    _depthmap_widget = new View2DWidget(_sim_widget);
    connect(this, SIGNAL(update_simulator(const Simulator&)), _depthmap_widget, SLOT(update_simulator(const Simulator&)));
    _phase_widgets[0] = new View2DWidget(_sim_widget);
    connect(this, SIGNAL(update_simulator(const Simulator&)), _phase_widgets[0], SLOT(update_simulator(const Simulator&)));
    _phase_widgets[1] = new View2DWidget(_sim_widget);
    connect(this, SIGNAL(update_simulator(const Simulator&)), _phase_widgets[1], SLOT(update_simulator(const Simulator&)));
    _phase_widgets[2] = new View2DWidget(_sim_widget);
    connect(this, SIGNAL(update_simulator(const Simulator&)), _phase_widgets[2], SLOT(update_simulator(const Simulator&)));
    _phase_widgets[3] = new View2DWidget(_sim_widget);
    connect(this, SIGNAL(update_simulator(const Simulator&)), _phase_widgets[3], SLOT(update_simulator(const Simulator&)));
    _pmd_depth_widget = new View2DWidget(_sim_widget);
    connect(this, SIGNAL(update_simulator(const Simulator&)), _pmd_depth_widget, SLOT(update_simulator(const Simulator&)));
    _pmd_amp_widget = new View2DWidget(_sim_widget);
    connect(this, SIGNAL(update_simulator(const Simulator&)), _pmd_amp_widget, SLOT(update_simulator(const Simulator&)));
    _pmd_intensity_widget = new View2DWidget(_sim_widget);
    connect(this, SIGNAL(update_simulator(const Simulator&)), _pmd_intensity_widget, SLOT(update_simulator(const Simulator&)));
    _anim_widget = new AnimWidget();
    connect(this, SIGNAL(update_animation(const Animation&)), _anim_widget, SLOT(update_animation(const Animation&)));
    connect(_anim_widget, SIGNAL(update_state(AnimWidget::state_t)), this, SLOT(animation_state_changed()));
    connect(_anim_widget, SIGNAL(update_time(long long)), this, SLOT(animation_time_changed(long long)));

    _background = Target(Target::variant_background_planar);
    if (script_mode) {
        // Load scripting parameters
        try {
            if (!script_simulator_file.isEmpty())
                _simulator.load(script_simulator_file.toLocal8Bit().constData());
            if (!script_background_file.isEmpty())
                _background.load(script_background_file.toLocal8Bit().constData());
            if (!script_target_file.isEmpty())
                _target.load(script_target_file.toLocal8Bit().constData());
            if (!script_animation_file.isEmpty())
                _animation.load(script_animation_file.toLocal8Bit().constData());
        }
        catch (std::exception& e) {
            QMessageBox::critical(this, "Error", e.what());
            std::exit(1);
        }
        emit update_simulator(_simulator);
        emit update_scene(_background, _target);
        emit update_animation(_animation);
    } else {
        // Restore last simulator, background, target, and animation from the last session
        QString simulator_filename = _settings->value("Session/simulator").toString();
        if (!simulator_filename.isEmpty()) {
            try { _simulator.load(simulator_filename.toLocal8Bit().constData()); } catch (...) { }
        }
        emit update_simulator(_simulator);
        QString background_filename = _settings->value("Session/background").toString();
        if (!background_filename.isEmpty()) {
            try { _background.load(background_filename.toLocal8Bit().constData()); } catch (...) { }
        }
        QString target_filename = _settings->value("Session/target").toString();
        if (!target_filename.isEmpty()) {
            try { _target.load(target_filename.toLocal8Bit().constData()); } catch (...) { }
        }
        emit update_scene(_background, _target);
        QString animation_filename = _settings->value("Session/animation").toString();
        if (!animation_filename.isEmpty()) {
            try { _animation.load(animation_filename.toLocal8Bit().constData()); } catch (...) { }
        }
        emit update_animation(_animation);
    }

    // Create central widget
    QWidget* widget = new QWidget;
    QGridLayout* row0_layout = new QGridLayout;
    row0_layout->addWidget(_anim_widget, 0, 0);
    QGridLayout* row1_layout = new QGridLayout;
    row1_layout->addWidget(_osg_widget, 0, 0, 2, 2);
    row1_layout->addWidget(_depthmap_widget, 0, 2, 2, 2);
    row1_layout->addWidget(_phase_widgets[0], 0, 4);
    row1_layout->addWidget(_phase_widgets[1], 0, 5);
    row1_layout->addWidget(_phase_widgets[2], 1, 4);
    row1_layout->addWidget(_phase_widgets[3], 1, 5);
    QGridLayout* row2_layout = new QGridLayout;
    row2_layout->addWidget(_pmd_depth_widget, 0, 0);
    row2_layout->addWidget(_pmd_amp_widget, 0, 1);
    row2_layout->addWidget(_pmd_intensity_widget, 0, 2);
    QGridLayout* layout = new QGridLayout;
    layout->addLayout(row0_layout, 0, 0);
    layout->addLayout(row1_layout, 1, 0);
    layout->addLayout(row2_layout, 2, 0);
    layout->setRowStretch(1, 1);
    layout->setRowStretch(2, 1);
    widget->setLayout(layout);
    setCentralWidget(widget);

    /* Create menus */

    // File menu
    QMenu* file_menu = menuBar()->addMenu("&File");
    QAction* file_export_frame_act = new QAction("&Export current frame...", this);
    file_export_frame_act->setShortcut(tr("Ctrl+E"));
    connect(file_export_frame_act, SIGNAL(triggered()), this, SLOT(file_export_frame()));
    file_menu->addAction(file_export_frame_act);
    QAction* file_export_anim_act = new QAction("Export &all frames...", this);
    file_export_anim_act->setShortcut(tr("Ctrl+A"));
    connect(file_export_anim_act, SIGNAL(triggered()), this, SLOT(file_export_anim()));
    file_menu->addAction(file_export_anim_act);
    file_menu->addSeparator();
    QAction* quit_act = new QAction("&Quit...", this);
    quit_act->setShortcut(QKeySequence::Quit);
    connect(quit_act, SIGNAL(triggered()), this, SLOT(close()));
    file_menu->addAction(quit_act);
    // Simulator menu
    QMenu* simulator_menu = menuBar()->addMenu("&Simulator");
    QAction* simulator_load_act = new QAction("&Load...", this);
    connect(simulator_load_act, SIGNAL(triggered()), this, SLOT(simulator_load()));
    simulator_menu->addAction(simulator_load_act);
    QAction* simulator_save_act = new QAction("&Save...", this);
    connect(simulator_save_act, SIGNAL(triggered()), this, SLOT(simulator_save()));
    simulator_menu->addAction(simulator_save_act);
    simulator_menu->addSeparator();
    QAction* simulator_edit_act = new QAction("&Edit...", this);
    connect(simulator_edit_act, SIGNAL(triggered()), this, SLOT(simulator_edit()));
    simulator_menu->addAction(simulator_edit_act);
    QAction* simulator_export_modelfile_act = new QAction("Export to &model file...", this);
    connect(simulator_export_modelfile_act, SIGNAL(triggered()), this, SLOT(simulator_export_modelfile()));
    simulator_menu->addAction(simulator_export_modelfile_act);
    simulator_menu->addSeparator();
    QAction* simulator_reset_act = new QAction("&Reset...", this);
    connect(simulator_reset_act, SIGNAL(triggered()), this, SLOT(simulator_reset()));
    simulator_menu->addAction(simulator_reset_act);
    // Background menu
    QMenu* background_menu = menuBar()->addMenu("&Background");
    QAction* background_load_act = new QAction("&Load...", this);
    connect(background_load_act, SIGNAL(triggered()), this, SLOT(background_load()));
    background_menu->addAction(background_load_act);
    QAction* background_save_act = new QAction("&Save...", this);
    connect(background_save_act, SIGNAL(triggered()), this, SLOT(background_save()));
    background_menu->addAction(background_save_act);
    background_menu->addSeparator();
    QAction* background_generate_planar_act = new QAction("Generate &planar background...", this);
    connect(background_generate_planar_act, SIGNAL(triggered()), this, SLOT(background_generate_planar()));
    background_menu->addAction(background_generate_planar_act);
    QAction* background_export_modelfile_act = new QAction("Export to &model file...", this);
    connect(background_export_modelfile_act, SIGNAL(triggered()), this, SLOT(background_export_modelfile()));
    background_menu->addAction(background_export_modelfile_act);
    background_menu->addSeparator();
    QAction* background_reset_act = new QAction("&Reset...", this);
    connect(background_reset_act, SIGNAL(triggered()), this, SLOT(background_reset()));
    background_menu->addAction(background_reset_act);
    // Target menu
    QMenu* target_menu = menuBar()->addMenu("&Target");
    QAction* target_load_act = new QAction("&Load...", this);
    connect(target_load_act, SIGNAL(triggered()), this, SLOT(target_load()));
    target_menu->addAction(target_load_act);
    QAction* target_save_act = new QAction("&Save...", this);
    connect(target_save_act, SIGNAL(triggered()), this, SLOT(target_save()));
    target_menu->addAction(target_save_act);
    target_menu->addSeparator();
    QAction* target_use_modelfile_act = new QAction("Use &model file...", this);
    connect(target_use_modelfile_act, SIGNAL(triggered()), this, SLOT(target_use_modelfile()));
    target_menu->addAction(target_use_modelfile_act);
    QAction* target_generate_bar_pattern_act = new QAction("Generate &bar pattern...", this);
    connect(target_generate_bar_pattern_act, SIGNAL(triggered()), this, SLOT(target_generate_bar_pattern()));
    target_menu->addAction(target_generate_bar_pattern_act);
    QAction* target_generate_star_pattern_act = new QAction("Generate Siemens s&tar...", this);
    connect(target_generate_star_pattern_act, SIGNAL(triggered()), this, SLOT(target_generate_star_pattern()));
    target_menu->addAction(target_generate_star_pattern_act);
    QAction* target_export_modelfile_act = new QAction("Export to &model file...", this);
    connect(target_export_modelfile_act, SIGNAL(triggered()), this, SLOT(target_export_modelfile()));
    target_menu->addAction(target_export_modelfile_act);
    target_menu->addSeparator();
    QAction* target_reset_act = new QAction("&Reset...", this);
    connect(target_reset_act, SIGNAL(triggered()), this, SLOT(target_reset()));
    target_menu->addAction(target_reset_act);
    // Animation menu
    QMenu* animation_menu = menuBar()->addMenu("&Animation");
    QAction* animation_load_act = new QAction("&Load...", this);
    connect(animation_load_act, SIGNAL(triggered()), this, SLOT(animation_load()));
    animation_menu->addAction(animation_load_act);
    animation_menu->addSeparator();
    QAction* animation_reset_act = new QAction("&Reset...", this);
    connect(animation_reset_act, SIGNAL(triggered()), this, SLOT(animation_reset()));
    animation_menu->addAction(animation_reset_act);
    // Help menu
    QMenu* help_menu = menuBar()->addMenu("&Help");
    QAction* help_about_act = new QAction("&About", this);
    connect(help_about_act, SIGNAL(triggered()), this, SLOT(help_about()));
    help_menu->addAction(help_about_act);

    if (script_mode && script_minimize_window) {
        showMinimized();
    } else {
        show();
    }

    _sim_timer = new QTimer(this);
    connect(_sim_timer, SIGNAL(timeout()), this, SLOT(simulation_step()));
    _anim_time_requested = false;
    _sim_timer->start(0);
    if (script_mode && (std::isfinite(script_export_frame) || script_export_animation)) {
        if (!_animation.is_valid()) {
            QMessageBox::critical(this, "Error", "No valid animation available.");
            std::exit(1);
        }
        _anim_widget->enable();
        QApplication::processEvents();
        if (std::isfinite(script_export_frame)) {
            _anim_widget->start();
            _anim_widget->pause();
            _anim_time_requested = true;
            _anim_time_request = script_export_frame * 1e6;
            simulation_step();
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            try {
                export_frame(script_export_dir.toLocal8Bit().constData());
            }
            catch (std::exception& e) {
                QApplication::restoreOverrideCursor();
                QMessageBox::critical(this, "Error", e.what());
                std::exit(1);
            }
            QApplication::restoreOverrideCursor();
        } else if (script_export_animation) {
            try {
                export_animation(script_export_dir.toLocal8Bit().constData(), !script_minimize_window);
            }
            catch (std::exception& e) {
                QMessageBox::critical(this, "Error", e.what());
                std::exit(1);
            }
        }
        std::exit(0);
    }
}

MainWindow::~MainWindow()
{
    delete _settings;
}

class SleepThread : public QThread
{
public:
    static void usleep(unsigned long usecs) { QThread::usleep(usecs); }
};

static void active_wait(QElapsedTimer& timer, long long until_usecs)
{
    // Allow Qt to process events while waiting, so that e.g. the OSG interaction
    // can move on. Otherwise we would always have identical phase images.
    long long usecs;
    bool waited = false;
    while ((usecs = timer.nsecsElapsed() / 1000) < until_usecs) {
        QApplication::processEvents(QEventLoop::AllEvents, (until_usecs - usecs) / 1000);
        SleepThread::usleep(10); // prevent busy looping
        waited = true;
    }
    if (!waited) {
        //fprintf(stderr, "Warning: simulation not fast enough for accurate timing\n");
        QApplication::processEvents();
    }
}

void MainWindow::simulation_step()
{
    float ambiguity_range = static_cast<double>(Simulator::c) / static_cast<double>(_simulator.modulation_frequency) * 0.5;
    float max_energy = _simulator.lightsource_simple_power * 1e4f;
    float max_pmd_amp = max_energy * static_cast<float>(M_PI) / static_cast<float>(M_SQRT1_2);
    float max_pmd_intensity = 2.0f * max_energy;

    long long anim_time = 0;
    QElapsedTimer timer;
    AnimWidget::state_t anim_state = _anim_widget->state();
    if (anim_state == AnimWidget::state_stopped) {
        anim_time = _animation.start_time();
    } else if (anim_state == AnimWidget::state_active || anim_state == AnimWidget::state_paused) {
        long long total_frame_duration = 4 * (_simulator.exposure_time + _simulator.readout_time);
        if (_anim_time_requested) {
            anim_time = ((_anim_time_request - _animation.start_time()) / total_frame_duration)
                * total_frame_duration + _animation.start_time();
            _anim_time_requested = false;
        } else {
            anim_time = _last_anim_time;
            if (anim_state != AnimWidget::state_paused)
                anim_time += total_frame_duration;
        }
        if (anim_time > _animation.end_time()) {
            if (_anim_widget->loop())
                anim_time = _animation.start_time();
            else
                anim_time = ((_animation.end_time() - _animation.start_time()) / total_frame_duration)
                    * total_frame_duration + _animation.start_time();
        }
        _last_anim_time = anim_time;
        _anim_widget->update(anim_time);
    } else {
        timer.start();
    }

    // Simulate the four phase images
    for (int i = 0; i < 4; i++) {
        long long phase_start_time = anim_time + i * (_simulator.exposure_time + _simulator.readout_time);
        for (int j = 0; j < _simulator.exposure_time_samples; j++) {
            long long phase_step_time = phase_start_time + j * _simulator.exposure_time / _simulator.exposure_time_samples;
            if (anim_state != AnimWidget::state_disabled) {
                float pos[3], rot[4];
                _animation.interpolate(phase_step_time, pos, rot);
                _osg_widget->set_fixed_target_transformation(pos, rot);
            }
            // Draw target in OSG for navigation and visual control
            _osg_widget->draw_frame();
            // Render the energy map
            if (_scene.size() == 0)
                _osg_widget->capture_scene(&_scene);
            else
                _osg_widget->update_scene(&_scene);
            _sim_widget->render_map(_scene_id, _scene, i);
            // Compute a phase image time step from the reduced map
            _sim_widget->simulate_phase_img(i, j);
            // Let time pass in free interaction mode.
            if (anim_state == AnimWidget::state_disabled) {
                long long wait_until;
                if (j < _simulator.exposure_time_samples - 1) // wait until next phase time step
                    wait_until = phase_start_time + (j + 1) * _simulator.exposure_time / _simulator.exposure_time_samples;
                else // wait until next phase start time
                    wait_until = anim_time + (i + 1) * (_simulator.exposure_time + _simulator.readout_time);
                active_wait(timer, wait_until);
            }
        }
        // Show the ideal depth from the last time sample
        _depthmap_widget->view(_sim_widget->get_map(), _simulator.map_aspect_ratio(),
                2, 0.0f, std::min(ambiguity_range, _simulator.far_plane));
        // Show the phase image
        _phase_widgets[i]->view(_sim_widget->get_phase(i), _simulator.aspect_ratio(), 0, -max_energy, +max_energy, false);
        // Let time pass in free interaction mode.
        if (anim_state == AnimWidget::state_disabled)
            active_wait(timer, (i + 1) * (_simulator.exposure_time + _simulator.readout_time));
    }
    // Compute the results from the four phase images
    _sim_widget->simulate_result();
    // Show the results
    _pmd_depth_widget->view(_sim_widget->get_result(), _simulator.aspect_ratio(),
            0, 0.0f, std::min(ambiguity_range, _simulator.far_plane));
    _pmd_amp_widget->view(_sim_widget->get_result(), _simulator.aspect_ratio(),
            1, 0.0f, max_pmd_amp);
    _pmd_intensity_widget->view(_sim_widget->get_result(), _simulator.aspect_ratio(),
            2, 0.0f, max_pmd_intensity);
    // Let time pass in free interaction mode.
    if (anim_state == AnimWidget::state_disabled)
        active_wait(timer, (3 + 1) * (_simulator.exposure_time + _simulator.readout_time));
}

void MainWindow::reset_scene()
{
    _scene.clear();
    _scene_id++;
}

void MainWindow::animation_state_changed()
{
    AnimWidget::state_t state = _anim_widget->state();
    if (state == AnimWidget::state_disabled)
        _osg_widget->set_mode(OSGWidget::mode_free_interaction);
    else
        _osg_widget->set_mode(OSGWidget::mode_fixed_target);
}

void MainWindow::animation_time_changed(long long t)
{
    _anim_time_requested = true;
    _anim_time_request = t;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Save settings
    _settings->beginGroup("MainWindow");
    _settings->setValue("geometry", saveGeometry());
    _settings->setValue("window_state", saveState());
    _settings->endGroup();

    event->accept();
}

void MainWindow::get_sim_data(int w, int h)
{
    _sim_widget->makeCurrent();

    GLint tex_bak;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &tex_bak);

    _export_phase0.resize(4 * w * h);
    _export_phase1.resize(4 * w * h);
    _export_phase2.resize(4 * w * h);
    _export_phase3.resize(4 * w * h);
    _export_result.resize(3 * w * h);

    glBindTexture(GL_TEXTURE_2D, _sim_widget->get_phase(0));
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, &_export_phase0[0]);
    glBindTexture(GL_TEXTURE_2D, _sim_widget->get_phase(1));
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, &_export_phase1[0]);
    glBindTexture(GL_TEXTURE_2D, _sim_widget->get_phase(2));
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, &_export_phase2[0]);
    glBindTexture(GL_TEXTURE_2D, _sim_widget->get_phase(3));
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, &_export_phase3[0]);
    glBindTexture(GL_TEXTURE_2D, _sim_widget->get_result());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, &_export_result[0]);

    glBindTexture(GL_TEXTURE_2D, tex_bak);
}

static std::string export_worker(const std::string& filename, const Simulator& sim, bool compute_coords, int stride, const float* data)
{
    int w = sim.sensor_width;
    int h = sim.sensor_height;
    float aa = sim.aperture_angle * static_cast<float>(M_PI) / 180.0f;
    float ar = sim.aspect_ratio();
    float top = std::tan(aa / 2.0f);    // top border of near plane at z==-1
    float right = ar * top;             // right border of near plane at z==-1

    std::string exc_what;
    try {
        FILE* f = fopen(filename.c_str(), "wb");
        if (!f) {
            throw std::system_error(errno, std::system_category(),
                    std::string("Cannot open ").append(filename));
        }
#ifdef HAVE_GTA
        gta::header hdr;
        hdr.set_dimensions(w, h);
        if (compute_coords) {
            hdr.set_components(gta::float32, gta::float32, gta::float32);
            hdr.component_taglist(0).set("INTERPRETATION", "X");
            hdr.component_taglist(1).set("INTERPRETATION", "Y");
            hdr.component_taglist(2).set("INTERPRETATION", "Z");
        } else {
            hdr.set_components(gta::float32);
        }
        hdr.set_compression(gta::xz);
        hdr.write_to(f);
        gta::io_state ios;
#endif
        for (int y = h - 1; y >= 0; y--) {
            for (int x = 0; x < w; x++) {
                if (compute_coords) {
                    float depth = data[(y * w + x) * stride];
                    float c[3] = {
                        (2.0f * (x + 0.5f) / w - 1.0f) * right,
                        (2.0f * (y + 0.5f) / h - 1.0f) * top,
                        -1.0f
                    };
                    float cl = std::sqrt(c[0] * c[0] + c[1] * c[1] + c[2] * c[2]);
                    for (int i = 0; i < 3; i++)
                        c[i] *= depth / cl;
#ifdef HAVE_GTA
                    hdr.write_elements(ios, f, 1, c);
#else
                    std::fprintf(f, "%.9g,%.9g,%.9g%s", c[0], c[1], c[2], x < w - 1 ? "," : "\r\n");
#endif
                } else {
#ifdef HAVE_GTA
                    hdr.write_elements(ios, f, 1, &data[(y * w + x) * stride]);
#else
                    std::fprintf(f, "%.9g%s", data[(y * w + x) * stride], x < w - 1 ? "," : "\r\n");
#endif
                }
            }
        }
        if (fflush(f) != 0 || ferror(f)) {
            fclose(f);
            throw std::system_error(errno, std::system_category(),
                    std::string("Cannot write ").append(filename));
        }
        fclose(f);
    }
    catch (std::exception& e) {
        exc_what = e.what();
    }
    return exc_what;
}

void MainWindow::export_frame(const std::string& dirname, int frameno)
{
    int w = _simulator.sensor_width;
    int h = _simulator.sensor_height;
    get_sim_data(w, h);
    std::string framestr;
    if (frameno >= 0)
        framestr = QString("%1").arg(QString::number(frameno), 5, QChar('0')).toStdString() + "-";
    std::string base = (dirname.empty() ? std::string(".") : dirname) + "/" + framestr;
#ifdef HAVE_GTA
    std::string ext = ".gta";
#else
    std::string ext = ".csv";
    // Force the C locale so that we get the decimal point '.'
    const char* locbak = setlocale(LC_NUMERIC, "C");
#endif
    QFuture<std::string> f_rd0 = QtConcurrent::run(export_worker, base + "raw-depth-0" + ext, _simulator, false, 4, &_export_phase0[2]);
    QFuture<std::string> f_rd1 = QtConcurrent::run(export_worker, base + "raw-depth-1" + ext, _simulator, false, 4, &_export_phase1[2]);
    QFuture<std::string> f_rd2 = QtConcurrent::run(export_worker, base + "raw-depth-2" + ext, _simulator, false, 4, &_export_phase2[2]);
    QFuture<std::string> f_rd3 = QtConcurrent::run(export_worker, base + "raw-depth-3" + ext, _simulator, false, 4, &_export_phase3[2]);
    QFuture<std::string> f_re0 = QtConcurrent::run(export_worker, base + "raw-energy-0" + ext, _simulator, false, 4, &_export_phase0[3]);
    QFuture<std::string> f_re1 = QtConcurrent::run(export_worker, base + "raw-energy-1" + ext, _simulator, false, 4, &_export_phase1[3]);
    QFuture<std::string> f_re2 = QtConcurrent::run(export_worker, base + "raw-energy-2" + ext, _simulator, false, 4, &_export_phase2[3]);
    QFuture<std::string> f_re3 = QtConcurrent::run(export_worker, base + "raw-energy-3" + ext, _simulator, false, 4, &_export_phase3[3]);
    QFuture<std::string> f_pa0 = QtConcurrent::run(export_worker, base + "sim-phase-a-0" + ext, _simulator, false, 4, &_export_phase0[0]);
    QFuture<std::string> f_pa1 = QtConcurrent::run(export_worker, base + "sim-phase-a-1" + ext, _simulator, false, 4, &_export_phase1[0]);
    QFuture<std::string> f_pa2 = QtConcurrent::run(export_worker, base + "sim-phase-a-2" + ext, _simulator, false, 4, &_export_phase2[0]);
    QFuture<std::string> f_pa3 = QtConcurrent::run(export_worker, base + "sim-phase-a-3" + ext, _simulator, false, 4, &_export_phase3[0]);
    QFuture<std::string> f_pb0 = QtConcurrent::run(export_worker, base + "sim-phase-b-0" + ext, _simulator, false, 4, &_export_phase0[1]);
    QFuture<std::string> f_pb1 = QtConcurrent::run(export_worker, base + "sim-phase-b-1" + ext, _simulator, false, 4, &_export_phase1[1]);
    QFuture<std::string> f_pb2 = QtConcurrent::run(export_worker, base + "sim-phase-b-2" + ext, _simulator, false, 4, &_export_phase2[1]);
    QFuture<std::string> f_pb3 = QtConcurrent::run(export_worker, base + "sim-phase-b-3" + ext, _simulator, false, 4, &_export_phase3[1]);
    QFuture<std::string> f_sd = QtConcurrent::run(export_worker, base + "sim-depth" + ext, _simulator, false, 3, &_export_result[0]);
    QFuture<std::string> f_sa = QtConcurrent::run(export_worker, base + "sim-amplitude" + ext, _simulator, false, 3, &_export_result[1]);
    QFuture<std::string> f_si = QtConcurrent::run(export_worker, base + "sim-intensity" + ext, _simulator, false, 3, &_export_result[2]);
    QFuture<std::string> f_sc = QtConcurrent::run(export_worker, base + "sim-coords" + ext, _simulator, true, 4, &_export_phase0[2]);
#ifdef HAVE_GTA
#else
    // Restore original locale
    setlocale(LC_NUMERIC, locbak);
#endif
    std::string result = f_rd0.result();
    if (result.empty()) result = f_rd1.result();
    if (result.empty()) result = f_rd2.result();
    if (result.empty()) result = f_rd3.result();
    if (result.empty()) result = f_re0.result();
    if (result.empty()) result = f_re1.result();
    if (result.empty()) result = f_re2.result();
    if (result.empty()) result = f_re3.result();
    if (result.empty()) result = f_pa0.result();
    if (result.empty()) result = f_pa1.result();
    if (result.empty()) result = f_pa2.result();
    if (result.empty()) result = f_pa3.result();
    if (result.empty()) result = f_pb0.result();
    if (result.empty()) result = f_pb1.result();
    if (result.empty()) result = f_pb2.result();
    if (result.empty()) result = f_pb3.result();
    if (result.empty()) result = f_sd.result();
    if (result.empty()) result = f_sa.result();
    if (result.empty()) result = f_si.result();
    if (result.empty()) result = f_sc.result();
    if (!result.empty())
        throw std::runtime_error(result);
}

void MainWindow::export_animation(const std::string& dirname, bool show_progress)
{
    QProgressDialog progress("Exporting all animation frames...", "Cancel", 0, 1000, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    _sim_timer->stop();
    _anim_widget->stop();
    _anim_widget->start();
    try {
        int frame = 0;
        long long last_anim_time;
        do {
            last_anim_time = _last_anim_time;
            simulation_step();
            if (frame == 0 || _last_anim_time > last_anim_time)
                export_frame(dirname, frame);
            frame++;
            if (show_progress)
                progress.setValue((_last_anim_time - _animation.start_time())
                        / ((_animation.end_time() - _animation.start_time()) / 1000));
            QApplication::processEvents();
        }
        while ((frame == 1 || _last_anim_time > last_anim_time) && !progress.wasCanceled());
    }
    catch (std::exception& e) {
        if (show_progress)
            progress.setValue(1000);
        throw e;
    }
    if (show_progress)
        progress.setValue(1000);
    _anim_widget->stop();
    _sim_timer->start(0);
}

void MainWindow::file_export_frame()
{
    QString dirname = QFileDialog::getExistingDirectory(this, "Export directory",
            _settings->value("Session/directory", QDir::currentPath()).toString());
    if (dirname.isEmpty())
        return;
    _settings->setValue("Session/directory", QFileInfo(dirname).path());
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    try {
        export_frame(dirname.toLocal8Bit().constData());
    }
    catch (std::exception& e) {
        QApplication::restoreOverrideCursor();
        QMessageBox::critical(this, "Error", e.what());
    }
    QApplication::restoreOverrideCursor();
}

void MainWindow::file_export_anim()
{
    if (!_animation.is_valid() || _anim_widget->state() == AnimWidget::state_disabled) {
        QMessageBox::critical(this, "Error", "Animation is not activated.");
        return;
    }
    QString dirname = QFileDialog::getExistingDirectory(this, "Export directory",
            _settings->value("Session/directory", QDir::currentPath()).toString());
    if (dirname.isEmpty())
        return;
    _settings->setValue("Session/directory", QFileInfo(dirname).path());
    try {
        export_animation(dirname.toLocal8Bit().constData());
    }
    catch (std::exception& e) {
        QMessageBox::critical(this, "Error", e.what());
    }
}

void MainWindow::simulator_load()
{
    QString filename = QFileDialog::getOpenFileName(this, "Load simulator",
            _settings->value("Session/directory", QDir::currentPath()).toString(),
            tr("Simulator descriptions (*.txt)"));
    if (filename.isEmpty())
        return;
    _settings->setValue("Session/directory", QFileInfo(filename).path());
    try {
        _simulator.load(filename.toLocal8Bit().constData());
    }
    catch (std::exception& e) {
        QMessageBox::critical(this, "Error", e.what());
        return;
    }
    emit update_simulator(_simulator);
    _settings->setValue("Session/simulator", filename);
}

void MainWindow::simulator_save()
{
    QString filename = QFileDialog::getSaveFileName(this, "Save simulator",
            _settings->value("Session/directory", QDir::currentPath()).toString(),
            tr("Simulator descriptions (*.txt)"));
    if (filename.isEmpty())
        return;
    _settings->setValue("Session/directory", QFileInfo(filename).path());
    try {
        _simulator.save(filename.toLocal8Bit().constData());
    }
    catch (std::exception& e) {
        QMessageBox::critical(this, "Error", e.what());
        return;
    }
    _settings->setValue("Session/simulator", filename);
}

class OddSpinBox : public QSpinBox
{
public:
    OddSpinBox(QWidget* parent = NULL) : QSpinBox(parent)
    {
        setSingleStep(2);
    }

    QValidator::State validate(QString& input, int &) const
    {
        int number;
        bool number_valid;
        number = input.toInt(&number_valid);
        if (number_valid && number >= minimum() && number <= maximum() && number % 2 == 1)
            return QValidator::Acceptable;
        else
            return QValidator::Invalid;
    }
};

void MainWindow::simulator_edit()
{
    QDialog* dlg = new QDialog(this);
    dlg->setWindowTitle("Edit simulator");

    int row = 0;
    QGridLayout* l0 = new QGridLayout;

    l0->addWidget(new QLabel("<b>Rasterization</b>"), row++, 0);

    l0->addWidget(new QLabel("Aperture angle [deg]:"), row, 0);
    QDoubleSpinBox* aperture_angle_spinbox = new QDoubleSpinBox;
    aperture_angle_spinbox->setDecimals(4);
    aperture_angle_spinbox->setRange(1.0, 179.0);
    aperture_angle_spinbox->setValue(_simulator.aperture_angle);
    l0->addWidget(aperture_angle_spinbox, row++, 1);
    l0->addWidget(new QLabel("Near plane [m]:"), row, 0);
    QDoubleSpinBox* near_plane_spinbox = new QDoubleSpinBox;
    near_plane_spinbox->setDecimals(4);
    near_plane_spinbox->setRange(0.01, 100.0);
    near_plane_spinbox->setValue(_simulator.near_plane);
    l0->addWidget(near_plane_spinbox, row++, 1);
    l0->addWidget(new QLabel("Far plane [m]:"), row, 0);
    QDoubleSpinBox* far_plane_spinbox = new QDoubleSpinBox;
    far_plane_spinbox->setDecimals(4);
    far_plane_spinbox->setRange(0.01, 100.0);
    far_plane_spinbox->setValue(_simulator.far_plane);
    l0->addWidget(far_plane_spinbox, row++, 1);
    l0->addWidget(new QLabel("Exposure time samples:"), row, 0);
    QSpinBox* exposure_time_samples_spinbox = new QSpinBox;
    exposure_time_samples_spinbox->setRange(1, 512);
    exposure_time_samples_spinbox->setValue(_simulator.exposure_time_samples);
    l0->addWidget(exposure_time_samples_spinbox, row++, 1);
    l0->addWidget(new QLabel("Rendering method:"), row, 0);
    QComboBox* rendering_box = new QComboBox;
    rendering_box->addItem("Default");
    rendering_box->setCurrentIndex(_simulator.rendering_method);
    l0->addWidget(rendering_box, row++, 1);

    l0->addWidget(new QLabel("<b>Material</b>"), row++, 0);

    l0->addWidget(new QLabel("Model:"), row, 0);
    QComboBox* material_model_box = new QComboBox;
    material_model_box->addItem("Lambertian");
    material_model_box->setCurrentIndex(_simulator.material_model);
    l0->addWidget(material_model_box, row++, 1);
    l0->addWidget(new QLabel("Lambertian material: reflectivity [0,1]:"), row, 0);
    QDoubleSpinBox* material_lambertian_reflectivity_spinbox = new QDoubleSpinBox;
    material_lambertian_reflectivity_spinbox->setDecimals(4);
    material_lambertian_reflectivity_spinbox->setRange(0.0, 1.0);
    material_lambertian_reflectivity_spinbox->setValue(_simulator.material_lambertian_reflectivity);
    l0->addWidget(material_lambertian_reflectivity_spinbox, row++, 1);

    l0->addWidget(new QLabel("<b>Light Source</b>"), row++, 0);

    l0->addWidget(new QLabel("Model:"), row, 0);
    QComboBox* lightsource_model_box = new QComboBox;
    lightsource_model_box->addItem("Simple");
    lightsource_model_box->addItem("Measured");
    lightsource_model_box->setCurrentIndex(_simulator.lightsource_model);
    l0->addWidget(lightsource_model_box, row++, 1);
    l0->addWidget(new QLabel("Simple model: power [mW]:"), row, 0);
    QDoubleSpinBox* lightsource_simple_power_spinbox = new QDoubleSpinBox;
    lightsource_simple_power_spinbox->setRange(1, 1000);
    lightsource_simple_power_spinbox->setValue(_simulator.lightsource_simple_power);
    l0->addWidget(lightsource_simple_power_spinbox, row++, 1);
    l0->addWidget(new QLabel("Simple model: aperture angle [deg]:"), row, 0);
    QDoubleSpinBox* lightsource_simple_aperture_angle_spinbox = new QDoubleSpinBox;
    lightsource_simple_aperture_angle_spinbox->setRange(1, 1000);
    lightsource_simple_aperture_angle_spinbox->setValue(_simulator.lightsource_simple_aperture_angle);
    l0->addWidget(lightsource_simple_aperture_angle_spinbox, row++, 1);
    l0->addWidget(new QLabel("Measured model: table file [.gta]:"), row, 0);
    QGridLayout* l2 = new QGridLayout;
    QLineEdit* lightsource_measured_intensities = new QLineEdit;
    lightsource_measured_intensities->setText(_simulator.lightsource_measured_intensities.filename.c_str());
    l2->addWidget(lightsource_measured_intensities, 0, 0);
    l2->addWidget(new QLabel(" "), 0, 1);
    QFileDialog lmidlg(this, "Open lightsource measured intensities",
            _settings->value("Session/directory", QDir::currentPath()).toString(),
            tr("Generic Tagged Array files (*.gta)"));
    QPushButton* lightsource_measured_intensities_btn = new QPushButton("Choose...");
    connect(lightsource_measured_intensities_btn, SIGNAL(clicked()), &lmidlg, SLOT(exec()));
    connect(&lmidlg, SIGNAL(fileSelected(const QString&)), lightsource_measured_intensities, SLOT(setText(const QString&)));
    l2->addWidget(lightsource_measured_intensities_btn, 0, 2);
    l0->addItem(l2, row++, 1);

    l0->addWidget(new QLabel("<b>Lens</b>"), row++, 0);

    l0->addWidget(new QLabel("Aperture diameter [mm]:"), row, 0);
    QDoubleSpinBox* lens_aperture_diameter_spinbox = new QDoubleSpinBox;
    lens_aperture_diameter_spinbox->setRange(1, 1000);
    lens_aperture_diameter_spinbox->setValue(_simulator.lens_aperture_diameter);
    l0->addWidget(lens_aperture_diameter_spinbox, row++, 1);
    l0->addWidget(new QLabel("Focal length [mm]:"), row, 0);
    QDoubleSpinBox* lens_focal_length_spinbox = new QDoubleSpinBox;
    lens_focal_length_spinbox->setRange(1, 1000);
    lens_focal_length_spinbox->setValue(_simulator.lens_focal_length);
    l0->addWidget(lens_focal_length_spinbox, row++, 1);

    l0->addWidget(new QLabel("<b>Pixels</b>"), row++, 0);

    l0->addWidget(new QLabel("Sensor width [pixels]:"), row, 0);
    QSpinBox* sensor_width_spinbox = new QSpinBox;
    sensor_width_spinbox->setRange(2, 1024);
    sensor_width_spinbox->setValue(_simulator.sensor_width);
    l0->addWidget(sensor_width_spinbox, row++, 1);
    l0->addWidget(new QLabel("Sensor height [pixels]:"), row, 0);
    QSpinBox* sensor_height_spinbox = new QSpinBox;
    sensor_height_spinbox->setRange(2, 1024);
    sensor_height_spinbox->setValue(_simulator.sensor_height);
    l0->addWidget(sensor_height_spinbox, row++, 1);
    l0->addWidget(new QLabel("Pixel mask x [0-1]:"), row, 0);
    QDoubleSpinBox* pixel_mask_x_spinbox = new QDoubleSpinBox;
    pixel_mask_x_spinbox->setDecimals(4);
    pixel_mask_x_spinbox->setRange(0.0, 1.0);
    pixel_mask_x_spinbox->setValue(_simulator.pixel_mask_x);
    l0->addWidget(pixel_mask_x_spinbox, row++, 1);
    l0->addWidget(new QLabel("Pixel mask y [0-1]:"), row, 0);
    QDoubleSpinBox* pixel_mask_y_spinbox = new QDoubleSpinBox;
    pixel_mask_y_spinbox->setDecimals(4);
    pixel_mask_y_spinbox->setRange(0.0, 1.0);
    pixel_mask_y_spinbox->setValue(_simulator.pixel_mask_y);
    l0->addWidget(pixel_mask_y_spinbox, row++, 1);
    l0->addWidget(new QLabel("Pixel mask width [0-1]:"), row, 0);
    QDoubleSpinBox* pixel_mask_width_spinbox = new QDoubleSpinBox;
    pixel_mask_width_spinbox->setDecimals(4);
    pixel_mask_width_spinbox->setRange(0.0, 1.0);
    pixel_mask_width_spinbox->setValue(_simulator.pixel_mask_width);
    l0->addWidget(pixel_mask_width_spinbox, row++, 1);
    l0->addWidget(new QLabel("Pixel mask height [0-1]:"), row, 0);
    QDoubleSpinBox* pixel_mask_height_spinbox = new QDoubleSpinBox;
    pixel_mask_height_spinbox->setDecimals(4);
    pixel_mask_height_spinbox->setRange(0.0, 1.0);
    pixel_mask_height_spinbox->setValue(_simulator.pixel_mask_height);
    l0->addWidget(pixel_mask_height_spinbox, row++, 1);
    l0->addWidget(new QLabel("Pixel width [subpixels, odd]:"), row, 0);
    OddSpinBox* pixel_width_spinbox = new OddSpinBox;
    pixel_width_spinbox->setRange(1, 31);
    pixel_width_spinbox->setValue(_simulator.pixel_width);
    l0->addWidget(pixel_width_spinbox, row++, 1);
    l0->addWidget(new QLabel("Pixel height [subpixels, odd]:"), row, 0);
    OddSpinBox* pixel_height_spinbox = new OddSpinBox;
    pixel_height_spinbox->setRange(1, 31);
    pixel_height_spinbox->setValue(_simulator.pixel_height);
    l0->addWidget(pixel_height_spinbox, row++, 1);
    l0->addWidget(new QLabel("Pitch [micrometer]:"), row, 0);
    QDoubleSpinBox* pixel_pitch_spinbox = new QDoubleSpinBox;
    pixel_pitch_spinbox->setRange(1, 1000);
    pixel_pitch_spinbox->setValue(_simulator.pixel_pitch);
    l0->addWidget(pixel_pitch_spinbox, row++, 1);
    l0->addWidget(new QLabel("Read-Out time [microseconds]:"), row, 0);
    QSpinBox* readout_time_spinbox = new QSpinBox;
    readout_time_spinbox->setRange(1, 50000);
    readout_time_spinbox->setValue(_simulator.readout_time);
    l0->addWidget(readout_time_spinbox, row++, 1);
    l0->addWidget(new QLabel("Contrast (0-1):"), row, 0);
    QDoubleSpinBox* contrast_spinbox = new QDoubleSpinBox;
    contrast_spinbox->setDecimals(4);
    contrast_spinbox->setRange(0.0, 1.0);
    contrast_spinbox->setValue(_simulator.contrast);
    l0->addWidget(contrast_spinbox, row++, 1);

    l0->addWidget(new QLabel("<b>User-modifiable parameters</b>"), row++, 0);

    l0->addWidget(new QLabel("Modulation frequency (MHz):"), row, 0);
    QSpinBox* modulation_frequency_spinbox = new QSpinBox;
    modulation_frequency_spinbox->setRange(1, 200);
    modulation_frequency_spinbox->setValue(_simulator.modulation_frequency / (1000 * 1000));
    l0->addWidget(modulation_frequency_spinbox, row++, 1);
    l0->addWidget(new QLabel("Exposure time (microseconds):"), row, 0);
    QSpinBox* exposure_time_spinbox = new QSpinBox;
    exposure_time_spinbox->setRange(1, 50000);
    exposure_time_spinbox->setValue(_simulator.exposure_time);
    l0->addWidget(exposure_time_spinbox, row++, 1);

    QGridLayout* l1 = new QGridLayout;
    QPushButton* ok_btn = new QPushButton("OK");
    QPushButton* cancel_btn = new QPushButton("Cancel");
    connect(ok_btn, SIGNAL(pressed()), dlg, SLOT(accept()));
    connect(cancel_btn, SIGNAL(pressed()), dlg, SLOT(reject()));
    l1->addWidget(ok_btn, 0, 0);
    l1->addWidget(cancel_btn, 0, 1);
    QGridLayout* layout = new QGridLayout;
    layout->addLayout(l0, 0, 0);
    layout->addLayout(l1, 1, 0);
    dlg->setLayout(layout);

    dlg->exec();
    if (dlg->result() == QDialog::Accepted) {
        if (lightsource_model_box->currentIndex() == 1) {
            try {
                _simulator.lightsource_measured_intensities.load(
                        qPrintable(lightsource_measured_intensities->text()));
            }
            catch (std::exception& e) {
                QMessageBox::critical(this, "Error", e.what());
                return;
            }
        }
        _simulator.aperture_angle = aperture_angle_spinbox->value();
        _simulator.near_plane = near_plane_spinbox->value();
        _simulator.far_plane = far_plane_spinbox->value();
        _simulator.exposure_time_samples = exposure_time_samples_spinbox->value();
        _simulator.rendering_method = rendering_box->currentIndex();
        _simulator.material_model = material_model_box->currentIndex();
        _simulator.material_lambertian_reflectivity = material_lambertian_reflectivity_spinbox->value();
        _simulator.lightsource_model = lightsource_model_box->currentIndex();
        _simulator.lightsource_simple_power = lightsource_simple_power_spinbox->value();
        _simulator.lightsource_simple_aperture_angle = lightsource_simple_aperture_angle_spinbox->value();
        _simulator.lens_aperture_diameter = lens_aperture_diameter_spinbox->value();
        _simulator.lens_focal_length = lens_focal_length_spinbox->value();
        _simulator.sensor_width = sensor_width_spinbox->value();
        _simulator.sensor_height = sensor_height_spinbox->value();
        _simulator.pixel_mask_x = pixel_mask_x_spinbox->value();
        _simulator.pixel_mask_y = pixel_mask_y_spinbox->value();
        _simulator.pixel_mask_width = pixel_mask_width_spinbox->value();
        _simulator.pixel_mask_height = pixel_mask_height_spinbox->value();
        _simulator.pixel_width = pixel_width_spinbox->value();
        _simulator.pixel_height = pixel_height_spinbox->value();
        _simulator.pixel_pitch = pixel_pitch_spinbox->value();
        _simulator.readout_time = readout_time_spinbox->value();
        _simulator.contrast = contrast_spinbox->value();
        _simulator.modulation_frequency = modulation_frequency_spinbox->value() * 1000 * 1000;
        _simulator.exposure_time = exposure_time_spinbox->value();
        emit update_simulator(_simulator);
    }
    delete dlg;
}

void MainWindow::simulator_export_modelfile()
{
    QString filename = QFileDialog::getSaveFileName(this, "Save model file",
            _settings->value("Session/directory", QDir::currentPath()).toString(),
            tr("Model files (*.obj)"));
    if (filename.isEmpty())
        return;
    _settings->setValue("Session/directory", QFileInfo(filename).path());
    try {
        _osg_widget->export_frustum(filename.toLocal8Bit().constData());
    }
    catch (std::exception& e) {
        QMessageBox::critical(this, "Error", e.what());
        return;
    }
}

void MainWindow::simulator_reset()
{
    _simulator = Simulator();
    _settings->setValue("Session/simulator", QString());
    emit update_simulator(_simulator);
}

void MainWindow::background_load()
{
    QString filename = QFileDialog::getOpenFileName(this, "Load background",
            _settings->value("Session/directory", QDir::currentPath()).toString(),
            tr("Target descriptions (*.txt)"));
    if (filename.isEmpty())
        return;
    _settings->setValue("Session/directory", QFileInfo(filename).path());
    try {
        _background.load(filename.toLocal8Bit().constData());
    }
    catch (std::exception& e) {
        QMessageBox::critical(this, "Error", e.what());
        return;
    }
    emit update_scene(_background, _target);
    _settings->setValue("Session/background", filename);
}

void MainWindow::background_save()
{
    QString filename = QFileDialog::getSaveFileName(this, "Save background",
            _settings->value("Session/directory", QDir::currentPath()).toString(),
            tr("Target descriptions (*.txt)"));
    if (filename.isEmpty())
        return;
    _settings->setValue("Session/directory", QFileInfo(filename).path());
    try {
        _background.save(filename.toLocal8Bit().constData());
    }
    catch (std::exception& e) {
        QMessageBox::critical(this, "Error", e.what());
        return;
    }
    _settings->setValue("Session/background", filename);
}

void MainWindow::background_generate_planar()
{
    QDialog* dlg = new QDialog(this);
    dlg->setWindowTitle("Generate planar background");
    QGridLayout* l0 = new QGridLayout;

    l0->addWidget(new QLabel("Width (m):"), 1, 0);
    QDoubleSpinBox* width_spinbox = new QDoubleSpinBox;
    width_spinbox->setDecimals(4);
    width_spinbox->setRange(0.001, 10.0);
    width_spinbox->setValue(_background.background_planar_width);
    l0->addWidget(width_spinbox, 1, 1);
    l0->addWidget(new QLabel("Height (m):"), 2, 0);
    QDoubleSpinBox* height_spinbox = new QDoubleSpinBox;
    height_spinbox->setDecimals(4);
    height_spinbox->setRange(0.001, 10.0);
    height_spinbox->setValue(_background.background_planar_height);
    l0->addWidget(height_spinbox, 2, 1);
    l0->addWidget(new QLabel("Distance (m; 0=disabled):"), 3, 0);
    QDoubleSpinBox* dist_spinbox = new QDoubleSpinBox;
    dist_spinbox->setDecimals(4);
    dist_spinbox->setRange(0.0, 10.0);
    dist_spinbox->setValue(_background.background_planar_dist);
    l0->addWidget(dist_spinbox, 3, 1);

    QGridLayout* l1 = new QGridLayout;
    QPushButton* ok_btn = new QPushButton("OK");
    QPushButton* cancel_btn = new QPushButton("Cancel");
    connect(ok_btn, SIGNAL(pressed()), dlg, SLOT(accept()));
    connect(cancel_btn, SIGNAL(pressed()), dlg, SLOT(reject()));
    l1->addWidget(ok_btn, 0, 0);
    l1->addWidget(cancel_btn, 0, 1);
    QGridLayout* layout = new QGridLayout;
    layout->addLayout(l0, 0, 0);
    layout->addLayout(l1, 1, 0);
    dlg->setLayout(layout);

    dlg->exec();
    if (dlg->result() == QDialog::Accepted) {
        _background.variant = Target::variant_background_planar;
        _background.background_planar_width = width_spinbox->value();
        _background.background_planar_height = height_spinbox->value();
        _background.background_planar_dist = dist_spinbox->value();
        emit update_scene(_background, _target);
    }
    delete dlg;
}

void MainWindow::background_export_modelfile()
{
    QString filename = QFileDialog::getSaveFileName(this, "Save model file",
            _settings->value("Session/directory", QDir::currentPath()).toString(),
            tr("Model files (*.obj)"));
    if (filename.isEmpty())
        return;
    _settings->setValue("Session/directory", QFileInfo(filename).path());
    try {
        _osg_widget->export_background(filename.toLocal8Bit().constData());
    }
    catch (std::exception& e) {
        QMessageBox::critical(this, "Error", e.what());
        return;
    }
}

void MainWindow::background_reset()
{
    _background = Target(Target::variant_background_planar);
    _settings->setValue("Session/background", QString());
    emit update_scene(_background, _target);
}

void MainWindow::target_load()
{
    QString filename = QFileDialog::getOpenFileName(this, "Load target",
            _settings->value("Session/directory", QDir::currentPath()).toString(),
            tr("Target descriptions (*.txt)"));
    if (filename.isEmpty())
        return;
    _settings->setValue("Session/directory", QFileInfo(filename).path());
    try {
        _target.load(filename.toLocal8Bit().constData());
    }
    catch (std::exception& e) {
        QMessageBox::critical(this, "Error", e.what());
        return;
    }
    emit update_scene(_background, _target);
    _settings->setValue("Session/target", filename);
}

void MainWindow::target_save()
{
    QString filename = QFileDialog::getSaveFileName(this, "Save target",
            _settings->value("Session/directory", QDir::currentPath()).toString(),
            tr("Target descriptions (*.txt)"));
    if (filename.isEmpty())
        return;
    _settings->setValue("Session/directory", QFileInfo(filename).path());
    try {
        _target.save(filename.toLocal8Bit().constData());
    }
    catch (std::exception& e) {
        QMessageBox::critical(this, "Error", e.what());
        return;
    }
    _settings->setValue("Session/target", filename);
}

void MainWindow::target_use_modelfile()
{
    QString filename = QFileDialog::getOpenFileName(this, "Read model file",
            _settings->value("Session/directory", QDir::currentPath()).toString(),
            tr("Target models (*.obj, *.ply)"));
    if (filename.isEmpty())
        return;
    _settings->setValue("Session/directory", QFileInfo(filename).path());
    _target.variant = Target::variant_model;
    _target.model_filename = filename.toLocal8Bit().constData();
    emit update_scene(_background, _target);
}

void MainWindow::target_generate_bar_pattern()
{
    QDialog* dlg = new QDialog(this);
    dlg->setWindowTitle("Generate bar pattern");

    QGridLayout* l0 = new QGridLayout;
    l0->addWidget(new QLabel("Number of bars:"), 0, 0);
    QSpinBox* number_of_bars_spinbox = new QSpinBox;
    number_of_bars_spinbox->setRange(1, 99);
    number_of_bars_spinbox->setValue(_target.number_of_bars);
    l0->addWidget(number_of_bars_spinbox, 0, 1);
    l0->addWidget(new QLabel("First bar width (m):"), 1, 0);
    QDoubleSpinBox* first_bar_width_spinbox = new QDoubleSpinBox;
    first_bar_width_spinbox->setDecimals(4);
    first_bar_width_spinbox->setRange(0.001, 10.0);
    first_bar_width_spinbox->setValue(_target.first_bar_width);
    l0->addWidget(first_bar_width_spinbox, 1, 1);
    l0->addWidget(new QLabel("First bar height (m):"), 2, 0);
    QDoubleSpinBox* first_bar_height_spinbox = new QDoubleSpinBox;
    first_bar_height_spinbox->setDecimals(4);
    first_bar_height_spinbox->setRange(0.001, 10.0);
    first_bar_height_spinbox->setValue(_target.first_bar_height);
    l0->addWidget(first_bar_height_spinbox, 2, 1);
    l0->addWidget(new QLabel("First offset x (m):"), 3, 0);
    QDoubleSpinBox* first_offset_x_spinbox = new QDoubleSpinBox;
    first_offset_x_spinbox->setDecimals(4);
    first_offset_x_spinbox->setRange(-10, 10);
    first_offset_x_spinbox->setValue(_target.first_offset_x);
    l0->addWidget(first_offset_x_spinbox, 3, 1);
    l0->addWidget(new QLabel("First offset y (m):"), 4, 0);
    QDoubleSpinBox* first_offset_y_spinbox = new QDoubleSpinBox;
    first_offset_y_spinbox->setDecimals(4);
    first_offset_y_spinbox->setRange(-10, 10);
    first_offset_y_spinbox->setValue(_target.first_offset_y);
    l0->addWidget(first_offset_y_spinbox, 4, 1);
    l0->addWidget(new QLabel("First offset z (m):"), 5, 0);
    QDoubleSpinBox* first_offset_z_spinbox = new QDoubleSpinBox;
    first_offset_z_spinbox->setDecimals(4);
    first_offset_z_spinbox->setRange(-10, 10);
    first_offset_z_spinbox->setValue(_target.first_offset_z);
    l0->addWidget(first_offset_z_spinbox, 5, 1);
    l0->addWidget(new QLabel("Next bar width factor:"), 6, 0);
    QDoubleSpinBox* next_bar_width_factor_spinbox = new QDoubleSpinBox;
    next_bar_width_factor_spinbox->setDecimals(4);
    next_bar_width_factor_spinbox->setRange(0.1, 10.0);
    next_bar_width_factor_spinbox->setValue(_target.next_bar_width_factor);
    l0->addWidget(next_bar_width_factor_spinbox, 6, 1);
    l0->addWidget(new QLabel("Next bar width offset:"), 7, 0);
    QDoubleSpinBox* next_bar_width_offset_spinbox = new QDoubleSpinBox;
    next_bar_width_offset_spinbox->setDecimals(4);
    next_bar_width_offset_spinbox->setRange(-10, 10);
    next_bar_width_offset_spinbox->setValue(_target.next_bar_width_offset);
    l0->addWidget(next_bar_width_offset_spinbox, 7, 1);
    l0->addWidget(new QLabel("Next bar height factor:"), 8, 0);
    QDoubleSpinBox* next_bar_height_factor_spinbox = new QDoubleSpinBox;
    next_bar_height_factor_spinbox->setDecimals(4);
    next_bar_height_factor_spinbox->setRange(0.1, 10.0);
    next_bar_height_factor_spinbox->setValue(_target.next_bar_height_factor);
    l0->addWidget(next_bar_height_factor_spinbox, 8, 1);
    l0->addWidget(new QLabel("Next bar height offset:"), 9, 0);
    QDoubleSpinBox* next_bar_height_offset_spinbox = new QDoubleSpinBox;
    next_bar_height_offset_spinbox->setDecimals(4);
    next_bar_height_offset_spinbox->setRange(-10, 10);
    next_bar_height_offset_spinbox->setValue(_target.next_bar_height_offset);
    l0->addWidget(next_bar_height_offset_spinbox, 9, 1);
    l0->addWidget(new QLabel("Next offset x factor:"), 10, 0);
    QDoubleSpinBox* next_offset_x_factor_spinbox = new QDoubleSpinBox;
    next_offset_x_factor_spinbox->setDecimals(4);
    next_offset_x_factor_spinbox->setRange(-10, 10);
    next_offset_x_factor_spinbox->setValue(_target.next_offset_x_factor);
    l0->addWidget(next_offset_x_factor_spinbox, 10, 1);
    l0->addWidget(new QLabel("Next offset x offset:"), 11, 0);
    QDoubleSpinBox* next_offset_x_offset_spinbox = new QDoubleSpinBox;
    next_offset_x_offset_spinbox->setDecimals(4);
    next_offset_x_offset_spinbox->setRange(-10, 10);
    next_offset_x_offset_spinbox->setValue(_target.next_offset_x_offset);
    l0->addWidget(next_offset_x_offset_spinbox, 11, 1);
    l0->addWidget(new QLabel("Next offset y factor:"), 12, 0);
    QDoubleSpinBox* next_offset_y_factor_spinbox = new QDoubleSpinBox;
    next_offset_y_factor_spinbox->setDecimals(4);
    next_offset_y_factor_spinbox->setRange(-10, 10);
    next_offset_y_factor_spinbox->setValue(_target.next_offset_y_factor);
    l0->addWidget(next_offset_y_factor_spinbox, 12, 1);
    l0->addWidget(new QLabel("Next offset y offset:"), 13, 0);
    QDoubleSpinBox* next_offset_y_offset_spinbox = new QDoubleSpinBox;
    next_offset_y_offset_spinbox->setDecimals(4);
    next_offset_y_offset_spinbox->setRange(-10, 10);
    next_offset_y_offset_spinbox->setValue(_target.next_offset_y_offset);
    l0->addWidget(next_offset_y_offset_spinbox, 13, 1);
    l0->addWidget(new QLabel("Next offset z factor:"), 14, 0);
    QDoubleSpinBox* next_offset_z_factor_spinbox = new QDoubleSpinBox;
    next_offset_z_factor_spinbox->setDecimals(4);
    next_offset_z_factor_spinbox->setRange(-10, 10);
    next_offset_z_factor_spinbox->setValue(_target.next_offset_z_factor);
    l0->addWidget(next_offset_z_factor_spinbox, 14, 1);
    l0->addWidget(new QLabel("Next offset z offset:"), 15, 0);
    QDoubleSpinBox* next_offset_z_offset_spinbox = new QDoubleSpinBox;
    next_offset_z_offset_spinbox->setDecimals(4);
    next_offset_z_offset_spinbox->setRange(-10, 10);
    next_offset_z_offset_spinbox->setValue(_target.next_offset_z_offset);
    l0->addWidget(next_offset_z_offset_spinbox, 15, 1);
    l0->addWidget(new QLabel("Background orientation:"), 16, 0);
    QComboBox* bar_background_near_side_combobox = new QComboBox;
    bar_background_near_side_combobox->addItem("Disable background");
    bar_background_near_side_combobox->addItem("Left side near, right side far");
    bar_background_near_side_combobox->addItem("Top side near, bottom side far");
    bar_background_near_side_combobox->addItem("Right side near, left side far");
    bar_background_near_side_combobox->addItem("Bottom side near, top side far");
    bar_background_near_side_combobox->setCurrentIndex(_target.bar_background_near_side + 1);
    l0->addWidget(bar_background_near_side_combobox, 16, 1);
    l0->addWidget(new QLabel("Background near side distance to bars (m):"), 17, 0);
    QDoubleSpinBox* bar_background_dist_near_spinbox = new QDoubleSpinBox;
    bar_background_dist_near_spinbox->setDecimals(4);
    bar_background_dist_near_spinbox->setRange(-1, 1);
    bar_background_dist_near_spinbox->setValue(_target.bar_background_dist_near);
    l0->addWidget(bar_background_dist_near_spinbox, 17, 1);
    l0->addWidget(new QLabel("Background far side distance to bars (m):"), 18, 0);
    QDoubleSpinBox* bar_background_dist_far_spinbox = new QDoubleSpinBox;
    bar_background_dist_far_spinbox->setDecimals(4);
    bar_background_dist_far_spinbox->setRange(-1, 1);
    bar_background_dist_far_spinbox->setValue(_target.bar_background_dist_far);
    l0->addWidget(bar_background_dist_far_spinbox, 18, 1);
    l0->addWidget(new QLabel("Rotation around view direction (degrees):"), 19, 0);
    QDoubleSpinBox* bar_rotation_spinbox = new QDoubleSpinBox;
    bar_rotation_spinbox->setDecimals(4);
    bar_rotation_spinbox->setRange(-180.0, +180.0);
    bar_rotation_spinbox->setValue(_target.bar_rotation / M_PI * 180.0);
    l0->addWidget(bar_rotation_spinbox, 19, 1);

    QGridLayout* l1 = new QGridLayout;
    QPushButton* ok_btn = new QPushButton("OK");
    QPushButton* cancel_btn = new QPushButton("Cancel");
    connect(ok_btn, SIGNAL(pressed()), dlg, SLOT(accept()));
    connect(cancel_btn, SIGNAL(pressed()), dlg, SLOT(reject()));
    l1->addWidget(ok_btn, 0, 0);
    l1->addWidget(cancel_btn, 0, 1);
    QGridLayout* layout = new QGridLayout;
    layout->addLayout(l0, 0, 0);
    layout->addLayout(l1, 1, 0);
    dlg->setLayout(layout);

    dlg->exec();
    if (dlg->result() == QDialog::Accepted) {
        _target.variant = Target::variant_bars;
        _target.number_of_bars = number_of_bars_spinbox->value();
        _target.first_bar_width = first_bar_width_spinbox->value();
        _target.first_bar_height = first_bar_height_spinbox->value();
        _target.first_offset_x = first_offset_x_spinbox->value();
        _target.first_offset_y = first_offset_y_spinbox->value();
        _target.first_offset_z = first_offset_z_spinbox->value();
        _target.next_bar_width_factor = next_bar_width_factor_spinbox->value();
        _target.next_bar_width_offset = next_bar_width_offset_spinbox->value();
        _target.next_bar_height_factor = next_bar_height_factor_spinbox->value();
        _target.next_bar_height_offset = next_bar_height_offset_spinbox->value();
        _target.next_offset_x_factor = next_offset_x_factor_spinbox->value();
        _target.next_offset_x_offset = next_offset_x_offset_spinbox->value();
        _target.next_offset_y_factor = next_offset_y_factor_spinbox->value();
        _target.next_offset_y_offset = next_offset_y_offset_spinbox->value();
        _target.next_offset_z_factor = next_offset_z_factor_spinbox->value();
        _target.next_offset_z_offset = next_offset_z_offset_spinbox->value();
        _target.bar_background_near_side = bar_background_near_side_combobox->currentIndex() - 1;
        _target.bar_background_dist_near = bar_background_dist_near_spinbox->value();
        _target.bar_background_dist_far = bar_background_dist_far_spinbox->value();
        _target.bar_rotation = bar_rotation_spinbox->value() / 180.0 * M_PI;
        emit update_scene(_background, _target);
    }
    delete dlg;
}

void MainWindow::target_generate_star_pattern()
{
    QDialog* dlg = new QDialog(this);
    dlg->setWindowTitle("Generate Siemens star pattern");

    QGridLayout* l0 = new QGridLayout;
    l0->addWidget(new QLabel("Star spokes:"), 0, 0);
    QSpinBox* star_spokes_spinbox = new QSpinBox;
    star_spokes_spinbox->setRange(2, 50);
    star_spokes_spinbox->setValue(_target.star_spokes);
    l0->addWidget(star_spokes_spinbox, 0, 1);
    l0->addWidget(new QLabel("Star radius (m):"), 1, 0);
    QDoubleSpinBox* star_radius_spinbox = new QDoubleSpinBox;
    star_radius_spinbox->setDecimals(4);
    star_radius_spinbox->setRange(0.1, 10.0);
    star_radius_spinbox->setValue(_target.star_radius);
    l0->addWidget(star_radius_spinbox, 1, 1);
    l0->addWidget(new QLabel("Background distance at center (m):"), 2, 0);
    QDoubleSpinBox* star_background_dist_center_spinbox = new QDoubleSpinBox;
    star_background_dist_center_spinbox->setDecimals(4);
    star_background_dist_center_spinbox->setRange(0, 10);
    star_background_dist_center_spinbox->setValue(_target.star_background_dist_center);
    l0->addWidget(star_background_dist_center_spinbox, 2, 1);
    l0->addWidget(new QLabel("Background distance at rim (m):"), 3, 0);
    QDoubleSpinBox* star_background_dist_rim_spinbox = new QDoubleSpinBox;
    star_background_dist_rim_spinbox->setDecimals(4);
    star_background_dist_rim_spinbox->setRange(0, 10);
    star_background_dist_rim_spinbox->setValue(_target.star_background_dist_rim);
    l0->addWidget(star_background_dist_rim_spinbox, 3, 1);

    QGridLayout* l1 = new QGridLayout;
    QPushButton* ok_btn = new QPushButton("OK");
    QPushButton* cancel_btn = new QPushButton("Cancel");
    connect(ok_btn, SIGNAL(pressed()), dlg, SLOT(accept()));
    connect(cancel_btn, SIGNAL(pressed()), dlg, SLOT(reject()));
    l1->addWidget(ok_btn, 0, 0);
    l1->addWidget(cancel_btn, 0, 1);
    QGridLayout* layout = new QGridLayout;
    layout->addLayout(l0, 0, 0);
    layout->addLayout(l1, 1, 0);
    dlg->setLayout(layout);

    dlg->exec();
    if (dlg->result() == QDialog::Accepted) {
        _target.variant = Target::variant_star;
        _target.star_spokes = star_spokes_spinbox->value();
        _target.star_radius = star_radius_spinbox->value();
        _target.star_background_dist_center = star_background_dist_center_spinbox->value();
        _target.star_background_dist_rim = star_background_dist_rim_spinbox->value();
        emit update_scene(_background, _target);
    }
    delete dlg;
}

void MainWindow::target_export_modelfile()
{
    QString filename = QFileDialog::getSaveFileName(this, "Save model file",
            _settings->value("Session/directory", QDir::currentPath()).toString(),
            tr("Model files (*.obj)"));
    if (filename.isEmpty())
        return;
    _settings->setValue("Session/directory", QFileInfo(filename).path());
    try {
        _osg_widget->export_target(filename.toLocal8Bit().constData());
    }
    catch (std::exception& e) {
        QMessageBox::critical(this, "Error", e.what());
        return;
    }
}

void MainWindow::target_reset()
{
    _target = Target();
    _settings->setValue("Session/target", QString());
    emit update_scene(_background, _target);
}

void MainWindow::animation_load()
{
    QString filename = QFileDialog::getOpenFileName(this, "Load animation",
            _settings->value("Session/directory", QDir::currentPath()).toString(),
            tr("Animation descriptions (*.txt)"));
    if (filename.isEmpty())
        return;
    _settings->setValue("Session/directory", QFileInfo(filename).path());
    try {
        _animation.load(filename.toLocal8Bit().constData());
    }
    catch (std::exception& e) {
        QMessageBox::critical(this, "Error", e.what());
        return;
    }
    emit update_animation(_animation);
    _settings->setValue("Session/animation", filename);
}

void MainWindow::animation_reset()
{
    _animation = Animation();
    _settings->setValue("Session/animation", QString());
    emit update_animation(_animation);
}

void MainWindow::help_about()
{
    QMessageBox::about(this, tr("About PMDSim"), tr(
                "<p>PMDSim version %1</p>"
                "<p>Copyright (C) 2014<br>"
                "   <a href=\"http://www.cg.informatik.uni-siegen.de/\">"
                "   Computer Graphics Group, University of Siegen</a>.<br>"
                "   All rights reserved.<br>"
                "</p>").arg(PROJECT_VERSION));
}
