#include "tigr.h"

int main() {
    Tigr* img = tigrLoadImage("./car.png");

    Tigr* win = tigrWindow(800, 600, "PNG", 0);

    while (!tigrClosed(win)) {
        tigrBlit(win, img, 200, 200, 200, 200, img->w, img->h);
        tigrUpdate(win);
    }

    tigrFree(img);
    tigrFree(win);
    return 0;
}
