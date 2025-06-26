//#include <QApplication>
//#include <QString>
//#include <opencv2/opencv.hpp>
//#include <iostream>
//
//#include "CommonGlobals.h"     // loadEmbeddedImage()
//#include "ScreenGrabber.h"     // ScreenGrabber::grabScreen()
//#include "TemplateMatch.h"     // locateOnScreen(), Match
//
//void sendMouseMove(int x, int y)
//{
//#ifdef _WIN32
//    // ::SetCursorPos(x, y);
//#elif defined(__linux__)
//    // Display* d = XOpenDisplay(nullptr);
//    // XWarpPointer(d, None, DefaultRootWindow(d), 0,0,0,0, x, y);
//    // XFlush(d); XCloseDisplay(d);
//#else
//    std::cout << "Move mouse to (" << x << ", " << y << ")\n";
//#endif
//}
//
//int main(int argc, char* argv[])
//{
//    QApplication app(argc, argv);                     // for Qt resources
//
//    // --- 1. Load template (needle) -------------------------------------------------
//    const QString resPath = ":/resources/Maps/monkey_meadow.png";
//    cv::Mat needle = loadEmbeddedImage(resPath);
//    if (needle.empty()) {
//        std::cerr << "Failed to load template: " << resPath.toStdString() << "\n";
//        return 1;
//    }
//    cv::imwrite("template_debug.png", needle);
//
//    // --- 2. Grab screen (haystack) -------------------------------------------------
//    cv::Mat haystack = ScreenGrabber::grabScreen();
//    if (haystack.empty()) {
//        std::cerr << "Screen grab failed.\n";
//        return 1;
//    }
//    cv::imwrite("screen_debug.png", haystack);
//
//    // --- 3. Template matching ------------------------------------------------------
//    auto matchOpt = locateOnScreen(needle, 0.7);     // 90 % similarity
//    if (!matchOpt) {
//        std::cout << "Template not found on screen.\n";
//        return 0;
//    }
//    const Match& m = *matchOpt;
//
//    // --- 4. Draw bounding box & save ----------------------------------------------
//    cv::rectangle(haystack,
//                  m.bbox,
//                  cv::Scalar(0, 255, 0),              // green
//                  3);                                 // thickness
//    cv::imwrite("match_debug.png", haystack);
//
//    // (Optional) show on-screen windows
//    cv::imshow("Template", needle);
//    cv::imshow("Screen", haystack);
//    cv::waitKey(0);
//
//    // --- 5. Move mouse to centre ---------------------------------------------------
//    int cx = m.bbox.x + m.bbox.width  / 2;
//    int cy = m.bbox.y + m.bbox.height / 2;
//    sendMouseMove(cx, cy);
//
//    std::cout << "Match score = " << m.score
//              << "   centre = (" << cx << ", " << cy << ")\n";
//    return 0;
//}


//
//#include <QApplication>
//#include <QString>
//#include <opencv2/opencv.hpp>
//#include <iostream>
//
//#include "CommonGlobals.h"     // loadEmbeddedImage()
//#include "ScreenGrabber.h"     // ScreenGrabber::grabScreen()
//#include "TemplateMatch.h"     // locateOnScreen(), Match
//
//#ifdef __linux__
//#include <X11/Xlib.h>
//#include <X11/extensions/XTest.h>
//#endif
//
//void sendMouseMove(int x, int y)
//{
//#ifdef _WIN32
//    // ::SetCursorPos(x, y);
//#elif __linux__
//     Display* d = XOpenDisplay(nullptr);
//     XWarpPointer(d, None, DefaultRootWindow(d), 0, 0, 0, 0, x, y);
//     XFlush(d); XCloseDisplay(d);
//#else
//    std::cout << "Move mouse to (" << x << ", " << y << ")\n";
//#endif
//}
//
//int main(int argc, char* argv[])
//{
//    QApplication app(argc, argv);  // enables Qt resource system
//
//    // 1. Load embedded image
//    const QString resPath = ":/resources/MapDifficulty/expert.png";
//    cv::Mat needle = loadEmbeddedImage(resPath);
//    if (needle.empty()) {
//        std::cerr << "Failed to load template from " << resPath.toStdString() << "\n";
//        return 1;
//    }
//
//    // 2. Screen capture and locate image
//    auto matchOpt = locateOnScreen(needle, 0.9);  // 90% similarity threshold
//    if (!matchOpt) {
//        std::cout << "Template not found on screen.\n";
//        return 0;
//    }
//
//    const Match& m = *matchOpt;
//    int centerX = m.bbox.x + m.bbox.width / 2;
//    int centerY = m.bbox.y + m.bbox.height / 2;
//    sendMouseMove(centerX, centerY);
//
//    std::cout << "Found at (" << centerX << ", " << centerY << "), score = " << m.score << "\n";
//    return 0;
//}






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

