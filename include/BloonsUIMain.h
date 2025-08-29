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
#include <atomic>

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
#include "InputHelpers.h"

#ifdef _WIN32
#include <windows.h>
#endif

class BloonsUIMain : public QMainWindow{
private:
    std::vector<QPushButton*> farmButtonList;
    QPushButton* quitButton;
    QLabel* mainLabel;
    QLabel* activeLabel;
    std:: string activeFile;

    std::thread loopThread;
    std::atomic_bool running{false};
    bool abort() const {return !running.load();}

public:
    // NOTES:
    // Deflation, Half Cash, Impoppable, CHIMPS all have tooltips
    // Apopalypse has one giant tooltip
    // Get images for each tooltip as well as saved game
    // Get new image for expert/(advanced/intermediate/beginner)?

    // For collection event, endhome->wait 5 sec->collect->click instamonkey->wait 2 sec->loop until no more instamonkey (or until sees continue)->back button

    BloonsUIMain();

    void editCoords();

    void setMaxFontSize(QPushButton* button, double maxFontSizePt);

    void farmLoop();

    void towerPlacement(std::string fileName);

    void menuNav(std::string fileName);

    void startLoop();

    void endLoop();
};

#endif
