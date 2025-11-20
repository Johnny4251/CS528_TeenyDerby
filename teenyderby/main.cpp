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

void tigrFillTriangle(Tigr* win,
                      float x1, float y1,
                      float x2, float y2,
                      float x3, float y3,
                      TPixel color)
{
    // Sort by y-coordinate (y1 <= y2 <= y3)
    if (y1 > y2) { std::swap(y1, y2); std::swap(x1, x2); }
    if (y2 > y3) { std::swap(y2, y3); std::swap(x2, x3); }
    if (y1 > y2) { std::swap(y1, y2); std::swap(x1, x2); }

    // Simple linear interpolation: given a → b, with t from 0 to 1, returns a value between them.
    // Used to compute the X position along an edge at a given Y.
    auto interp = [](float a, float b, float t) {
        return a + (b - a) * t;
    };


    // Draws a horizontal line from xStart → xEnd at row y.
    // Handles clipping (doesn’t draw outside the window).

    auto drawSpan = [&](float y, float xStart, float xEnd) {
        if (y < 0 || y >= win->h) return;
        if (xStart > xEnd) std::swap(xStart, xEnd);
        int xs = (int)std::floor(xStart);
        int xe = (int)std::ceil(xEnd);
        if (xe < 0 || xs >= win->w) return;

        xs = std::max(xs, 0);
        xe = std::min(xe, win->w - 1);

        for (int x = xs; x <= xe; ++x)
            tigrPlot(win, x, (int)y, color);
    };

    // dy1 = height of the top half (y1 → y2)
    // dy2 = total triangle height (y1 → y3)
    // dy3 = bottom half (y2 → y3)
    // Used in t / dy calculations for interpolation - a way to figure out how far along each edge you are when filling a horizontal row.
    float dy1 = y2 - y1;
    float dy2 = y3 - y1;
    float dy3 = y3 - y2;

    // Iterates row by row from top to bottom of triangle (y1 → y3).
    // Divides triangle into top and bottom halves using y2.
    // Computes X positions on the left/right edges at that row using interp.
    // Draws a horizontal line between those two X positions (drawSpan).
    for (float y = y1; y <= y3; y++) {
        if (y < y2) {
            float t1 = dy1 == 0 ? 0 : (y - y1) / dy1;
            float t2 = dy2 == 0 ? 0 : (y - y1) / dy2;
            float xa = interp(x1, x2, t1);
            float xb = interp(x1, x3, t2);
            drawSpan(y, xa, xb);
        } else {
            float t1 = dy3 == 0 ? 0 : (y - y2) / dy3;
            float t2 = dy2 == 0 ? 0 : (y - y1) / dy2;
            float xa = interp(x2, x3, t1);
            float xb = interp(x1, x3, t2);
            drawSpan(y, xa, xb);
        }
    }
    // Result is a filled triangle.
}

void drawRotatedCar(Tigr* win, const Car& car)
{   
    // (cx, cy) is the center of the rectangle
    float cx = car.x + car.w / 2.0f;
    float cy = car.y + car.h / 2.0f;

    // hw/hh = half-width and half-height
    // c/s = cosine/sine of rotation angle
    float hw = car.w / 2.0f;
    float hh = car.h / 2.0f;

    float c = cosf(car.angle);
    float s = sinf(car.angle);

    // 4 rectangle corners relative to the center
    float rx[4] = { -hw,  hw,  hw, -hw };
    float ry[4] = { -hh, -hh,  hh,  hh };

    float px[4], py[4];

    // x′=xcosθ−ysinθ
    // y′=xsinθ+ycosθ
    for (int i = 0; i < 4; i++) {
        float x = rx[i];
        float y = ry[i];
        // rotate
        float xr = x * c - y * s;
        float yr = x * s + y * c;
        // translate back to screen center
        px[i] = cx + xr;
        py[i] = cy + yr;
    }

    // Draw two triangles that form the rectangle
    tigrFillTriangle(win, px[0], py[0], px[1], py[1], px[2], py[2], car.color);
    tigrFillTriangle(win, px[0], py[0], px[2], py[2], px[3], py[3], car.color);
}

void getRotatedCorners(int x, int y, int w, int h, float angle, float px[4], float py[4]) {
    float cx = x + w / 2.0f;
    float cy = y + h / 2.0f;

    float hw = w / 2.0f;
    float hh = h / 2.0f;

    float c = cosf(angle);
    float s = sinf(angle);

    float rx[4] = { -hw,  hw,  hw, -hw };
    float ry[4] = { -hh, -hh,  hh,  hh };

    // x′=xcosθ−ysinθ
    // y′=xsinθ+ycosθ
    for (int i = 0; i < 4; i++) {
        float xr = rx[i] * c - ry[i] * s;
        float yr = rx[i] * s + ry[i] * c;

        px[i] = cx + xr;
        py[i] = cy + yr;
    }
}

// Checks if all four corners of the rotated rectangle are inside the window.
// Returns true if it’s fully inside, false if any corner is out-of-bounds.
bool rotatedInBounds(const Car& car, float nx, float ny, float angle)
{
    float px[4], py[4];
    getRotatedCorners(nx, ny, car.w, car.h, angle, px, py);

    for (int i = 0; i < 4; i++) {
        if (px[i] < 0 || px[i] > WIN_W ||
            py[i] < 0 || py[i] > WIN_H)
            return false;
    }
    return true;
}


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

                    for (size_t j = 0; j < cars.size(); ++j) {
                        if (j == i) continue;
                        if (!(nx + cars[i].w <= cars[j].x || nx >= cars[j].x + cars[j].w ||
                                ny + cars[i].h <= cars[j].y || ny >= cars[j].y + cars[j].h)) {
                            collides = true;
                            break;
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