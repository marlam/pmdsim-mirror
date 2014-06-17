/*
 * Copyright (C) 2012, 2013, 2014
 * Computer Graphics Group, University of Siegen, Germany.
 * http://www.cg.informatik.uni-siegen.de/
 * All rights reserved.
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>.
 */

#include <cmath>

#include <QDebug>
#include <QApplication>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
#ifdef Q_WS_X11
    // Switch sync-to-vblank off by default on Linux
    setenv("__GL_SYNC_TO_VBLANK", "0", 1);
#endif
    QCoreApplication::setAttribute(Qt::AA_X11InitThreads);
    QCoreApplication::setOrganizationName("PMDSim");
    QCoreApplication::setApplicationName("PMDSim");
    QApplication app(argc, argv, QApplication::GuiClient);

    QStringList cmdline = app.arguments();
    QString simulator_file;
    QString background_file;
    QString target_file;
    QString animation_file;
    QString export_dir;
    bool export_animation = false;
    double export_frame = 1.0 / 0.0;
    bool minimize_window = false;
    for (int i = 1; i < cmdline.size(); i++) {
        bool conv_ok = true;
        if (cmdline.at(i).startsWith("--simulator=")) {
            simulator_file = cmdline.at(i).section('=', 1);
        } else if (cmdline.at(i).startsWith("--background=")) {
            background_file = cmdline.at(i).section('=', 1);
        } else if (cmdline.at(i).startsWith("--target=")) {
            target_file = cmdline.at(i).section('=', 1);
        } else if (cmdline.at(i).startsWith("--animation=")) {
            animation_file = cmdline.at(i).section('=', 1);
        } else if (cmdline.at(i).startsWith("--export-dir=")) {
            export_dir = cmdline.at(i).section('=', 1);
        } else if (cmdline.at(i).compare("--export-animation") == 0) {
            export_animation = true;
        } else if (cmdline.at(i).startsWith("--export-frame=")
                && std::isfinite((export_frame = cmdline.at(i).section('=', 1).toDouble(&conv_ok)))
                && conv_ok) {
        } else if (cmdline.at(i).compare("--minimize") == 0) {
            minimize_window = true;
        } else {
            qWarning() << "Invalid argument" << cmdline.at(i);
            return 1;
        }
    }
    MainWindow* mainwindow = new MainWindow(simulator_file, background_file, target_file, animation_file,
            export_dir, export_animation, export_frame, minimize_window);
    int ret = app.exec();
    delete mainwindow;
    return ret;
}
