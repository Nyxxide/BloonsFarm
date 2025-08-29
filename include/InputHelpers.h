#ifndef BLOONSFARM_INPUTHELPERS_H
#define BLOONSFARM_INPUTHELPERS_H

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#elif defined(__linux__)
#  include <X11/Xlib.h>
#  include <X11/extensions/XTest.h>
#  include <X11/keysym.h>
#elif defined(__APPLE__)
#  include <ApplicationServices/ApplicationServices.h>
#endif

#include <chrono>
#include <thread>
#include <string_view>
#include <string>
#include <cstdint>
#include <iostream>
#include <cctype>   // toupper

// ------------------------------------------------------------
// Small sleep helper
// ------------------------------------------------------------
inline void msleep(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// ------------------------------------------------------------
// Mouse click at (x,y)
// ------------------------------------------------------------
inline void clickAt(int x, int y)
{
#ifdef _WIN32
    SetCursorPos(x, y);
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
#elif defined(__linux__)
    Display* d = XOpenDisplay(nullptr);
    if (!d) return;
    XTestFakeMotionEvent(d, /*screen-num*/ 0, x, y, CurrentTime);
    XTestFakeButtonEvent(d, 1, True, CurrentTime);
    XTestFakeButtonEvent(d, 1, False, CurrentTime);
    XFlush(d);
    XCloseDisplay(d);
#elif defined(__APPLE__)
    CGEventRef move = CGEventCreateMouseEvent(nullptr, kCGEventMouseMoved,
        CGPointMake(x, y), kCGMouseButtonLeft);
    CGEventRef down = CGEventCreateMouseEvent(nullptr, kCGEventLeftMouseDown,
        CGPointMake(x, y), kCGMouseButtonLeft);
    CGEventRef up = CGEventCreateMouseEvent(nullptr, kCGEventLeftMouseUp,
        CGPointMake(x, y), kCGMouseButtonLeft);
    CGEventPost(kCGHIDEventTap, move);  CGEventPost(kCGHIDEventTap, down);
    CGEventPost(kCGHIDEventTap, up);    CFRelease(move); CFRelease(down); CFRelease(up);
#endif
}

#ifdef _WIN32
// ============================
// Windows keyboard utilities
// ============================
static void log_last_error(const char* where, UINT sent, UINT expected) {
    if (sent != expected) {
        std::cerr << "[SendInput] " << where << " sent=" << sent
            << " expected=" << expected << " gle=" << GetLastError() << "\n";
    }
}

static inline bool is_extended_vk(WORD vk) {
    switch (vk) {
    case VK_INSERT: case VK_DELETE: case VK_HOME: case VK_END:
    case VK_PRIOR:  case VK_NEXT:   case VK_LEFT: case VK_RIGHT:
    case VK_UP:     case VK_DOWN:   case VK_RMENU: case VK_RCONTROL:
    case VK_DIVIDE: case VK_NUMLOCK:case VK_SNAPSHOT:
        return true;
    default: return false;
    }
}

static inline void send_key_scancode(WORD vk, bool keydown, bool isExtended) {
    WORD sc = static_cast<WORD>(MapVirtualKeyW(vk, MAPVK_VK_TO_VSC));
    INPUT in{}; in.type = INPUT_KEYBOARD;
    in.ki.wVk = 0;
    in.ki.wScan = sc;
    in.ki.dwFlags = KEYEVENTF_SCANCODE | (keydown ? 0 : KEYEVENTF_KEYUP);
    if (isExtended) in.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    UINT sent = SendInput(1, &in, sizeof(INPUT));
    log_last_error("send_key_scancode", sent, 1);
}

static inline void press_vk(WORD vk) {
    bool ext = is_extended_vk(vk);
    send_key_scancode(vk, true, ext);
    send_key_scancode(vk, false, ext);
}

// Press with optional modifiers (SHIFT/CTRL/ALT) using scancodes.
static inline void press_vk_with_mods(WORD vk, BYTE modState /*bit0=SHIFT, bit1=CTRL, bit2=ALT*/) {
    bool ext = is_extended_vk(vk);

    if (modState & 0x04) send_key_scancode(VK_MENU, true, true);  // ALT down
    if (modState & 0x02) send_key_scancode(VK_CONTROL, true, false); // CTRL down
    if (modState & 0x01) send_key_scancode(VK_SHIFT, true, false); // SHIFT down

    send_key_scancode(vk, true, ext);
    send_key_scancode(vk, false, ext);

    if (modState & 0x01) send_key_scancode(VK_SHIFT, false, false); // SHIFT up
    if (modState & 0x02) send_key_scancode(VK_CONTROL, false, false); // CTRL up
    if (modState & 0x04) send_key_scancode(VK_MENU, false, true);  // ALT up
}
#endif // _WIN32

// ------------------------------------------------------------
// Press "special" named keys
// ------------------------------------------------------------
inline void pressSpecial(const std::string& key)
{
#ifdef _WIN32
    if (key == "space")  press_vk(VK_SPACE);
    else if (key == "esc")    press_vk(VK_ESCAPE);
    else if (key == ",")      press_vk(VK_OEM_COMMA);
    else if (key == ".")      press_vk(VK_OEM_PERIOD);
    else if (key == "/")      press_vk(VK_OEM_2);
    return;

#elif defined(__linux__)
    Display* d = XOpenDisplay(nullptr);
    if (!d) return;

    KeySym ks;
    if (key == "space")  ks = XK_space;
    else if (key == "esc")    ks = XK_Escape;
    else if (key == ",")      ks = XK_comma;
    else if (key == ".")      ks = XK_period;
    else if (key == "/")      ks = XK_slash;
    else { XCloseDisplay(d); return; }

    KeyCode kc = XKeysymToKeycode(d, ks);
    XTestFakeKeyEvent(d, kc, True, CurrentTime);   // press
    XTestFakeKeyEvent(d, kc, False, CurrentTime);   // release
    XFlush(d);
    XCloseDisplay(d);
    return;

#elif defined(__APPLE__)
    // macOS virtual-key codes (ANSI US layout)
    CGKeyCode kc;
    if (key == "space")  kc = 49;
    else if (key == "esc")    kc = 53;
    else if (key == ",")      kc = 0x2B;   // kVK_ANSI_Comma
    else if (key == ".")      kc = 0x2F;   // kVK_ANSI_Period
    else if (key == "/")      kc = 0x2C;   // kVK_ANSI_Slash
    else                      return;

    CGEventRef down = CGEventCreateKeyboardEvent(nullptr, kc, true);
    CGEventRef up = CGEventCreateKeyboardEvent(nullptr, kc, false);
    CGEventPost(kCGHIDEventTap, down);
    CGEventPost(kCGHIDEventTap, up);
    CFRelease(down);  CFRelease(up);
    return;
#endif
}

// ------------------------------------------------------------
// Press a character
// ------------------------------------------------------------
inline void pressChar(char c)
{
#ifdef _WIN32
    // Map ASCII -> (VK, modifiers) using current layout.
    // Prefer scancode delivery; only fall back to Unicode if mapping fails.
    HKL layout = GetKeyboardLayout(0);
    SHORT m = VkKeyScanExA(static_cast<CHAR>(c), layout);
    if (m == -1) {
        // Try uppercase variant
        m = VkKeyScanExA(static_cast<CHAR>(std::toupper(static_cast<unsigned char>(c))), layout);
    }

    if (m != -1) {
        BYTE vk = LOBYTE(m);
        BYTE mod = HIBYTE(m) & 0x07; // bit0=SHIFT, bit1=CTRL, bit2=ALT
        press_vk_with_mods(vk, mod);
        return;
    }

    // Fallback: Unicode injection (many games ignore this, but editors will accept it)
    INPUT in[2] = {};
    in[0].type = INPUT_KEYBOARD;
    in[0].ki.wVk = 0;
    in[0].ki.wScan = static_cast<WORD>(static_cast<unsigned char>(c));
    in[0].ki.dwFlags = KEYEVENTF_UNICODE;

    in[1] = in[0];
    in[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;

    UINT sent = SendInput(2, in, sizeof(INPUT));
    log_last_error("unicode_fallback", sent, 2);

#elif defined(__linux__)
    Display* d = XOpenDisplay(nullptr);
    if (!d) return;
    KeySym ks = XStringToKeysym(std::string(1, c).c_str());
    KeyCode kc = XKeysymToKeycode(d, ks);
    XTestFakeKeyEvent(d, kc, True, CurrentTime);
    XTestFakeKeyEvent(d, kc, False, CurrentTime);
    XFlush(d);
    XCloseDisplay(d);
#elif defined(__APPLE__)
    CGEventRef down = CGEventCreateKeyboardEvent(nullptr, 0, true);
    CGEventRef up = CGEventCreateKeyboardEvent(nullptr, 0, false);
    UniChar uc = static_cast<UniChar>(static_cast<unsigned char>(c));
    CGEventKeyboardSetUnicodeString(down, 1, &uc);
    CGEventKeyboardSetUnicodeString(up, 1, &uc);
    CGEventPost(kCGHIDEventTap, down);
    CGEventPost(kCGHIDEventTap, up);
    CFRelease(down); CFRelease(up);
#endif
}

// ------------------------------------------------------------
// Click center of a rect (OpenCV type expected)
// ------------------------------------------------------------
inline void clickCenter(const cv::Rect& bbox, int yOffset = 0)
{
    int cx = bbox.x + bbox.width / 2;
    int cy = bbox.y + bbox.height / 2 + yOffset;
    std::cout << cx << std::endl;
    std::cout << cy << std::endl;
    clickAt(cx, cy);
}

#endif // BLOONSFARM_INPUTHELPERS_H
