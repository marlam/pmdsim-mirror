/*
 * Copyright (C) 2012, 2013, 2014
 * Computer Graphics Group, University of Siegen, Germany.
 * http://www.cg.informatik.uni-siegen.de/
 * All rights reserved.
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>.
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <vector>

#include <GL/glew.h>

#include <QMainWindow>

#include "simulator.h"
#include "target.h"
#include "animation.h"
#include "trianglepatch.h"

class QSettings;
class QTimer;

class SimWidget;
class OSGWidget;
class View2DWidget;
class AnimWidget;


class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    QSettings* _settings;

    Simulator _simulator;
    Target _background;
    Target _target;
    Animation _animation;

    int _scene_id;
    std::vector<TrianglePatch> _scene;
    SimWidget* _sim_widget;
    OSGWidget* _osg_widget;
    View2DWidget* _depthmap_widget;
    View2DWidget* _phase_widgets[4];
    View2DWidget* _pmd_depth_widget;
    View2DWidget* _pmd_amp_widget;
    View2DWidget* _pmd_intensity_widget;
    AnimWidget* _anim_widget;

    QTimer* _sim_timer;
    long long _last_anim_time;
    bool _anim_time_requested;
    long long _anim_time_request;

    // For data export
    std::vector<float> _export_phase0;
    std::vector<float> _export_phase1;
    std::vector<float> _export_phase2;
    std::vector<float> _export_phase3;
    std::vector<float> _export_result;
    void get_sim_data(int w, int h);
    void export_frame(const std::string& dirname, int frameno = -1);
    void export_animation(const std::string& dirname, bool show_progress = true);

protected:
    void closeEvent(QCloseEvent* event);

public:
    MainWindow(
            // if any of the following is not the default value, then
            // we go into script mode and don't load saved GUI settings
            QString script_simulator_file = QString(),
            QString script_background_file = QString(),
            QString script_target_file = QString(),
            QString script_animation_file = QString(),
            QString script_export_dir = QString(),
            bool script_export_animation = false,
            double script_export_frame = 1.0 / 0.0,
            bool script_minimize_window = false);
    ~MainWindow();

private slots:
    // Scene handling
    void reset_scene();
    // Animation handling
    void animation_state_changed();
    void animation_time_changed(long long);
    // Simulator loop
    void simulation_step();
    // Menu actions
    void file_export_frame();
    void file_export_anim();
    void simulator_load();
    void simulator_save();
    void simulator_edit();
    void simulator_export_modelfile();
    void simulator_reset();
    void background_load();
    void background_save();
    void background_generate_planar();
    void background_export_modelfile();
    void background_reset();
    void target_load();
    void target_save();
    void target_use_modelfile();
    void target_generate_bar_pattern();
    void target_generate_star_pattern();
    void target_export_modelfile();
    void target_reset();
    void animation_load();
    void animation_reset();
    void help_about();

signals:
    void update_simulator(const Simulator&);
    void update_scene(const Target&, const Target&);
    void update_animation(const Animation&);
};

#endif
