#ifndef MENU_H
#define MENU_H

#include <vector>
#include <string>
#include "tigr.h"
#include "utils.h"

struct MenuState {
    std::vector<std::string> allBinFiles;
    std::vector<std::string> agentNames;
    std::vector<bool> agentEnabled;
    int cursor;
};

struct MenuResult {
    bool startGame;
    bool quit;
    std::vector<std::string> selectedBins;
};

void initMenu(MenuState& menu, const std::vector<std::string>& binFiles);
MenuResult runMenuFrame(Tigr* win, MenuState& menu);

#endif
