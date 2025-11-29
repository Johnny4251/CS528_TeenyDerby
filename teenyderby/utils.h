#ifndef __UTILS_H_
#define __UTILS_H_

#include "tigr.h"
#include "../teenyat.h"
#include <vector>
#include <string>
#include <cmath>

#define AGENT_MAX_CNT 8
#define WIN_W 800
#define WIN_H 600

#define CAR_VERTICAL_MOVE_RATE   6

struct Car {
    int x, y;
    int w, h;
    float angle;
    TPixel color;
};

struct DerbyState {
    int16_t throttle;
    uint8_t direction;

    uint8_t id;
    uint8_t sensor_target;

    int16_t speed;
    uint8_t health;
};

extern DerbyState*       g_derby_state;
extern size_t            g_derby_state_count;
extern std::vector<Car>* g_cars;

void get_binaries(std::vector<std::string> &bin_files);
void load_agents(const std::vector<std::string>& bin_files,
                 std::vector<teenyat>& agents,
                 DerbyState* derby_state);

void randomize_cars(std::vector<Car> &cars, std::vector<teenyat> &agents);

void drawRotatedCar(Tigr* win, const Car& car);
void drawHealthBar(Tigr* win, const Car& car); 
bool rotatedInBounds(const Car& car, float nx, float ny, float angle);

static void projectOntoAxis(const float px[4], const float py[4],
                            float ax, float ay,
                            float &minProj, float &maxProj);
bool checkCarCollision(const Car &a, const Car &b);
#endif