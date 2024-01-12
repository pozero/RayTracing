#include "shader.h"

#include <iostream>
#include <fstream>
#include <array>
#include <iterator>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "glad/glad.h"
#pragma clang diagnostic pop

Shader::Shader(std::string const& path, unsigned int type)
    : m_id(glCreateShader(type)) {
    std::ifstream fstream{path.data()};
    if (!fstream.is_open()) {
        std::cerr << "Can't read shader code " << path << "\n";
        return;
    }
    std::string source{std::istreambuf_iterator<char>{fstream}, {}};
    const char* source_cstr = source.c_str();
    glShaderSource(m_id, 1, &source_cstr, nullptr);
    glCompileShader(m_id);
    int success = 0;
    size_t constexpr BUF_SIZE = 1024;
    std::array<char, BUF_SIZE> buf{};
    glGetShaderiv(m_id, GL_COMPILE_STATUS, &success);
    if (success == 0) {
        glGetShaderInfoLog(m_id, buf.size(), nullptr, buf.data());
        std::cerr << "Compilation error: " << buf.data() << '\n';
    }
}

Shader::~Shader() {
    glDeleteShader(m_id);
}

Program::Program(Shader const& vert, Shader const& frag)
    : m_id(glCreateProgram()) {
    glAttachShader(m_id, vert.m_id);
    glAttachShader(m_id, frag.m_id);
    glLinkProgram(m_id);
    check_link_error();
}

Program::Program(std::string const& path) : m_id(glCreateProgram()) {
    Shader shader{path, GL_COMPUTE_SHADER};
    glAttachShader(m_id, shader.m_id);
    glLinkProgram(m_id);
    check_link_error();
}

Program::~Program() {
    glDeleteProgram(m_id);
}

void Program::use() const {
    glUseProgram(m_id);
}

void Program::setBool(const std::string& name, bool value) const {
    glUniform1i(glGetUniformLocation(m_id, name.c_str()), (int) value);
}

void Program::setInt(const std::string& name, int value) const {
    glUniform1i(glGetUniformLocation(m_id, name.c_str()), value);
}

void Program::setFloat(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(m_id, name.c_str()), value);
}

void Program::check_link_error() const {
    int success = 0;
    size_t constexpr BUF_SIZE = 1024;
    std::array<char, BUF_SIZE> buf{};
    glGetProgramiv(m_id, GL_LINK_STATUS, &success);
    if (success == 0) {
        glGetProgramInfoLog(m_id, buf.size(), nullptr, buf.data());
        std::cerr << "Link error: " << buf.data() << '\n';
    }
}
