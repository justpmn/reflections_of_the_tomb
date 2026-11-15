#ifndef TEMPLE_PLAYER_H
#define TEMPLE_PLAYER_H

#include <functional>

class Player {
public:
    // A function that places a pixel at coordinates (x, y).
    using PlotPixel = std::function<void(int, int)>;

    Player(int x = 120, int y = 180, int radius = 28);

    void draw(const PlotPixel& plot) const;
    void translate(int dx, int dy);
    void reset();

    int getX() const { return x_; }
    int getY() const { return y_; }
    int getRadius() const { return radius_; }

private:
    int x_;
    int y_;
    int radius_;

    int startX_;
    int startY_;

    static void midpointCircle(
        int cx,
        int cy,
        int radius,
        const PlotPixel& plot
    );
};

#endif
