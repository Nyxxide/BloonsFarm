#ifndef BLOONSFARM_COMMONFUNCTIONS_H
#define BLOONSFARM_COMMONFUNCTIONS_H

#include "json.hpp"
#include <opencv2/opencv.hpp>

#include "CoordinateHandler.h"
#include "TemplateMatch.h"

#include <filesystem>
#include <optional>
#include <chrono>
#include <thread>
#include <utility>

#include <QFile>
#include <QByteArray>
#include <QString>

//JSON Object Automated Presets
inline nlohmann::json deflationTowData = {
        {{"hotkey", "d"}, {"x", 824}, {"y", 366}, {"top", 4}, {"middle", 0}, {"bottom", 2}},
        {{"hotkey", "f"}, {"x", 831}, {"y", 307}, {"top", 4}, {"middle", 2}, {"bottom", 0}},
        {{"hotkey", "f"}, {"x", 834}, {"y", 764}, {"top", 4}, {"middle", 2}, {"bottom", 0}},
        {{"hotkey", "d"}, {"x", 835}, {"y", 696}, {"top", 4}, {"middle", 0}, {"bottom", 2}}
};
inline nlohmann::json deflation2xTowData = {
        {{"hotkey", "z"}, {"x", 1608}, {"y", 501}, {"top", 2}, {"middle", 0}, {"bottom", 5}},
        {{"hotkey", "f"}, {"x", 1551}, {"y", 554}, {"top", 4}, {"middle", 2}, {"bottom", 0}},
        {{"hotkey", "k"}, {"x", 1581}, {"y", 622}, {"top", 2}, {"middle", 3}, {"bottom", 0}}
};
inline nlohmann::json deflationMenuNav = {
        {"mapDifficulty", "expert"}, {"map", "infernal"}, {"difficulty", "easy"}, {"mode", "deflation"}
};
inline nlohmann::json deflation2xMenuNav = {
        {"mapDifficulty", "expert"}, {"map", "infernal"}, {"difficulty", "easy"}, {"mode", "deflation"}
};

// Helper function to replace all occurrences of a character
inline std::string replaceChar(std::string str, char oldChar, char newChar) {
    std::replace(str.begin(), str.end(), oldChar, newChar);
    return str;
}

// Helper function to convert string to title case
inline std::string toTitleCase(std::string str) {
    bool capitalize = true;
    for (auto &ch : str) {
        if (std::isspace(ch)) {
            capitalize = true;  // Next character should be capitalized
        } else if (capitalize && std::isalpha(ch)) {
            ch = std::toupper(ch);  // Capitalize the letter
            capitalize = false;
        } else {
            ch = std::tolower(ch);  // Lowercase the rest
        }
    }
    return str;
}

// Helper function to remove substring
inline std::string removeSubstring(std::string str, const std::string &toRemove) {
    size_t pos = str.find(toRemove);
    if (pos != std::string::npos) {
        str.erase(pos, toRemove.length());
    }
    return str;
}

inline int genData(){
    if(!std::filesystem::exists("Tower Positions")){
        std::filesystem::create_directory("Tower Positions");
    }
    if(!std::filesystem::exists("Tower Positions/deflation.json")) {
        CoordinateHandler::gen(deflationTowData, deflationMenuNav, "deflation");
    }
    if(!std::filesystem::exists("Tower Positions/deflation_2x_cash.json")){
        CoordinateHandler::gen(deflation2xTowData, deflation2xMenuNav, "deflation_2x_cash");
    }
    return 1;
}

// Load images embedded in the object file
inline cv::Mat loadEmbeddedImage(const QString& path)
{
    QFile f(path);                       // e.g. ":/resources/Maps/MonkeyMeadow.png"
    if (!f.open(QIODevice::ReadOnly))
        return {};
    QByteArray bytes = f.readAll();
    return cv::imdecode(
            std::vector<uchar>(bytes.begin(), bytes.end()),
            cv::IMREAD_UNCHANGED);
}

inline std::optional<Match>
waitForTemplate(const QString&   resPath,
                double           threshold,
                int              maxTries   = 3,   // -1  →  infinite
                int              delayMs    = 300,
                const std::atomic_bool* running = nullptr)
{
    cv::Mat tpl = loadEmbeddedImage(resPath);
    if (tpl.empty()) return std::nullopt;

    int tries = 0;
    while (maxTries < 0 || tries < maxTries)
    {
        if (running && !*running) return std::nullopt;   // user aborted

        if (auto m = locateOnScreen(tpl, threshold))
            return m;

        ++tries;
//        cout << tries << endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
    }
    return std::nullopt;      // exhausted tries
}

#endif
