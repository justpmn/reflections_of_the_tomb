
#include <iostream>
#include <iomanip>

#include "mirror/Mirror.h"

void displayMirror(const Mirror& mirror) {
    glm::vec2 a = mirror.getEndpointA();
    glm::vec2 b = mirror.getEndpointB();
    glm::vec2 normal = mirror.getNormal();
    glm::vec2 center = mirror.getCenter();

    std::cout << "\n--- Mirror Information ---\n";
    std::cout << std::fixed << std::setprecision(2);

    std::cout << "Centre: (" << center.x << ", "
              << center.y << ")\n";

    std::cout << "Angle: " << mirror.getAngle() << " degrees\n";

    std::cout << "Endpoint A: (" << a.x << ", "
              << a.y << ")\n";

    std::cout << "Endpoint B: (" << b.x << ", "
              << b.y << ")\n";

    std::cout << "Normal: (" << normal.x << ", "
              << normal.y << ")\n";
}

int main() {
    Mirror mirror(glm::vec2(0.0f, 0.0f), 4.0f,
                  0.0f, -80.0f, 80.0f);

    char input;

    std::cout << "=== MIRROR TRANSFORMATION DEMO ===\n";
    std::cout << "Q - Rotate left\n";
    std::cout << "E - Rotate right\n";
    std::cout << "R - Reset mirror\n";
    std::cout << "X - Exit\n";

    displayMirror(mirror);

    while (true) {
        std::cout << "\nEnter command: ";
        std::cin >> input;

        if (input == 'q' || input == 'Q') {
            mirror.rotateLeft();
        }
        else if (input == 'e' || input == 'E') {
            mirror.rotateRight();
        }
        else if (input == 'r' || input == 'R') {
            mirror.setAngle(0.0f);
        }
        else if (input == 'x' || input == 'X') {
            break;
        }
        else {
            std::cout << "Invalid command.\n";
            continue;
        }

        displayMirror(mirror);
    }

    std::cout << "\nDemo finished.\n";
    return 0;
}
