#include <cstdlib>
#include "menu.h"

static const char* g_puns[] = {
    "Pipeline your contenders into the arena.",
    "Cache carnage.",
    "Schedule some out-of-order mayhem.",
    "Pick your favorite branch mispredictors.",
    "Who’s ready to thrash the cache?",
    "Load, store, and floor your rivals.",
};

static const int   g_numPuns    = (int)(sizeof(g_puns) / sizeof(g_puns[0]));
static const char* g_currentPun = "";
static bool        g_punChosen  = false;

static int drawBigTitle(Tigr* win,
                        const char* title,
                        int panelX,
                        int panelY,
                        int panelW)
{
    int baseW = tigrTextWidth(tfont, title);
    int baseH = tigrTextHeight(tfont, title);
    int scale = 2;

    int scaledW = baseW * scale;
    int scaledH = baseH * scale;

    int tx = panelX + (panelW - scaledW) / 2;
    int ty = panelY + 18;

    Tigr* tmp = tigrBitmap(baseW, baseH);
    if (!tmp)
        return ty + scaledH;

    int total = baseW * baseH;
    for (int i = 0; i < total; ++i) {
        tmp->pix[i].r = 0;
        tmp->pix[i].g = 0;
        tmp->pix[i].b = 0;
        tmp->pix[i].a = 0;
    }

    tigrPrint(tmp, tfont, 0, 0, tigrRGB(255, 255, 255), "%s", title);

    for (int y = 0; y < baseH; ++y) {
        for (int x = 0; x < baseW; ++x) {
            TPixel p = tmp->pix[y * baseW + x];
            if (p.a == 0)
                continue;

            int dstX0 = tx + x * scale;
            int dstY0 = ty + y * scale;

            for (int oy = 0; oy < scale; ++oy) {
                int dy = dstY0 + oy;
                if (dy < 0 || dy >= win->h) continue;

                for (int ox = 0; ox < scale; ++ox) {
                    int dx = dstX0 + ox;
                    if (dx < 0 || dx >= win->w) continue;

                    win->pix[dy * win->w + dx] = p;
                }
            }
        }
    }

    tigrFree(tmp);
    return ty + scaledH;
}

void initMenu(MenuState& menu, const std::vector<std::string>& binFiles) {
    menu.allBinFiles = binFiles;
    menu.agentNames.clear();
    menu.agentEnabled.clear();
    for (size_t i = 0; i < binFiles.size(); ++i) {
        std::string full = binFiles[i];
        size_t slash = full.find_last_of("/\\");
        size_t dot   = full.find_last_of('.');
        if (dot == std::string::npos) dot = full.size();
        if (slash == std::string::npos) slash = 0; else slash++;
        menu.agentNames.push_back(full.substr(slash, dot - slash));
        menu.agentEnabled.push_back(true);
    }
    menu.cursor = 0;
    g_punChosen = false;
    g_currentPun = "";
}

MenuResult runMenuFrame(Tigr* win, MenuState& menu) {
    MenuResult result;
    result.startGame = false;
    result.quit = false;
    result.selectedBins.clear();

    tigrClear(win, tigrRGB(10, 10, 14));

    size_t count = menu.agentNames.size();

    if (!g_punChosen && g_numPuns > 0) {
        int idx = rand() % g_numPuns;
        g_currentPun = g_puns[idx];
        g_punChosen = true;
    }

    if (count > 0) {
        int maxPerCol = (int)((count + 1) / 2);
        int idx = menu.cursor;
        if (idx < 0) idx = 0;
        if ((size_t)idx >= count) idx = (int)count - 1;

        int col = (idx < maxPerCol) ? 0 : 1;
        int row = (col == 0) ? idx : idx - maxPerCol;

        if (tigrKeyDown(win, TK_UP)) {
            if (row > 0) {
                row--;
            } else {
                int lastRowCol0 = maxPerCol - 1;
                int lastRowCol1 = (int)(count - maxPerCol) - 1;
                if (col == 0) {
                    row = lastRowCol0;
                } else {
                    if (lastRowCol1 >= 0)
                        row = lastRowCol1;
                }
            }
        }

        if (tigrKeyDown(win, TK_DOWN)) {
            int lastRowCol0 = maxPerCol - 1;
            int lastRowCol1 = (int)(count - maxPerCol) - 1;
            if (col == 0) {
                if (row < lastRowCol0)
                    row++;
                else
                    row = 0;
            } else {
                if (row < lastRowCol1)
                    row++;
                else
                    row = 0;
            }
        }

        if (tigrKeyDown(win, TK_LEFT)) {
            if (col == 1) {
                col = 0;
            }
        }

        if (tigrKeyDown(win, TK_RIGHT)) {
            if (col == 0) {
                int rightIdx = maxPerCol + row;
                if ((size_t)rightIdx < count)
                    col = 1;
            }
        }

        if (col == 0) {
            idx = row;
        } else {
            idx = maxPerCol + row;
        }

        if (idx < 0) idx = 0;
        if ((size_t)idx >= count) idx = (int)count - 1;

        menu.cursor = idx;

        if (tigrKeyDown(win, TK_SPACE)) {
            menu.agentEnabled[menu.cursor] = !menu.agentEnabled[menu.cursor];
        }
    }

    const int panelW = 520;
    const int panelH = 420;
    const int panelX = (WIN_W - panelW) / 2;
    const int panelY = (WIN_H - panelH) / 2;

    tigrFillRect(win, panelX, panelY, panelW, panelH, tigrRGB(22, 22, 34));
    tigrRect(win, panelX, panelY, panelW, panelH, tigrRGB(80, 80, 120));

    const char* title = "TeenyDerby";
    int titleBottom = drawBigTitle(win, title, panelX, panelY, panelW);

    const char* subtitle = g_currentPun;
    int subW = tigrTextWidth(tfont, subtitle);
    int subX = panelX + (panelW - subW) / 2;
    int subY = titleBottom + 10;
    tigrPrint(win, tfont, subX, subY, tigrRGB(200, 200, 220), "%s", subtitle);

    int lineH = 20;
    int maxPerCol = (count > 0) ? (int)((count + 1) / 2) : 0;
    int rowsCol0 = maxPerCol;
    int rowsCol1 = (int)count - maxPerCol;
    if (rowsCol1 < 0) rowsCol1 = 0;
    int maxRows = std::max(rowsCol0, rowsCol1);
    int totalListHeight = maxRows * lineH;

    int listAreaTop = subY + 10;
    int listAreaBottom = panelY + panelH - 60;
    int listAreaHeight = listAreaBottom - listAreaTop;
    if (listAreaHeight < totalListHeight)
        listAreaHeight = totalListHeight;

    int listTopY = listAreaTop + (listAreaHeight - totalListHeight) / 2;
    if (listTopY < listAreaTop)
        listTopY = listAreaTop;

    int maxLabelWidth = 0;
    for (size_t i = 0; i < count; ++i) {
        char buf[256];
        snprintf(buf, sizeof(buf), "[X] %s", menu.agentNames[i].c_str());
        int w = tigrTextWidth(tfont, buf);
        if (w > maxLabelWidth)
            maxLabelWidth = w;
    }

    int colWidth = maxLabelWidth + 8;
    int interColSpacing = 40;
    bool twoCols = (count > (size_t)maxPerCol);
    int numCols = twoCols ? 2 : 1;

    int totalColumnsWidth = numCols * colWidth + (numCols - 1) * interColSpacing;
    if (totalColumnsWidth > panelW - 40)
        totalColumnsWidth = panelW - 40;

    int startX = panelX + (panelW - totalColumnsWidth) / 2;
    int col0X = startX;
    int col1X = col0X + colWidth + interColSpacing;

    for (size_t i = 0; i < count; ++i) {
        int col = (i < (size_t)maxPerCol) ? 0 : 1;
        int row = (col == 0) ? (int)i : (int)i - maxPerCol;

        int y = listTopY + row * lineH;
        if (y > panelY + panelH - 80)
            continue;

        int x = (col == 0) ? col0X : col1X;

        TPixel colPix = (i == (size_t)menu.cursor)
                        ? tigrRGB(255, 255, 0)
                        : tigrRGB(220, 220, 220);
        char mark = menu.agentEnabled[i] ? 'X' : ' ';
        tigrPrint(win, tfont, x, y, colPix,
                  "[%c] %s", mark, menu.agentNames[i].c_str());
    }

    const char* help = "UP/DOWN: row   LEFT/RIGHT: column   SPACE: toggle   G: play   ESC: quit";
    int helpW = tigrTextWidth(tfont, help);
    int helpX = panelX + (panelW - helpW) / 2;
    int helpY = panelY + panelH - 30;
    tigrPrint(win, tfont, helpX, helpY, tigrRGB(180, 180, 200), "%s", help);

    if (tigrKeyDown(win, 'g') || tigrKeyDown(win, 'G')) {
        for (size_t i = 0; i < menu.allBinFiles.size(); ++i) {
            if (i < menu.agentEnabled.size() && menu.agentEnabled[i])
                result.selectedBins.push_back(menu.allBinFiles[i]);
        }
        if (!result.selectedBins.empty())
            result.startGame = true;
    }

    if (tigrKeyDown(win, TK_ESCAPE)) {
        result.quit = true;
    }

    return result;
}
