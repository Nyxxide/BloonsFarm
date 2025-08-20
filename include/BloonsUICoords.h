#ifndef BLOONSFARM_BLOONSUICOORDS_H
#define BLOONSFARM_BLOONSUICOORDS_H

#include <json.hpp>
#include <uiohook.h>

#include <fstream>
#include <iostream>
#include <filesystem>
#include <vector>

#include <QPushButton>
#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFontDatabase>
#include <QLabel>

#include "CoordinateHandler.h"
#include "BloonsUIPopup.h"

class BloonsUICoords : public QMainWindow{
private:
    std::string title;
    std::string fileName;
    std::vector<std::string> button_labels;
    std::vector<QPushButton*> buttons;
    QLabel *subwinlab;
    QLabel *subwinlab2;

public:
    BloonsUICoords(std::string name, std::vector<std::string> labels, std::string file);

    void resetpos();
};


#endif