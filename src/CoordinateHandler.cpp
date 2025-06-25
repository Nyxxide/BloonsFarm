#include "CoordinateHandler.h"

#include <json.hpp>

#include <iostream>
#include <fstream>
#include <cmath>
#include <string>

#include <QRect>
#include <QApplication>
#include <QScreen>
#include <QGuiApplication>

using namespace std;
using namespace nlohmann;


std::string CoordinateHandler::nameConversion(char hotkey) {
    switch(hotkey){
        case 'q':
            return "dart";
        case 'w':
            return "boomerang";
        case 'e':
            return "bomb_shooter";
        case 'r':
            return "tack_shooter";
        case 't':
            return "ice";
        case 'y':
            return "glue_gunner";
        case 'z':
            return "sniper";
        case 'x':
            return "sub";
        case 'c':
            return "buccaneer";
        case 'v':
            return "plane";
        case 'b':
            return "heli_pilot";
        case 'n':
            return "mortar";
        case 'm':
            return "dartling";
        case 'a':
            return "wizard";
        case 's':
            return "super_monkey";
        case 'd':
            return "ninja";
        case 'f':
            return "alchemist";
        case 'g':
            return "druid";
        case 'h':
            return "farm";
        case 'j':
            return "spike";
        case 'k':
            return "village";
        case 'l':
            return "engineer";
        case 'i':
            return "beast_handler";
        default:
            return "error";
    }

}

void CoordinateHandler::gen(json towerData, json menuNavData, string fileName) {
    QSize size = qApp->screens()[0]->size();
    int screenHeight = size.height();
    int screenWidth = size.width();

    double x_fact = screenWidth/1920;
    double y_fact = screenHeight/1080;
    double towerx_fact = x_fact;
    double towery_fact = y_fact;

    double check = abs(y_fact/x_fact);
    if(check > 1.3 || check < 0.74){
        towerx_fact = x_fact*1.1578125;
        towery_fact = y_fact/1.10925925925;
    }

    json data = { {"towers" , {}}, {"menuNav", {}} };
    json counter = {};

    for(const auto& tower : towerData){
        string hotkey = tower["hotkey"];
        string towerName = CoordinateHandler::nameConversion(hotkey[0]) + "_pos";
        if(data["towers"].contains(towerName)){
            if(!counter.contains(towerName)){
                counter[towerName] = 2;
            }
            else{
                counter[towerName] += 1;
            }
            string towerNum = to_string(counter[towerName]);
            towerName = CoordinateHandler::nameConversion(hotkey[0]) + "_" + towerNum + "_pos";
            data["towers"][towerName] = {
                    {"hotkey", tower["hotkey"]},
                    {"x", tower["x"]},
                    {"y", tower["y"]},
                    {"top", tower["top"]},
                    {"middle", tower["middle"]},
                    {"bottom", tower["bottom"]}
            };
        }
        else{

            data["towers"][towerName] = {
                    {"hotkey", tower["hotkey"]},
                    {"x", tower["x"]},
                    {"y", tower["y"]},
                    {"top", tower["top"]},
                    {"middle", tower["middle"]},
                    {"bottom", tower["bottom"]}
            };
        }
    }

    cout << "Test";
    flush(cout);
    data["menuNav"] = {
            {"mapDifficulty", menuNavData["mapDifficulty"]},
            {"map", menuNavData["map"]},
            {"difficulty", menuNavData["difficulty"]},
            {"mode", menuNavData["mode"]}
    };

    fileName = "Tower Positions/" + fileName + ".json";
    ofstream outFile(fileName);
    if(!outFile){
        cerr << "Can't open file." << endl;
        return;
    }

    outFile << data.dump(4);
    outFile.close();
}
