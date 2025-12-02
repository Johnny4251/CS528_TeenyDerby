#include <iostream>
#include <vector>
#include <string>
#include <ctime>
#include <cmath>

#include "tigr.h"
#include "utils.h"
#include "bus.h"
#include "menu.h"
#include "game.h"

enum GameMode {
    MODE_MENU,
    MODE_GAME
};

int main() {
    srand(time(NULL));

    Tigr* win = tigrWindow(WIN_W, WIN_H, "Teeny Derby", TIGR_FIXED);
    g_carSprite = tigrLoadImage("car.png");
    if (!g_carSprite) {
        printf("could not find car.png\n");
        printf("exiting...\n");
        return 1;
    }

    std::vector<std::string> all_bin_files;
    get_binaries(all_bin_files);

    if (all_bin_files.empty()) {
        printf("no agents/*.bin found\n");
        return 1;
    }

    MenuState menu;
    initMenu(menu, all_bin_files);

    GameState game;
    game.active = false;

    GameMode mode = MODE_MENU;
    bool running = true;

    while (running) {
        if (mode == MODE_MENU) {
            MenuResult r = runMenuFrame(win, menu);
            if (r.quit) {
                running = false;
            } else if (r.startGame) {
                gameInit(game, r.selectedBins);
                mode = MODE_GAME;
            }
        } else if (mode == MODE_GAME) {
            bool backToMenu = gameFrame(win, game);
            if (backToMenu) {
                mode = MODE_MENU;
            }
        }

        tigrUpdate(win);
        if (tigrClosed(win))
            running = false;
    }

    if (g_carSprite)
        tigrFree(g_carSprite);
    tigrFree(win);
    return 0;
}