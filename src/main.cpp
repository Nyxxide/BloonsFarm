#include "BloonsUIMain.h"

#include <QApplication>

int main(int argc, char *argv[]) {

    QApplication MainUI(argc, argv);

    auto BloonsUI = BloonsUIMain();

    return MainUI.exec();

//    QApplication app(argc, argv);
//
//    std::thread t(genData);
//    t.join();
//
//
//    QMainWindow mainWindow;
//    mainWindow.resize(qApp->screens()[0]->size().width(), qApp->screens()[0]->size().height());
//    mainWindow.show();


};

