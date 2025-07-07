#ifndef BLOONSFARM_SCREENGRABBER_H
#define BLOONSFARM_SCREENGRABBER_H

#include <opencv2/opencv.hpp>


class ScreenGrabber
{
public:
    // Returns a BGR 8-bit cv::Mat of the entire primary monitor.
    // On failure: returns empty Mat.
    static cv::Mat grabScreen();
};


#endif //BLOONSFARM_SCREENGRABBER_H
