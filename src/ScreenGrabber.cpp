#include "ScreenGrabber.h"

#if defined(_WIN32)                                              /* Windows */
#define WIN32_LEAN_AND_MEAN
    #include <windows.h>

    cv::Mat ScreenGrabber::grabScreen()
    {
        HDC hScreen = GetDC(nullptr);
        int w = GetSystemMetrics(SM_CXSCREEN);
        int h = GetSystemMetrics(SM_CYSCREEN);

        HDC     hMem = CreateCompatibleDC(hScreen);
        HBITMAP hBmp = CreateCompatibleBitmap(hScreen, w, h);
        SelectObject(hMem, hBmp);

        BitBlt(hMem, 0,0, w,h, hScreen, 0,0, SRCCOPY | CAPTUREBLT);

        BITMAPINFO bmi{ sizeof(BITMAPINFOHEADER) };
        bmi.bmiHeader.biWidth       = w;
        bmi.bmiHeader.biHeight      = -h;          // top-down
        bmi.bmiHeader.biPlanes      = 1;
        bmi.bmiHeader.biBitCount    = 24;          // BGR24
        bmi.bmiHeader.biCompression = BI_RGB;

        cv::Mat mat(h, w, CV_8UC3);
        GetDIBits(hMem, hBmp, 0, h, mat.data, &bmi, DIB_RGB_COLORS);

        DeleteObject(hBmp);
        DeleteDC(hMem);
        ReleaseDC(nullptr, hScreen);
        return mat;
    }

#elif defined(__linux__)                                         /* Linux X11 */
#include <X11/Xlib.h>
#include <X11/Xutil.h>

cv::Mat ScreenGrabber::grabScreen()
{
    Display* dpy = XOpenDisplay(nullptr);
    if (!dpy) return {};

    Window root = DefaultRootWindow(dpy);
    XWindowAttributes gwa{};  XGetWindowAttributes(dpy, root, &gwa);
    int w = gwa.width, h = gwa.height;

    XImage* img = XGetImage(dpy, root, 0,0, w,h, AllPlanes, ZPixmap);
    if (!img) { XCloseDisplay(dpy); return {}; }

    cv::Mat bgra(h, w, CV_8UC4, img->data);   // BGRA
    cv::Mat bgr;  cv::cvtColor(bgra, bgr, cv::COLOR_BGRA2BGR);
    cv::Mat copy = bgr.clone();               // own the pixels

    XDestroyImage(img);
    XCloseDisplay(dpy);
    return copy;
}

#elif defined(__APPLE__)                                         /* macOS */
#include <ApplicationServices/ApplicationServices.h>

    cv::Mat ScreenGrabber::grabScreen()
    {
        CGImageRef img = CGDisplayCreateImage(CGMainDisplayID());
        if (!img) return {};

        size_t w = CGImageGetWidth(img);
        size_t h = CGImageGetHeight(img);

        const void* buf = CFDataGetBytePtr(CGDataProviderCopyData(CGImageGetDataProvider(img)));
        cv::Mat rgba(h, w, CV_8UC4, const_cast<void*>(buf));
        cv::Mat bgr;  cv::cvtColor(rgba, bgr, cv::COLOR_RGBA2BGR);
        cv::Mat copy = bgr.clone();

        CGImageRelease(img);
        CFRelease(buf);
        return copy;
    }

#else                                                            /* Unsupported */
    cv::Mat ScreenGrabber::grabScreen() { return {}; }
#endif
