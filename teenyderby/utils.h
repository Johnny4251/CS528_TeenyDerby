#ifndef __UTILS_H_
#define __UTILS_H_

#include "tigr.h"
#include "../teenyat.h"
#include <vector>
#include <string>
#include <cmath>

#define AGENT_MAX_CNT 16
#define WIN_W 850
#define WIN_H 650

#define PLAYFIELD_W 700
#define PLAYFIELD_H 600

#define CAR_VERTICAL_MOVE_RATE   6

enum car_type {
    CAR_TYPE_CONVERTABLE,
    CAR_TYPE_CORVETTE,
    CAR_TYPE_GARBAGETRUCK,
    CAR_TYPE_JEEP,
    CAR_TYPE_MOTORCYCLE,
    CAR_TYPE_MUSTANG,
    CAR_TYPE_STATIONWAGON,
    CAR_TYPE_TOWTRUCK,
    CAR_TYPE_DEFAULT
};
struct Car {
    int x, y;
    int w, h;
    float mass;
    float angle;
    TPixel color;
    std::string name;
    car_type type;
};

struct DerbyState {
    int16_t throttle;
    uint8_t direction;

    uint8_t id;
    uint8_t sensor_target;

    int16_t speed;
    int8_t health;
    int smokeLevel;

    bool isDead;
};
struct SmokeParticle {
    float x, y;
    float vx, vy;
    float alpha;   // fade
    float size;
    int color;     // gray value 0–255
};
static const int SMOKE_MAX = 700;
static SmokeParticle smokePool[SMOKE_MAX];

extern DerbyState*       g_derby_state;
extern size_t            g_derby_state_count;
extern std::vector<Car>* g_cars;

extern float g_speeds[AGENT_MAX_CNT];
extern int   g_hitCooldown[AGENT_MAX_CNT];
extern float g_scoreRowY[AGENT_MAX_CNT];
extern bool  g_scoreRowInit;


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
int drawCarSprite(Tigr* win, const Car& car, int idx);        

// Draw namne tag above car
void drawNameTag(Tigr* win, const Car& car);

// Collects all .tny binary filenames into bin_files.
void get_binaries(std::vector<std::string> &bin_files);

// Parses car type from agent binary filename (e.g., "agent_corvette.bin" -> CAR_TYPE_CORVETTE).
car_type parse_car_type_from_filename(const std::string& filename);

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

void spawnSmoke(float x, float y, int level);

void updateSmoke(Tigr* win);

// Retrieves smoke level based on health
int getSmokeLevel(int health);

void emitSmoke(Tigr* win, const Car& car, int level);


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

// Draws the sidebar with agent names, health, and icon.
void drawScoreboard(Tigr* win);

// Draws the bottom title/footer bar.
void drawTitleBar(Tigr* win);

float computeSmoothedAngle(int /*idx*/, const DerbyState* state, float currentAngle);

bool computeOBBPenetration(const Car& A, const Car& B,
                           float& outNx, float& outNy,
                           float& outDepth);

#endif