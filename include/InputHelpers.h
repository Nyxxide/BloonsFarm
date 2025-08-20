#ifndef BLOONSFARM_INPUTHELPERS_H
#define BLOONSFARM_INPUTHELPERS_H

#ifdef _WIN32
# include <windows.h>
#elif defined(__linux__)
# include <X11/Xlib.h>
# include <X11/extensions/XTest.h>
# include <X11/keysym.h>
#elif defined(__APPLE__)
# include <ApplicationServices/ApplicationServices.h>
#endif

#include <chrono>
#include <thread>
#include <string_view>
#include <cstdint>
#include <iostream>

inline void msleep(int ms){
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

inline void clickAt(int x, int y)
{
#ifdef _WIN32
    SetCursorPos(x, y);
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    mouse_event(MOUSEEVENTF_LEFTUP,   0, 0, 0, 0);
#elif defined(__linux__)
    Display* d = XOpenDisplay(nullptr);
    if (!d) return;
    XTestFakeMotionEvent(d, /*screen-num*/ 0, x, y, CurrentTime);
    XTestFakeButtonEvent(d, 1, True,  CurrentTime);
    XTestFakeButtonEvent(d, 1, False, CurrentTime);
    XFlush(d);
    XCloseDisplay(d);
#elif defined(__APPLE__)
    CGEventRef move  = CGEventCreateMouseEvent(nullptr, kCGEventMouseMoved,
                                               CGPointMake(x, y), kCGMouseButtonLeft);
    CGEventRef down  = CGEventCreateMouseEvent(nullptr, kCGEventLeftMouseDown,
                                               CGPointMake(x, y), kCGMouseButtonLeft);
    CGEventRef up    = CGEventCreateMouseEvent(nullptr, kCGEventLeftMouseUp,
                                               CGPointMake(x, y), kCGMouseButtonLeft);
    CGEventPost(kCGHIDEventTap, move);  CGEventPost(kCGHIDEventTap, down);
    CGEventPost(kCGHIDEventTap, up);    CFRelease(move); CFRelease(down); CFRelease(up);
#endif
}

inline void pressSpecial(const std::string& key)
{
#ifdef _WIN32
    INPUT in{};  in.type = INPUT_KEYBOARD;

    if      (key == "space")  in.ki.wVk = VK_SPACE;
    else if (key == "esc")    in.ki.wVk = VK_ESCAPE;
    else if (key == ",")      in.ki.wVk = VK_OEM_COMMA;
    else if (key == ".")      in.ki.wVk = VK_OEM_PERIOD;
    else if (key == "/")      in.ki.wVk = VK_OEM_2;        // slash / question-mark
    else                      return;                      // unsupported

    SendInput(1, &in, sizeof(INPUT));              // key down
    in.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &in, sizeof(INPUT));              // key up
    return;

#elif defined(__linux__)
    Display* d = XOpenDisplay(nullptr);
    if (!d) return;

    KeySym ks;
    if      (key == "space")  ks = XK_space;
    else if (key == "esc")    ks = XK_Escape;
    else if (key == ",")      ks = XK_comma;
    else if (key == ".")      ks = XK_period;
    else if (key == "/")      ks = XK_slash;
    else { XCloseDisplay(d); return; }

    KeyCode kc = XKeysymToKeycode(d, ks);
    XTestFakeKeyEvent(d, kc, True,  CurrentTime);   // press
    XTestFakeKeyEvent(d, kc, False, CurrentTime);   // release
    XFlush(d);
    XCloseDisplay(d);
    return;

#elif defined(__APPLE__)
    // macOS virtual-key codes (ANSI US layout)
    CGKeyCode kc;
    if      (key == "space")  kc = 49;
    else if (key == "esc")    kc = 53;
    else if (key == ",")      kc = 0x2B;   // kVK_ANSI_Comma
    else if (key == ".")      kc = 0x2F;   // kVK_ANSI_Period
    else if (key == "/")      kc = 0x2C;   // kVK_ANSI_Slash
    else                      return;

    CGEventRef down = CGEventCreateKeyboardEvent(nullptr, kc, true);
    CGEventRef up   = CGEventCreateKeyboardEvent(nullptr, kc, false);
    CGEventPost(kCGHIDEventTap, down);
    CGEventPost(kCGHIDEventTap, up);
    CFRelease(down);  CFRelease(up);
    return;
#endif
}

inline void pressChar(char c)
{
#ifdef _WIN32
    SHORT vk = VkKeyScanA(c);
    INPUT in{0};
    in.type         = INPUT_KEYBOARD;
    in.ki.wVk       = LOBYTE(vk);
    in.ki.wScan     = 0;
    SendInput(1, &in, sizeof(INPUT));          // key down
    in.ki.dwFlags   = KEYEVENTF_KEYUP;
    SendInput(1, &in, sizeof(INPUT));          // key up
#elif defined(__linux__)
    Display* d = XOpenDisplay(nullptr);
    if (!d) return;
    KeySym ks = XStringToKeysym(std::string(1, c).c_str());
    KeyCode kc = XKeysymToKeycode(d, ks);
    XTestFakeKeyEvent(d, kc, True,  CurrentTime);
    XTestFakeKeyEvent(d, kc, False, CurrentTime);
    XFlush(d);
    XCloseDisplay(d);
#elif defined(__APPLE__)
    CGEventRef down = CGEventCreateKeyboardEvent(nullptr, 0, true);
    CGEventRef up   = CGEventCreateKeyboardEvent(nullptr, 0, false);
    UniChar uc = c; CGEventKeyboardSetUnicodeString(down, 1, &uc);
                    CGEventKeyboardSetUnicodeString(up,   1, &uc);
    CGEventPost(kCGHIDEventTap, down);
    CGEventPost(kCGHIDEventTap, up);
    CFRelease(down); CFRelease(up);
#endif
}

inline void clickCenter(const cv::Rect& bbox, int yOffset = 0)
{
    int cx = bbox.x + bbox.width  / 2;
    int cy = bbox.y + bbox.height / 2 + yOffset;
    std::cout << cx << std::endl;
    std::cout << cy << std::endl;
    clickAt(cx, cy);
}

#endif
