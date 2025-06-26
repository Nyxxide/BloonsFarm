#ifndef BLOONSFARM_SCREENGRABBER_H
#define BLOONSFARM_SCREENGRABBER_H

#include <opencv2/opencv.hpp>
#include "ScreenCapture.h"
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>

class ScreenGrabber {
public:
    static cv::Mat grabScreen();

private:
    static void init();
    static std::mutex frameMutex;
    static cv::Mat lastFrame;
    static std::once_flag started;
};


#endif //BLOONSFARM_SCREENGRABBER_H
