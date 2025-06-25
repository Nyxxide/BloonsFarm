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

using namespace std;
using namespace nlohmann;

class BloonsUICoords : public QMainWindow{
private:
    string title;
    string fileName;
    vector<string> button_labels;
    vector<QPushButton*> buttons;
    QLabel *subwinlab;
    QLabel *subwinlab2;

public:
    BloonsUICoords(string name, vector<string> labels, string file);

    void resetpos();
};


#endif