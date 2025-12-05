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
float g_shakeTimer[AGENT_MAX_CNT] = {0.0f};
float g_scoreRowY[AGENT_MAX_CNT] = {0.0f};
bool  g_scoreRowInit = false;

float g_spinOffset[AGENT_MAX_CNT]  = {0.0f};  
float g_spinVel[AGENT_MAX_CNT]     = {0.0f};  

const float MAX_SPEED        = CAR_VERTICAL_MOVE_RATE;
const float SPEED_SMOOTHING  = 0.08f;
const float IDLE_FRICTION    = 0.03f;

void getRotatedCorners(int x, int y, int w, int h,
                       float angle, float px[4], float py[4]);

void projectOntoAxis(const float px[4], const float py[4],
                     float ax, float ay,
                     float &minProj, float &maxProj);

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
// Computes the 4 world-space corners of a rotated rectangle (OBB).
// px[4], py[4] will contain the corners in order.
void getRotatedCorners(int x, int y, int w, int h,
                       float angle,
                       float px[4], float py[4])
{
    // Center of the car
    float cx = x + w * 0.5f;
    float cy = y + h * 0.5f;

    // Half extents
    float hw = w * 0.5f;
    float hh = h * 0.5f;

    // Precompute rotation
    float c = cosf(angle);
    float s = sinf(angle);

    // Local unrotated corners relative to center
    float lx[4] = { -hw,  hw,  hw, -hw };
    float ly[4] = { -hh, -hh,  hh,  hh };

    // Rotate each corner and translate to world space
    for (int i = 0; i < 4; ++i) {
        float rx = lx[i] * c - ly[i] * s;
        float ry = lx[i] * s + ly[i] * c;

        px[i] = cx + rx;
        py[i] = cy + ry;
    }
}


bool computeOBBPenetration(const Car& A, const Car& B,
                           float& outNx, float& outNy,
                           float& outDepth)
{
    float ax[4], ay[4];
    float bx[4], by[4];

    getRotatedCorners(A.x, A.y, A.w, A.h, A.angle, ax, ay);
    getRotatedCorners(B.x, B.y, B.w, B.h, B.angle, bx, by);

    // Candidate axes: normals of edges of both boxes
    float axes[4][2];
    axes[0][0] = ax[1] - ax[0]; axes[0][1] = ay[1] - ay[0];
    axes[1][0] = ax[3] - ax[0]; axes[1][1] = ay[3] - ay[0];
    axes[2][0] = bx[1] - bx[0]; axes[2][1] = by[1] - by[0];
    axes[3][0] = bx[3] - bx[0]; axes[3][1] = by[3] - by[0];

    float bestDepth = std::numeric_limits<float>::infinity();
    float bestNx = 0.0f, bestNy = 0.0f;

    for (int i = 0; i < 4; ++i) {
        float ex = axes[i][0];
        float ey = axes[i][1];

        float len = std::sqrt(ex * ex + ey * ey);
        if (len < 1e-6f) continue;

        // Normalized axis
        float nx = ex / len;
        float ny = ey / len;

        float minA, maxA, minB, maxB;
        projectOntoAxis(ax, ay, nx, ny, minA, maxA);
        projectOntoAxis(bx, by, nx, ny, minB, maxB);

        float overlap = std::min(maxA, maxB) - std::max(minA, minB);
        if (overlap <= 0.0f) {
            // Separating axis -> no collision
            return false;
        }

        if (overlap < bestDepth) {
            bestDepth = overlap;
            bestNx = nx;
            bestNy = ny;
        }
    }

    if (!std::isfinite(bestDepth))
        return false;

    // Ensure normal points from B -> A
    float centerAx = 0.0f, centerAy = 0.0f;
    float centerBx = 0.0f, centerBy = 0.0f;
    for (int i = 0; i < 4; ++i) {
        centerAx += ax[i];
        centerAy += ay[i];
        centerBx += bx[i];
        centerBy += by[i];
    }
    centerAx *= 0.25f;
    centerAy *= 0.25f;
    centerBx *= 0.25f;
    centerBy *= 0.25f;

    float cx = centerAx - centerBx;
    float cy = centerAy - centerBy;

    // If MTV points opposite the center delta, flip it
    if (cx * bestNx + cy * bestNy < 0.0f) {
        bestNx = -bestNx;
        bestNy = -bestNy;
    }

    outNx    = bestNx;
    outNy    = bestNy;
    outDepth = bestDepth;
    return true;
}


void applyCollisionPush(Car& A, Car& B,
                        int idxA, int idxB)
{
    float mA = A.mass;
    float mB = B.mass;

    float vA = g_speeds[idxA];
    float vB = g_speeds[idxB];

    float aA = A.angle;
    float aB = B.angle;

    // Velocity vectors
    float vAx = cosf(aA) * vA;
    float vAy = sinf(aA) * vA;
    float vBx = cosf(aB) * vB;
    float vBy = sinf(aB) * vB;

    float Px = mA * vAx + mB * vBx;
    float Py = mA * vAy + mB * vBy;
    float totalMass = mA + mB;

    if (totalMass <= 0.0f)
        totalMass = 1.0f;

    float newVx = Px / totalMass;
    float newVy = Py / totalMass;

    // Blend their speeds toward the combined momentum direction, with damping
    float newSpeedA = (newVx * cosf(aA) + newVy * sinf(aA));
    float newSpeedB = (newVx * cosf(aB) + newVy * sinf(aB));

    g_speeds[idxA] = (g_speeds[idxA] * 0.4f + newSpeedA * 0.6f) * 0.9f;
    g_speeds[idxB] = (g_speeds[idxB] * 0.4f + newSpeedB * 0.6f) * 0.9f;

    // --------------------
    // POSITION SEPARATION (MTV via SAT)
    // --------------------
    float nx, ny, depth;
    if (computeOBBPenetration(A, B, nx, ny, depth)) {
        // Small bias so they don't re-penetrate immediately
        const float EXTRA_GAP = 1.0f;
        float move = depth + EXTRA_GAP;

        float pushA = (mB / totalMass);
        float pushB = (mA / totalMass);

        // NOTE: no 0.5f here – we use the full MTV split by mass.
        float moveA = move * pushA;
        float moveB = move * pushB;

        // Move A along n, B opposite n
        A.x += nx * moveA;
        A.y += ny * moveA;

        B.x -= nx * moveB;
        B.y -= ny * moveB;
        return;
    }

    // ------------- FALLBACK (radial circle approximation) -------------
    float dx = A.x - B.x;
    float dy = A.y - B.y;
    float dist = sqrtf(dx * dx + dy * dy);

    if (dist < 0.001f) {
        dx = 1.0f;
        dy = 0.0f;
        dist = 1.0f;
    }

    float rx = dx / dist;
    float ry = dy / dist;

    float rA = 0.5f * sqrtf(A.w * A.w + A.h * A.h);
    float rB = 0.5f * sqrtf(B.w * B.w + B.h * B.h);

    float desiredDist = rA + rB + 1.0f;
    float penetration = desiredDist - dist;
    if (penetration <= 0.0f)
        return;

    float totalPush = penetration * 0.5f;
    float pushAmountA = totalPush * (mB / totalMass);
    float pushAmountB = totalPush * (mA / totalMass);

    A.x += rx * pushAmountA;
    A.y += ry * pushAmountA;

    B.x -= rx * pushAmountB;
    B.y -= ry * pushAmountB;
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

        // --- Query sprite size so collision box matches what we draw ---
        int spriteW = 32;
        int spriteH = 16;

        {
            Tigr* tmp = get_sprite(c);
            if (tmp) {
                spriteW = tmp->w;
                spriteH = tmp->h;
                tigrFree(tmp);
            }
        }

        // Optional: scale hitbox slightly smaller than sprite for nicer feel
        const float HITBOX_SCALE = 0.8f;   // 80% of sprite size
        c.w = (int)(spriteW * HITBOX_SCALE);
        c.h = (int)(spriteH * HITBOX_SCALE);
        if (c.w < 4) c.w = 4;
        if (c.h < 4) c.h = 4;

        // Now that w/h are correct, place inside playfield
        c.x = rand() % std::max(1, PLAYFIELD_W - c.w);
        c.y = rand() % std::max(1, PLAYFIELD_H - c.h);

        c.angle = 0.0f;
        c.color = tigrRGB(30, 30, 30);

        // -------- Name trimming from filename --------
        std::string full = bin_files[i];
        size_t slash = full.find_last_of("/\\");
        size_t dot   = full.find_last_of('.');
        if (dot == std::string::npos) dot = full.size();
        if (slash == std::string::npos) slash = 0; else slash++;

        c.name = full.substr(slash, dot - slash);

        const char* car_types[] = {
            "_convertable", "_corvette", "_garbagetruck", "_jeep",
            "_motorcycle", "_mustang", "_stationwagon", "_towtruck"
        };
        for (const char* suffix : car_types) {
            size_t pos = c.name.find(suffix);
            if (pos != std::string::npos) {
                c.name = c.name.substr(0, pos);
                break;
            }
        }

        // -------- Mass by type --------
        switch (c.type) {
            case CAR_TYPE_MOTORCYCLE:   c.mass = 0.5f; break;
            case CAR_TYPE_CORVETTE:     c.mass = 1.0f; break;
            case CAR_TYPE_CONVERTABLE:  c.mass = 1.0f; break;
            case CAR_TYPE_JEEP:         c.mass = 1.3f; break;
            case CAR_TYPE_MUSTANG:      c.mass = 1.1f; break;
            case CAR_TYPE_STATIONWAGON: c.mass = 1.6f; break;
            case CAR_TYPE_GARBAGETRUCK: c.mass = 2.2f; break;
            case CAR_TYPE_TOWTRUCK:     c.mass = 3.5f; break;
            default:                    c.mass = 1.0f; break;
        }

        if (i < AGENT_MAX_CNT) {
            g_spinOffset[i] = 0.0f;
            g_spinVel[i]    = 0.0f;
        }

        cars.push_back(c);
    }
}

void applyMassDamageAndElasticity(Car& A, Car& B,
                                  int idxA, int idxB)
{
    float mA = A.mass;
    float mB = B.mass;

    float vA = g_speeds[idxA];
    float vB = g_speeds[idxB];

    // Convert scalar speed into real 2D velocity
    float vAx = cosf(A.angle) * vA;
    float vAy = sinf(A.angle) * vA;
    float vBx = cosf(B.angle) * vB;
    float vBy = sinf(B.angle) * vB;

    // Compute collision normal (pointing from B → A)
    float nx = A.x - B.x;
    float ny = A.y - B.y;
    float dist = sqrtf(nx * nx + ny * ny);
    if (dist < 1.0f) dist = 1.0f;
    nx /= dist;
    ny /= dist;

    // Project velocities onto collision normal
    float vA_norm = vAx * nx + vAy * ny;
    float vB_norm = vBx * nx + vBy * ny;

    float relSpeed = fabsf(vA_norm - vB_norm);

    // -------------------------
    // MASS-BASED DAMAGE MODEL
    // -------------------------
    float dmgA = relSpeed * (mB / mA) * 1.4f;
    float dmgB = relSpeed * (mA / mB) * 1.4f;

    if (g_hitCooldown[idxA] == 0) {
        g_derby_state[idxA].health -= (int)roundf(dmgA);
        if (g_derby_state[idxA].health < 0) g_derby_state[idxA].health = 0;
        g_hitCooldown[idxA] = 10;
    }

    if (g_hitCooldown[idxB] == 0) {
        g_derby_state[idxB].health -= (int)roundf(dmgB);
        if (g_derby_state[idxB].health < 0) g_derby_state[idxB].health = 0;
        g_hitCooldown[idxB] = 10;
    }

    // ------------------------------------
    // 2D ELASTICITY (PHYSICS-CORRECT)
    // ------------------------------------
    const float e = 0.45f;

    float newVA_norm =
        (vA_norm * (mA - e * mB) + (1.0f + e) * mB * vB_norm) / (mA + mB);

    float newVB_norm =
        (vB_norm * (mB - e * mA) + (1.0f + e) * mA * vA_norm) / (mA + mB);

    // Tangential components (unchanged)
    float vA_tan_x = vAx - vA_norm * nx;
    float vA_tan_y = vAy - vA_norm * ny;
    float vB_tan_x = vBx - vB_norm * nx;
    float vB_tan_y = vBy - vB_norm * ny;

    // Reconstruct final 2D velocities
    float newVax = vA_tan_x + newVA_norm * nx;
    float newVay = vA_tan_y + newVA_norm * ny;

    float newVbx = vB_tan_x + newVB_norm * nx;
    float newVby = vB_tan_y + newVB_norm * ny;

    // Convert 2D velocity back to forward scalar speed
    g_speeds[idxA] = newVax * cosf(A.angle) + newVay * sinf(A.angle);
    g_speeds[idxB] = newVbx * cosf(B.angle) + newVby * sinf(B.angle);

    if (fabsf(g_speeds[idxA]) < 0.02f) g_speeds[idxA] = 0.0f;
    if (fabsf(g_speeds[idxB]) < 0.02f) g_speeds[idxB] = 0.0f;

    // ------------------------------------
    // SMOOTH SPIN IMPULSE (NO SNAP)
    // ------------------------------------
    // Relative velocity (for spin direction)
    float rvx = vAx - vBx;
    float rvy = vAy - vBy;

    // Cross product of normal and relative velocity gives spin sign
    float cross = nx * rvy - ny * rvx;  // z-component
    float sign  = (cross >= 0.0f) ? 1.0f : -1.0f;

    float baseImpulse = relSpeed * 0.01f;  // softer spin response
    if (baseImpulse > 0.10f) baseImpulse = 0.10f;  // smaller cap


    float totalMass = mA + mB;
    float impulseA = baseImpulse * (mB / totalMass);
    float impulseB = baseImpulse * (mA / totalMass);

    g_spinVel[idxA] += sign * impulseA;
    g_spinVel[idxB] -= sign * impulseB;

    // Clamp angular velocity to avoid crazy spinning
    auto clamp = [](float v, float lo, float hi) {
        if (v < lo) return lo;
        if (v > hi) return hi;
        return v;
    };

    g_spinVel[idxA] = clamp(g_spinVel[idxA], -0.5f, 0.5f);
    g_spinVel[idxB] = clamp(g_spinVel[idxB], -0.5f, 0.5f);
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

int drawCarSprite(Tigr* win, const Car& car, int idx)
{
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

    float visualAngle = car.angle;

    // Resolve index: prefer explicit idx, fall back to pointer math
    int localIdx = idx;
    if ((localIdx < 0 || localIdx >= AGENT_MAX_CNT) && g_cars && !g_cars->empty()) {
        const Car* base = g_cars->data();
        const Car* ptr  = &car;
        int computedIdx = (int)(ptr - base);
        if (computedIdx >= 0 && computedIdx < AGENT_MAX_CNT) {
            localIdx = computedIdx;
        }
    }

    // --- WOBBLE (spring-damped spin) ---
    if (localIdx >= 0 && localIdx < AGENT_MAX_CNT) {
        const float SPRING   = 0.05f;
        const float DAMP     = 0.70f;
        const float MAX_SLIP = 0.25f;

        g_spinVel[localIdx]   += -SPRING * g_spinOffset[localIdx];
        g_spinVel[localIdx]   *= DAMP;
        g_spinOffset[localIdx] += g_spinVel[localIdx];

        if (fabsf(g_spinOffset[localIdx]) < 0.003f &&
            fabsf(g_spinVel[localIdx])    < 0.003f) {
            g_spinOffset[localIdx] = 0.0f;
            g_spinVel[localIdx]    = 0.0f;
        }

        if (g_spinOffset[localIdx] >  MAX_SLIP) g_spinOffset[localIdx] =  MAX_SLIP;
        if (g_spinOffset[localIdx] < -MAX_SLIP) g_spinOffset[localIdx] = -MAX_SLIP;

        visualAngle = car.angle + g_spinOffset[localIdx] * 0.85f;
    }

    // Center at car’s collision box center
    int dx = (int)(car.x + car.w / 2.0f);
    int dy = (int)(car.y + car.h / 2.0f);

    // --- SCREEN SHAKE (on hit) ---
    if (localIdx >= 0 && localIdx < AGENT_MAX_CNT && g_shakeTimer[localIdx] > 0.0f) {
        float shake = g_shakeTimer[localIdx];
        float mag   = shake * 0.4f;

        float offX = ((rand() % 100) / 100.0f - 0.5f) * mag;
        float offY = ((rand() % 100) / 100.0f - 0.5f) * mag;

        dx += (int)offX;
        dy += (int)offY;
    }

    int sw = carSprite->w;
    int sh = carSprite->h;

    tigrBlitCenteredRotate(
        win,
        carSprite,
        dx, dy,
        0, 0,
        sw, sh,
        visualAngle * 180.0f / 3.14159265f
    );

    tigrFree(carSprite);
    return 0;
}

// Optional: keep a 2-arg version for any old calls
int drawCarSprite(Tigr* win, const Car& car)
{
    return drawCarSprite(win, car, -1);
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

void drawRotatedCar(Tigr* win, const Car& car)
{
    float visualAngle = car.angle;

    if (g_cars && !g_cars->empty()) {
        const Car* base = g_cars->data();
        const Car* ptr  = &car;
        int idx = (int)(ptr - base);

        if (idx >= 0 && idx < AGENT_MAX_CNT) {
            // Just *use* the wobble state; do NOT update it here.
            visualAngle = car.angle + g_spinOffset[idx] * 0.85f;
        }
    }

    float cx = car.x + car.w / 2.0f;
    float cy = car.y + car.h / 2.0f;

    float hw = car.w / 2.0f;
    float hh = car.h / 2.0f;

    float c = cosf(visualAngle);
    float s = sinf(visualAngle);

    float rx[4] = { -hw,  hw,  hw, -hw };
    float ry[4] = { -hh, -hh,  hh,  hh };

    float px[4], py[4];

    for (int i = 0; i < 4; i++) {
        float x = rx[i];
        float y = ry[i];
        float xr = x * c - y * s;
        float yr = x * s + y * c;
        px[i] = cx + xr;
        py[i] = cy + yr;
    }

    tigrFillTriangle(win, px[0], py[0], px[1], py[1], px[2], py[2], car.color);
    tigrFillTriangle(win, px[0], py[0], px[2], py[2], px[3], py[3], car.color);

    tigrLine(win,
        (int)px[1], (int)py[1],
        (int)px[2], (int)py[2],
        tigrRGB(255, 0, 0));

    tigrLine(win,
        (int)px[0], (int)py[0],
        (int)px[3], (int)py[3],
        tigrRGB(0, 0, 255));
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

int getSmokeLevel(int i) {
    if (g_derby_state[i].health <= 0) return 4;   // black, max smoke
    if (g_derby_state[i].health <= 25) return 3;  // dark gray
    if (g_derby_state[i].health <= 50) return 2;  // gray
    if (g_derby_state[i].health <= 75) return 1;  // white
    return 0;                    // no smoke
}

void spawnSmoke(float x, float y, int level) {
    for (int i = 0; i < SMOKE_MAX; i++) {
        if (smokePool[i].alpha <= 0.0f) {

            smokePool[i].x = x + (rand() % 6 - 3);
            smokePool[i].y = y + (rand() % 6 - 3);

            smokePool[i].vy = -0.4f - (rand() % 20) * 0.01f; // rise speed
            smokePool[i].alpha = 1.0f;
            smokePool[i].size = 1.0f + level;

            switch (level) {
                case 1: smokePool[i].color = 230; break;
                case 2: smokePool[i].color = 150; break;
                case 3: smokePool[i].color = 90;  break;
                case 4: smokePool[i].color = 0;   break;
            }

            return;
        }
    }
}

void updateSmoke(Tigr* win) {
    for (int i = 0; i < SMOKE_MAX; i++) {
        if (smokePool[i].alpha > 0.0f) {

            smokePool[i].y += smokePool[i].vy;
            smokePool[i].alpha -= 0.01f;

            int a = (int)(smokePool[i].alpha * 255);
            if (a < 0) a = 0;

            TPixel c;
            c.r = smokePool[i].color;
            c.g = smokePool[i].color;
            c.b = smokePool[i].color;
            c.a = a;

            int sz = (int)smokePool[i].size;
            for (int dy = -sz; dy <= sz; dy++)
                for (int dx = -sz; dx <= sz; dx++)
                    tigrPlot(win, (int)smokePool[i].x + dx, (int)smokePool[i].y + dy, c);
        }
    }
}

void emitSmoke(Tigr* win, const Car& car, int level) {
    if (level == 0) return;

   
    float backX = car.x + car.w / 2 - cosf(car.angle) * car.w / 2;
    float backY = car.y + car.h / 2 - sinf(car.angle) * car.h / 2;

    for (int i = 0; i < level; i++) {
        if ((rand() % 100) < 20) {  
            spawnSmoke(backX, backY, level);
        }
    }
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
    if (g_shakeTimer[idx] > 0)
        g_shakeTimer[idx] -= 0.5f;
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

// Removes sideways sliding after collisions.
// Projects velocity onto the car's forward direction and damps lateral slip.
float applySlipReduction(float speed, float angle, float slipFactor)
{
    // Speed along forward direction (already correct)
    float forward = speed;

    // Compute full velocity
    float vx = cosf(angle) * speed;
    float vy = sinf(angle) * speed;

    // Find sideways axis = forward rotated 90°
    float sx = -sinf(angle);
    float sy =  cosf(angle);

    // Lateral velocity component (the sliding part)
    float lateral = vx * sx + vy * sy;

    // Apply damping to lateral slip
    lateral *= (1.0f - slipFactor);

    // Rebuild a corrected velocity vector
    float newVx = cosf(angle) * forward + sx * lateral;
    float newVy = sinf(angle) * forward + sy * lateral;

    // Convert back to scalar forward speed
    return newVx * cosf(angle) + newVy * sinf(angle);
}


static float smoothAngleToTarget(float current, float target, float factor)
{
    // Normalize difference to [-pi, pi] so we always turn the shortest way.
    float diff = target - current;

    const float PI = 3.14159265f;
    const float TWO_PI = 6.28318530f;

    while (diff > PI)      diff -= TWO_PI;
    while (diff < -PI)     diff += TWO_PI;

    return current + diff * factor;
}

// Smooths car.angle toward the 8-way direction in DerbyState.
float computeSmoothedAngle(int /*idx*/, const DerbyState* state, float currentAngle)
{
    if (!state)
        return currentAngle;

    uint8_t dir = state->direction & 7;
    float target = dir * (3.14159265f / 4.0f);   // same 8-way grid

    const float TURN_SMOOTH = 0.22f;  // 0.1 → slow, 0.3 → snappier
    return smoothAngleToTarget(currentAngle, target, TURN_SMOOTH);
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

    // Apply slip reduction (0.05–0.20 recommended)
    currentSpeed = applySlipReduction(currentSpeed, state->direction * (3.14159265f/4.0f), 0.12f);

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

        g_shakeTimer[i] = 6;
        g_shakeTimer[collidedWith] = 6;

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

        applyMassDamageAndElasticity((*g_cars)[i],
                             (*g_cars)[collidedWith],
                             i,
                             collidedWith);

        applyCollisionPush((*g_cars)[i],
                       (*g_cars)[collidedWith],
                       i,
                       collidedWith);

        return;
    }

    // Wall collision
    float speed = std::fabs(g_speeds[i]);
    int wallDamage = std::max(1, (int)std::round(speed * 0.2f));

    if (g_hitCooldown[i] == 0) {
        g_derby_state[i].health -= wallDamage;
        g_derby_state[i].health = std::max<int>(0, g_derby_state[i].health);
        g_hitCooldown[i] = 10;
        g_shakeTimer[i] = 6;
    }
}

void applyMovementOrClamp(Car& car, DerbyState* state, bool blocked, int nx, int ny, int idx)
{
    if (!blocked) {
        // Free move: just go to the new position.
        car.x = nx;
        car.y = ny;
        return;
    }

    // Check if this "blocked" is actually a wall collision.
    bool hitLeft   = (nx < 0);
    bool hitTop    = (ny < 0);
    bool hitRight  = (nx + car.w > PLAYFIELD_W);
    bool hitBottom = (ny + car.h > PLAYFIELD_H);

    bool hitWall = hitLeft || hitRight || hitTop || hitBottom;

    if (hitWall) {
        // --- SIMPLE WALL RESPONSE (NO ANGLE FIGHTING) ---

        // Clamp back inside the playfield based on *current* position.
        // car.x/car.y are still the pre-collision position here, so we only
        // need to ensure they are valid (just in case).
        if (car.x < 0) car.x = 0;
        if (car.y < 0) car.y = 0;
        if (car.x + car.w > PLAYFIELD_W) car.x = PLAYFIELD_W - car.w;
        if (car.y + car.h > PLAYFIELD_H) car.y = PLAYFIELD_H - car.h;

        // Reflect speed along the existing heading, with damping.
        // No atan2f here → we don't fight the steering direction.
        float& s = g_speeds[idx];

        const float BOUNCE_DAMP = 0.5f;   // how "bouncy" walls feel
        s = -s * BOUNCE_DAMP;            // flip + damp

        if (std::fabs(s) < 0.05f)
            s = 0.0f;

        if (state)
            state->speed = (int16_t)std::round(std::fabs(s));

        // Optionally calm wobble a bit on wall hit, so it doesn't go crazy.
        if (idx >= 0 && idx < AGENT_MAX_CNT) {
            g_spinVel[idx]    *= 0.4f;   // reduce spin velocity
            g_spinOffset[idx] *= 0.7f;   // pull wobble closer to center
        }

        return;
    }

    // If we get here, the block was due to another car, not a wall.
    // Keep your existing "stop and clamp" behavior.
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

    const float SMOOTH = .025f;

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
