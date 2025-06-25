#ifndef BLOONSFARM_BLOONSUIMAIN_H
#define BLOONSFARM_BLOONSUIMAIN_H

#include <json.hpp>
#include <uiohook.h>
#include <opencv2/opencv.hpp>

#include <fstream>
#include <iostream>
#include <filesystem>
#include <thread>
#include <vector>

#include <QPushButton>
#include <QScreen>
#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFontDatabase>
#include <QLabel>
#include <QMenuBar>

#include "BloonsUIPopup.h"
#include "BloonsUICoords.h"
#include "CommonGlobals.h"
#include "CoordinateHandler.h"

class BloonsUIMain : public QMainWindow{
private:
    vector<QPushButton*> farmButtonList;
    QPushButton* quitButton;
    QLabel* mainLabel;
    QLabel* activeLabel;

public:
    BloonsUIMain();

    void editcoords();

    void setMaxFontSize(QPushButton* button, double maxFontSizePt);

    void startloop();

    void endloop();
};

#endif
