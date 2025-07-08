//
//// File: scale_demo.cpp
//// Build:  g++ -std=c++20 -O2 scale_demo.cpp `pkg-config --cflags --libs opencv4`
//// Run:    ./a.out path/to/template.png
//// ---------------------------------------------------------------------------
//
//#include <opencv2/opencv.hpp>
//#include <iostream>
//#include <filesystem>
//
//constexpr int REF_W = 1920;      // resolution the template was captured on
//constexpr int REF_H = 1080;
//
//int main(int argc, char** argv)
//{
//
//
//    const std::string filename = "/home/nyx/Desktop/collectionevent.png";
//    cv::Mat src = cv::imread(filename, cv::IMREAD_UNCHANGED);
//
//
//    // --- scale factors ---
//    double scaleUp   = 2560.0 / REF_W;   // ≈1.333
//    double scaleDown = 1600.0 / REF_W;   // ≈0.833
//
//    cv::Mat up, down;
//    cv::resize(src, up,   cv::Size(), scaleUp,   scaleUp,   cv::INTER_CUBIC);  // nicer upsampling
//    cv::resize(src, down, cv::Size(), scaleDown, scaleDown, cv::INTER_AREA);   // nicer downsampling
//
//    // --- save files next to original ---
//    namespace fs = std::filesystem;
//    fs::path inPath  = fs::absolute(filename);
//    fs::path outUp   = inPath.parent_path() / (inPath.stem().string() + "_2560x1440" + inPath.extension().string());
//    fs::path outDown = inPath.parent_path() / (inPath.stem().string() + "_1600x900"   + inPath.extension().string());
//
//    cv::imwrite(outUp.string(),   up);
//    cv::imwrite(outDown.string(), down);
//
//    std::cout << "Written:\n  " << outUp   << "\n  " << outDown << '\n';
//
//    // --- on-screen preview (Esc closes) ---
//    cv::imshow("Original (1920×1080 basis)", src);
//    cv::imshow("Upscaled → 2560×1440 basis", up);
//    cv::imshow("Downscaled → 1600×900 basis", down);
//    while (true) { if (cv::waitKey(30) == 27) break; }   // Esc key
//    return 0;
//}




#include "BloonsUIMain.h"

#include <QApplication>

int main(int argc, char *argv[]) {

    QApplication MainUI(argc, argv);

    auto BloonsUI = BloonsUIMain();

    return MainUI.exec();


};

