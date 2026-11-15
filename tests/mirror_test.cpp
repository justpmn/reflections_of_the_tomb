
#include <iostream>
#include <cmath>
#include <stdexcept>

#include "mirror/Mirror.h"
#include "mirror/MirrorSystem.h"

const float EPSILON = 0.001f;

bool approximatelyEqual(float a, float b) {
    return std::abs(a - b) < EPSILON;
}

void check(bool condition, const char* testName) {
    if (!condition) {
        throw std::runtime_error(testName);
    }

    std::cout << "[PASS] " << testName << '\n';
}

void testInitialProperties() {
    Mirror mirror(glm::vec2(0.0f, 0.0f), 4.0f);

    check(approximatelyEqual(mirror.getAngle(), 0.0f),
          "Initial mirror angle");

    check(approximatelyEqual(mirror.getCenter().x, 0.0f) &&
          approximatelyEqual(mirror.getCenter().y, 0.0f),
          "Mirror centre position");
}

void testRotation() {
    Mirror mirror(glm::vec2(0.0f, 0.0f), 4.0f);

    mirror.rotateRight(10.0f);
    check(approximatelyEqual(mirror.getAngle(), 10.0f),
          "Rotate right");

    mirror.rotateLeft(5.0f);
    check(approximatelyEqual(mirror.getAngle(), 5.0f),
          "Rotate left");
}

void testRotationLimits() {
    Mirror mirror(glm::vec2(0.0f, 0.0f), 4.0f,
                  0.0f, -30.0f, 30.0f);

    mirror.setAngle(50.0f);
    check(approximatelyEqual(mirror.getAngle(), 30.0f),
          "Maximum rotation limit");

    mirror.setAngle(-50.0f);
    check(approximatelyEqual(mirror.getAngle(), -30.0f),
          "Minimum rotation limit");
}

void testEndpoints() {
    Mirror mirror(glm::vec2(0.0f, 0.0f), 4.0f);

    glm::vec2 a = mirror.getEndpointA();
    glm::vec2 b = mirror.getEndpointB();

    check(approximatelyEqual(a.x, -2.0f) &&
          approximatelyEqual(a.y, 0.0f),
          "First endpoint at zero degrees");

    check(approximatelyEqual(b.x, 2.0f) &&
          approximatelyEqual(b.y, 0.0f),
          "Second endpoint at zero degrees");
}

void testSurfaceNormal() {
    Mirror mirror(glm::vec2(0.0f, 0.0f), 4.0f);

    glm::vec2 normal = mirror.getNormal();

    float length = std::sqrt(
        normal.x * normal.x + normal.y * normal.y
    );

    check(approximatelyEqual(length, 1.0f),
          "Surface normal has unit length");

    glm::vec2 a = mirror.getEndpointA();
    glm::vec2 b = mirror.getEndpointB();

    glm::vec2 direction = b - a;

    float dotProduct =
        direction.x * normal.x +
        direction.y * normal.y;

    check(approximatelyEqual(dotProduct, 0.0f),
          "Surface normal is perpendicular to mirror");
}

void testInvalidMirror() {
    bool exceptionThrown = false;

    try {
        Mirror mirror(glm::vec2(0.0f, 0.0f), 0.0f);
    }
    catch (const std::invalid_argument&) {
        exceptionThrown = true;
    }

    check(exceptionThrown, "Reject invalid mirror length");
}

void testMirrorSystem() {
    MirrorSystem system;

    system.addMirror(Mirror(glm::vec2(0.0f, 0.0f), 4.0f));
    system.addMirror(Mirror(glm::vec2(2.0f, 2.0f), 3.0f));

    check(system.getMirrorCount() == 2,
          "Add multiple mirrors");

    system.selectMirror(1);
    check(system.getSelectedMirrorIndex() == 1,
          "Select mirror");

    system.rotateSelectedRight(10.0f);

    check(approximatelyEqual(
        system.getMirror(1).getAngle(), 10.0f),
        "Rotate selected mirror");
}

int main() {
    try {
        testInitialProperties();
        testRotation();
        testRotationLimits();
        testEndpoints();
        testSurfaceNormal();
        testInvalidMirror();
        testMirrorSystem();

        std::cout << "\nAll mirror tests passed!\n";
        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "\nTEST FAILED: " << error.what() << '\n';
        return 1;
    }
}
