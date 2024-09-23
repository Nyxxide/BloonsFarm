#include "CoordinateHandler.h"
#include <json.hpp>
#include <iostream>
#include <fstream>

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

void CoordinateHandler::gen(std::map<std::string, int> towerData, std::map<std::string, std::string> menuNavData,
                                   std::string) {

//    QSize size = qApp->screens()[0]->size();
//    std::cout << size.height();
//    std::cout << "\n";
//    std::cout << size.width();
}
