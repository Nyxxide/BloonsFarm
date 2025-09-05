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
#include <CoreGraphics/CoreGraphics.h>

cv::Mat ScreenGrabber::grabScreen()
{
    // Capture the whole main display. Returns null if screen-recording permission missing.
    CGImageRef img = CGDisplayCreateImage(kCGDirectMainDisplay);
    if (!img) return {};

    const size_t w = CGImageGetWidth(img);
    const size_t h = CGImageGetHeight(img);
    if (w == 0 || h == 0) { CGImageRelease(img); return {}; }

    // Allocate our own pixel buffer (BGRA 8-bit)
    cv::Mat bgra((int)h, (int)w, CV_8UC4);

    // Create a bitmap context that writes directly into our cv::Mat
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();                 // CREATE → must release
    if (!cs) { CGImageRelease(img); return {}; }

    CGContextRef ctx = CGBitmapContextCreate(                           // CREATE → must release
        bgra.data, w, h, 8, (size_t)bgra.step[0], cs,
        kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst);  // BGRA on little-endian

    if (!ctx) {
        CGColorSpaceRelease(cs);
        CGImageRelease(img);
        return {};
    }

    // Draw CGImage into our buffer
    CGContextDrawImage(ctx, CGRectMake(0, 0, (CGFloat)w, (CGFloat)h), img);

    // Release ONLY what we created
    CGContextRelease(ctx);
    CGColorSpaceRelease(cs);
    CGImageRelease(img);

    // Convert to BGR if your downstream expects 3-channel OpenCV images
    cv::Mat bgr;
    cv::cvtColor(bgra, bgr, cv::COLOR_BGRA2BGR);
    return bgr;   // or return bgra if you want to keep alpha
}

#else                                                            /* Unsupported */
    cv::Mat ScreenGrabber::grabScreen() { return {}; }
#endif
