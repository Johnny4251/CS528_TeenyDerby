#include <stdio.h>
#include <dirent.h>
#include <string>

#include "utils.h"
#include "bus.h"
#include "teenyat.h"
#include <cstdlib>


DerbyState*       g_derby_state       = nullptr;
size_t            g_derby_state_count = 0;
std::vector<Car>* g_cars              = nullptr;


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


void randomize_cars(std::vector<Car> &cars, std::vector<teenyat> &agents) {
    g_cars = &cars;
    cars.clear();
    for (size_t i = 0; i < agents.size(); ++i) {
        Car c;
        c.w = 32;
        c.h = 16;
        c.x = rand() % (WIN_W - c.w);
        c.y = rand() % (WIN_H - c.h);
        c.angle = 0;
        c.color = tigrRGB(rand()%255, rand()%255, rand()%255);
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
        if (px[i] < 0 || px[i] > WIN_W ||
            py[i] < 0 || py[i] > WIN_H)
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
