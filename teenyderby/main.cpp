#include <iostream>
#include <vector>
#include <ctime>
#include <cmath>
#include <algorithm>   
#include "tigr.h"
#include "utils.h"
#include "bus.h"

int main() {
    srand(time(NULL));

    Tigr* win = tigrWindow(WIN_W, WIN_H, "Teeny Derby", TIGR_FIXED);

    std::vector<std::string> bin_files;
    get_binaries(bin_files);

    DerbyState derby_state[AGENT_MAX_CNT] = {};
    std::vector<teenyat> agents;
    load_agents(bin_files, agents, derby_state);

    std::vector<Car> cars;
    randomize_cars(cars, agents);

    while (!tigrClosed(win)) {
        tigrClear(win, tigrRGB(30,30,30));

        for (size_t i = 0; i < agents.size(); i++) {

            DerbyState* state = (DerbyState*)agents[i].ex_data;

            updateHitCooldown(i);
            updateAgentState(agents[i], state);

            float angle = computeDirectionAngle(state);
            cars[i].angle = angle;

            float newSpeed = computeSmoothedSpeed(i, state);
            g_speeds[i] = newSpeed;
            state->speed = (int16_t)std::round(std::fabs(newSpeed));

            int nx, ny;
            computeNextPosition(cars[i], angle, newSpeed, nx, ny);

            int collidedWith;
            bool blocked = detectCollision(cars, i, nx, ny, collidedWith);

            if (blocked)
                applyCollisionDamage(i, collidedWith);

            applyMovementOrClamp(cars[i], state, blocked, nx, ny, i);

            drawRotatedCar(win, cars[i]);
            drawHealthBar(win, cars[i]);
        }

        tigrUpdate(win);
    }


    tigrFree(win);
    return 0;
}
