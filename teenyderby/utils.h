#ifndef __UTILS_H_
#define __UTILS_H_

#include "tigr.h"
#include "../teenyat.h"
#include <vector>
#include <string>
#include <cmath>

#define AGENT_MAX_CNT 16
#define WIN_W 800
#define WIN_H 600

#define CAR_VERTICAL_MOVE_RATE   6

struct Car {
    int x, y;
    int w, h;
    float angle;
    TPixel color;
    std::string name;
};

struct DerbyState {
    int16_t throttle;
    uint8_t direction;

    uint8_t id;
    uint8_t sensor_target;

    int16_t speed;
    int8_t health;

    bool isDead;
};

extern DerbyState*       g_derby_state;
extern size_t            g_derby_state_count;
extern std::vector<Car>* g_cars;

extern float g_speeds[AGENT_MAX_CNT];
extern int   g_hitCooldown[AGENT_MAX_CNT];
extern Tigr* g_carSprite;

extern const float MAX_SPEED;
extern const float SPEED_SMOOTHING;
extern const float IDLE_FRICTION;

// Author: William Confer
void tigrBlitCenteredRotate(Tigr *dst, Tigr *src,
                            int dx, int dy,
                            int sx, int sy,
                            int sw, int sh,
                            float angle);

// Draw car image to the car                            
void drawCarSprite(Tigr* win, const Car& car);        

// Draw namne tag above car
void drawNameTag(Tigr* win, const Car& car);

// Collects all .tny binary filenames into bin_files.
void get_binaries(std::vector<std::string> &bin_files);

// Loads agent binaries and initializes DerbyState array.
void load_agents(const std::vector<std::string>& bin_files,
                 std::vector<teenyat>& agents,
                 DerbyState* derby_state);

// Randomizes initial car positions/colors based on agents.
void randomize_cars(std::vector<Car> &cars,
                    const std::vector<teenyat> &agents,
                    const std::vector<std::string> &bin_files);

// Draws a rotated car sprite based on its angle & position.
void drawRotatedCar(Tigr* win, const Car& car);

// Renders a health bar above the car.
void drawHealthBar(Tigr* win, const Car& car);

// Tests if rotated bounding box of car is within window bounds.
bool rotatedInBounds(const Car& car, float nx, float ny, float angle);

// Projects rectangle corners onto an axis for SAT collision testing.
void projectOntoAxis(const float px[4], const float py[4],
                            float ax, float ay,
                            float &minProj, float &maxProj);

// Returns true if car A overlaps car B (2D SAT test).
bool checkCarCollision(const Car &a, const Car &b);

// Decrements the hit cooldown timer for a given agent.
void updateHitCooldown(int idx);

// Updates health, death flags, and the VM clock tick for an agent.
void updateAgentState(teenyat& agent, DerbyState* state);

// Converts DerbyState.direction (0–7) into a rotation angle.
float computeDirectionAngle(const DerbyState* state);

// Computes smoothed acceleration/friction to update car speed.
float computeSmoothedSpeed(int idx, const DerbyState* state);

// Calculates new candidate X/Y position based on angle + speed.
void computeNextPosition(const Car& car, float angle, float speed, int& nx, int& ny);

// Checks for wall or car collisions; returns collided index (or -1).
bool detectCollision(const std::vector<Car>& cars, size_t i, int nx, int ny, int& collidedWith);

// Applies damage rules for car-to-car or wall collisions.
void applyCollisionDamage(int i, int collidedWith);

// Moves car to new position or clamps to arena while resetting speed.
void applyMovementOrClamp(Car& car, DerbyState* state, bool blocked, int nx, int ny, int idx);

#endif