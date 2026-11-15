
#include <iostream>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "rendering/Renderer.h"

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

int main()
{
    // Initialize GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    // OpenGL 3.3 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    GLFWwindow* window = glfwCreateWindow(
        1280, 720,
        "Reflections of the Tomb - Rendering",
        nullptr, nullptr
    );

    if (!window)
    {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Initialize GLAD
    if (!gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress)))
    {
        std::cerr << "Failed to initialize GLAD\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    std::cout << "OpenGL initialized successfully!\n";
    std::cout << "Version: " << glGetString(GL_VERSION) << '\n';

    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    // Initialize our rendering subsystem
    Renderer renderer;

    if (!renderer.initialize())
    {
        std::cerr << "Failed to initialize Renderer\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // Main rendering loop
    while (!glfwWindowShouldClose(window))
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
        }

        // Clear the screen
        renderer.beginFrame();

        // Draw a gold rectangle to test the renderer
        renderer.drawRectangle(
            -0.35f, -0.35f,
             0.70f,  0.70f,
             0.90f,  0.65f,  0.15f
        );

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Release rendering resources while the OpenGL context still exists
    renderer.shutdown();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
