/*
 * Copyright (C) 2012, 2013, 2014
 * Computer Graphics Group, University of Siegen, Germany.
 * http://www.cg.informatik.uni-siegen.de/
 * All rights reserved.
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>.
 */

#ifndef OSGWIDGET_H
#define OSGWIDGET_H

#include <vector>
#include <QWidget>

#include "simulator.h"
#include "target.h"
#include "trianglepatch.h"

class QGLWidget;
class QPaintEvent;


/* The OSG headers do not work with GLEW. We have to hide all OSG stuff in a private structure
 * so that this header file does not have to pull in the OSG headers. This way, OSG's problems
 * only affect our corresponding cpp file, but not files that include this header. */

class OSGWidget : public QWidget
{
    Q_OBJECT

public:
    typedef enum {
        mode_free_interaction,  // for free user interaction
        mode_fixed_target       // for animation mode: the target pos/rot is given by the animation
    } mode_t;

private:
    Simulator _simulator;
    Target _background;
    Target _target;
    struct HideOSGProblems* _osg;
    bool _force_mode_update;
    mode_t _mode;

public:
    OSGWidget(QGLWidget* sharing_widget);
    ~OSGWidget();

    void draw_frame();
    void export_frustum(const std::string& filename);
    void export_background(const std::string& filename);
    void export_target(const std::string& filename);
    void set_mode(mode_t mode);
    void set_fixed_target_transformation(const float pos[3], const float rot[4]);

    virtual void paintEvent(QPaintEvent*) {} // Ignore; updates are triggered manually.

    // Create a scene description: a map of triangle patches.
    void capture_scene(std::vector<TrianglePatch>* scene) const;
    // Update the patch transformations in the scene description. The scene must not otherwise change!
    void update_scene(std::vector<TrianglePatch>* scene) const;

public slots:
    void update_simulator(const Simulator&);
    void update_scene(const Target&, const Target&);

private:
    #include "simviewhelper.inl"
};

#endif
