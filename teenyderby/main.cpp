#include <stdio.h>
#include "../teenyat.h"
#include "tigr.h"
#include <vector>
#include <string>
#include <dirent.h>
#include <cstdlib>
#include <ctime>
#include <cstdio>
#define DERBY_MOVE_UP_ADDR    0x9000
#define DERBY_MOVE_DOWN_ADDR  0x9001
#define DERBY_MOVE_LEFT_ADDR  0x9002
#define DERBY_MOVE_RIGHT_ADDR 0x9003

#define DERBY_AGENT_BASE_ADDR 0xA000
#define DERBY_AGENT_ADDR_STRIDE 0x10
#define DERBY_AGENT_MOVE_ADDR 0xA001
#define CAR_HORIZONTAL_MOVE_RATE 4
#define CAR_VERTICAL_MOVE_RATE 4

void derby_bus_read(teenyat *t, tny_uword addr, tny_word *data, uint16_t *delay);
void derby_bus_write(teenyat *t, tny_uword addr, tny_word data, uint16_t *delay);

typedef struct {
	int move_cmd; // 0=none, 1=up, 2=down, 3=left, 4=right
    int id;
} DerbyState;

struct Car {
    int x, y;
    int w, h;
    TPixel color;
};

int main() {
    Tigr *win = tigrWindow(800, 600, "TeenyDerby", TIGR_FIXED);

    std::vector<std::string> bin_files;
    DIR *dir = opendir("asm");
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != nullptr) {
            std::string fname = ent->d_name;
            if (fname.size() > 4 && fname.substr(fname.size()-4) == ".bin") {
                bin_files.push_back(std::string("asm/") + fname);
                if (bin_files.size() >= 8) break;
            }
        }
        closedir(dir);
    }
    std::srand((unsigned)std::time(nullptr)); 

    DerbyState derby_state[8] = {};

    std::vector<teenyat> agents(bin_files.size());
    for (size_t i = 0; i < bin_files.size(); ++i) {
        FILE *f = fopen(bin_files[i].c_str(), "rb");
        if (f) {
            tny_init_from_file(&agents[i], f, derby_bus_read, derby_bus_write);
            if (i < 8) agents[i].ex_data = &derby_state[i];
            else agents[i].ex_data = nullptr;
            fclose(f);
        }
    }
    std::vector<Car> cars;

    const int WIN_W = 800;
    const int WIN_H = 600;
    const int MARGIN = 100;
    const int W = 40;
    const int H = 20;
    TPixel color;

    // random starting locations
    for (size_t i = 0; i < agents.size(); ++i) {
        int x, y;
        bool ok;
        int attempts = 0;

        do {
            x = MARGIN + (std::rand() % (WIN_W - W - 2 * MARGIN + 1));
            y = MARGIN + (std::rand() % (WIN_H - H - 2 * MARGIN + 1));
            ok = true;
            for (const auto &c : cars) {
                if (abs(c.x - x) < W && abs(c.y - y) < H) {
                    ok = false;
                    break;
                }
            }
            attempts++;

            color = tigrRGB(std::rand()%256, std::rand()%256, std::rand()%256);
            
        } while (!ok && attempts < 20);

        cars.push_back({x, y, W, H, color});
    }

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
                    case 1:  ny -= CAR_VERTICAL_MOVE_RATE; break; /* up */
                    case 2:  ny += CAR_VERTICAL_MOVE_RATE; break; /* down */
                    case 3:  nx -= CAR_HORIZONTAL_MOVE_RATE; break; /* left */
                    case 4:  nx += CAR_HORIZONTAL_MOVE_RATE; break; /* right */
                    default: break;
                }

                    bool in_bounds = !(nx < 0 || ny < 0 || nx + cars[i].w > WIN_W || ny + cars[i].h > WIN_H);

                    bool collides = false;
                    if (in_bounds) {
                        for (size_t j = 0; j < cars.size(); ++j) {
                            if (j == i) continue;
                            if (!(nx + cars[i].w <= cars[j].x || nx >= cars[j].x + cars[j].w ||
                                  ny + cars[i].h <= cars[j].y || ny >= cars[j].y + cars[j].h)) {
                                collides = true;
                                break;
                            }
                        }
                    }

                    if (in_bounds && !collides) {
                        cars[i].x = nx;
                        cars[i].y = ny;
                    }

                tigrRect(win, cars[i].x, cars[i].y, cars[i].w, cars[i].h, cars[i].color);
            }
        }

        tigrUpdate(win);
    }
    tigrFree(win);
    return 0;
}

void derby_bus_read(teenyat *t, tny_uword addr, tny_word *data, uint16_t *delay) {
    return;
}

void derby_bus_write(teenyat *t, tny_uword addr, tny_word data, uint16_t *delay) {
	DerbyState *state = (DerbyState *)t->ex_data;
    switch(addr) {
        case DERBY_MOVE_UP_ADDR:
            state->move_cmd = 1;
            break;
        case DERBY_MOVE_DOWN_ADDR:
            state->move_cmd = 2;
            break;
        case DERBY_MOVE_LEFT_ADDR:
            state->move_cmd = 3;
            break;
        case DERBY_MOVE_RIGHT_ADDR:
            state->move_cmd = 4;
            break;
        default: 
            state->move_cmd = 0;
            break;
    }

    return;
}