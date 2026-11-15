
#include "Shader.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {
    std::string readShaderFile(const std::string& path) {
        std::ifstream file(path);

        if (!file.is_open()) {
            std::cerr << "Error: Could not open shader file: "
                      << path << '\n';
            return "";
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    unsigned int compileShader(
        GLenum shaderType,
        const std::string& source,
        const std::string& path
    ) {
        unsigned int shader = glCreateShader(shaderType);

        const char* sourceCode = source.c_str();
        glShaderSource(shader, 1, &sourceCode, nullptr);
        glCompileShader(shader);

        int success = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

        if (!success) {
            char log[1024] = {};
            glGetShaderInfoLog(shader, sizeof(log), nullptr, log);

            std::cerr << "Shader compilation error in "
                      << path << ":\n" << log << '\n';

            glDeleteShader(shader);
            return 0;
        }

        return shader;
    }
}

Shader::~Shader() {
    destroy();
}

bool Shader::loadFromFiles(
    const std::string& vertexPath,
    const std::string& fragmentPath
) {
    std::string vertexSource = readShaderFile(vertexPath);
    std::string fragmentSource = readShaderFile(fragmentPath);

    if (vertexSource.empty() || fragmentSource.empty()) {
        return false;
    }

    unsigned int vertexShader =
        compileShader(GL_VERTEX_SHADER, vertexSource, vertexPath);

    if (vertexShader == 0) {
        return false;
    }

    unsigned int fragmentShader =
        compileShader(GL_FRAGMENT_SHADER, fragmentSource, fragmentPath);

    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return false;
    }

    unsigned int program = glCreateProgram();

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    int success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success) {
        char log[1024] = {};
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);

        std::cerr << "Shader linking error:\n"
                  << log << '\n';

        glDeleteProgram(program);
        program = 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    if (program == 0) {
        return false;
    }

    destroy();
    ID = program;

    std::cout << "Shader program loaded successfully.\n";
    return true;
}

void Shader::use() const {
    if (ID != 0) {
        glUseProgram(ID);
    }
}

void Shader::destroy() {
    if (ID != 0) {
        glDeleteProgram(ID);
        ID = 0;
    }
}
