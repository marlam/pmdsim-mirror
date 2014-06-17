/*
 * Copyright (C) 2012, 2013, 2014
 * Computer Graphics Group, University of Siegen, Germany.
 * http://www.cg.informatik.uni-siegen.de/
 * All rights reserved.
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>.
 */

#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
#include <stdexcept>

#include <QGridLayout>
#include <QMessageBox>

#include <osg/Geode>
#include <osg/MatrixTransform>
#include <osg/ShapeDrawable>
#include <osg/Shader>
#include <osg/Program>
#include <osg/ClampColor>
#include <osg/Texture2D>
#include <osg/Program>
#include <osg/TriangleFunctor>
#include <osgViewer/CompositeViewer>
#include <osgQt/GraphicsWindowQt>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgGA/GUIEventHandler>
#include <osgUtil/Optimizer>

#include "osgwidget.h"

/* Hide all OSG details in a private struct; see corresponding header file. */
struct HideOSGProblems {
    osg::ref_ptr<osgViewer::CompositeViewer> _viewer;
    osg::ref_ptr<osg::Group> _root;
    osg::ref_ptr<osg::MatrixTransform> _background_animation;
    osg::ref_ptr<osg::MatrixTransform> _background_transformation;
    osg::ref_ptr<osg::Node> _background;
    osg::ref_ptr<osg::MatrixTransform> _target_animation;
    osg::ref_ptr<osg::MatrixTransform> _target_transformation;
    osg::ref_ptr<osg::Node> _target;
    osg::ref_ptr<osg::GraphicsContext::Traits> _traits;
    osg::ref_ptr<osg::Camera> _camera;
    osg::ref_ptr<osg::Camera> _clear_camera;
    osg::ref_ptr<osgViewer::View> _view;
    osg::ref_ptr<osgQt::GraphicsWindowQt> _graphics_window;
};
/* When linking statically, tell OSG which plugins to use. */
#ifdef OSG_LIBRARY_STATIC
USE_OSGPLUGIN(obj)
USE_OSGPLUGIN(ply)
USE_OSGPLUGIN(osg)
#endif

class KeyboardEventHandler : public osgGA::GUIEventHandler
{
private:
    struct HideOSGProblems* _osg;

public:
    KeyboardEventHandler(struct HideOSGProblems* osg) : _osg(osg) {}

    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& /* aa */)
    {
        switch (ea.getEventType()) {
        case osgGA::GUIEventAdapter::KEYDOWN:
            switch (ea.getKey()) {
            case 'l':
                // toggle lighting
                _osg->_root->getOrCreateStateSet()->setMode(GL_LIGHTING,
                        _osg->_root->getOrCreateStateSet()->getMode(GL_LIGHTING)
                        == osg::StateAttribute::OFF ?  osg::StateAttribute::ON : osg::StateAttribute::OFF);
                return true;
            default:
                return false;
            } 
        default:
            return false;
        }
    }

    virtual void accept(osgGA::GUIEventHandlerVisitor& v)
    {
        v.visit(*this);
    };
};

OSGWidget::OSGWidget(QGLWidget* sharing_widget) : QWidget()
{
    assert(sharing_widget);
    setMinimumSize(32, 32);
    _osg = new struct HideOSGProblems;

    _osg->_viewer = new osgViewer::CompositeViewer;
    // Set to single-thread mode to avoid spurious rendering errors:
    _osg->_viewer->setThreadingModel(osgViewer::CompositeViewer::SingleThreaded);
    // Require manual triggering of redraws; don't let OSG interfere:
    _osg->_viewer->setRunFrameScheme(osgViewer::ViewerBase::ON_DEMAND);

    // Create view widget
    _osg->_traits = osgQt::GraphicsWindowQt::createTraits(sharing_widget);
    _osg->_graphics_window = new osgQt::GraphicsWindowQt(_osg->_traits, NULL, sharing_widget);
    if (!_osg->_graphics_window || !_osg->_graphics_window->getGLWidget()) {
        QMessageBox::critical(this, "Error", "Cannot get valid OpenGL context.");
        std::exit(1);
    }
    QGLWidget* view_widget = _osg->_graphics_window->getGLWidget();

    // Create a camera just for clearing the complete widget
    _osg->_clear_camera = new osg::Camera;
    _osg->_clear_camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    _osg->_clear_camera->setRenderOrder(osg::Camera::PRE_RENDER);
    _osg->_clear_camera->setViewport(new osg::Viewport(_osg->_traits->x, _osg->_traits->y, _osg->_traits->width, _osg->_traits->height));

    // Create main camera
    _osg->_camera = new osg::Camera;
    _osg->_camera->setGraphicsContext(_osg->_graphics_window);
    //_osg->_camera->setClearColor(osg::Vec4f(0.2f, 0.2f, 0.4f, 1.0f));   // default taken from osg::View
    _osg->_camera->setClearColor(osg::Vec4f(0.0f, 0.0f, 0.0f, 1.0f));
    _osg->_camera->setViewport(new osg::Viewport(_osg->_traits->x, _osg->_traits->y, _osg->_traits->width, _osg->_traits->height));
    _osg->_camera->setComputeNearFarMode(osgUtil::CullVisitor::DO_NOT_COMPUTE_NEAR_FAR);

    // Create root
    _osg->_root = new osg::Group();
    _osg->_root->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
    _osg->_root->addChild(_osg->_clear_camera);

    // Setup main view
    _osg->_view = new osgViewer::View;
    _osg->_view->setCamera(_osg->_camera);
    _osg->_view->addEventHandler(new KeyboardEventHandler(_osg));
    _osg->_viewer->addView(_osg->_view);

    // Finish Qt setup
    QGridLayout* layout = new QGridLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(view_widget, 0, 0);
    setLayout(layout);
    show();

    // Initialize Simulator and Target
    update_simulator(Simulator());
    update_scene(Target(Target::variant_background_planar), Target());
    _force_mode_update = true;
    set_mode(mode_free_interaction);
}

OSGWidget::~OSGWidget()
{
    delete _osg;
}

void OSGWidget::set_mode(mode_t mode)
{
    if (_force_mode_update || _mode != mode) {
        _mode = mode;
        _osg->_view->setCameraManipulator(new osgGA::TrackballManipulator);     // force reset
        _force_mode_update = false;
    }
    if (_mode == mode_free_interaction) {
        _osg->_background_animation->setMatrix(osg::Matrixf::identity());
        _osg->_target_animation->setMatrix(osg::Matrixf::identity());
        _osg->_view->getCameraManipulator()->setNode(_osg->_target_animation);  // only use the target for home(), not the background
        _osg->_view->home();
    } else {
        _osg->_view->getCameraManipulator()->setByMatrix(osg::Matrixf::identity());
    }
}

void OSGWidget::set_fixed_target_transformation(const float pos[3], const float rot[4])
{
    if (_mode == mode_fixed_target) {
        // The first rotation is necessary for OSG, I don't know why.
        _osg->_target_animation->setMatrix(osg::Matrixf::rotate(static_cast<float>(M_PI_2), -1.0f, 0.0f, 0.0f));
        // The rest is the specified transformation.
        _osg->_target_animation->postMult(osg::Matrixf::rotate(osg::Quat(rot[0], rot[1], rot[2], rot[3])));
        _osg->_target_animation->postMult(osg::Matrixf::translate(pos[0], pos[1], pos[2]));
        // The background is always fixed.
        _osg->_background_animation->setMatrix(osg::Matrixf::identity());
    }
}

void OSGWidget::update_simulator(const Simulator& sim)
{
    _simulator = sim;
}

static osg::Geometry* create_triangle(
        const osg::Vec3f& v0, const osg::Vec3f& v1, const osg::Vec3f& v2,
        const osg::Vec3f& color)
{
    osg::ref_ptr<osg::Vec3Array> vrt = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec3Array> nrm = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> clr = new osg::Vec4Array();
    // front face
    vrt->push_back(v0);
    vrt->push_back(v1);
    vrt->push_back(v2);
    osg::Vec3f fn((v0 - v1) ^ (v2 - v1));
    fn.normalize();
    nrm->resize(nrm->size() + 3, -fn);
    clr->resize(clr->size() + 3, osg::Vec4f(color.x(), color.y(), color.z(), 1.0f));
    // back face
    vrt->push_back(v0);
    vrt->push_back(v2);
    vrt->push_back(v1);
    osg::Vec3f bn((v1 - v0) ^ (v2 - v0));
    bn.normalize();
    nrm->resize(nrm->size() + 3, -bn);
    clr->resize(clr->size() + 3, osg::Vec4f(color.x(), color.y(), color.z(), 1.0f));
    // put it together
    osg::Geometry* geom  = new osg::Geometry();
    geom->setVertexArray(vrt);
    geom->setNormalArray(nrm);
    geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    geom->setColorArray(clr);
    geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, 2 * 3));
    return geom;
}

static osg::Geometry* create_quad(
        const osg::Vec3f& v0, const osg::Vec3f& v1, const osg::Vec3f& v2, const osg::Vec3f& v3,
        const osg::Vec3f& color)
{
    osg::ref_ptr<osg::Vec3Array> vrt = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec3Array> nrm = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> clr = new osg::Vec4Array();
    // front face
    vrt->push_back(v0);
    vrt->push_back(v1);
    vrt->push_back(v2);
    vrt->push_back(v3);
    osg::Vec3f fn((v0 - v1) ^ (v2 - v1));
    fn.normalize();
    nrm->resize(nrm->size() + 4, -fn);
    clr->resize(clr->size() + 4, osg::Vec4f(color.x(), color.y(), color.z(), 1.0f));
    // back face
    vrt->push_back(v0);
    vrt->push_back(v3);
    vrt->push_back(v2);
    vrt->push_back(v1);
    osg::Vec3f bn((v1 - v0) ^ (v3 - v0));
    bn.normalize();
    nrm->resize(nrm->size() + 4, -bn);
    clr->resize(clr->size() + 4, osg::Vec4f(color.x(), color.y(), color.z(), 1.0f));
    // put it together
    osg::Geometry* geom  = new osg::Geometry();
    geom->setVertexArray(vrt);
    geom->setNormalArray(nrm);
    geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    geom->setColorArray(clr);
    geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, 2 * 4));
    return geom;
}

void OSGWidget::update_scene(const Target& background, const Target& target)
{
    static const osg::Vec3f blueish(128 / 255.0f, 128 / 255.0f, 192/ 255.0f);
    static const osg::Vec3f reddish(192 / 255.0f, 128 / 255.0f, 128 / 255.0f);
    static const osg::Vec3f greenish(128 / 255.0f, 192 / 255.0f, 128 / 255.0f);
    static const osg::Vec3f grayish(128 / 255.0f, 128 / 255.0f, 128 / 255.0f);

    _background = background;
    _target = target;

    // Recreate background
    _osg->_root->removeChild(_osg->_background_animation);
    _osg->_background_animation = new osg::MatrixTransform();
    _osg->_background_animation->setMatrix(osg::Matrixf::identity());
    _osg->_background_transformation = new osg::MatrixTransform();
    assert(_background.variant == Target::variant_background_planar);
    if (_background.variant == Target::variant_background_planar) {
        _osg->_background = new osg::Geode;
        if (_background.background_planar_dist > 0.0f) {
            float bg_x0 = - _background.background_planar_width / 2.0f;
            float bg_x1 = - bg_x0;
            float bg_y0 = - _background.background_planar_height / 2.0f;
            float bg_y1 = - bg_y0;
            float bg_z = - _background.background_planar_dist;
            _osg->_background->asGeode()->addDrawable(create_quad(
                        osg::Vec3f(bg_x0, bg_y0, bg_z),
                        osg::Vec3f(bg_x1, bg_y0, bg_z),
                        osg::Vec3f(bg_x1, bg_y1, bg_z),
                        osg::Vec3f(bg_x0, bg_y1, bg_z),
                        grayish));
        }
        _osg->_background->getOrCreateStateSet()->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
        _osg->_background_transformation->setMatrix(osg::Matrixf::identity());
    }

    // Recreate target
    _osg->_root->removeChild(_osg->_target_animation);
    _osg->_target_animation = new osg::MatrixTransform();
    _osg->_target_animation->setMatrix(osg::Matrixf::identity());
    _osg->_target_transformation = new osg::MatrixTransform();
    if (_target.variant == Target::variant_bars) {
        // Compute all bars, starting at (0,0,0)
        std::vector<float> bars;
        bars.reserve(_target.number_of_bars * 5);
        float bar_width = _target.first_bar_width;
        float bar_height = _target.first_bar_height;
        float offset_x = _target.first_offset_x;
        float offset_y = _target.first_offset_y;
        float offset_z = _target.first_offset_z;
        float tlx = 0.0f, tly = 0.0f, tlz = 0.0f;
        float min_x = +std::numeric_limits<float>::max();
        float max_x = -std::numeric_limits<float>::max();
        float min_y = +std::numeric_limits<float>::max();
        float max_y = -std::numeric_limits<float>::max();
        float min_z = +std::numeric_limits<float>::max();
        float max_z = -std::numeric_limits<float>::max();
        for (int i = 0; i < _target.number_of_bars; i++) {
            bars.push_back(tlx);
            bars.push_back(tly);
            bars.push_back(bar_width);
            bars.push_back(bar_height);
            bars.push_back(tlz);
            min_x = std::min(min_x, tlx);
            max_x = std::max(max_x, tlx + bar_width);
            min_y = std::min(min_y, tly);
            max_y = std::max(max_y, tly + bar_height);
            min_z = std::min(min_z, tlz);
            max_z = std::max(max_z, tlz);
            tlx += offset_x;
            tly += offset_y;
            tlz += offset_z;
            bar_width = bar_width * _target.next_bar_width_factor + _target.next_bar_width_offset;
            bar_height = bar_height * _target.next_bar_height_factor + _target.next_bar_height_offset;
            offset_x = offset_x * _target.next_offset_x_factor + _target.next_offset_x_offset;
            offset_y = offset_y * _target.next_offset_y_factor + _target.next_offset_y_offset;
            offset_z = offset_z * _target.next_offset_z_factor + _target.next_offset_z_offset;
        }
        // Shift all bars so that they are centered in the xy plane and the scene starts at z=0
        for (int i = 0; i < _target.number_of_bars; i++) {
            bars[5 * i + 0] -= (max_x - min_x) / 2.0f;
            bars[5 * i + 1] -= (max_y - min_y) / 2.0f;
            bars[5 * i + 4] -= max_z;
        }
        // Add geometry for all bars
        _osg->_target = new osg::Geode;
        for (int i = 0; i < _target.number_of_bars; i++)
            _osg->_target->asGeode()->addDrawable(create_quad(
                        osg::Vec3f(bars[5*i+0]              , bars[5*i+1]              , bars[5*i+4]),
                        osg::Vec3f(bars[5*i+0] + bars[5*i+2], bars[5*i+1]              , bars[5*i+4]),
                        osg::Vec3f(bars[5*i+0] + bars[5*i+2], bars[5*i+1] + bars[5*i+3], bars[5*i+4]),
                        osg::Vec3f(bars[5*i+0]              , bars[5*i+1] + bars[5*i+3], bars[5*i+4]),
                        i % 2 == 0 ? greenish : reddish));
        // Add background plane
        if (_target.bar_background_near_side >= 0 && _target.bar_background_near_side <= 3) {
            float bg_x0 = -(max_x - min_x) / 2.0f;
            float bg_x1 = -bg_x0;
            float bg_y0 = -(max_y - min_y) / 2.0f;
            float bg_y1 = -bg_y0;
            float bg_z_tl = min_z - max_z;
            float bg_z_tr = min_z - max_z;
            float bg_z_bl = min_z - max_z;
            float bg_z_br = min_z - max_z;
            if (_target.bar_background_near_side == 0) {
                bg_z_tl -= _target.bar_background_dist_near;
                bg_z_tr -= _target.bar_background_dist_far;
                bg_z_bl -= _target.bar_background_dist_near;
                bg_z_br -= _target.bar_background_dist_far;
            } else if (_target.bar_background_near_side == 1) {
                bg_z_tl -= _target.bar_background_dist_near;
                bg_z_tr -= _target.bar_background_dist_near;
                bg_z_bl -= _target.bar_background_dist_far;
                bg_z_br -= _target.bar_background_dist_far;
            } else if (_target.bar_background_near_side == 2) {
                bg_z_tl -= _target.bar_background_dist_far;
                bg_z_tr -= _target.bar_background_dist_near;
                bg_z_bl -= _target.bar_background_dist_far;
                bg_z_br -= _target.bar_background_dist_near;
            } else {
                bg_z_tl -= _target.bar_background_dist_far;
                bg_z_tr -= _target.bar_background_dist_far;
                bg_z_bl -= _target.bar_background_dist_near;
                bg_z_br -= _target.bar_background_dist_near;
            }
            _osg->_target->asGeode()->addDrawable(create_quad(
                        osg::Vec3f(bg_x0, bg_y0, bg_z_bl),
                        osg::Vec3f(bg_x1, bg_y0, bg_z_br),
                        osg::Vec3f(bg_x1, bg_y1, bg_z_tr),
                        osg::Vec3f(bg_x0, bg_y1, bg_z_tl),
                        blueish));
        }
        _osg->_target->getOrCreateStateSet()->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
        _osg->_target_transformation->setMatrix(osg::Matrixf::rotate(static_cast<float>(M_PI_2), 1.0f, 0.0f, 0.0f));
        _osg->_target_transformation->postMult(osg::Matrixf::rotate(static_cast<float>(_target.bar_rotation), 0.0f, 1.0f, 0.0f));
    } else if (_target.variant == Target::variant_star) {
        _osg->_target = new osg::Geode;
        float spoke_width = static_cast<float>(M_PI) / _target.star_spokes;
        float spokes_start = - spoke_width / 2.0f;
        for (int i = 0; i < 2 * _target.star_spokes; i++) {
            float start_angle = spokes_start + i * spoke_width;
            float end_angle = start_angle + spoke_width;
            osg::Vec2f v1(_target.star_radius * cosf(start_angle), _target.star_radius * sinf(start_angle));
            osg::Vec2f v2(_target.star_radius * cosf(end_angle), _target.star_radius * sinf(end_angle));
            // background spoke
            _osg->_target->asGeode()->addDrawable(create_triangle(
                        osg::Vec3f(0.0f, 0.0f, -_target.star_background_dist_center),
                        osg::Vec3f(v1.x(), v1.y(), -_target.star_background_dist_rim),
                        osg::Vec3f(v2.x(), v2.y(), -_target.star_background_dist_rim),
                        blueish));
            if (i % 2 == 0) {
                // flat spoke in front of the background
                _osg->_target->asGeode()->addDrawable(create_triangle(
                            osg::Vec3f(0.0f, 0.0f, 0.0f),
                            osg::Vec3f(v1.x(), v1.y(), 0.0f),
                            osg::Vec3f(v2.x(), v2.y(), 0.0f),
                            greenish));
            }
        }
        _osg->_target->getOrCreateStateSet()->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
        _osg->_target_transformation->setMatrix(osg::Matrixf::rotate(static_cast<float>(M_PI_2), 1.0f, 0.0f, 0.0f));
    } else {
        _osg->_target = osgDB::readNodeFile(_target.model_filename);
        if (!_osg->_target) {
            // Fallback geometry
            _osg->_target = new osg::Geode;
            _osg->_target->asGeode()->addDrawable(new osg::ShapeDrawable(
                        new osg::Box(osg::Vec3f(0.0f, 0.0f, 0.0f), 0.3f, 0.2f, 0.15f)));
        }
        _osg->_target_transformation->setMatrix(osg::Matrixf::identity());
    }

    // Convert the scene to optimized, index triangles. We need that for capturing
    // the geometry into triangle patches (see capture() and update()).
    osgUtil::Optimizer optimizer;
    optimizer.optimize(_osg->_background.get(), osgUtil::Optimizer::DEFAULT_OPTIMIZATIONS
            | osgUtil::Optimizer::INDEX_MESH);
    optimizer.optimize(_osg->_target.get(), osgUtil::Optimizer::DEFAULT_OPTIMIZATIONS
            | osgUtil::Optimizer::INDEX_MESH);

    _osg->_background_transformation->addChild(_osg->_background);
    _osg->_background_animation->addChild(_osg->_background_transformation);
    _osg->_root->addChild(_osg->_background_animation);
    _osg->_target_transformation->addChild(_osg->_target);
    _osg->_target_animation->addChild(_osg->_target_transformation);
    _osg->_root->addChild(_osg->_target_animation);

    _osg->_view->setSceneData(_osg->_root.get());
#if 0
    if (_mode == mode_free_interaction) {
        _force_mode_update = true;
        set_mode(_mode);
    }
#endif
}

void OSGWidget::draw_frame()
{
    _osg->_clear_camera->getViewport()->setViewport(0, 0, width(), height());
    int viewport[4];
    float clearcolor[3];
    get_viewport(_simulator.aspect_ratio(), viewport, clearcolor);
    _osg->_clear_camera->setClearColor(osg::Vec4f(clearcolor[0], clearcolor[1], clearcolor[2], 1.0f));
    _osg->_camera->getViewport()->setViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    _osg->_camera->setProjectionMatrixAsPerspective(_simulator.aperture_angle,
            _simulator.aspect_ratio(), _simulator.near_plane, _simulator.far_plane);
    // _osg->_viewer->frame() calls advance(), eventTraversal(), updateTraversal(), and finally renderingTraversals().
    // We want to compensate camera manipulation so that the background stays in place. This has to be done between
    // updateTraversal() and renderingTraversals().
    // Therfore, we cannot simply use frame(); we have to call everything manually.
    _osg->_viewer->advance();
    _osg->_viewer->eventTraversal();
    _osg->_viewer->updateTraversal();
    _osg->_background_animation->setMatrix(_osg->_view->getCameraManipulator()->getMatrix());
    _osg->_viewer->renderingTraversals();
}

void OSGWidget::export_frustum(const std::string& filename)
{
    bool ok;

    // Get frustum
    double l, r, b, t, n, f;
    ok = _osg->_camera->getProjectionMatrixAsFrustum(l, r, b, t, n, f);
    assert(ok);
    osg::Vec3f near_tl(l, t, -n);
    osg::Vec3f near_tr(r, t, -n);
    osg::Vec3f near_bl(l, b, -n);
    osg::Vec3f near_br(r, b, -n);
    osg::Vec3f far_tl(f / n * l, f / n * t, -f);
    osg::Vec3f far_tr(f / n * r, f / n * t, -f);
    osg::Vec3f far_bl(f / n * l, f / n * b, -f);
    osg::Vec3f far_br(f / n * r, f / n * b, -f);

    // Create frustum geometry
    osg::ref_ptr<osg::Vec3Array> vrt = new osg::Vec3Array();
    vrt->push_back(near_tl); vrt->push_back(near_tr);
    vrt->push_back(near_tr); vrt->push_back(near_br);
    vrt->push_back(near_br); vrt->push_back(near_bl);
    vrt->push_back(near_bl); vrt->push_back(near_tl);
    vrt->push_back(far_tl); vrt->push_back(far_tr);
    vrt->push_back(far_tr); vrt->push_back(far_br);
    vrt->push_back(far_br); vrt->push_back(far_bl);
    vrt->push_back(far_bl); vrt->push_back(far_tl);
    vrt->push_back(osg::Vec3f(0.0f, 0.0f, 0.0f)); vrt->push_back(far_tl);
    vrt->push_back(osg::Vec3f(0.0f, 0.0f, 0.0f)); vrt->push_back(far_tr);
    vrt->push_back(osg::Vec3f(0.0f, 0.0f, 0.0f)); vrt->push_back(far_br);
    vrt->push_back(osg::Vec3f(0.0f, 0.0f, 0.0f)); vrt->push_back(far_bl);
    osg::Geometry* geom  = new osg::Geometry();
    geom->setVertexArray(vrt);
    geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, 3 * 4 * 2));
    osg::ref_ptr<osg::Node> frustum = new osg::Geode;
    frustum->asGeode()->addDrawable(geom);

    // Export
    ok = osgDB::writeNodeFile(*frustum, filename);
    if (!ok) {
        throw std::runtime_error("Cannot export model file.");
    }
}

void OSGWidget::export_background(const std::string& filename)
{
    bool ok = osgDB::writeNodeFile(*_osg->_background, filename);
    if (!ok) {
        throw std::runtime_error("Cannot export model file.");
    }
}

void OSGWidget::export_target(const std::string& filename)
{
    bool ok = osgDB::writeNodeFile(*_osg->_target, filename);
    if (!ok) {
        throw std::runtime_error("Cannot export model file.");
    }
}


/* Code to extract geometry from the scene graph */

class Extractor : public osg::NodeVisitor
{
private:
    osg::Matrix _cam_matrix;
    std::vector<TrianglePatch>* _scene;
    int _index;
    bool _update_only;

public:
    Extractor(const osg::Matrix& cam_matrix, std::vector<TrianglePatch>* scene, bool update_only)
        : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ACTIVE_CHILDREN),
        _cam_matrix(cam_matrix), _scene(scene), _index(0), _update_only(update_only)
    {
    }

    virtual void apply(osg::Geode& node)
    {
        osg::Matrix mat = osg::computeLocalToEye(_cam_matrix, getNodePath());
        if (_update_only) {
            for (int i = 0; i < 16; i++)
                _scene->at(_index).transformation[i] = mat.ptr()[i];
        } else {
            _scene->push_back(TrianglePatch());
            for (int i = 0; i < 16; i++)
                _scene->back().transformation[i] = mat.ptr()[i];
            if (node.getNumDrawables() > 0) {
                assert(node.getNumDrawables() == 1);
                // Get vertex data. We expect index triangles!
                const osg::Drawable* drawable = node.getDrawable(0);
                const osg::Geometry* geom = dynamic_cast<const osg::Geometry*>(drawable);
                const osg::Array* vertex_array = geom->getVertexArray();
                const osg::Array* normal_array = geom->getNormalArray();
                const osg::Array* color_array = geom->getColorArray();
                const osg::Array* texcoord_array = geom->getTexCoordArray(0);
                // Sanity check.
                assert(vertex_array);
                assert(vertex_array->getType() == osg::Array::Vec3ArrayType);
                assert(normal_array);
                assert(normal_array->getType() == osg::Array::Vec3ArrayType);
                assert(normal_array->getNumElements() == vertex_array->getNumElements());
                assert(!color_array || color_array->getType() == osg::Array::Vec4ArrayType);
                assert(!color_array || color_array->getNumElements() == vertex_array->getNumElements());
                assert(!texcoord_array || texcoord_array->getType() == osg::Array::Vec2ArrayType);
                assert(!texcoord_array || texcoord_array->getNumElements() == vertex_array->getNumElements());
                // Copy the vertex and attribute data to our triangle patches
                _scene->back().vertex_array.resize(3 * vertex_array->getNumElements());
                std::memcpy(&(_scene->back().vertex_array[0]), vertex_array->getDataPointer(), sizeof(float) * 3 * vertex_array->getNumElements());
                _scene->back().normal_array.resize(3 * normal_array->getNumElements());
                std::memcpy(&(_scene->back().normal_array[0]), normal_array->getDataPointer(), sizeof(float) * 3 * normal_array->getNumElements());
                if (color_array) {
                    _scene->back().color_array.resize(4 * color_array->getNumElements());
                    std::memcpy(&(_scene->back().color_array[0]), color_array->getDataPointer(), sizeof(float) * 4 * color_array->getNumElements());
                }
                if (texcoord_array) {
                    _scene->back().texcoord_array.resize(2 * color_array->getNumElements());
                    std::memcpy(&(_scene->back().texcoord_array[0]), texcoord_array->getDataPointer(), sizeof(float) * 2 * texcoord_array->getNumElements());
                }
                // Now get the correct sequence of indices. We need a visitor just for that...
                for (unsigned int i = 0; i < geom->getNumPrimitiveSets(); i++) {
                    const osg::PrimitiveSet* ps = geom->getPrimitiveSet(i);
                    IndexVisitor iv(&_scene->back());
                    ps->accept(iv);
                }
                // TODO: get texture: _scene->back().texture = ...;
            }
        }
        traverse(node);
        _index++;
    }

    class IndexVisitor : public osg::PrimitiveIndexFunctor
    {
    private:
        TrianglePatch* _tp;

        template<typename T>
        void _drawElements(GLenum mode, GLsizei count, const T* indices)
        {
            assert(mode == GL_TRIANGLES || mode == GL_TRIANGLE_STRIP);
            assert(count > 0);
            assert(indices);
            if (mode == GL_TRIANGLES) {
                for (int i = 2; i < count; i += 3) {
                    _tp->index_array.push_back(indices[i - 2]);
                    _tp->index_array.push_back(indices[i - 1]);
                    _tp->index_array.push_back(indices[i    ]);
                }
            } else if (mode == GL_TRIANGLE_STRIP) {
                for (int i = 2; i < count; i++) {
                    if ((i % 2) == 0) {
                        _tp->index_array.push_back(indices[i - 2]);
                        _tp->index_array.push_back(indices[i - 1]);
                        _tp->index_array.push_back(indices[i    ]);
                    } else {
                        _tp->index_array.push_back(indices[i - 2]);
                        _tp->index_array.push_back(indices[i    ]);
                        _tp->index_array.push_back(indices[i - 1]);
                    }
                }
            }
        }

    public:
        IndexVisitor(TrianglePatch* tp) : _tp(tp)
        {
        }

        virtual void setVertexArray(unsigned int, const osg::Vec2*) {}
        virtual void setVertexArray(unsigned int, const osg::Vec3*) {}
        virtual void setVertexArray(unsigned int, const osg::Vec4*) {}
        virtual void setVertexArray(unsigned int, const osg::Vec2d*) {}
        virtual void setVertexArray(unsigned int, const osg::Vec3d*) {}
        virtual void setVertexArray(unsigned int, const osg::Vec4d*) {}

        virtual void drawArrays(GLenum, GLint, GLsizei) {}
        virtual void drawElements(GLenum mode, GLsizei count, const GLuint* indices) { _drawElements<GLuint>(mode, count, indices); }
        virtual void drawElements(GLenum mode, GLsizei count, const GLubyte* indices) { _drawElements<GLubyte>(mode, count, indices); }
        virtual void drawElements(GLenum mode, GLsizei count, const GLushort* indices) { _drawElements<GLushort>(mode, count, indices); }

        virtual void begin(GLenum) {}
        virtual void vertex(unsigned int) {}
        virtual void end() {}
    };
};

void OSGWidget::capture_scene(std::vector<TrianglePatch>* scene) const
{
    scene->clear();
    Extractor extractor(_osg->_camera->getViewMatrix(), scene, false);
    static_cast<osg::Node*>(_osg->_root)->accept(extractor);
}

void OSGWidget::update_scene(std::vector<TrianglePatch>* scene) const
{
    Extractor extractor(_osg->_camera->getViewMatrix(), scene, true);
    static_cast<osg::Node*>(_osg->_root)->accept(extractor);
}
