#ifndef GAME_H
#define GAME_H

#include <vector>
#include <string>
#include "tigr.h"
#include "utils.h"
#include "../teenyat.h"

struct GameState {
    std::vector<teenyat> agents;
    std::vector<Car> cars;
    DerbyState derby_state[AGENT_MAX_CNT];
    bool active;
};

void gameInit(GameState& game, const std::vector<std::string>& bins);
bool gameFrame(Tigr* win, GameState& game);

#endif
