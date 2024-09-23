#ifndef BLOONSAUTOFARM_COORDINATEHANDLER_H
#define BLOONSAUTOFARM_COORDINATEHANDLER_H

#include <string>
#include <map>

class CoordinateHandler {
public:
    std::string nameConversion(char hotkey);
    void gen(std::map<std::string, int> towerData, std::map<std::string, std::string> menuNavData, std::string);
};


#endif
