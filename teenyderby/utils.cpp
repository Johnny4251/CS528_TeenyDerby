#include <stdio.h>
#include <dirent.h>
#include <string>
#include <cstdlib>
#include <algorithm>
#include "utils.h"
#include "bus.h"
#include "teenyat.h"

#define TIGR_BLEND_ALPHA(C, A, B) (unsigned char)(((C) * (A) + (B) * (255 - (A))) / 255)

DerbyState*       g_derby_state       = nullptr;
size_t            g_derby_state_count = 0;
std::vector<Car>* g_cars              = nullptr;

float g_speeds[AGENT_MAX_CNT] = {0.0f};
int   g_hitCooldown[AGENT_MAX_CNT] = {0};
float g_scoreRowY[AGENT_MAX_CNT] = {0.0f};
bool  g_scoreRowInit = false;

const float MAX_SPEED        = CAR_VERTICAL_MOVE_RATE;
const float SPEED_SMOOTHING  = 0.08f;
const float IDLE_FRICTION    = 0.03f;

/* Author: William Confer */
void tigrBlitCenteredRotate(Tigr *dst, Tigr *src,
                            int dx, int dy,
                            int sx, int sy,
                            int sw, int sh,
                            float angle)
{
    // --- Angle Normalization (Toroidal) ---
    angle = fmodf(angle, 360.0f);
    if (angle < 0) angle += 360.0f;
    float rad = angle * (float)M_PI / 180.0f;

    // --- Inverse Rotation Matrix Components ---
    float c = cosf(rad); // cos(-rad) = cos(rad)
    float s = sinf(rad); // sin(-rad) = -sin(rad)

    // --- Source Center ---
    float src_cx = sw / 2.0f;
    float src_cy = sh / 2.0f;

    // --- Destination Bounding Box ---
    float hsw = sw / 2.0f;
    float hsh = sh / 2.0f;
    float max_dist = sqrtf(hsw * hsw + hsh * hsh);

    int dst_x_min = (int)floorf(dx - max_dist);
    int dst_y_min = (int)floorf(dy - max_dist);
    int dst_x_max = (int)ceilf(dx + max_dist);
    int dst_y_max = (int)ceilf(dy + max_dist);

    // Clamp to bitmap bounds
    if (dst_x_min < 0) dst_x_min = 0;
    if (dst_y_min < 0) dst_y_min = 0;
    if (dst_x_max > dst->w) dst_x_max = dst->w;
    if (dst_y_max > dst->h) dst_y_max = dst->h;

    // --- Iterate Pixels ---
    for (int y = dst_y_min; y < dst_y_max; y++)
    {
        for (int x = dst_x_min; x < dst_x_max; x++)
        {
            // Pixel relative to center
            float p_x = x - dx;
            float p_y = y - dy;

            // Inverse rotation → source space
            float s_x = p_x * c + p_y * s;
            float s_y = p_x * -s + p_y * c;

            // Convert to source bitmap coords
            int src_map_x = sx + (int)floorf(s_x + src_cx);
            int src_map_y = sy + (int)floorf(s_y + src_cy);

            // Bounds check
            if (src_map_x >= sx && src_map_x < sx + sw &&
                src_map_y >= sy && src_map_y < sy + sh)
            {
                TPixel src_pix = src->pix[src_map_x + src_map_y * src->w];
                TPixel *dst_pix = &dst->pix[x + y * dst->w];

                // Transparency cases
                if (src_pix.a == 0)
                    continue;

                if (src_pix.a == 255)
                {
                    *dst_pix = src_pix;
                }
                else
                {
                    int A = src_pix.a;
                    dst_pix->r = TIGR_BLEND_ALPHA(src_pix.r, A, dst_pix->r);
                    dst_pix->g = TIGR_BLEND_ALPHA(src_pix.g, A, dst_pix->g);
                    dst_pix->b = TIGR_BLEND_ALPHA(src_pix.b, A, dst_pix->b);
                    dst_pix->a = 255;
                }
            }
        }
    }
}

void drawNameTag(Tigr* win, const Car& car) {
    if (car.name.empty()) return;

    float cx = car.x + car.w / 2.0f;
    float cy = car.y - 12; 

    int textW = tigrTextWidth(tfont, car.name.c_str());
    int textH = tigrTextHeight(tfont, car.name.c_str());

    int tx = (int)(cx - textW / 2);
    int ty = (int)(cy - textH);

    tigrFillRect(win, tx - 2, ty - 1, textW + 4, textH + 2, tigrRGB(0,0,0));
    tigrPrint(win, tfont, tx, ty, tigrRGB(255,255,255), "%s", car.name.c_str());
}

Tigr* get_sprite (Car &car) {
    Tigr* carSprite = nullptr;
    switch(car.type) {
        case CAR_TYPE_CONVERTABLE:
            carSprite = tigrLoadImage("sprites/convertable.png");
            break;
        case CAR_TYPE_CORVETTE:
            carSprite = tigrLoadImage("sprites/corvette.png");
            break;
        case CAR_TYPE_GARBAGETRUCK:
            carSprite = tigrLoadImage("sprites/garbagetruck.png");
            break;
        case CAR_TYPE_JEEP:
            carSprite = tigrLoadImage("sprites/jeep.png");
            break;
        case CAR_TYPE_MOTORCYCLE:
            carSprite = tigrLoadImage("sprites/motorcycle.png");
            break;
        case CAR_TYPE_MUSTANG:
            carSprite = tigrLoadImage("sprites/mustang.png");
            break;
        case CAR_TYPE_STATIONWAGON:
            carSprite = tigrLoadImage("sprites/stationwagon.png");
            break;
        case CAR_TYPE_TOWTRUCK:
            carSprite = tigrLoadImage("sprites/towtruck.png");
            break;
        default:
            carSprite = tigrLoadImage("sprites/corvette.png");
            break;
    }
    return carSprite;
}

int drawCarSprite(Tigr* win, const Car& car) {

    Tigr* carSprite = get_sprite(const_cast<Car&>(car));

    if (!carSprite) {
        const char* sprite_names[] = {
            "convertable.png", "corvette.png", "garbagetruck.png", "jeep.png",
            "motorcycle.png", "mustang.png", "stationwagon.png", "towtruck.png", "default.png"
        };
        printf("could not find sprites/%s\n", sprite_names[car.type]);
        printf("exiting...\n");
        return -1;
    }
    
    int dx = (int)(car.x + car.w / 2.0f);
    int dy = (int)(car.y + car.h / 2.0f);

    int sw = carSprite->w;
    int sh = carSprite->h;

    tigrBlitCenteredRotate(
        win,
        carSprite,
        dx, dy,    
        0, 0,        
        sw, sh,      
        car.angle * 180.0f / 3.14159265f 
    );
    // Free per-draw to avoid leaks (Option B)
    tigrFree(carSprite);
    return 0;
}

void get_binaries(std::vector<std::string> &bin_files) {
    DIR* dir = opendir("agents");
    if (!dir)
        return;

    struct dirent* ent;
    while ((ent = readdir(dir)) != nullptr) {
        std::string fname = ent->d_name;
        if (fname.size() > 4 && fname.substr(fname.size() - 4) == ".bin") {
            bin_files.push_back("agents/" + fname);
            if (bin_files.size() >= AGENT_MAX_CNT)
                break;
        }
    }
    closedir(dir);
}

void load_agents(const std::vector<std::string>& bin_files,
                 std::vector<teenyat>& agents,
                 DerbyState* derby_state)
{
    agents.clear();
    agents.resize(bin_files.size());

    g_derby_state       = derby_state;
    g_derby_state_count = bin_files.size();

    for (size_t i = 0; i < bin_files.size(); ++i) {
        derby_state[i].id            = i;
        derby_state[i].sensor_target = 0;
        derby_state[i].speed         = 0;
        derby_state[i].health        = 100;
    }

    for (size_t i = 0; i < bin_files.size(); ++i) {
        FILE* f = fopen(bin_files[i].c_str(), "rb");
        if (!f) continue;

        tny_init_from_file(&agents[i], f, derby_bus_read, derby_bus_write);
        agents[i].ex_data = &derby_state[i];

        fclose(f);
    }
}


car_type parse_car_type_from_filename(const std::string& filename) {
    std::string lower = filename;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    if (lower.find("convertable") != std::string::npos)
        return CAR_TYPE_CONVERTABLE;
    if (lower.find("corvette") != std::string::npos)
        return CAR_TYPE_CORVETTE;
    if (lower.find("garbagetruck") != std::string::npos)
        return CAR_TYPE_GARBAGETRUCK;
    if (lower.find("jeep") != std::string::npos)
        return CAR_TYPE_JEEP;
    if (lower.find("motorcycle") != std::string::npos)
        return CAR_TYPE_MOTORCYCLE;
    if (lower.find("mustang") != std::string::npos)
        return CAR_TYPE_MUSTANG;
    if (lower.find("stationwagon") != std::string::npos)
        return CAR_TYPE_STATIONWAGON;
    if (lower.find("towtruck") != std::string::npos)
        return CAR_TYPE_TOWTRUCK;
    
    return CAR_TYPE_DEFAULT;
}

void randomize_cars(std::vector<Car> &cars,
                    const std::vector<teenyat> &agents,
                    const std::vector<std::string> &bin_files)
{
    g_cars = &cars;
    cars.clear();

    for (size_t i = 0; i < agents.size(); ++i) {
        Car c;

        // Parse car type from binary filename
        c.type = parse_car_type_from_filename(bin_files[i]);

        c.w = 32;
        c.h = 16;
        c.x = rand() % (PLAYFIELD_W - c.w);
        c.y = rand() % (PLAYFIELD_H - c.h);
        c.angle = 0;
        c.color = tigrRGB(30, 30, 30);

        std::string full = bin_files[i];
        size_t slash = full.find_last_of("/\\");
        size_t dot   = full.find_last_of('.');
        if (dot == std::string::npos) dot = full.size();
        if (slash == std::string::npos) slash = 0; else slash++;

        c.name = full.substr(slash, dot - slash);
        
        const char* car_types[] = {"_convertable", "_corvette", "_garbagetruck", "_jeep",
                                   "_motorcycle", "_mustang", "_stationwagon", "_towtruck"};
        for (const char* suffix : car_types) {
            size_t pos = c.name.find(suffix);
            if (pos != std::string::npos) {
                c.name = c.name.substr(0, pos);
                break;
            }
        }

        cars.push_back(c);
    }
}


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
    // Top-left, Top-right, Bottom-right, Bottom-left
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


    // FRONT EDGE = between corners 0 → 1
    tigrLine(win,
        (int)px[1], (int)py[1],
        (int)px[2], (int)py[2],
        tigrRGB(255, 0, 0));  // red

    // BACK EDGE = between corners 3 → 2
    tigrLine(win,
        (int)px[0], (int)py[0],
        (int)px[3], (int)py[3],
        tigrRGB(0, 0, 255));  // blue
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

bool rotatedInBounds(const Car& car, float nx, float ny, float angle)
{
    float px[4], py[4];
    getRotatedCorners(nx, ny, car.w, car.h, angle, px, py);

    for (int i = 0; i < 4; i++) {
        if (px[i] < 0 || px[i] > PLAYFIELD_W ||
            py[i] < 0 || py[i] > PLAYFIELD_H)
            return false;
    }
    return true;
}


void drawHealthBar(Tigr* win, const Car& car)
{
    if (!g_cars || !g_derby_state || g_cars->empty())
        return;

    const Car* base = g_cars->data();
    const Car* ptr  = &car;
    int index = static_cast<int>(ptr - base);

    if (index < 0 || static_cast<size_t>(index) >= g_derby_state_count)
        return;

    const DerbyState& st = g_derby_state[index];

    const int maxHealth = 100;
    int health = static_cast<int>(st.health);

    if (health < 0)         health = 0;
    if (health > maxHealth) health = maxHealth;

    float ratio = (maxHealth > 0)
                  ? (static_cast<float>(health) / static_cast<float>(maxHealth))
                  : 0.0f;

    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;

    int barWidth  = car.w;
    int barHeight = 4;

    int barX = car.x;
    int barY = car.y + car.h + 4;     

    TPixel bg = tigrRGB(40, 40, 40);  

    TPixel fg;
    if (ratio > 0.66f)
        fg = tigrRGB(0, 255, 0);      
    else if (ratio > 0.33f)
        fg = tigrRGB(255, 255, 0);    
    else
        fg = tigrRGB(255, 0, 0);      

    TPixel border = tigrRGB(0, 0, 0);

    
    tigrFillRect(win, barX, barY, barWidth, barHeight, bg);

    
    int filledWidth = static_cast<int>(barWidth * ratio);
    if (filledWidth > 0)
        tigrFillRect(win, barX, barY, filledWidth, barHeight, fg);

    tigrRect(win,
             barX - 1,
             barY - 1,
             barWidth  + 2,
             barHeight + 2,
             border);
}

// Projects set of points onto axis and return min/max
 void projectOntoAxis(const float px[4], const float py[4],
                            float ax, float ay,
                            float &minProj, float &maxProj)
{
    minProj = maxProj = px[0] * ax + py[0] * ay;
    for (int i = 1; i < 4; i++) {
        float proj = px[i] * ax + py[i] * ay;
        if (proj < minProj) minProj = proj;
        if (proj > maxProj) maxProj = proj;
    }
}

// Returns true if rotated cars overlap
bool checkCarCollision(const Car &a, const Car &b)
{
    float ax[4], ay[4];
    float bx[4], by[4];

    getRotatedCorners(a.x, a.y, a.w, a.h, a.angle, ax, ay);
    getRotatedCorners(b.x, b.y, b.w, b.h, b.angle, bx, by);

    
    float axes[4][2];
    axes[0][0] = ax[1] - ax[0]; axes[0][1] = ay[1] - ay[0];
    axes[1][0] = ax[3] - ax[0]; axes[1][1] = ay[3] - ay[0];
    axes[2][0] = bx[1] - bx[0]; axes[2][1] = by[1] - by[0];
    axes[3][0] = bx[3] - bx[0]; axes[3][1] = by[3] - by[0];

    for (int i = 0; i < 4; i++) {
        float axx = axes[i][0];
        float ayy = axes[i][1];

        
        float len = sqrtf(axx * axx + ayy * ayy);
        axx /= len; ayy /= len;

        float minA, maxA, minB, maxB;
        projectOntoAxis(ax, ay, axx, ayy, minA, maxA);
        projectOntoAxis(bx, by, axx, ayy, minB, maxB);

        
        if (maxA < minB || maxB < minA)
            return false;     
    }

    return true;               
}

void updateHitCooldown(int idx) {
    if (g_hitCooldown[idx] > 0)
        g_hitCooldown[idx]--;
}

void updateAgentState(teenyat& agent, DerbyState* state) {
    if (!state) return;

    if (state->health > 100)
        state->health = 100;

    if (state->health <= 0) {
        state->health = 0;
        state->isDead = true;
    }

    if (!state->isDead)
        tny_clock(&agent);
}


float computeDirectionAngle(const DerbyState* state) {
    if (!state) return 0.0f;

    uint8_t dir = state->direction & 7;
    return dir * (3.14159265f / 4.0f);
}

void computeNextPosition(const Car& car, float angle, float speed, int& nx, int& ny) {
    nx = (int)(car.x + cosf(angle) * speed);
    ny = (int)(car.y + sinf(angle) * speed);
}

bool detectCollision(const std::vector<Car>& cars, size_t i, int nx, int ny, int& collidedWith) {
    collidedWith = -1;
    
    // Wall check
    if (nx < 0 || ny < 0 || nx + cars[i].w > PLAYFIELD_W || ny + cars[i].h > PLAYFIELD_H)
        return true;

    Car temp = cars[i];
    temp.x = nx;
    temp.y = ny;

    // Other cars
    for (size_t j = 0; j < cars.size(); j++) {
        if (j == i) continue;

        if (checkCarCollision(temp, cars[j])) {
            collidedWith = (int)j;
            return true;
        }
    }

    return false;
}


float computeSmoothedSpeed(int idx, const DerbyState* state) {
    if (!state) return 0.0f;

    float& currentSpeed = g_speeds[idx];

    if (state->isDead) {
        currentSpeed *= 0.3f;  

        if (std::fabs(currentSpeed) < 0.05f)
            currentSpeed = 0.0f;

        return currentSpeed;
    }

    float tval = std::clamp((float)state->throttle, -100.0f, 100.0f);

    float targetSpeed = (tval / 100.0f) * MAX_SPEED;
    float accel = (targetSpeed - currentSpeed) * SPEED_SMOOTHING;
    currentSpeed += accel;

    currentSpeed *= 0.995f;

    if (std::fabs(tval) < 5.0f)
        currentSpeed *= (1.0f - IDLE_FRICTION);

    if (std::fabs(currentSpeed) < 0.02f)
        currentSpeed = 0.0f;

    currentSpeed = std::clamp(currentSpeed, -MAX_SPEED, MAX_SPEED);

    return currentSpeed;
}


void applyCollisionDamage(int i, int collidedWith) {

    if (collidedWith != -1) {
        float speedA = std::fabs(g_speeds[i]);
        float speedB = std::fabs(g_speeds[collidedWith]);

        int attacker = (speedA > speedB ? i : collidedWith);
        int victim   = (attacker == i ? collidedWith : i);

        float attackerSpeed = std::max(speedA, speedB);

        int bigDamage   = std::max(1, (int)std::round(attackerSpeed * 2.0f));
        int smallDamage = std::max(1, (int)std::round(attackerSpeed * 0.6f));

        if (g_hitCooldown[victim] == 0) {
            g_derby_state[victim].health -= bigDamage;
            g_derby_state[victim].health = std::max<int>(0, g_derby_state[victim].health);
            g_hitCooldown[victim] = 15;
        }

        if (g_hitCooldown[attacker] == 0) {
            g_derby_state[attacker].health -= smallDamage;
            g_derby_state[attacker].health = std::max<int>(0, g_derby_state[attacker].health);
            g_hitCooldown[attacker] = 15;
        }

        return;
    }

    // Wall collision
    float speed = std::fabs(g_speeds[i]);
    int wallDamage = std::max(1, (int)std::round(speed * 0.2f));

    if (g_hitCooldown[i] == 0) {
        g_derby_state[i].health -= wallDamage;
        g_derby_state[i].health = std::max<int>(0, g_derby_state[i].health);
        g_hitCooldown[i] = 10;
    }
}

void applyMovementOrClamp(Car& car, DerbyState* state, bool blocked, int nx, int ny, int idx)
{
    if (!blocked) {
        car.x = nx;
        car.y = ny;
        return;
    }

    g_speeds[idx] = 0.0f;
    if (state)
        state->speed = 0;

    if (car.x < 0) car.x = 0;
    if (car.y < 0) car.y = 0;
    if (car.x + car.w > PLAYFIELD_W) car.x = PLAYFIELD_W - car.w;
    if (car.y + car.h > PLAYFIELD_H) car.y = PLAYFIELD_H - car.h;
}

void drawScoreboard(Tigr* win) {
    if (!g_cars || !g_derby_state || g_cars->empty())
        return;

    const int sbX = PLAYFIELD_W;
    const int sbY = 0;
    const int sbW = WIN_W - PLAYFIELD_W;
    const int sbH = PLAYFIELD_H;

    tigrFillRect(win, sbX, sbY, sbW, sbH, tigrRGB(14, 14, 22));
    tigrRect(win, sbX, sbY, sbW, sbH, tigrRGB(60, 60, 80));

    const int headerH = 22;
    tigrFillRect(win, sbX, sbY, sbW, headerH, tigrRGB(26, 26, 40));
    tigrLine(win, sbX, sbY + headerH, sbX + sbW, sbY + headerH, tigrRGB(80, 80, 110));

    tigrPrint(win, tfont, sbX + 8, sbY + 6, tigrRGB(220, 220, 255), "DERBY AGENTS");

    const int rowH    = 34;
    const int startY  = sbY + headerH + 4;
    const size_t cnt  = std::min(g_derby_state_count, g_cars->size());
    if (cnt == 0) return;

    std::vector<int> order(cnt);
    for (size_t i = 0; i < cnt; ++i)
        order[i] = (int)i;

    std::sort(order.begin(), order.end(), [](int a, int b) {
        const DerbyState& sa = g_derby_state[a];
        const DerbyState& sb = g_derby_state[b];

        if (sa.isDead != sb.isDead)
            return !sa.isDead && sb.isDead;

        if (sa.health != sb.health)
            return sa.health > sb.health;

        return sa.id < sb.id;
    });

    if (!g_scoreRowInit) {
        for (size_t rank = 0; rank < cnt; ++rank) {
            int idx = order[rank];
            g_scoreRowY[idx] = (float)(startY + (int)rank * rowH);
        }
        g_scoreRowInit = true;
    }

    const float SMOOTH = .95f; 

    for (size_t rank = 0; rank < cnt; ++rank)
    {
        int idx = order[rank];
        const DerbyState& st = g_derby_state[idx];
        const Car&        car = (*g_cars)[idx];

        float targetY = (float)(startY + (int)rank * rowH);
        float currentY = g_scoreRowY[idx];
        currentY += (targetY - currentY) * SMOOTH;
        g_scoreRowY[idx] = currentY;

        int boxY = (int)currentY;
        if (boxY + rowH > sbY + sbH - 4)
            break; 

        bool evenRow = (rank % 2 == 0);
        TPixel boxBg     = evenRow ? tigrRGB(30, 30, 42) : tigrRGB(24, 24, 36);
        TPixel boxBorder = tigrRGB(70, 70, 100);

        if (st.isDead) {
            boxBg     = tigrRGB(18, 18, 22);
            boxBorder = tigrRGB(110, 50, 50);
        }

        tigrFillRect(win, sbX + 4, boxY, sbW - 8, rowH - 4, boxBg);
        tigrRect(win, sbX + 4, boxY, sbW - 8, rowH - 4, boxBorder);

        const int iconSize = 20;
        const int iconX = sbX + 8;
        const int iconY = boxY + (rowH - iconSize) / 2;

        Tigr* carSprite = get_sprite(const_cast<Car&>(car));
        if (carSprite) {
            int srcW = std::min(iconSize, carSprite->w);
            int srcH = std::min(iconSize, carSprite->h);
            int srcX = (carSprite->w - srcW) / 2;
            int srcY = (carSprite->h - srcH) / 2;

            tigrBlit(win, carSprite, iconX, iconY, srcX, srcY, srcW, srcH);
            // Free per-draw for scoreboard icon
            tigrFree(carSprite);
        } else {
            tigrFillRect(win, iconX, iconY, iconSize, iconSize, car.color);
        }

        int textX = iconX + iconSize + 6;
        int textY = boxY + 6;

        const char* name = car.name.empty() ? "Agent" : car.name.c_str();
        TPixel nameColor = st.isDead ? tigrRGB(220, 120, 120)
                                     : tigrRGB(235, 235, 255);

        tigrPrint(win, tfont, textX, textY, nameColor, "%s", name);

        int hp = std::max(0, (int)st.health);
        float ratio = std::clamp(hp / 100.0f, 0.0f, 1.0f);

        int barW = sbW - (textX - sbX) - 10;
        int barH = 5;
        int barX = textX;
        int barY = boxY + rowH - barH - 6;

        TPixel bgBar = tigrRGB(40, 40, 55);
        TPixel fgBar = tigrRGB(
            (int)(255 * (1.0f - ratio)),
            (int)(255 * ratio),
            60
        );

        tigrFillRect(win, barX, barY, barW, barH, bgBar);
        int filledW = (int)(barW * ratio);
        if (filledW > 0)
            tigrFillRect(win, barX, barY, filledW, barH, fgBar);
    }
}


void drawTitleBar(Tigr* win)
{
    const int barY = PLAYFIELD_H;
    const int barH = WIN_H - PLAYFIELD_H;

    tigrFillRect(win, 0, barY, WIN_W, barH, tigrRGB(16, 16, 28));
    tigrRect(win, 0, barY, WIN_W, barH, tigrRGB(80, 80, 130));

    const char* title = "TeenyDerby";

    int baseW = tigrTextWidth(tfont, title);
    int baseH = tigrTextHeight(tfont, title);

    const int scale = 2;

    int scaledW = baseW * scale;
    int scaledH = baseH * scale;

    int tx = (WIN_W - scaledW) / 2;
    int ty = barY + (barH - scaledH) / 2;

    Tigr* tmp = tigrBitmap(baseW, baseH);
    if (!tmp)
        return;

    int total = baseW * baseH;
    for (int i = 0; i < total; ++i)
    {
        tmp->pix[i].r = 0;
        tmp->pix[i].g = 0;
        tmp->pix[i].b = 0;
        tmp->pix[i].a = 0;
    }

    tigrPrint(tmp, tfont, 0, 0, tigrRGB(255, 255, 255), "%s", title);

    for (int y = 0; y < baseH; ++y)
    {
        for (int x = 0; x < baseW; ++x)
        {
            TPixel p = tmp->pix[y * baseW + x];
            if (p.a == 0)
                continue;

            int dstX0 = tx + x * scale;
            int dstY0 = ty + y * scale;
            for (int oy = 0; oy < scale; ++oy)
            {
                int dy = dstY0 + oy;
                if (dy < 0 || dy >= win->h) continue;

                for (int ox = 0; ox < scale; ++ox)
                {
                    int dx = dstX0 + ox;
                    if (dx < 0 || dx >= win->w) continue;
                    win->pix[dy * win->w + dx] = p;
                }
            }
        }
    }

    tigrFree(tmp);

    if (g_derby_state && g_derby_state_count > 0) {
        int alive = 0;
        for (size_t i = 0; i < g_derby_state_count; ++i)
            if (!g_derby_state[i].isDead)
                alive++;

        char buf[64];
        snprintf(buf, sizeof(buf), "Alive: %d / %zu", alive, g_derby_state_count);

        int textW = tigrTextWidth(tfont, buf);
        int textH = tigrTextHeight(tfont, buf);

        int ax = WIN_W - textW - 8;
        int ay = barY + (barH - textH) / 2;

        tigrPrint(win, tfont, ax, ay, tigrRGB(220, 220, 240), "%s", buf);
    }
}
