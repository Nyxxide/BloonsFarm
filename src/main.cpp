#include "BloonsUIMain.h"

#include <QApplication>

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char *argv[]) {

    QApplication MainUI(argc, argv);

    auto BloonsUI = BloonsUIMain();

    return MainUI.exec();


};

