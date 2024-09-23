#include <json.hpp>
#include <fstream>
#include <iostream>
#include <thread>
#include "CoordinateHandler.h"

#include <QScreen>
#include <QApplication>
#include <QMainWindow>

using namespace std;
using namespace nlohmann;

int genData(){
    CoordinateHandler::gen(std::map<std::string, int>{}, std::map<std::string, std::string>{}, "hi");
}

int main(int argc, char *argv[]) {

    //TODO: Multithread the bitch to get it working

    QApplication app(argc, argv);



    std::thread t(genData);

    t.join();

    QMainWindow mainWindow;

    mainWindow.resize(qApp->screens()[0]->size().width(), qApp->screens()[0]->size().height());

    mainWindow.show();


//    json test = {
//            {"name", "JoHn"},
//            {"age", 26},
//            {"isStudent", false}
//    };
//
//    ifstream inputFile("../test.json");
//    json testData;
//    inputFile >> testData;
//    string color = testData["color"];
//
//    cout << "Color: " << color << "\n";


    return app.exec();


};

