/*
 * Copyright (C) 2012, 2013, 2014
 * Computer Graphics Group, University of Siegen, Germany.
 * http://www.cg.informatik.uni-siegen.de/
 * All rights reserved.
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>.
 */

#ifndef ANIMWIDGET_H
#define ANIMWIDGET_H

#include <QGroupBox>

#include "animation.h"

class QPushButton;
class QSlider;
class QDoubleSpinBox;
class QLabel;

class AnimWidget : public QGroupBox
{
    Q_OBJECT

public:
    typedef enum {
        state_disabled,
        state_stopped,
        state_active,
        state_paused
    } state_t;

private:
    state_t _state;
    long long _duration;
    long long _t;
    Animation _animation;
    bool _lock;
    QPushButton* _play_btn;
    QPushButton* _pause_btn;
    QPushButton* _stop_btn;
    QSlider* _slider;
    QDoubleSpinBox* _elapsed_box;
    QLabel* _elapsed_label;
    QPushButton* _loop_btn;

private slots:
    void update_enabled();
    void do_update_state();
    void play_clicked();
    void pause_clicked();
    void stop_clicked();
    void slider_changed();
    void box_changed();

public:
    AnimWidget();

    state_t state() const { return _state; }
    bool loop() const;
    void enable();
    void start();
    void pause();
    void stop();

public slots:
    void update_animation(const Animation&);
    void update(long long t);

signals:
    void update_state(AnimWidget::state_t);
    void update_time(long long);
};

#endif
