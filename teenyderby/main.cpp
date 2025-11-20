#include <stdio.h>
#include "../teenyat.h"
#include "tigr.h"
#include <vector>
#include <string>
#include <dirent.h>
#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <iostream> 
#include <cmath>
#include "utils.h"



int main() {
    std::srand((unsigned)std::time(nullptr)); 

    // open TIGR window
    Tigr *win = tigrWindow(WIN_W, WIN_H, "TeenyDerby", TIGR_FIXED);

    // get bin files
    std::vector<std::string> bin_files;
    get_binaries(bin_files);
    if(bin_files.size() <= 0) {
        std::cout << "No agent binaries found" << std::endl;
        return -1;
    }

    // get agents
    DerbyState derby_state[AGENT_MAX_CNT] = {};
    std::vector<teenyat> agents(bin_files.size());
    load_agents(bin_files, agents, derby_state);

    std::vector<Car> cars;
    randomize_cars(cars, agents);

    while (!tigrClosed(win)) {
        tigrClear(win, tigrRGB(30,30,30));

        for (size_t i = 0; i < agents.size(); ++i) {
            tny_clock(&agents[i]);
            teenyat *t = &agents[i];
            DerbyState *state = (DerbyState *)t->ex_data;

            {
                int nx = cars[i].x;
                int ny = cars[i].y;


                switch(state->move_cmd) {
                    case 0: break;
                    case 1: { // forward
                        nx = cars[i].x + cosf(cars[i].angle) * CAR_VERTICAL_MOVE_RATE;
                        ny = cars[i].y + sinf(cars[i].angle) * CAR_VERTICAL_MOVE_RATE;
                    break;
                    }
                    case 2: { // backward
                        nx = cars[i].x - cosf(cars[i].angle) * CAR_VERTICAL_MOVE_RATE;
                        ny = cars[i].y - sinf(cars[i].angle) * CAR_VERTICAL_MOVE_RATE;
                    break;
                    }
                    case 3:{
                        float newAngle = cars[i].angle - 0.05f;
                        if (rotatedInBounds(cars[i], cars[i].x, cars[i].y, newAngle))
                            cars[i].angle = newAngle;
                    }
                    break;
                    case 4:{
                        float newAngle = cars[i].angle + 0.05f;
                        if (rotatedInBounds(cars[i], cars[i].x, cars[i].y, newAngle))
                            cars[i].angle = newAngle;
                        }
                    break;
                    default: break;
                }

                    bool in_bounds = !(nx < 0 || ny < 0 || nx + cars[i].w > WIN_W || ny + cars[i].h > WIN_H);

                    bool collides = false;
                    if (in_bounds){
                        for (size_t j = 0; j < cars.size(); ++j) {
                            if (j == i) continue;
                            if (!(nx + cars[i].w <= cars[j].x || nx >= cars[j].x + cars[j].w ||
                                    ny + cars[i].h <= cars[j].y || ny >= cars[j].y + cars[j].h)) {
                                collides = true;
                                break;
                            }
                        }
                    }
                    

                    if (rotatedInBounds(cars[i], nx, ny, cars[i].angle)&& in_bounds && !collides) {
                        cars[i].x = nx;
                        cars[i].y = ny;
                    }

                drawRotatedCar(win, cars[i]);
            }
        }

        tigrUpdate(win);
    }
    tigrFree(win);
    return 0;
}