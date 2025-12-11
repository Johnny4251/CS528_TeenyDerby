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

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

ma_engine engine;
ma_sound collisionSound;
ma_sound collisionSound1;
ma_sound breakingDownSound;

enum GameMode {
    MODE_MENU,
    MODE_GAME
};

int main() {
    srand(time(NULL));

    Tigr* win = tigrWindow(WIN_W, WIN_H, "Teeny Derby", TIGR_FIXED);

    if (ma_engine_init(NULL, &engine) != MA_SUCCESS) {
        std::cout << "Failed to initialize audio engine.\n";
        return 1;
    }

    if (ma_sound_init_from_file(&engine, "collision.wav", 0, NULL, NULL, &collisionSound) != MA_SUCCESS) {
        std::cout << "Failed to load collision.wav.\n";
        return 1;
    }

    if (ma_sound_init_from_file(&engine, "collision1.wav", 0, NULL, NULL, &collisionSound1) != MA_SUCCESS) {
        std::cout << "Failed to load collision1.wav.\n";
        return 1;
    }

    if (ma_sound_init_from_file(&engine, "breakdown.mp3", 0, NULL, NULL, &breakingDownSound) != MA_SUCCESS) {
    std::cout << "Failed to load breakdown.mp3.\n";
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
    ma_sound_uninit(&collisionSound);
    ma_sound_uninit(&collisionSound1);
    ma_sound_uninit(&breakingDownSound);
    ma_engine_uninit(&engine);

    tigrFree(win);
    return 0;
}