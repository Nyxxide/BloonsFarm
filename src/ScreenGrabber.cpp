#include "ScreenGrabber.h"
#include "ScreenCapture.h"
#include <thread>
#include <chrono>

using namespace SL::Screen_Capture;

std::mutex          ScreenGrabber::frameMutex;
cv::Mat             ScreenGrabber::lastFrame;
std::once_flag      ScreenGrabber::started;

/*  NEW: keep the manager alive for the whole run  */
static std::shared_ptr<IScreenCaptureManager> gCaptureManager;

void ScreenGrabber::init()
{
    auto monitors = GetMonitors();
    if (monitors.empty()) return;

    gCaptureManager = CreateCaptureConfiguration([monitors]{ return monitors; })
            ->onNewFrame([](const Image& img, const Monitor&)
                         {
                             std::lock_guard<std::mutex> lk(frameMutex);
                             cv::Mat tmp(Height(img), Width(img), CV_8UC4,
                                         (void*)StartSrc(img));
                             lastFrame = tmp.clone();
                         })
            ->start_capturing();

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}


cv::Mat ScreenGrabber::grabScreen() {
    std::call_once(started, init);
    std::lock_guard<std::mutex> lock(frameMutex);
    return lastFrame.clone();  // May be empty briefly on startup
}


