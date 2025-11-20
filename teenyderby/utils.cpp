#include <stdio.h>
#include <dirent.h>
#include <string>

#include "utils.h"
#include "bus.h"
#include "teenyat.h"
#include <cstdlib>

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

    for (size_t i = 0; i < bin_files.size(); ++i) {
        FILE* f = fopen(bin_files[i].c_str(), "rb");
        if (!f) continue;
        tny_init_from_file(&agents[i], f, derby_bus_read, derby_bus_write);
        if (i < 8)
            agents[i].ex_data = &derby_state[i];
        else
            agents[i].ex_data = nullptr;

        fclose(f);
    }
}

void randomize_cars(std::vector<Car> &cars, std::vector<teenyat> &agents) {
    TPixel color;
    
    for (size_t i = 0; i < agents.size(); ++i) {
            int x, y;
            bool ok;
            int attempts = 0;

            do {
                x = MARGIN + (std::rand() % (WIN_W - CAR_W - 2 * MARGIN + 1));
                y = MARGIN + (std::rand() % (WIN_H - CAR_H - 2 * MARGIN + 1));
                ok = true;
                for (const auto &c : cars) {
                    if (abs(c.x - x) < CAR_W && abs(c.y - y) < CAR_H) {
                        ok = false;
                        break;
                    }
                }
                attempts++;

                color = tigrRGB(std::rand()%256, std::rand()%256, std::rand()%256);
                
            } while (!ok && attempts < 20);

            cars.push_back({x, y, CAR_W, CAR_H, 0.0f, color});
        }
}
