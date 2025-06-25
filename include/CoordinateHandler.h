#ifndef BLOONSFARM_COORDINATEHANDLER_H
#define BLOONSFARM_COORDINATEHANDLER_H

#include <string>
#include <map>
#include <json.hpp>

class CoordinateHandler {
public:
    static std::string nameConversion(char hotkey);
    static void gen(nlohmann::json towerData, nlohmann::json menuNavData, std::string);
};


#endif
