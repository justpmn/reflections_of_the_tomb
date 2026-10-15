#include "Level.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace world {

// ---------- Bresenham's line algorithm (all octants, integer only) ----------
void drawLine(const PlotFn& plot, Point a, Point b, Color c) {
    int dx = std::abs(b.x - a.x), dy = std::abs(b.y - a.y);
    int sx = (a.x < b.x) ? 1 : -1;
    int sy = (a.y < b.y) ? 1 : -1;
    int err = dx - dy;
    for (;;) {
        plot(a.x, a.y, c);
        if (a.x == b.x && a.y == b.y) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; a.x += sx; }
        if (e2 <  dx) { err += dx; a.y += sy; }
    }
}

void drawPolygonOutline(const PlotFn& plot, const Polygon& p, Color c) {
    for (std::size_t i = 0; i < p.size(); ++i)
        drawLine(plot, p[i], p[(i + 1) % p.size()], c);
}

// ---------- Scanline polygon fill (even-odd rule) ----------
void fillPolygon(const PlotFn& plot, const Polygon& p, Color c) {
    if (p.size() < 3) return;
    int yMin = p[0].y, yMax = p[0].y;
    for (const Point& v : p) { yMin = std::min(yMin, v.y); yMax = std::max(yMax, v.y); }

    for (int y = yMin; y <= yMax; ++y) {
        std::vector<int> xs;
        for (std::size_t i = 0; i < p.size(); ++i) {
            Point a = p[i], b = p[(i + 1) % p.size()];
            if (a.y == b.y) continue;                 // skip horizontal edges
            if (a.y > b.y) std::swap(a, b);
            if (y >= a.y && y < b.y) {                // half-open: no double-counted vertices
                double x = a.x + double(y - a.y) * (b.x - a.x) / (b.y - a.y);
                xs.push_back(int(std::lround(x)));
            }
        }
        std::sort(xs.begin(), xs.end());
        for (std::size_t i = 0; i + 1 < xs.size(); i += 2)
            for (int x = xs[i]; x <= xs[i + 1]; ++x) plot(x, y, c);
    }
}

Polygon makeRect(int x, int y, int w, int h) {
    return { {x, y}, {x + w, y}, {x + w, y + h}, {x, y + h} };
}

// ---------- Level ----------
Level::Level(int x, int y, int w, int h, int wallThickness, int tileSize)
    : x_(x), y_(y), w_(w), h_(h), wall_(wallThickness), tile_(tileSize) {
    int ix = x + wall_, iy = y + wall_, iw = w - 2 * wall_, ih = h - 2 * wall_;
    floor_ = makeRect(ix, iy, iw, ih);

    walls_.push_back(makeRect(x, y, w, wall_));                       // top
    walls_.push_back(makeRect(x, y + h - wall_, w, wall_));           // bottom
    walls_.push_back(makeRect(x, y + wall_, wall_, h - 2 * wall_));   // left
    walls_.push_back(makeRect(x + w - wall_, y + wall_, wall_, h - 2 * wall_)); // right

    for (int ty = iy; ty < iy + ih; ty += tile_)
        for (int tx = ix; tx < ix + iw; tx += tile_)
            tiles_.push_back(makeRect(tx, ty, std::min(tile_, ix + iw - tx),
                                              std::min(tile_, iy + ih - ty)));

    int pr = tile_ / 2;
    int cx[2] = { ix + tile_ * 2, ix + iw - tile_ * 2 };
    int cy[2] = { iy + tile_ * 2, iy + ih - tile_ * 2 };
    for (int i = 0; i < 2; ++i)
        for (int j = 0; j < 2; ++j)
            pillars_.push_back({ {cx[i], cy[j] - pr}, {cx[i] + pr, cy[j]},
                                 {cx[i], cy[j] + pr}, {cx[i] - pr, cy[j]} });
}

void Level::draw(const PlotFn& plot) const {
    const Color floorA{176, 160, 128}, floorB{160, 144, 112}, grout{110, 96, 72};
    const Color wallFill{84, 76, 66}, wallEdge{40, 36, 30};
    const Color pillarFill{120, 110, 96}, pillarEdge{50, 46, 40};

    // floor tiles (checker pattern) with Bresenham grout lines
    for (const Polygon& t : tiles_) {
        int col = (t[0].x - floor_[0].x) / tile_;
        int row = (t[0].y - floor_[0].y) / tile_;
        fillPolygon(plot, t, ((col + row) % 2) ? floorA : floorB);
        drawPolygonOutline(plot, t, grout);
    }
    // walls
    for (const Polygon& w : walls_) {
        fillPolygon(plot, w, wallFill);
        drawPolygonOutline(plot, w, wallEdge);
    }
    // brick lines on top and bottom walls
    for (int bx = x_; bx <= x_ + w_; bx += 40) {
        drawLine(plot, {bx, y_}, {bx, y_ + wall_}, wallEdge);
        drawLine(plot, {bx, y_ + h_ - wall_}, {bx, y_ + h_}, wallEdge);
    }
    // pillars
    for (const Polygon& p : pillars_) {
        fillPolygon(plot, p, pillarFill);
        drawPolygonOutline(plot, p, pillarEdge);
    }
    drawPolygonOutline(plot, makeRect(x_, y_, w_, h_), wallEdge);
    drawPolygonOutline(plot, floor_, wallEdge);
}

bool Level::isWalkable(int px, int py) const {
    return px >= floor_[0].x && px < floor_[2].x && py >= floor_[0].y && py < floor_[2].y;
}

std::vector<Segment> Level::wallSegments() const {
    std::vector<Segment> s;
    for (std::size_t i = 0; i < floor_.size(); ++i)
        s.push_back({ floor_[i], floor_[(i + 1) % floor_.size()] });
    return s;
}

} // namespace world

// TODO: Implement Level
