#include <iostream>
#include "game.h"

void gameInit(GameState& game, const std::vector<std::string>& bins) {
    game.agents.clear();
    game.cars.clear();

    for (int i = 0; i < AGENT_MAX_CNT; ++i) {
        g_speeds[i] = 0.0f;
        g_hitCooldown[i] = 0;
        game.derby_state[i].throttle = 0;
        game.derby_state[i].direction = 0;
        game.derby_state[i].id = 0;
        game.derby_state[i].sensor_target = 0;
        game.derby_state[i].speed = 0;
        game.derby_state[i].health = 100;
        game.derby_state[i].isDead = false;
    }

    g_scoreRowInit = false;

    load_agents(bins, game.agents, game.derby_state);
    randomize_cars(game.cars, game.agents, bins);

    game.active = true;
}

bool gameFrame(Tigr* win, GameState& game) {
    if (!game.active)
        return false;

    tigrClear(win, tigrRGB(30, 30, 30));

    bool paused = tigrKeyHeld(win, 'p') || tigrKeyHeld(win, 'P');

    for (size_t i = 0; i < game.agents.size(); ++i) {
        DerbyState* state = (DerbyState*)game.agents[i].ex_data;
        if (!paused) {
            updateHitCooldown((int)i);
            updateAgentState(game.agents[i], state);

            float angle = computeDirectionAngle(state);
            game.cars[i].angle = angle;

            float newSpeed = computeSmoothedSpeed((int)i, state);
            g_speeds[i] = newSpeed;
            state->speed = (int16_t)std::round(std::fabs(newSpeed));

            int nx, ny;
            computeNextPosition(game.cars[i], game.cars[i].angle, newSpeed, nx, ny);

            int collidedWith;
            bool blocked = detectCollision(game.cars, i, nx, ny, collidedWith);

            if (blocked)
                applyCollisionDamage((int)i, collidedWith);

            applyMovementOrClamp(game.cars[i], state, blocked, nx, ny, (int)i);
        }

        drawRotatedCar(win, game.cars[i]);
        if(drawCarSprite(win, game.cars[i],i)  < 0) {
            game.active = false;
            std::cout << "Error drawing car sprite." << std::endl;
        }
        g_derby_state[i].smokeLevel = getSmokeLevel(i);
        emitSmoke(win, game.cars[i], g_derby_state[i].smokeLevel);
        drawNameTag(win, game.cars[i]);
        drawHealthBar(win, game.cars[i]);
    }
    updateSmoke(win);

    drawScoreboard(win);
    drawTitleBar(win);

    if (paused) {
        const char* pauseMsg = "PAUSED - Hold P to resume";
        int pw = tigrTextWidth(tfont, pauseMsg);
        int ph = tigrTextHeight(tfont, pauseMsg);
        int px = (WIN_W - pw) / 2;
        int py = 10;
        tigrFillRect(win, px - 6, py - 3, pw + 12, ph + 6, tigrRGB(0, 0, 0));
        tigrPrint(win, tfont, px, py, tigrRGB(255, 255, 0), "%s", pauseMsg);
    }

    if (tigrKeyDown(win, TK_TAB))
        return true;

    return false;
}
