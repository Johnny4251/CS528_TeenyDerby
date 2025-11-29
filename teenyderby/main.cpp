#include <iostream>
#include <vector>
#include <ctime>
#include <cmath>
#include <algorithm>   
#include "tigr.h"
#include "utils.h"
#include "bus.h"

static float g_speeds[AGENT_MAX_CNT] = {0.0f};
static const float MAX_SPEED       = CAR_VERTICAL_MOVE_RATE;
static const float SPEED_SMOOTHING = 0.08f;
static const float IDLE_FRICTION   = 0.03f;

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

                float targetSpeed = (tval / 100.0f) * MAX_SPEED;
                float currentSpeed = g_speeds[i];

                float accel = (targetSpeed - currentSpeed) * SPEED_SMOOTHING;
                currentSpeed += accel;

                currentSpeed *= 0.995f;

                if (std::fabs(tval) < 5.0f) {
                    currentSpeed *= (1.0f - IDLE_FRICTION);
                }

                if (std::fabs(currentSpeed) < 0.02f) {
                    currentSpeed = 0.0f;
                }

                if (currentSpeed >  MAX_SPEED) currentSpeed =  MAX_SPEED;
                if (currentSpeed < -MAX_SPEED) currentSpeed = -MAX_SPEED;

                g_speeds[i] = currentSpeed;

                state->speed = (int16_t)std::round(std::fabs(currentSpeed));

                nx = static_cast<int>(cars[i].x + cosf(angle) * currentSpeed);
                ny = static_cast<int>(cars[i].y + sinf(angle) * currentSpeed);
            }



            bool in_bounds = !(nx < 0 || ny < 0 ||
            nx + cars[i].w > WIN_W ||
            ny + cars[i].h > WIN_H);

            bool blocked = false;
            Car temp = cars[i];
            temp.x = nx;
            temp.y = ny;
            
            if (!in_bounds)
                blocked = true;

            if (!blocked) {
                for (size_t j = 0; j < cars.size(); j++) {
                    if (j == i) continue;

                    if (checkCarCollision(temp, cars[j])) {
                        blocked = true;
                        break;
                    }
                }
            }

            if (!blocked) {
                cars[i].x = nx;
                cars[i].y = ny;
            } else {
                g_speeds[i] = 0.0f;
                if (state) {
                    state->speed = 0;
                }

                if (cars[i].x < 0) cars[i].x = 0;
                if (cars[i].y < 0) cars[i].y = 0;
                if (cars[i].x + cars[i].w > WIN_W) cars[i].x = WIN_W - cars[i].w;
                if (cars[i].y + cars[i].h > WIN_H) cars[i].y = WIN_H - cars[i].h;
            }



            drawRotatedCar(win, cars[i]);
            drawHealthBar(win, cars[i]);

        }

        tigrUpdate(win);
    }

    tigrFree(win);
    return 0;
}
