#include "player/Player.h"
#include "player/PlayerController.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

struct Colour {
    unsigned char r;
    unsigned char g;
    unsigned char b;
};

const Colour GOLD{246, 193, 86};
const Colour BLUE{111, 199, 239};
const Colour GREEN{133, 220, 165};

class Canvas {
public:
    static constexpr int width = 640;
    static constexpr int height = 400;

    Canvas()
        : pixels(width * height, Colour{22, 28, 36}) {

        // Draw a reference grid to make movement visible.
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                if (x % 40 == 0 || y % 40 == 0) {
                    setPixel(x, y, {39, 47, 57});
                }
            }
        }
    }

    void setPixel(int x, int y, Colour colour) {
        if (
            x >= 0 && x < width &&
            y >= 0 && y < height
        ) {
            pixels[y * width + x] = colour;
        }
    }

    void draw(const Player& player, Colour colour) {
        // Player calculates the circle pixels.
        // Canvas stores each calculated pixel.
        player.draw(
            [&](int x, int y) {
                setPixel(x, y, colour);
            }
        );
    }

    void saveBMP(const std::string& filename) const {
        std::ofstream out(filename, std::ios::binary);

        if (!out) {
            throw std::runtime_error(
                "Cannot create " + filename
            );
        }

        // Each BMP row must occupy a multiple of 4 bytes.
        const std::uint32_t stride =
            (width * 3 + 3) & ~3u;

        // Write an integer in little-endian byte order.
        auto writeLE = [&](std::uint32_t value, int bytes) {
            for (int i = 0; i < bytes; ++i) {
                out.put(
                    static_cast<char>(
                        (value >> (8 * i)) & 255
                    )
                );
            }
        };

        // BMP file header.
        out.put('B');
        out.put('M');

        writeLE(54 + stride * height, 4);
        writeLE(0, 4);
        writeLE(54, 4);

        // BMP information header.
        writeLE(40, 4);
        writeLE(width, 4);
        writeLE(height, 4);
        writeLE(1, 2);
        writeLE(24, 2);
        writeLE(0, 4);
        writeLE(stride * height, 4);
        writeLE(2835, 4);
        writeLE(2835, 4);
        writeLE(0, 4);
        writeLE(0, 4);

        // BMP stores these rows from bottom to top.
        for (int y = height - 1; y >= 0; --y) {
            for (int x = 0; x < width; ++x) {
                const Colour colour =
                    pixels[y * width + x];

                // BMP pixel order: blue, green, red.
                out.put(static_cast<char>(colour.b));
                out.put(static_cast<char>(colour.g));
                out.put(static_cast<char>(colour.r));
            }

            // Add row padding where necessary.
            for (
                std::uint32_t pad = width * 3;
                pad < stride;
                ++pad
            ) {
                out.put(0);
            }
        }

        if (!out) {
            throw std::runtime_error(
                "Image write failed: " + filename
            );
        }
    }

private:
    std::vector<Colour> pixels;
};

void saveFrame(
    const Player& player,
    const std::string& filename,
    Colour colour = GOLD
) {
    // A fresh canvas removes the previous position.
    Canvas canvas;

    canvas.draw(player, colour);
    canvas.saveBMP(filename);

    std::cout
        << filename
        << " : centre ("
        << player.getX()
        << ", "
        << player.getY()
        << ")\n";
}

int main(int argc, char* argv[]) {
    try {
        Player player;

        PlayerController controller(
            player,
            Canvas::width,
            Canvas::height
        );

        Canvas comparison;

        // Evidence 1: starting position.
        saveFrame(player, "01_start.bmp");
        comparison.draw(player, GOLD);

        // Evidence 2: move 160 pixels to the right.
        for (int i = 0; i < 8; ++i) {
            controller.handleInput('d');
        }

        saveFrame(player, "02_right.bmp", BLUE);
        comparison.draw(player, BLUE);

        // Evidence 3: move 120 pixels down.
        for (int i = 0; i < 6; ++i) {
            controller.handleInput('s');
        }

        saveFrame(player, "03_down.bmp", GREEN);
        comparison.draw(player, GREEN);

        // Show successive positions of ONE player.
        comparison.saveBMP("04_comparison.bmp");

        controller.handleInput('r');

        // Optional mode: generate evidence and exit.
        if (
            argc > 1 &&
            std::string(argv[1]) == "--evidence"
        ) {
            return 0;
        }

        std::cout
            << "\nW/A/S/D then Enter: move\n"
            << "R then Enter: reset\n"
            << "Q then Enter: quit\n"
            << "Open each saved BMP to see the result.\n"
            << "This demo does not open a live window.\n";

        int frame = 0;
        saveFrame(player, "frame_0.bmp");

        char key;

        while (std::cout << "> " && std::cin >> key) {
            if (key == 'q' || key == 'Q') {
                break;
            }

            if (controller.handleInput(key)) {
                ++frame;

                const std::string filename =
                    "frame_" +
                    std::to_string(frame) +
                    ".bmp";

                saveFrame(player, filename);
            } else {
                std::cout
                    << "Unknown command or movement "
                    << "blocked by canvas edge.\n";
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}