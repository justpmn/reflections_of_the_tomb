#pragma once
#include <functional>
#include <vector>

namespace world {

struct Point { int x, y; };
struct Color { unsigned char r, g, b; };
struct Segment { Point a, b; };
using Polygon = std::vector<Point>;

// The renderer supplies this (e.g. a lambda that calls Joe's pixel plotting).
using PlotFn = std::function<void(int x, int y, Color c)>;

// Graphics algorithms (implemented from scratch)
void drawLine(const PlotFn& plot, Point a, Point b, Color c);          // Bresenham
void drawPolygonOutline(const PlotFn& plot, const Polygon& p, Color c);
void fillPolygon(const PlotFn& plot, const Polygon& p, Color c);       // scanline, even-odd
Polygon makeRect(int x, int y, int w, int h);

class Level {
public:
    Level(int x, int y, int w, int h, int wallThickness, int tileSize);

    void draw(const PlotFn& plot) const;

    // Queries for the rest of the team
    bool isWalkable(int px, int py) const;                // inside the inner floor?
    std::vector<Segment> wallSegments() const;            // inner wall edges (beam reflection)
    const Polygon& floor() const { return floor_; }
    const std::vector<Polygon>& walls() const { return walls_; }
    const std::vector<Polygon>& pillars() const { return pillars_; }

private:
    int x_, y_, w_, h_, wall_, tile_;
    Polygon floor_;
    std::vector<Polygon> walls_, tiles_, pillars_;
};

} // namespace world

