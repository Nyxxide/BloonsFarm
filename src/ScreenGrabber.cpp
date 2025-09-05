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
#include <cmath>

cv::Mat ScreenGrabber::grabScreen()
{
    // Capture visible content of the main display in *backing pixels* (Retina = 2x).
    CGDirectDisplayID display = kCGDirectMainDisplay;
    CGImageRef img = CGDisplayCreateImage(display);
    if (!img) return {};

    const size_t wpx = CGImageGetWidth(img);
    const size_t hpx = CGImageGetHeight(img);
    if (wpx == 0 || hpx == 0) { CGImageRelease(img); return {}; }

    // Render into our own BGRA buffer so we control lifetime.
    cv::Mat bgra((int)hpx, (int)wpx, CV_8UC4);

    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB(); // CREATE → release
    if (!cs) { CGImageRelease(img); return {}; }

    CGContextRef ctx = CGBitmapContextCreate(
        bgra.data, wpx, hpx, 8, (size_t)bgra.step[0], cs,
        kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst); // BGRA on little-endian
    if (!ctx) {
        CGColorSpaceRelease(cs);
        CGImageRelease(img);
        return {};
    }

    CGContextDrawImage(ctx, CGRectMake(0, 0, (CGFloat)wpx, (CGFloat)hpx), img);

    CGContextRelease(ctx);
    CGColorSpaceRelease(cs);
    CGImageRelease(img);

    // ---- Normalize Retina scaling to 1× so templates match size ----
    // CGDisplayBounds returns logical "points". scale ≈ backingPixels / points.
    CGRect bounds = CGDisplayBounds(display);
    const double pw = (double)CGRectGetWidth(bounds);
    const double ph = (double)CGRectGetHeight(bounds);
    double scale = 1.0;
    if (pw > 0.0 && ph > 0.0) {
        const double sx = (double)wpx / pw;
        const double sy = (double)hpx / ph;
        scale = std::max(sx, sy); // typically 2.0 on Retina
    }

    if (scale > 1.01) {
        cv::Mat bgra1x;
        cv::resize(bgra, bgra1x,
                   cv::Size((int)std::lround(bgra.cols / scale),
                            (int)std::lround(bgra.rows / scale)),
                   0, 0, cv::INTER_AREA);
        bgra = std::move(bgra1x);
    }

    // Convert to BGR (to match your Windows/Linux pipeline)
    cv::Mat bgr;
    cv::cvtColor(bgra, bgr, cv::COLOR_BGRA2BGR);
    return bgr;
}

#else                                                            /* Unsupported */
    cv::Mat ScreenGrabber::grabScreen() { return {}; }
#endif
