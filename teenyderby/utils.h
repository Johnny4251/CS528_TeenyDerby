#ifndef __UTILS_H_
#define __UTILS_H_

#include "tigr.h"
#include "../teenyat.h"
#include <vector>
#include <string>
#include <cmath>

// temp
#define DERBY_MOVE_UP_ADDR    0x9000
#define DERBY_MOVE_DOWN_ADDR  0x9001
#define DERBY_MOVE_LEFT_ADDR  0x9002
#define DERBY_MOVE_RIGHT_ADDR 0x9003
#define DERBY_FACE_ADDR  0x9004

#define CAR_HORIZONTAL_MOVE_RATE 4
#define CAR_VERTICAL_MOVE_RATE 4

#define AGENT_MAX_CNT 8

#define WIN_W 800
#define WIN_H 600

#define CAR_W 60
#define CAR_H 20

#define MARGIN 100

typedef struct {
	int move_cmd; // 0=none, 1=up, 2=down, 3=left, 4=right
    int id;
} DerbyState;

struct Car {
    int x, y;
    int w, h;
    float angle;
    TPixel color;
};

/**
 * @brief bla bla
 */
void get_binaries(std::vector<std::string> &bin_files);

/**
 * @brief bla bla bla
 */
void load_agents(const std::vector<std::string>& bin_files,
                 std::vector<teenyat>& agents,
                 DerbyState* derby_state);

/**
 * @brief bla bla
 */
void randomize_cars(std::vector<Car> &cars, std::vector<teenyat> &agents);

void tigrFillTriangle(Tigr* win,
                      float x1, float y1,
                      float x2, float y2,
                      float x3, float y3,
                      TPixel color);


void drawRotatedCar(Tigr* win, const Car& car);

void getRotatedCorners(int x, int y, int w, int h, float angle, float px[4], float py[4]);

bool rotatedInBounds(const Car& car, float nx, float ny, float angle);

#endif