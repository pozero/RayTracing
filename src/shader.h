#pragma once

#include <string_view>
#include <vector>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "glm/vec3.hpp"
#pragma clang diagnostic pop

class Shader {
public:
    Shader(std::string const &path, unsigned int type);

    ~Shader();

private:
    friend class Program;
    unsigned int m_id;
};

class Program {
public:
    Program(Shader const &vert, Shader const &frag);

    Program(std::string const &path);

    ~Program();

    void use() const;

    void set_bool(std::string_view name, bool value) const;

    void set_int(std::string_view name, int value) const;

    void set_float(std::string_view name, float value) const;

    void set_vec3(std::string_view name, glm::vec3 const &vec3) const;

    void set_vec3(std::string_view name, float x, float y, float z) const;

private:
    void check_link_error() const;

    unsigned int m_id;
};
