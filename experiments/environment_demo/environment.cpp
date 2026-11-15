// Reflections of the Tomb - Zahra: Environment
// Pure C++ (no libraries): Bresenham line, polygon representation, scanline polygon fill.
// Output: environment.ppm  (open with any image viewer / GIMP / IrfanView)

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

// ---------- Pixel plotting (swap with Joe's renderer when integrating) ----------
struct Color { unsigned char r, g, b; };

struct Canvas {
    int w, h;
    std::vector<Color> px;
    Canvas(int W, int H, Color bg) : w(W), h(H), px(W * H, bg) {}
    void setPixel(int x, int y, Color c) {
        if (x < 0 || y < 0 || x >= w || y >= h) return;   // clip to canvas
        px[y * w + x] = c;
    }
    bool savePPM(const std::string& file) const {
        std::ofstream f(file, std::ios::binary);
        if (!f) return false;
        f << "P6\n" << w << " " << h << "\n255\n";
        for (const Color& c : px) f.write(reinterpret_cast<const char*>(&c), 3);
        return true;
    }
};

// ---------- 1. Bresenham's line algorithm (all octants, integer only) ----------
void drawLine(Canvas& cv, int x0, int y0, int x1, int y1, Color c) {
    int dx = std::abs(x1 - x0), dy = std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    while (true) {
        cv.setPixel(x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}

// ---------- 2. Polygon representation ----------
struct Point { int x, y; };
using Polygon = std::vector<Point>;           // ordered vertices, closed implicitly

Polygon makeRect(int x, int y, int w, int h) {
    return { {x, y}, {x + w, y}, {x + w, y + h}, {x, y + h} };
}

void drawPolygonOutline(Canvas& cv, const Polygon& p, Color c) {
    for (size_t i = 0; i < p.size(); ++i) {
        const Point& a = p[i];
        const Point& b = p[(i + 1) % p.size()];
        drawLine(cv, a.x, a.y, b.x, b.y, c);
    }
}

// ---------- 3. Polygon filling: scanline, even-odd rule ----------
void fillPolygon(Canvas& cv, const Polygon& p, Color c) {
    if (p.size() < 3) return;
    int yMin = p[0].y, yMax = p[0].y;
    for (const Point& v : p) { yMin = std::min(yMin, v.y); yMax = std::max(yMax, v.y); }

    for (int y = yMin; y <= yMax; ++y) {
        std::vector<int> xs;                       // x-intersections on this scanline
        for (size_t i = 0; i < p.size(); ++i) {
            Point a = p[i], b = p[(i + 1) % p.size()];
            if (a.y == b.y) continue;              // skip horizontal edges
            if (a.y > b.y) std::swap(a, b);        // make a the upper vertex
            if (y >= a.y && y < b.y) {             // half-open: avoids double-counting vertices
                double x = a.x + double(y - a.y) * (b.x - a.x) / (b.y - a.y);
                xs.push_back(int(x + 0.5));
            }
        }
        std::sort(xs.begin(), xs.end());
        for (size_t i = 0; i + 1 < xs.size(); i += 2)   // fill between pairs
            for (int x = xs[i]; x <= xs[i + 1]; ++x) cv.setPixel(x, y, c);
    }
}

void fillWithBorder(Canvas& cv, const Polygon& p, Color fill, Color border) {
    fillPolygon(cv, p, fill);
    drawPolygonOutline(cv, p, border);
}

// ---------- 4. Temple chamber (top-down view) ----------
// Reusable by the team: other members can query these.
struct Chamber {
    int x, y, w, h;      // outer bounds
    int wall;            // wall thickness
    Polygon floor;       // inner walkable area
    std::vector<Polygon> walls;
    std::vector<Polygon> tiles;
    std::vector<Polygon> pillars;
};

Chamber buildChamber(int x, int y, int w, int h, int wall, int tile) {
    Chamber c{x, y, w, h, wall, {}, {}, {}, {}};
    int ix = x + wall, iy = y + wall, iw = w - 2 * wall, ih = h - 2 * wall;
    c.floor = makeRect(ix, iy, iw, ih);

    // four wall polygons (top, bottom, left, right)
    c.walls.push_back(makeRect(x, y, w, wall));
    c.walls.push_back(makeRect(x, y + h - wall, w, wall));
    c.walls.push_back(makeRect(x, y + wall, wall, h - 2 * wall));
    c.walls.push_back(makeRect(x + w - wall, y + wall, wall, h - 2 * wall));

    // floor tiles
    for (int ty = iy; ty < iy + ih; ty += tile)
        for (int tx = ix; tx < ix + iw; tx += tile)
            c.tiles.push_back(makeRect(tx, ty, std::min(tile, ix + iw - tx),
                                               std::min(tile, iy + ih - ty)));

    // four corner pillars (diamond-shaped, to show non-rectangular polygon fill)
    int pr = tile / 2;
    int cx[2] = {ix + tile * 2, ix + iw - tile * 2};
    int cy[2] = {iy + tile * 2, iy + ih - tile * 2};
    for (int i = 0; i < 2; ++i)
        for (int j = 0; j < 2; ++j)
            c.pillars.push_back({ {cx[i], cy[j] - pr}, {cx[i] + pr, cy[j]},
                                  {cx[i], cy[j] + pr}, {cx[i] - pr, cy[j]} });
    return c;
}

void drawChamber(Canvas& cv, const Chamber& c) {
    const Color floorA{176, 160, 128}, floorB{160, 144, 112}, grout{110, 96, 72};
    const Color wallFill{84, 76, 66}, wallEdge{40, 36, 30};
    const Color pillarFill{120, 110, 96}, pillarEdge{50, 46, 40};

    // 1) filled floor tiles (checker pattern) + Bresenham grout lines
    int tileW = c.tiles.empty() ? 1 : (c.tiles[0][1].x - c.tiles[0][0].x);
    for (size_t i = 0; i < c.tiles.size(); ++i) {
        int col = (c.tiles[i][0].x - c.floor[0].x) / tileW;
        int row = (c.tiles[i][0].y - c.floor[0].y) / tileW;
        fillWithBorder(cv, c.tiles[i], ((col + row) % 2) ? floorA : floorB, grout);
    }
    // 2) walls
    for (const Polygon& w : c.walls) fillWithBorder(cv, w, wallFill, wallEdge);
    // 3) wall brick lines (Bresenham) along the top & bottom walls
    for (int bx = c.x; bx <= c.x + c.w; bx += 40) {
        drawLine(cv, bx, c.y, bx, c.y + c.wall, wallEdge);
        drawLine(cv, bx, c.y + c.h - c.wall, bx, c.y + c.h, wallEdge);
    }
    // 4) pillars
    for (const Polygon& p : c.pillars) fillWithBorder(cv, p, pillarFill, pillarEdge);
    // 5) outer & inner boundary
    Polygon outer = makeRect(c.x, c.y, c.w, c.h);
    drawPolygonOutline(cv, outer, wallEdge);
    drawPolygonOutline(cv, c.floor, wallEdge);
}

int main() {
    Canvas cv(800, 600, Color{20, 18, 16});
    Chamber chamber = buildChamber(50, 50, 700, 500, 30, 40);
    drawChamber(cv, chamber);

    // test: a concave (arrow-shaped) polygon to prove fill handles non-convex shapes
    Polygon arrow = { {380, 280}, {440, 300}, {380, 320}, {395, 300} };
    fillWithBorder(cv, arrow, Color{200, 60, 50}, Color{255, 220, 200});

    if (!cv.savePPM("environment.ppm")) { std::fprintf(stderr, "Could not write file\n"); return 1; }
    std::printf("Wrote environment.ppm (%dx%d)\n", cv.w, cv.h);
    std::printf("Walls: %zu  Tiles: %zu  Pillars: %zu\n",
                chamber.walls.size(), chamber.tiles.size(), chamber.pillars.size());
    return 0;
}
