#include "CoordinateHandler.h"

#include <json.hpp>

#include <iostream>
#include <fstream>
#include <cmath>
#include <string>

#include <QGuiApplication>

#include <cstdint>

#ifdef _WIN32                         // ───── Windows ───────────────────────
#define WIN32_LEAN_AND_MEAN
    #include <windows.h>

#elif defined(__APPLE__)              // ───── macOS ─────────────────────────
#include <CoreGraphics/CoreGraphics.h>

#elif defined(__linux__)              // ───── Linux (X11 + Xrandr) ─────────
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#endif

constexpr int BASE_WIDTH = 1920;
constexpr int BASE_HEIGHT = 1080;

// Returns scaled (x, y) coordinate on target screen size
std::pair<int, int> scaleCoords(int x, int y, int targetWidth, int targetHeight) {
    double scaleX = static_cast<double>(targetWidth) / BASE_WIDTH;
    double scaleY = static_cast<double>(targetHeight) / BASE_HEIGHT;

    int newX = static_cast<int>(x * scaleX);
    int newY = static_cast<int>(y * scaleY);
    return {newX, newY};
}

int getScreenResolution(int& width, int& height)
{
#ifdef _WIN32
    width  = ::GetSystemMetrics(SM_CXSCREEN);
    height = ::GetSystemMetrics(SM_CYSCREEN);
    return 0;

#elif defined(__APPLE__)
    width  = static_cast<int>(CGDisplayPixelsWide(CGMainDisplayID()));
    height = static_cast<int>(CGDisplayPixelsHigh(CGMainDisplayID()));
    return 0;

#elif defined(__linux__)
    Display* dpy = XOpenDisplay(nullptr);
    if (!dpy) return -1;

    Window root = DefaultRootWindow(dpy);
    XRRScreenResources* res = XRRGetScreenResources(dpy, root);
    if (!res) { XCloseDisplay(dpy); return -1; }

    // Take first connected output / CRTC
    for (int i = 0; i < res->noutput; ++i) {
        XRROutputInfo* out = XRRGetOutputInfo(dpy, res, res->outputs[i]);
        if (out && out->connection == RR_Connected && out->crtc != 0) {
            XRRCrtcInfo* crtc = XRRGetCrtcInfo(dpy, res, out->crtc);
            if (crtc) {
                width  = crtc->width;
                height = crtc->height;
                XRRFreeCrtcInfo(crtc);
                XRRFreeOutputInfo(out);
                XRRFreeScreenResources(res);
                XCloseDisplay(dpy);
                return 0;
            }
            XRRFreeOutputInfo(out);
        }
    }
    XRRFreeScreenResources(res);
    XCloseDisplay(dpy);
    return -1;
#else
    return -1;   // Unsupported platform
#endif
}

std::string CoordinateHandler::nameConversion(char hotkey) {
    switch(hotkey){
        case 'q':
            return "dart";
        case 'w':
            return "boomerang";
        case 'e':
            return "bomb_shooter";
        case 'r':
            return "tack_shooter";
        case 't':
            return "ice";
        case 'y':
            return "glue_gunner";
        case 'z':
            return "sniper";
        case 'x':
            return "sub";
        case 'c':
            return "buccaneer";
        case 'v':
            return "plane";
        case 'b':
            return "heli_pilot";
        case 'n':
            return "mortar";
        case 'm':
            return "dartling";
        case 'a':
            return "wizard";
        case 's':
            return "super_monkey";
        case 'd':
            return "ninja";
        case 'f':
            return "alchemist";
        case 'g':
            return "druid";
        case 'h':
            return "farm";
        case 'j':
            return "spike";
        case 'k':
            return "village";
        case 'l':
            return "engineer";
        case 'i':
            return "beast_handler";
        default:
            return "error";
    }

}

void CoordinateHandler::gen(nlohmann::json towerData, nlohmann::json menuNavData, std::string fileName) {
    int width = 0;
    int height = 0;
    std::cout << getScreenResolution(width, height) << std::endl;
    std::cout << width << std::endl;
    std::cout << height << std::endl;

    nlohmann::json data = { {"towers" , {}}, {"menuNav", {}} };
    nlohmann::json counter = {};

    for(const auto& tower : towerData){
        std::string hotkey = tower["hotkey"];
        std::string towerName = CoordinateHandler::nameConversion(hotkey[0]) + "_pos";
        auto [newx, newy] = scaleCoords(tower["x"].get<double>(), tower["y"].get<double>(), width, height);
        if(data["towers"].contains(towerName)){
            if(!counter.contains(towerName)){
                counter[towerName] = 2;
            }
            else{
                counter[towerName] += 1;
            }
            std::string towerNum = to_string(counter[towerName]);
            towerName = CoordinateHandler::nameConversion(hotkey[0]) + "_" + towerNum + "_pos";

            data["towers"][towerName] = {
                    {"hotkey", tower["hotkey"]},
                    {"x", newx},
                    {"y", newy},
                    {"top", tower["top"]},
                    {"middle", tower["middle"]},
                    {"bottom", tower["bottom"]}
            };
        }
        else{

            data["towers"][towerName] = {
                    {"hotkey", tower["hotkey"]},
                    {"x", newx},
                    {"y", newy},
                    {"top", tower["top"]},
                    {"middle", tower["middle"]},
                    {"bottom", tower["bottom"]}
            };
        }
    }

    data["menuNav"] = {
            {"mapDifficulty", menuNavData["mapDifficulty"]},
            {"map", menuNavData["map"]},
            {"difficulty", menuNavData["difficulty"]},
            {"mode", menuNavData["mode"]}
    };

    fileName = "Tower Positions/" + fileName + ".json";
    std::ofstream outFile(fileName);
    if(!outFile){
        std::cerr << "Can't open file." << std::endl;
        return;
    }

    outFile << data.dump(4);
    outFile.close();
}
