#ifndef BLOONSAUTOFARM_COORDINATEHANDLER_H
#define BLOONSAUTOFARM_COORDINATEHANDLER_H

#include <string>
#include <map>

class CoordinateHandler {
public:
    static std::string nameConversion(char hotkey);
    static void gen(std::map<std::string, int> towerData, std::map<std::string, std::string> menuNavData, std::string);
};


#endif
