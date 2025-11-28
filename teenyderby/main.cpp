#include <iostream>
#include <vector>
#include <ctime>
#include <cmath>
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
            tny_clock(&agents[i]);
            DerbyState* state = (DerbyState*)agents[i].ex_data;

            int nx = cars[i].x;
            int ny = cars[i].y;

            if (state) {
                uint8_t dir = state->direction & 7;
                float angle = dir * (3.14159265f / 4.0f);

                cars[i].angle = angle;
                
                float tval = state->throttle;
                tval = std::max(-100.0f, std::min(100.0f, tval));

                float speed = (tval / 100.0f) * CAR_VERTICAL_MOVE_RATE;
                state->speed = (int16_t)std::round(std::fabs(speed));

                nx = cars[i].x + cosf(angle) * speed;
                ny = cars[i].y + sinf(angle) * speed;
            }

            bool in_bounds = !(nx < 0 || ny < 0 ||
                nx + cars[i].w > WIN_W ||
                ny + cars[i].h > WIN_H);

            if (in_bounds) {
                cars[i].x = nx;
                cars[i].y = ny;
            }

            drawRotatedCar(win, cars[i]);
            drawHealthBar(win, cars[i]);

        }

        tigrUpdate(win);
    }

    tigrFree(win);
    return 0;
}
