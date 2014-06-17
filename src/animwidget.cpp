/*
 * Copyright (C) 2012, 2013, 2014
 * Computer Graphics Group, University of Siegen, Germany.
 * http://www.cg.informatik.uni-siegen.de/
 * All rights reserved.
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>.
 */

#include <QPushButton>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QGridLayout>

#include "animwidget.h"


// Helper function: Get the icon with the given name from the icon theme.
// If unavailable, fall back to the built-in icon. Icon names conform to this specification:
// http://standards.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html
static QIcon get_icon(const QString &name)
{
    return QIcon::fromTheme(name, QIcon(QString(":icons/") + name));
}

AnimWidget::AnimWidget() : QGroupBox("Animation"), _lock(false)
{
    setCheckable(true);
    setChecked(false);

    _play_btn = new QPushButton(get_icon("media-playback-start"), "");
    connect(_play_btn, SIGNAL(clicked()), this, SLOT(play_clicked()));
    _pause_btn = new QPushButton(get_icon("media-playback-pause"), "");
    connect(_pause_btn, SIGNAL(clicked()), this, SLOT(pause_clicked()));
    _stop_btn = new QPushButton(get_icon("media-playback-stop"), "");
    connect(_stop_btn, SIGNAL(clicked()), this, SLOT(stop_clicked()));
    _loop_btn = new QPushButton(get_icon("media-playlist-repeat"), "");
    _loop_btn->setCheckable(true);

    _slider = new QSlider(Qt::Horizontal);
    _slider->setRange(0, 2000);
    _slider->setTracking(false);
    connect(_slider, SIGNAL(valueChanged(int)), this, SLOT(slider_changed()));

    _elapsed_box = new QDoubleSpinBox;
    _elapsed_box->setDecimals(3);
    _elapsed_box->setSingleStep(0.1);
    _elapsed_box->setKeyboardTracking(false);
    connect(_elapsed_box, SIGNAL(valueChanged(double)), this, SLOT(box_changed()));

    _elapsed_label = new QLabel;
    _elapsed_label->setAlignment(Qt::AlignRight);
    _elapsed_label->setTextFormat(Qt::PlainText);
    _elapsed_label->setFrameShape(QFrame::StyledPanel);

    connect(this, SIGNAL(toggled(bool)), this, SLOT(update_enabled()));

    update_animation(_animation);

    QGridLayout* l = new QGridLayout;
    l->addWidget(_play_btn, 0, 0);
    l->addWidget(_pause_btn, 0, 1);
    l->addWidget(_stop_btn, 0, 2);
    l->addWidget(_slider, 0, 3);
    l->addWidget(_elapsed_box, 0, 4);
    l->addWidget(_elapsed_label, 0, 5);
    l->addWidget(_loop_btn, 0, 6);
    setLayout(l);
}

bool AnimWidget::loop() const
{
    return _loop_btn->isChecked();
}

void AnimWidget::enable()
{
    setChecked(true);
}

void AnimWidget::start()
{
    stop_clicked();
    play_clicked();
}

void AnimWidget::pause()
{
    pause_clicked();
}

void AnimWidget::stop()
{
    stop_clicked();
}

void AnimWidget::update_enabled()
{
    _state = (isChecked() ? state_stopped : state_disabled);
    do_update_state();
}

void AnimWidget::do_update_state()
{
    switch (_state) {
    case state_disabled:
        _play_btn->setEnabled(false);
        _pause_btn->setEnabled(false);
        _stop_btn->setEnabled(false);
        break;
    case state_stopped:
        _play_btn->setEnabled(true);
        _pause_btn->setEnabled(false);
        _stop_btn->setEnabled(false);
        break;
    case state_active:
        _play_btn->setEnabled(false);
        _pause_btn->setEnabled(true);
        _stop_btn->setEnabled(true);
        break;
    case state_paused:
        _play_btn->setEnabled(true);
        _pause_btn->setEnabled(false);
        _stop_btn->setEnabled(true);
        break;
    }
    emit update_state(_state);
}    

void AnimWidget::update_animation(const Animation& animation)
{
    _animation = animation;
    if (_animation.is_valid()) {
        _state = isChecked() ? state_stopped : state_disabled;
        _duration = _animation.end_time() - _animation.start_time();
        setEnabled(true);
    } else {
        _state = state_disabled;
        _duration = 0;
        setEnabled(false);
        setChecked(false);
    }
    update(0);
    do_update_state();
}

void AnimWidget::update(long long t)
{
    _lock = true;
    if (_animation.is_valid()) {
        _t = t - _animation.start_time();
        _slider->setValue(_duration == 0 ? 0 : _t * 2000 / _duration);
        if (!_elapsed_box->hasFocus()) {
            _elapsed_box->setRange(_animation.start_time() / 1e6, _animation.end_time() / 1e6);
            _elapsed_box->setValue(_t / 1e6);
        }
        _elapsed_label->setText(QString(" / %2").arg(_duration / 1e6, 0, 'f', 3));
    } else {
        _t = 0;
        _slider->setValue(0);
        _elapsed_box->setValue(0.0);
        _elapsed_label->setText("");
    }
    _lock = false;
}

void AnimWidget::play_clicked()
{
    bool resume = (_state == state_paused);
    _state = state_active;
    do_update_state();
    if (!resume)
        emit update_time(_animation.start_time());
}

void AnimWidget::pause_clicked()
{
    _state = state_paused;
    do_update_state();
}

void AnimWidget::stop_clicked()
{
    _state = state_stopped;
    update(0);
    do_update_state();
}

void AnimWidget::slider_changed()
{
    if (_lock)
        return;
    _t = _duration / 2000 * _slider->value();
    update(_t);
    emit update_time(_t + _animation.start_time());
}

void AnimWidget::box_changed()
{
    if (_lock)
        return;
    _t = _elapsed_box->value() * 1e6;
    update(_t);
    emit update_time(_t + _animation.start_time());
}
