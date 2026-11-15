
#pragma once

#include <string>
#include <glad/gl.h>

class Shader
{
public:
    unsigned int ID = 0;

    Shader() = default;
    ~Shader();

    // Load, compile and link vertex and fragment shaders.
    bool loadFromFiles(
        const std::string& vertexPath,
        const std::string& fragmentPath
    );

    // Activate this shader program.
    void use() const;

    // Release the OpenGL shader program.
    void destroy();
};
